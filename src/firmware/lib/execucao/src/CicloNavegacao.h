#pragma once

/// @file CicloNavegacao.h
/// @brief O ciclo percepção → mapa → decisão → atuação da tarefa de navegação (#184): liga o
/// algoritmo de resolução (`Navegacao`) aos movimentos (`ExecutorMovimentos`).

#include <stddef.h>
#include <stdint.h>

#include "AcompanhadorGiro.h"
#include "ClassificadorParede.h"
#include "ConfiguracaoColisao.h"
#include "ConfiguracaoControle.h"
#include "ConfiguracaoExecucao.h"
#include "ConfiguracaoGiro.h"
#include "ConfiguracaoLocalizacao.h"
#include "CorrecaoPelasParedes.h"
#include "ExecutorMovimentos.h"
#include "PerfilLateral.h"
#include "IMotores.h"
#include "LocalizadorLargada.h"
#include "Mapa.h"
#include "Movimento.h"
#include "Navegacao.h"
#include "Tipos.h"

namespace micromouse {

/// Etapa do ciclo.
enum class EstadoCiclo : uint8_t {
  AguardandoLargada,  ///< Parado na partida até a largada ser liberada (ou pausado sem liberação).
  Percebendo,         ///< Parado no centro da célula, juntando leituras para gravar as paredes.
  Realinhando,        ///< Indo para o centro do corredor antes de girar no lugar (#188).
  Girando,            ///< Girando para a direção decidida.
  Avancando,          ///< Avançando uma célula.
  Recuperando,        ///< Recuando depois de uma colisão, antes de perceber de novo (#192).
  ObjetivoAlcancado,  ///< Na célula objetivo: parado.
  SemCaminho,         ///< O mapa conhecido não tem caminho até o objetivo: parado.
  Falha               ///< Um movimento falhou (veja `falhaExecucao()`) ou a pose saiu do mapa: parado.
};

/// O que o ciclo recebe a cada 10 ms da tarefa de navegação.
struct EntradaCiclo {
  /// `true` quando o robô pode se mover (permissão provisória do `main.cpp`: boot aprovado,
  /// giroscópio calibrado, gatilho da largada disparado e objetivo ainda não alcançado; com a
  /// `FaseCorrida` da #242, a fase entra aqui).
  bool liberado = false;
  /// Pose estimada pela odometria (a direção já sincronizada com `direcao()`, veja a classe).
  Pose pose = {{0, 0}, Direcao::Norte};
  /// `false` se a odometria saiu da malha (`Odometria::posicaoValida()`).
  bool poseValida = true;
  /// Posição ao longo do eixo de avanço, em µm (`posicaoAoLongoDoAvancoUm(odometria)`).
  int64_t posicaoAvancoUm = 0;
  /// Leitura nova do ToF, a partir da face dos sensores, ou `nullptr`.
  const DistanciasLaterais* leituraNova = nullptr;
  /// Última leitura do giroscópio.
  LeituraGiro giro;
  /// Tempo desde o ciclo anterior, em ms.
  uint32_t dtMs = 0;
  /// Colisão percebida neste ciclo pelo `DetectorColisao` (#192), ou `Nenhuma`. Só vale com o robô em
  /// movimento (`emMovimento()`); nas outras etapas é ignorada.
  TipoColisao colisao = TipoColisao::Nenhuma;
};

/// Liga a percepção, o mapa, o Flood Fill e os movimentos, uma célula por vez:
///
/// 1. **Percebendo:** parado no centro da célula, junta `amostrasPercepcao` leituras novas do ToF,
///    classifica as paredes (`ClassificadorParede`, mediana) e as grava no mapa (`AtualizadorMapa`)
///    na pose da navegação.
/// 2. **Decisão:** `Navegacao::darPasso()` devolve a direção absoluta da próxima célula.
/// 3. **Realinhando** (#188): antes de um giro no lugar, se as paredes laterais mostram o robô mais
///    de `desvioMinimoMm` fora do centro do corredor, ele se desloca para o centro (giro pequeno,
///    trecho reto, volta ao rumo e trecho de volta; veja `planejarRealinhamento`). A quina do chassi
///    tem só 5,9 mm de folga no giro de 90°. O primeiro trecho vai para o lado livre: para trás (a
///    célula de onde veio) ou, se ela estiver fechada, para a frente.
/// 4. **Girando:** se a direção é outra, o giro de 90° ou 180° que leva a ela.
/// 5. **Avançando:** uma célula. No fim, `confirmarPose()` com a célula da odometria e a direção
///    do giroscópio, e volta a 1. Se uma parede não mapeada parar o avanço, o passo é cancelado e
///    o próximo planejamento já a considera.
///
/// **Direção.** No modo estimado, a direção da `Odometria` vem da diferença entre as rodas, que
/// é imprecisa; o giroscópio é a referência. Fora dos giros (`direcaoDefinida()`), quem chama deve
/// chamar `Odometria::definirDirecao(direcao())` a cada ciclo, antes de atualizar a odometria: além
/// de fixar a direção, isso zera a diferença entre as rodas, que nos giros pequenos do
/// realinhamento chegaria a virar a direção estimada.
///
/// **Largada em qualquer célula** (`ConfiguracaoExecucao::largadaEmQualquerCelula`): o robô não sabe
/// em que célula foi colocado nem o tamanho do labirinto, só que está virado para o Norte. O
/// `reiniciar()` troca o mapa pelo de exploração (7 × 23) e põe o robô no meio dele
/// (`poseInicialOdometria()`, que quem chama também dá à odometria). A cada percepção, a célula
/// entra como visitada no `LocalizadorLargada`, que descarta as hipóteses de posição e tamanho que
/// contradizem o que foi visto, e o Flood Fill vai para o objetivo mais próximo entre os das
/// hipóteses vivas (menos a célula atual). Quando sobra uma hipótese, ou quando o robô está na célula
/// que é o objetivo de todas, o ciclo **se localiza**: monta o mapa real com as paredes já vistas,
/// passa a navegação para ele e, dali em diante, converte a pose da odometria (que continua no
/// referencial da exploração) para o labirinto (`poseNoLabirinto()`). O resto da corrida é a normal.
/// Se o mapa da exploração deixar de fazer sentido (nenhuma hipótese sobra, ou nenhum objetivo
/// possível é alcançável), a exploração **recomeça** da célula atual, com o mapa limpo, até
/// `relargadasMaximas` vezes: a direção continua valendo, porque vem do giroscópio desde a largada.
///
/// **Colisão (#192).** Se `EntradaCiclo::colisao` chega com o robô em movimento (avançando, girando
/// ou realinhando), o ciclo cancela o movimento, freia, esquece o passo planejado e vai para
/// **Recuperando**: o `ExecutorMovimentos` recua `recuoMm` em linha reta (ou até o centro da célula,
/// se a parede da frente estiver à vista), com o rumo da direção atual. Depois, volta a **Percebendo**
/// na mesma célula e decide de novo. O mapa e a pose da navegação (célula e direção) ficam como
/// estavam: só a parede da frente é gravada, e só quando a colisão se repete na mesma célula e direção
/// e o ToF frontal não mostra a frente livre (uma parede que a percepção não viu). Depois de
/// `colisoesSeguidasMaximas` colisões sem concluir uma célula, pára em `Falha`.
///
/// **Fora do escopo** (outras issues): motor travado (#341) e tempo limite da corrida (#242/#185).
/// Nesses casos o ciclo só para em `Falha`, com os motores freados. Na falha do I²C (#241), o
/// `main.cpp` tira a permissão (`EntradaCiclo::liberado`) enquanto ela durar: o ciclo freia, cancela
/// o movimento e espera; quando a falha passa, percebe de novo a partir de onde parou.
///
/// Não depende do Arduino e não aloca memória: testado no ambiente `native`.
class CicloNavegacao {
 public:
  /// @param mapa Mapa da corrida (o de `ArmazenamentoMapa`); precisa continuar existindo.
  CicloNavegacao(Mapa& mapa, const ConfiguracaoExecucao& config = ConfiguracaoExecucao{},
                 const ConfiguracaoControle& controle = ConfiguracaoControle{},
                 const ConfiguracaoGiro& giro = ConfiguracaoGiro{},
                 const ConfiguracaoRealinhamento& realinhamento = ConfiguracaoRealinhamento{},
                 const ConfiguracaoColisao& colisao = ConfiguracaoColisao{});

