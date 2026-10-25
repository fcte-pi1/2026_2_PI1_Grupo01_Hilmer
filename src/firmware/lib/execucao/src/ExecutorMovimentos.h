#pragma once

/// @file ExecutorMovimentos.h
/// @brief Executa os movimentos entre células (#184): avançar uma célula, girar 90° e girar 180°.

#include <stdint.h>

#include "AcompanhadorGiro.h"
#include "ConfiguracaoControle.h"
#include "ConfiguracaoExecucao.h"
#include "ConfiguracaoGiro.h"
#include "ConfiguracaoLocalizacao.h"
#include "ControleGiro.h"
#include "ControleMovimento.h"
#include "IMotores.h"
#include "Movimento.h"
#include "Tipos.h"

namespace micromouse {

/// Situação do movimento depois da última atualização.
enum class EstadoExecucao : uint8_t {
  Ocioso,      ///< Nenhum movimento iniciado (ou o último foi cancelado).
  Executando,  ///< Movimento em andamento.
  Concluido,   ///< Movimento terminado: o robô está no centro da célula de destino ou no rumo novo.
  Falha        ///< Movimento interrompido; o motivo está em `falha()`. Os motores ficam freados.
};

/// Por que o último movimento falhou.
enum class FalhaExecucao : uint8_t {
  Nenhuma,
  ParedeNaFrente,     ///< O avanço parou numa parede antes do centro de destino.
  SemLeituraTof,      ///< Sem leitura nova do ToF por `tempoMaximoSemLeituraMs` durante o avanço.
  TempoEsgotado,      ///< O avanço (ou um trecho do realinhamento) passou do tempo máximo.
  GiroTempoEsgotado   ///< O giro não chegou ao rumo no tempo limite (inclusive sem giroscópio válido).
};

/// Plano de um realinhamento: o giro pequeno e o trecho reto que levam o robô ao centro do corredor.
struct PlanoRealinhamento {
  int32_t anguloMiligraus = 0;  ///< Giro de ida, relativo ao rumo (positivo = esquerda).
  int32_t trechoUm = 0;         ///< Primeiro trecho reto (positivo = para a frente).
  int32_t voltaUm = 0;          ///< Segundo trecho, no rumo original (sinal oposto ao primeiro).
  int32_t deslocamentoUm = 0;   ///< Quanto o robô vai para o lado (positivo = esquerda).
};

/// Planeja o realinhamento para um desvio lateral (positivo = o robô está à direita do centro).
///
/// O robô gira θ, anda `s` reto, volta ao rumo e anda `s·cos θ` no sentido oposto: termina
/// `s·sen θ` para o lado, no mesmo ponto do eixo e no mesmo rumo. Com `recuarPrimeiro`, o primeiro
/// trecho é de ré (para o lado de onde o robô veio, que está livre).
///
/// Girado θ, a quina do chassi do lado da parede vai a 78,1 mm × sen(50,2° + θ) do centro (73,5 mm
/// com 20°, contra 60 mm parado), e a quina que avança no eixo, a 78,1 mm × cos(50,2° − θ) (63,8 mm
/// com 15°, contra 50 mm). θ é o menor ângulo, entre 5° e 30°, cujo trecho cabe no espaço, com as
/// quinas a pelo menos `folgaMinimaMm` das paredes. O espaço é `trechoMaximoMm`; sem `trechoLivre`
/// (parede no fim do primeiro trecho), também o que sobra até ela com o robô girado. Se nenhum couber,
/// o plano usa o maior ângulo seguro com o maior trecho dele e corrige só parte do desvio.
PlanoRealinhamento planejarRealinhamento(int32_t desvioUm, bool recuarPrimeiro, bool trechoLivre,
                                          uint16_t trechoMaximoMm, uint16_t folgaMinimaMm);

/// O que o executor recebe a cada ciclo da navegação.
struct EntradaExecucao {
  /// Leitura nova do ToF, medida a partir da face de cada sensor, ou `nullptr` se não houver uma
  /// neste ciclo. Cada leitura deve vir uma vez só.
  const DistanciasLaterais* leituraNova = nullptr;
  /// Última leitura do giroscópio (ângulo e validade).
  LeituraGiro giro;
  /// Posição do robô ao longo do eixo para onde ele está virado, em µm, com o centro da célula 0
  /// em 0 (`posicaoAoLongoDoAvancoUm(odometria)`).
  int64_t posicaoAvancoUm = 0;
  /// Tempo desde o ciclo anterior, em ms.
  uint32_t dtMs = 0;
};

/// Converte o comando decidido pela navegação em potência para as rodas, um movimento por vez.
///
/// - **Avançar uma célula:** o destino é o centro da célula seguinte à mais próxima da posição
///   atual (assim um erro pequeno de parada não se acumula). O `ControleMovimento` centraliza no
///   corredor e mantém o rumo pelo giroscópio; perto do destino, o teto da potência base faz a
///   rampa de desaceleração, e o avanço termina a `toleranciaChegadaMm` do centro. A parada pela
///   parede da frente continua valendo e, perto do destino, também conclui.
/// - **Girar 90° ou 180°:** o `ControleGiro` gira até o **rumo alvo absoluto** (múltiplos de 90°
///   desde a largada), e não 90° a partir do ângulo atual: o erro de um giro não passa para o
///   próximo.
///
/// O ToF mede a partir da face de cada sensor; antes de ir para o `ControleMovimento`, que usa
/// distâncias a partir do centro do robô, a leitura recebe o recuo de cada sensor.
///
/// A fonte da posição não importa: com os encoders instalados, só muda quem calcula
/// `posicaoAvancoUm`. Não depende do Arduino e não aloca memória: testado no ambiente `native`.
///
/// @code
/// ExecutorMovimentos executor;
/// executor.iniciar(Movimento::Girar90Direita);
/// // a cada 10 ms, na tarefa de navegação:
/// EntradaExecucao entrada{leituraNova, lerGiro(), posicaoAoLongoDoAvancoUm(odometria), 10};
/// if (executor.atualizar(entrada, motoresTracao) == EstadoExecucao::Concluido) { /* próximo */ }
/// @endcode
class ExecutorMovimentos {
 public:
  explicit ExecutorMovimentos(const ConfiguracaoExecucao& config = ConfiguracaoExecucao{},
                              const ConfiguracaoControle& controle = ConfiguracaoControle{},
                              const ConfiguracaoGiro& giro = ConfiguracaoGiro{},
                              const ConfiguracaoRealinhamento& realinhamento = ConfiguracaoRealinhamento{});

