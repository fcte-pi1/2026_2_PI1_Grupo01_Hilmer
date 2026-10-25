#pragma once

/// @file SimulacaoCorrida.h
/// @brief O robô simulado, os sensores e o firmware ligados como no `main.cpp`, para os testes de
/// integração da execução dos movimentos (#184) e da correção pelas paredes (#188).
///
/// A cadeia: `RoboSimulado` (rodas com velocidade diferente da tabela, giro pela diferença das rodas
/// e colisão) → `ImuSimulado` + `AcompanhadorGiro` (viés, ruído, calibração e, se pedido, deriva
/// depois dela) e 3 raios de ToF → `CombinadorTof` (a cada 33 ms) → a fonte do deslocamento
/// (`DeslocamentoEstimado`, ou os encoders, com a distância real das rodas) + `Odometria` →
/// `CorrecaoPelasParedes` → `CicloNavegacao` / `ExecutorMovimentos` → motores.

#include <unity.h>

#include <cmath>

#include "AcompanhadorGiro.h"
#include "ArmazenamentoMapa.h"
#include "CicloNavegacao.h"
#include "CombinadorTof.h"
#include "CorrecaoPelasParedes.h"
#include "CorretorDeslocamentoTof.h"
#include "DeslocamentoEstimado.h"
#include "Direcao.h"
#include "EncoderSimulado.h"
#include "ExecutorMovimentos.h"
#include "ImuSimulado.h"
#include "LabirintoSimulado.h"
#include "Odometria.h"
#include "RoboSimulado.h"

namespace micromouse {
namespace simulacao_corrida {

constexpr uint32_t CICLO_MS = 10;
constexpr uint32_t PERIODO_TOF_MS = 33;
constexpr uint32_t DEZ_MINUTOS_MS = 10u * 60u * 1000u;

/// Como a simulação roda.
struct OpcoesSimulacao {
  ParametrosRoboSimulado robo;
  Pose inicial = Pose{{0, 0}, Direcao::Norte};
  double desvioLateralMm = 0.0;  ///< Largada fora do centro (positivo = à direita).
  double desvioRumoGraus = 0.0;  ///< Largada torta (positivo = para a esquerda).
  /// Fonte do deslocamento: `false` é o modo estimado da Rev 2.0; `true`, encoders ideais pela
  /// distância real das rodas (com `fatorEncoder` de erro de escala).
  bool encoders = false;
  double fatorEncoder = 1.0;
  /// Correção pelas paredes da #188 (lateral sempre; ao longo do avanço só com os encoders).
  bool correcaoParedes = true;
  /// Deriva do giroscópio depois da calibração, em décimos de °/s (o viés muda sozinho).
  int16_t derivaGiroDecimosDps = 0;
  ConfiguracaoRealinhamento realinhamento = ConfiguracaoRealinhamento{};
};

/// O robô, os sensores e o firmware, ligados como no `main.cpp`.
class Simulacao {
 public:
  explicit Simulacao(const char* texto, const OpcoesSimulacao& opcoes = OpcoesSimulacao{})
      : opcoes_(opcoes),
        robo_(labirinto_, opcoes.robo),
        deslocamento_(robo_),
        ciclo_(armazenamento_.mapa(), ConfiguracaoExecucao{}, ConfiguracaoControle{}, ConfiguracaoGiro{},
               opcoes.realinhamento),
        executor_(ConfiguracaoExecucao{}, ConfiguracaoControle{}, ConfiguracaoGiro{}, opcoes.realinhamento) {
    TEST_ASSERT_EQUAL(ResultadoCarga::Carregado, labirinto_.carregar(texto));
    armazenamento_.iniciarCorrida(labirinto_.obterDimensao());
    robo_.posicionar(opcoes.inicial, opcoes.desvioLateralMm, opcoes.desvioRumoGraus);
    deslocamento_.reiniciar(odometria_, opcoes.inicial);
    imu_.definirVies(12);           // 1,2 °/s parado
    imu_.definirRuidoAlternado(4);  // ±0,4 °/s
    acompanhador_.iniciarCalibracao();
    for (int i = 0; i < 400 && acompanhador_.ultima().calibracao != EstadoCalibracao::Calibrado; i++) {
      acompanhador_.ciclo(CICLO_MS);
    }
    TEST_ASSERT_EQUAL(EstadoCalibracao::Calibrado, acompanhador_.ultima().calibracao);
    // Depois da calibração, o viés muda: é a deriva que a calibração não vê.
    imu_.definirVies(static_cast<int16_t>(12 + opcoes.derivaGiroDecimosDps));
    ciclo_.reiniciar(opcoes.inicial, motores());
  }