  /// Volta a aguardar a largada na pose dada: refaz o objetivo pelo tamanho atual do mapa, define
  /// a pose da navegação, cancela o movimento em andamento e freia os motores. Chame depois de
  /// `ArmazenamentoMapa::iniciarCorrida()` e quando o robô for recolocado na partida.
  void reiniciar(const Pose& inicial, IMotores& motores);

  /// Um ciclo: executa a etapa atual e aplica a potência nos motores.
  EstadoCiclo atualizar(const EntradaCiclo& entrada, IMotores& motores);

  EstadoCiclo estado() const { return estado_; }

  /// Direção atual do robô, confirmada pelo giroscópio no fim de cada giro.
  Direcao direcao() const { return direcao_; }

  /// `false` durante um giro: aí a direção da odometria fica livre.
  bool direcaoDefinida() const { return estado_ != EstadoCiclo::Girando; }

  /// O que o robô está fazendo, para a correção pelas paredes (#188): `Manobrando` nos giros e
  /// no realinhamento, quando as leituras não servem de referência.
  MovimentoRobo movimentoRobo() const {
    if (estado_ == EstadoCiclo::Avancando) {
      return MovimentoRobo::Avancando;
    }
    if (estado_ == EstadoCiclo::Recuperando) {
      return MovimentoRobo::Manobrando;
    }
    const bool manobrando =
        estado_ == EstadoCiclo::Girando || (estado_ == EstadoCiclo::Realinhando && !conferindoRealinhamento_);
    return manobrando ? MovimentoRobo::Manobrando : MovimentoRobo::Parado;
  }