  /// Começa um movimento. O primeiro `atualizar()` fixa o destino (avanço) ou o ângulo de partida
  /// (giro). Um giro já soma a variação ao rumo alvo aqui. Para o realinhamento, use
  /// `iniciarRealinhamento()`.
  /// @return `false` se outro movimento está em andamento, ou se for `Realinhar` (nada muda).
  bool iniciar(Movimento movimento);

  /// Começa um realinhamento ao centro do corredor (#188), pelo plano dado.
  /// @return `false` se outro movimento está em andamento ou se o plano não desloca nada.
  bool iniciarRealinhamento(const PlanoRealinhamento& plano);

  /// Um ciclo do movimento em andamento: calcula e aplica a potência nos motores.
  /// Fora de `Executando`, não faz nada e devolve o estado atual.
  EstadoExecucao atualizar(const EntradaExecucao& entrada, IMotores& motores);

  /// Interrompe o movimento em andamento, freia os motores e volta a `Ocioso`. Um giro
  /// interrompido não desfaz a mudança do rumo alvo: quem cancela ajusta com `definirRumoAlvo()`.
  void cancelar(IMotores& motores);

  /// Define o rumo alvo absoluto, em mili-graus do giroscópio (por exemplo, na largada). Também é o
  /// rumo nominal, a referência do limite do rumo pelo corredor.
  void definirRumoAlvo(int32_t rumoMiligraus) {
    rumoAlvoMiligraus_ = rumoMiligraus;
    rumoNominalMiligraus_ = rumoMiligraus;
  }

  /// Rumo nominal: o da largada mais as variações dos giros, sem o ajuste pelo corredor.
  int32_t rumoNominalMiligraus() const { return rumoNominalMiligraus_; }

  /// Rumo que o robô deve ter ao fim do giro e durante o avanço, em mili-graus.
  int32_t rumoAlvoMiligraus() const { return rumoAlvoMiligraus_; }

  EstadoExecucao estado() const { return estado_; }
  FalhaExecucao falha() const { return falha_; }
  Movimento movimento() const { return movimento_; }

  /// `true` enquanto há um movimento em andamento.
  bool ocupado() const { return estado_ == EstadoExecucao::Executando; }

  /// Quanto falta até o centro de destino, em mm (negativo se passou). Zero fora do avanço ou antes
  /// do primeiro `atualizar()`. Depois do fim, guarda o valor da parada: é o erro de chegada que
  /// a bancada registra.
  int32_t restanteMm() const { return static_cast<int32_t>(restanteUm_ / 1000); }

  /// Tempo do movimento atual (ou do último), em ms.
  uint32_t tempoMs() const { return tempoMs_; }

  /// Plano do realinhamento em andamento (ou do último), para o log e a bancada.
  const PlanoRealinhamento& planoRealinhamento() const { return plano_; }

