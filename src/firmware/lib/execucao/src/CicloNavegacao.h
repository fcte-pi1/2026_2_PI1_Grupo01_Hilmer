#pragma once

/// @file CicloNavegacao.h
/// @brief O ciclo percepção → mapa → decisão → atuação da tarefa de navegação (#184): liga o
/// algoritmo de resolução (`Navegacao`) aos movimentos (`ExecutorMovimentos`).

#include <stddef.h>
#include <stdint.h>

#include "AcompanhadorGiro.h"
#include "ClassificadorParede.h"
#include "ConfiguracaoControle.h"
#include "ConfiguracaoExecucao.h"
#include "ConfiguracaoGiro.h"
#include "ConfiguracaoLocalizacao.h"
#include "CorrecaoPelasParedes.h"
#include "ExecutorMovimentos.h"
#include "IMotores.h"
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
/// **Fora do escopo** (outras issues): recuperação de colisão (#192), motor travado (#341), falha
/// no I²C (#241), tempo limite da corrida (#242/#185) e correção da posição pelas paredes (#188).
/// Nesses casos o ciclo só para em `Falha`, com os motores freados.
///
/// Não depende do Arduino e não aloca memória: testado no ambiente `native`.
class CicloNavegacao {
 public:
  /// @param mapa Mapa da corrida (o de `ArmazenamentoMapa`); precisa continuar existindo.
  CicloNavegacao(Mapa& mapa, const ConfiguracaoExecucao& config = ConfiguracaoExecucao{},
                 const ConfiguracaoControle& controle = ConfiguracaoControle{},
                 const ConfiguracaoGiro& giro = ConfiguracaoGiro{},
                 const ConfiguracaoRealinhamento& realinhamento = ConfiguracaoRealinhamento{});

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
    const bool manobrando =
        estado_ == EstadoCiclo::Girando || (estado_ == EstadoCiclo::Realinhando && !conferindoRealinhamento_);
    return manobrando ? MovimentoRobo::Manobrando : MovimentoRobo::Parado;
  }

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

  const Navegacao& navegacao() const { return navegacao_; }
  const ExecutorMovimentos& executor() const { return executor_; }

 private:
  void entrarEmPercepcao();
  void decidir(IMotores& motores);
  void iniciarGiro(Movimento giro);

  /// Realinha se o desvio medido nas amostras passa do limiar e ainda há tentativa; senão, gira.
  void realinharOuGirar(Movimento giro);

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
  uint32_t realinhamentos_ = 0;
  Movimento giroPendente_ = Movimento::Girar180;  ///< Giro a fazer depois do realinhamento.
  uint8_t realinhamentosNoPasso_ = 0;             ///< Realinhamentos antes do giro pendente.
  bool conferindoRealinhamento_ = false;          ///< Juntando leituras para medir o desvio de novo.
  DesvioLateral ultimoDesvio_;

  static constexpr size_t MAX_AMOSTRAS = 16;  ///< Mesma capacidade do filtro da percepção.
  DistanciasLaterais amostras_[MAX_AMOSTRAS] = {};
  uint8_t quantidadeAmostras_ = 0;
};

/// Nome da etapa para o log: `aguardando_largada`, `percebendo`, `girando`, `avancando`,
/// `objetivo_alcancado`, `sem_caminho` ou `falha`.
const char* nomeDoEstadoCiclo(EstadoCiclo estado);

}  // namespace micromouse