  /// Motores que o firmware aciona: pelo estimador no modo sem encoder, direto no robô com encoders.
  IMotores& motores() { return opcoes_.encoders ? static_cast<IMotores&>(robo_) : deslocamento_.motores(); }

  /// Um ciclo de 10 ms do mundo, dos sensores e da odometria; devolve a leitura nova do ToF, se
  /// houver. `movimento` diz se a leitura serve de referência para as correções.
  const DistanciasLaterais* sentir(bool sincronizarDirecao, Direcao direcao, MovimentoRobo movimento) {
    agoraMs_ += CICLO_MS;
    robo_.avancar(CICLO_MS);
    imu_.definirTaxa(robo_.taxaGiroDecimosDps());
    giro_ = acompanhador_.ciclo(CICLO_MS);

    bool nova = false;
    if (agoraMs_ - ultimaTofMs_ >= PERIODO_TOF_MS) {
      ultimaTofMs_ = agoraMs_;
      const DistanciasLaterais bruta = robo_.medirTof();
      tof_.registrar(Lado::Frente, bruta.frenteMm, false, agoraMs_);
      tof_.registrar(Lado::Esquerda, bruta.esquerdaMm, false, agoraMs_);
      tof_.registrar(Lado::Direita, bruta.direitaMm, false, agoraMs_);
      nova = tof_.distancias(agoraMs_, leitura_);
    }
    // Como no `main.cpp`: fora dos giros, a direção da odometria é a do giroscópio, a cada ciclo.
    if (sincronizarDirecao) {
      odometria_.definirDirecao(direcao);
    }
    const bool referencia = nova && movimento != MovimentoRobo::Manobrando;
    if (opcoes_.encoders) {
      const double umPorPulso = ConfiguracaoOdometria{}.micrometrosPorPulso;
      encoderEsq_.definirContagem(
          static_cast<int32_t>(std::lround(robo_.distanciaRodaEsquerdaMm() * 1000.0 * opcoes_.fatorEncoder / umPorPulso)));
      encoderDir_.definirContagem(
          static_cast<int32_t>(std::lround(robo_.distanciaRodaDireitaMm() * 1000.0 * opcoes_.fatorEncoder / umPorPulso)));
      odometria_.atualizar(encoderEsq_, encoderDir_);
    } else {
      if (movimento == MovimentoRobo::Manobrando) {
        deslocamento_.esquecerLaterais();
      }
      deslocamento_.ciclo(CICLO_MS, odometria_, referencia ? &leitura_ : nullptr);
    }
    if (nova && opcoes_.correcaoParedes) {
      correcao_.aplicar(odometria_, leitura_, movimento, opcoes_.encoders);
    }
    return nova ? &leitura_ : nullptr;
  }

  /// Um ciclo da tarefa de navegação com o `CicloNavegacao`.
  EstadoCiclo passoCiclo() {
    const DistanciasLaterais* nova = sentir(ciclo_.direcaoDefinida(), ciclo_.direcao(), ciclo_.movimentoRobo());
    EntradaCiclo e;
    e.liberado = true;
    e.pose = odometria_.pose();
    e.poseValida = odometria_.posicaoValida();
    e.posicaoAvancoUm = posicaoAoLongoDoAvancoUm(odometria_);
    e.leituraNova = nova;
    e.giro = giro_;
    e.dtMs = CICLO_MS;
    return ciclo_.atualizar(e, motores());
  }