  /// Etapa do realinhamento: 0 giro de ida, 1 trecho, 2 giro de volta, 3 trecho de volta.
  uint8_t etapaRealinhamento() const { return static_cast<uint8_t>(etapa_); }

  /// Controles internos, para a telemetria, a bancada e os testes.
  const ControleMovimento& controleMovimento() const { return controleMovimento_; }
  const ControleGiro& controleGiro() const { return controleGiro_; }

  /// A leitura do ToF convertida para distâncias a partir do centro do robô (veja a classe). Um lado
  /// acima de `limiarParedeLateralControleMm` (da face) sai como `LATERAL_SEM_PAREDE_MM`: a parede de
  /// uma célula vizinha não é referência para a centralização (#188).
  DistanciasLaterais paraOCentro(const DistanciasLaterais& leitura) const;

 private:
  EstadoExecucao atualizarAvanco(const EntradaExecucao& entrada, IMotores& motores);
  EstadoExecucao atualizarGiro(const EntradaExecucao& entrada, IMotores& motores);
  EstadoExecucao atualizarRealinhamento(const EntradaExecucao& entrada, IMotores& motores);

  /// Um ciclo de giro até `alvoMiligraus`; começa o `ControleGiro` no primeiro ciclo de cada giro.
  /// @return `Executando` enquanto gira, `Concluido` no rumo, `Falha` no tempo limite.
  EstadoExecucao passoDeGiro(int32_t alvoMiligraus, const EntradaExecucao& entrada, IMotores& motores);

  /// Um ciclo de trecho reto do realinhamento, mantendo `rumoMiligraus`.
  /// @return `Executando` até andar `distanciaUm` (com sinal), `Concluido` ao chegar.
  EstadoExecucao passoDeTrecho(int32_t distanciaUm, int32_t rumoMiligraus, const EntradaExecucao& entrada,
                               IMotores& motores);

  /// Rumo pelo corredor (#188): puxa o rumo alvo para o ângulo atual quando o robô anda paralelo às
  /// duas paredes (veja `AJUSTE_RUMO_PELO_CORREDOR_POR_MIL`).
  void ajustarRumoPeloCorredor(const LeituraGiro& giro);

  /// Teto da potência base para a distância que falta até o destino (a rampa de desaceleração).
  int16_t tetoParaRestante(int64_t restanteUm) const;

  EstadoExecucao terminar(EstadoExecucao estado, FalhaExecucao falha, IMotores& motores);

  ConfiguracaoExecucao config_;
  ConfiguracaoControle configControle_;
  ConfiguracaoGiro configGiro_;
  ConfiguracaoRealinhamento configRealinhamento_;
  ControleMovimento controleMovimento_;
  ControleGiro controleGiro_;

  EstadoExecucao estado_ = EstadoExecucao::Ocioso;
  FalhaExecucao falha_ = FalhaExecucao::Nenhuma;
  Movimento movimento_ = Movimento::AvancarCelula;
  int32_t rumoAlvoMiligraus_ = 0;
  int32_t rumoNominalMiligraus_ = 0;

  bool comecou_ = false;              ///< `false` até o primeiro `atualizar()` do movimento.
  uint32_t tempoMs_ = 0;
  int64_t destinoUm_ = 0;             ///< Centro da célula de destino, no eixo do avanço.
  int64_t restanteUm_ = 0;
  uint32_t semLeituraMs_ = 0;         ///< Tempo desde a última leitura nova do ToF.
  uint32_t desdeCalculoMs_ = 0;       ///< Tempo desde o último cálculo do `ControleMovimento`.
  bool temPotencia_ = false;          ///< `false` até a primeira leitura do avanço.
  PotenciaRodas ultimaPotencia_ = {0, 0};

  // Realinhamento: as quatro etapas do plano.
  enum class EtapaRealinhamento : uint8_t { GiroIda, Trecho, GiroVolta, Volta };
  PlanoRealinhamento plano_;
  EtapaRealinhamento etapa_ = EtapaRealinhamento::GiroIda;
  bool etapaComecou_ = false;
  int64_t inicioTrechoUm_ = 0;
  uint32_t tempoEtapaMs_ = 0;
};

/// Nome do estado para o log: `ocioso`, `executando`, `concluido` ou `falha`.
const char* nomeDoEstadoExecucao(EstadoExecucao estado);

/// Nome da falha para o log: `nenhuma`, `parede_na_frente`, `sem_leitura_tof`, `tempo_esgotado` ou
/// `giro_tempo_esgotado`.
const char* nomeDaFalhaExecucao(FalhaExecucao falha);

}  // namespace micromouse