  /// `true` enquanto o robô está sendo conduzido: avançando, girando ou realinhando (não parado
  /// para perceber). É quando a detecção de colisão fica ligada.
  bool emMovimento() const {
    return estado_ == EstadoCiclo::Avancando || estado_ == EstadoCiclo::Girando ||
           (estado_ == EstadoCiclo::Realinhando && !conferindoRealinhamento_);
  }

  /// Colisões tratadas desde a largada (o evento `colisao` da telemetria).
  uint32_t colisoes() const { return colisoes_; }

  /// Recuos concluídos desde a largada (colisões de que o robô se recuperou).
  uint32_t recuperacoes() const { return recuperacoes_; }

  /// Depuração: quantas percepções já gravaram o mapa, e o que a última viu e gravou. Mostra, no log, em qual
  /// célula e direção cada parede foi registrada.
  uint32_t percepcoes() const { return percepcoes_; }
  /// Quantas vezes o mapa foi apagado por "sem caminho" (`ConfiguracaoExecucao::reiniciosDoMapaNoSemCaminho`).
  uint8_t reiniciosDoMapa() const { return reiniciosDoMapa_; }
  /// Leituras laterais do último avanço concluído e quantos avanços já geraram um perfil (para o log).
  const PerfilLateral& perfilLateral() const { return perfil_; }
  uint32_t perfisConcluidos() const { return perfisConcluidos_; }
  Direcao direcaoDoPerfil() const { return direcaoDoPerfil_; }
  const ClassificadorParede::ResultadoClassificacao& ultimaClassificacao() const { return ultimaClassificacao_; }
  const Pose& poseDaUltimaPercepcao() const { return poseDaUltimaPercepcao_; }

  /// Paredes gravadas no mapa por colisões repetidas (veja a classe).
  uint32_t paredesPorColisao() const { return paredesPorColisao_; }

  /// Tipo da última colisão tratada.
  TipoColisao ultimaColisao() const { return ultimaColisao_; }

  /// Realinhamentos feitos desde a largada.
  uint32_t realinhamentos() const { return realinhamentos_; }

  /// Desvio lateral medido na última percepção, em mm (positivo = à direita do centro).
  int32_t ultimoDesvioLateralMm() const { return ultimoDesvio_.desvioUm / 1000; }