  /// Corre até o ciclo parar (objetivo, sem caminho ou falha) ou o tempo acabar.
  EstadoCiclo correr(uint32_t limiteMs = DEZ_MINUTOS_MS) {
    EstadoCiclo estado = ciclo_.estado();
    while (agoraMs_ < limiteMs) {
      estado = passoCiclo();
      if (estado == EstadoCiclo::ObjetivoAlcancado || estado == EstadoCiclo::SemCaminho ||
          estado == EstadoCiclo::Falha) {
        break;
      }
    }
    return estado;
  }

  /// Executa um movimento só, com o `ExecutorMovimentos` próprio da simulação, até ele terminar.
  EstadoExecucao executar(Movimento movimento, Direcao& direcao) {
    executor_.iniciar(movimento);
    return terminarMovimento(movimento, direcao);
  }

  /// Executa um realinhamento pelo plano dado.
  EstadoExecucao realinhar(const PlanoRealinhamento& plano, Direcao direcao) {
    executor_.iniciarRealinhamento(plano);
    return terminarMovimento(Movimento::Realinhar, direcao);
  }

  /// Fica parado `ms`, lendo os sensores (e corrigindo a posição pelas paredes).
  void esperar(uint32_t ms, Direcao direcao) {
    for (uint32_t t = 0; t < ms; t += CICLO_MS) {
      sentir(true, direcao, MovimentoRobo::Parado);
    }
  }

  ExecutorMovimentos& executor() { return executor_; }
  const RoboSimulado& robo() const { return robo_; }
  Odometria& odometria() { return odometria_; }
  const CicloNavegacao& ciclo() const { return ciclo_; }
  const LabirintoSimulado& labirinto() const { return labirinto_; }
  const Mapa& mapa() const { return armazenamento_.mapa(); }
  const CorrecaoPelasParedes& correcao() const { return correcao_; }
  uint32_t agoraMs() const { return agoraMs_; }
  const LeituraGiro& giro() const { return giro_; }

 private:
  EstadoExecucao terminarMovimento(Movimento movimento, Direcao& direcao) {
    EstadoExecucao estado = executor_.estado();
    const uint32_t limite = agoraMs_ + 15000;
    const MovimentoRobo tipo = ehManobra(movimento) ? MovimentoRobo::Manobrando : MovimentoRobo::Avancando;
    while (executor_.ocupado() && agoraMs_ < limite) {
      const DistanciasLaterais* nova = sentir(!ehGiro(movimento), direcao, tipo);
      EntradaExecucao e;
      e.leituraNova = nova;
      e.giro = giro_;
      e.posicaoAvancoUm = posicaoAoLongoDoAvancoUm(odometria_);
      e.dtMs = CICLO_MS;
      estado = executor_.atualizar(e, motores());
    }
    if (estado == EstadoExecucao::Concluido) {
      direcao = direcaoDepois(direcao, movimento);
      odometria_.definirDirecao(direcao);
    }
    // Deixa o robô assentar um ciclo, como na percepção.
    sentir(true, direcao, MovimentoRobo::Parado);
    return estado;
  }

  OpcoesSimulacao opcoes_;
  LabirintoSimulado labirinto_;
  RoboSimulado robo_;
  DeslocamentoEstimado deslocamento_;
  Odometria odometria_;
  EncoderSimulado encoderEsq_;
  EncoderSimulado encoderDir_;
  ImuSimulado imu_;
  AcompanhadorGiro acompanhador_{imu_};
  CombinadorTof tof_;
  ArmazenamentoMapa armazenamento_;
  CorrecaoPelasParedes correcao_;
  CicloNavegacao ciclo_;
  ExecutorMovimentos executor_;
  DistanciasLaterais leitura_ = {0, 0, 0};
  LeituraGiro giro_;
  uint32_t agoraMs_ = 0;
  uint32_t ultimaTofMs_ = 0;
};

}  // namespace simulacao_corrida
}  // namespace micromouse