  /// Motivo da última falha de movimento (`Nenhuma` se a falha foi a pose fora do mapa).
  FalhaExecucao falhaExecucao() const { return falhaExecucao_; }

  /// Resultado do último `confirmarPose()`.
  ResultadoConfirmacao ultimaConfirmacao() const { return ultimaConfirmacao_; }

  /// Células avançadas desde a largada (o percurso que a bancada e os testes conferem).
  uint32_t celulasAvancadas() const { return celulasAvancadas_; }

  /// Giros concluídos desde a largada.
  uint32_t girosFeitos() const { return girosFeitos_; }

  /// Pose que a odometria deve adotar depois do `reiniciar()`: a pose dada nele ou, com a largada em
  /// qualquer célula, o meio do mapa de exploração, na mesma direção.
  Pose poseInicialOdometria() const { return poseInicialOdometria_; }

  /// `false` enquanto o robô, largado numa célula desconhecida, ainda não descobriu onde está. Na
  /// largada fixa, sempre `true`.
  bool localizado() const { return !explorando_; }

  /// Pose da odometria convertida para o labirinto real (igual, na largada fixa). Antes de o robô se
  /// localizar, devolve a própria pose da exploração.
  Pose poseNoLabirinto(const Pose& poseOdometria) const;

  /// `false` se a pose da odometria cai fora do labirinto real depois da conversão.
  bool poseDentroDoLabirinto(const Pose& poseOdometria) const;

  /// Hipóteses de largada da exploração (veja `LocalizadorLargada`).
  const LocalizadorLargada& localizador() const { return localizador_; }

  /// Quantas vezes a exploração recomeçou da célula atual (veja a classe).
  uint8_t relargadas() const { return relargadas_; }

  /// Hipótese adotada ao se localizar (só vale com `localizado()` na largada em qualquer célula).
  HipoteseLargada hipoteseAdotada() const { return hipotese_; }

  const Navegacao& navegacao() const { return navegacao_; }
  const ExecutorMovimentos& executor() const { return executor_; }

 private:
  void entrarEmPercepcao();
  /// Grava no mapa o que as amostras mostram, tratando as laterais como desconhecidas quando os sensores
  /// laterais estão na diagonal e a frente é parede (`ConfiguracaoExecucao::lateraisEmDiagonal`).
  void atualizarMapaComAmostras();
  /// Apaga as paredes gravadas (fica só o perímetro), mantendo a célula e a direção atuais.
  void apagarMapaMantendoAPose();

  /// Exploração: registra a célula, atualiza as hipóteses e, se der, se localiza; senão, mira o
  /// objetivo mais próximo entre os das hipóteses vivas.
  void atualizarExploracao();

  /// Adota a hipótese `h`: mapa real, navegação no labirinto e conversão da odometria.
  void localizar(const HipoteseLargada& h);

  /// Recomeça a exploração da célula atual, com o mapa limpo e todas as hipóteses, gravando de novo
  /// as paredes da última percepção. `false` se já usou as `relargadasMaximas`.
  bool relargar();

  /// Entrada com a pose convertida para o referencial do mapa atual.
  EntradaCiclo paraOMapa(const EntradaCiclo& entrada) const;
  void decidir(IMotores& motores);
  void iniciarGiro(Movimento giro);

  /// Trata a colisão: cancela o movimento, grava a parede se ela se repete e começa o recuo.
  void iniciarRecuperacao(const EntradaCiclo& entrada, IMotores& motores);

  /// Realinha se o desvio medido nas amostras passa do limiar e ainda há tentativa; senão, gira.
  void realinharOuGirar(Movimento giro);
  /// `true` se a célula de destino do avanço fica na borda do mapa: há parede garantida no fim dela.
  bool destinoTerminaNaBorda() const;

  /// Desvio lateral pela mediana das leituras da percepção.
  DesvioLateral desvioDasAmostras() const;
  void confirmarChegada(const EntradaCiclo& entrada);
  void parar(EstadoCiclo estado, IMotores& motores);

  /// Rumo do giroscópio que corresponde à direção `d`, o mais próximo de `anguloAtual` (o ângulo
  /// acumula voltas inteiras).
  int32_t rumoDaDirecao(Direcao d, int32_t anguloAtual) const;

  Mapa& mapa_;
  Navegacao navegacao_;
  ClassificadorParede classificador_;
  ExecutorMovimentos executor_;
  ConfiguracaoExecucao config_;
  ConfiguracaoRealinhamento configRealinhamento_;
  ConfiguracaoColisao configColisao_;

  EstadoCiclo estado_ = EstadoCiclo::AguardandoLargada;
  Direcao direcao_ = Direcao::Norte;
  Direcao direcaoPasso_ = Direcao::Norte;
  Direcao direcaoLargada_ = Direcao::Norte;
  int32_t rumoLargadaMiligraus_ = 0;  ///< Ângulo do giroscópio na largada, com o robô em `direcaoLargada_`.
  bool largou_ = false;
  FalhaExecucao falhaExecucao_ = FalhaExecucao::Nenhuma;
  ResultadoConfirmacao ultimaConfirmacao_ = ResultadoConfirmacao::Confirmada;
  uint32_t celulasAvancadas_ = 0;
  uint32_t girosFeitos_ = 0;
  LocalizadorLargada localizador_;
  bool explorando_ = false;           ///< Largada em qualquer célula, ainda sem se localizar.
  HipoteseLargada hipotese_ = {0, 0, 4};
  int16_t deslocamentoLinhas_ = 0;    ///< Odometria → mapa atual: linhas a subtrair.
  int16_t deslocamentoColunas_ = 0;   ///< Odometria → mapa atual: colunas a subtrair.
  uint8_t relargadas_ = 0;
  Pose poseInicialOdometria_ = {{0, 0}, Direcao::Norte};
  uint32_t realinhamentos_ = 0;
  uint32_t colisoes_ = 0;
  uint32_t recuperacoes_ = 0;
  uint32_t paredesPorColisao_ = 0;
  uint8_t colisoesSeguidas_ = 0;        ///< Colisões desde a última célula concluída.
  uint32_t celulasNaUltimaColisao_ = 0;
  uint8_t colisoesNaMesmaFrente_ = 0;   ///< Colisões seguidas, avançando, na mesma célula e direção.
  PosicaoCelula celulaDaColisao_ = {0, 0};
  Direcao direcaoDaColisao_ = Direcao::Norte;
  TipoColisao ultimaColisao_ = TipoColisao::Nenhuma;
  uint16_t ultimaFrenteMm_ = 0;         ///< Última leitura do ToF frontal, a partir da face do sensor.
  bool temFrente_ = false;
  uint32_t percepcoes_ = 0;
  uint8_t reiniciosDoMapa_ = 0;
  ClassificadorParede::ResultadoClassificacao ultimaClassificacao_;
  Pose poseDaUltimaPercepcao_ = {{0, 0}, Direcao::Norte};
  PerfilLateral perfil_;                ///< Leituras laterais do último avanço (aberturas nas paredes).
  Direcao direcaoDoPerfil_ = Direcao::Norte;
  bool perfilValido_ = false;           ///< O perfil é do avanço que acabou de chegar nesta célula.
  uint32_t perfisConcluidos_ = 0;
  bool recuouParaGirar_ = false;        ///< Já recuou para este giro (evita recuar de novo antes dele).
  Movimento giroPendente_ = Movimento::Girar180;  ///< Giro a fazer depois do realinhamento.
  uint8_t realinhamentosNoPasso_ = 0;             ///< Realinhamentos antes do giro pendente.
  bool conferindoRealinhamento_ = false;          ///< Juntando leituras para medir o desvio de novo.
  DesvioLateral ultimoDesvio_;

  static constexpr size_t MAX_AMOSTRAS = 16;  ///< Mesma capacidade do filtro da percepção.
  DistanciasLaterais amostras_[MAX_AMOSTRAS] = {};
  uint8_t quantidadeAmostras_ = 0;
};

/// Nome da etapa para o log: `aguardando_largada`, `percebendo`, `realinhando`, `girando`,
/// `avancando`, `recuperando`, `objetivo_alcancado`, `sem_caminho` ou `falha`.
const char* nomeDoEstadoCiclo(EstadoCiclo estado);

}  // namespace micromouse
