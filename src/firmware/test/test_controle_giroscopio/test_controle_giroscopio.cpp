/// @file test_controle_giroscopio.cpp
/// @brief Testes do controle de rumo pelo giroscópio (#190):
/// - Giros de 90° e 180° em malha fechada com tolerância configurável (`ControleGiro`).
/// - Correção do desvio de rumo durante o avanço em linha reta (`ControleMovimento`).
/// - Testes com giroscópio simulado (`ImuSimulado`), incluindo deriva, viés e deslizamento de rodas.
///
/// Roda no computador: `pio test -e native -f test_controle_giroscopio`.

#include <unity.h>

#include <cmath>
#include <cstdint>

#include "AcompanhadorGiro.h"
#include "ConfiguracaoControle.h"
#include "ConfiguracaoGiro.h"
#include "ControleGiro.h"
#include "ControleMovimento.h"
#include "IMotores.h"
#include "ImuSimulado.h"
#include "IntegradorGiro.h"
#include "MotoresSimulados.h"
#include "SensorDistanciaSimulado.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

constexpr uint16_t LONGE = 1000;
constexpr uint16_t SEM_PAREDE = 400;

DistanciasLaterais leitura(uint16_t frente, uint16_t esquerda, uint16_t direita) {
  return DistanciasLaterais{frente, esquerda, direita};
}

/// Simula a física de rotação no próprio eixo para um ciclo de tempo dtMs.
/// @param potencia Potência diferencial aplicada às rodas {esq, dir}.
/// @param anguloAtualMiligraus Ângulo atual do robô em mili-graus.
/// @param dtMs Intervalo de tempo em ms.
/// @param fatorDeslizamento Fator de escala do atrito (1.0 = normal, 0.5 = 50% de deslizamento).
/// @param taxaGrausPorSegundoPorPotencia Eficiência angular do chassi.
int32_t simularRotacao(const PotenciaRodas& potencia, int32_t anguloAtualMiligraus, uint32_t dtMs,
                       double fatorDeslizamento = 1.0, double taxaGrausPorSegundoPorPotencia = 0.5) {
  // Com giro no lugar: roda esquerda recua (-P) e direita avança (+P) para girar à esquerda (+yaw).
  // Velocidade diferencial efetiva = (dir - esq) / 2.
  const double potenciaDiferencial = (potencia.direita - potencia.esquerda) / 2.0;
  const double velAngularDps = potenciaDiferencial * taxaGrausPorSegundoPorPotencia * fatorDeslizamento;
  const double deltaGraus = velAngularDps * (static_cast<double>(dtMs) / 1000.0);
  const int32_t deltaMiligraus = static_cast<int32_t>(std::round(deltaGraus * 1000.0));
  return anguloAtualMiligraus + deltaMiligraus;
}

}  // namespace

// ============================================================================
// PARTE 1: Giro de 90° e 180° em malha fechada (ControleGiro)
// ============================================================================

/// Critério 1: Giro de 90° para a esquerda (+90°) fecha dentro da tolerância configurável.
void test_giro_90_esquerda_conclui_na_tolerancia() {
  ControleGiro controle;
  MotoresSimulados motores;
  int32_t angulo = 0;

  controle.iniciarGiro90Esquerda(angulo);
  TEST_ASSERT_EQUAL(EstadoGiro::Girando, controle.estado());
  TEST_ASSERT_EQUAL_INT32(90000, controle.anguloAlvoMiligraus());

  // Simula o avanço do giro em passos de 10 ms
  for (int ciclo = 0; ciclo < 200; ++ciclo) {
    const EstadoGiro est = controle.atualizar(angulo, motores, 10);
    if (est == EstadoGiro::Concluido) {
      break;
    }
    angulo = simularRotacao(motores.ultimaPotencia(), angulo, 10);
  }

  TEST_ASSERT_TRUE(controle.concluido());
  TEST_ASSERT_EQUAL(EstadoGiro::Concluido, controle.estado());
  TEST_ASSERT_TRUE(motores.freados());
  const int32_t erroFinal = std::abs(90000 - angulo);
  TEST_ASSERT_TRUE(erroFinal <= controle.configuracao().toleranciaMiligraus);
}

/// Critério 1: Giro de 90° para a direita (-90°) fecha dentro da tolerância configurável.
void test_giro_90_direita_conclui_na_tolerancia() {
  ControleGiro controle;
  MotoresSimulados motores;
  int32_t angulo = 0;

  controle.iniciarGiro90Direita(angulo);
  TEST_ASSERT_EQUAL(EstadoGiro::Girando, controle.estado());
  TEST_ASSERT_EQUAL_INT32(-90000, controle.anguloAlvoMiligraus());

  for (int ciclo = 0; ciclo < 200; ++ciclo) {
    const EstadoGiro est = controle.atualizar(angulo, motores, 10);
    if (est == EstadoGiro::Concluido) {
      break;
    }
    angulo = simularRotacao(motores.ultimaPotencia(), angulo, 10);
  }

  TEST_ASSERT_TRUE(controle.concluido());
  TEST_ASSERT_TRUE(motores.freados());
  const int32_t erroFinal = std::abs(-90000 - angulo);
  TEST_ASSERT_TRUE(erroFinal <= controle.configuracao().toleranciaMiligraus);
}

/// Critério 1: Giro de 180° para a esquerda (+180°) fecha dentro da tolerância.
void test_giro_180_esquerda_conclui_na_tolerancia() {
  ControleGiro controle;
  MotoresSimulados motores;
  int32_t angulo = 0;

  controle.iniciarGiro180(angulo, true);
  TEST_ASSERT_EQUAL_INT32(180000, controle.anguloAlvoMiligraus());

  for (int ciclo = 0; ciclo < 300; ++ciclo) {
    const EstadoGiro est = controle.atualizar(angulo, motores, 10);
    if (est == EstadoGiro::Concluido) {
      break;
    }
    angulo = simularRotacao(motores.ultimaPotencia(), angulo, 10);
  }

  TEST_ASSERT_TRUE(controle.concluido());
  TEST_ASSERT_TRUE(motores.freados());
  const int32_t erroFinal = std::abs(180000 - angulo);
  TEST_ASSERT_TRUE(erroFinal <= controle.configuracao().toleranciaMiligraus);
}

/// Critério 1: Giro de 180° para a direita (-180°) fecha dentro da tolerância.
void test_giro_180_direita_conclui_na_tolerancia() {
  ControleGiro controle;
  MotoresSimulados motores;
  int32_t angulo = 0;

  controle.iniciarGiro180(angulo, false);
  TEST_ASSERT_EQUAL_INT32(-180000, controle.anguloAlvoMiligraus());

  for (int ciclo = 0; ciclo < 300; ++ciclo) {
    const EstadoGiro est = controle.atualizar(angulo, motores, 10);
    if (est == EstadoGiro::Concluido) {
      break;
    }
    angulo = simularRotacao(motores.ultimaPotencia(), angulo, 10);
  }

  TEST_ASSERT_TRUE(controle.concluido());
  TEST_ASSERT_TRUE(motores.freados());
  const int32_t erroFinal = std::abs(-180000 - angulo);
  TEST_ASSERT_TRUE(erroFinal <= controle.configuracao().toleranciaMiligraus);
}

/// Critério 1: Tolerância configurável altera a precisão final da parada.
void test_tolerancia_configuravel_afeta_precisao() {
  ConfiguracaoGiro configEstrita;
  configEstrita.toleranciaMiligraus = 500;  // 0,5°

  ConfiguracaoGiro configLarga;
  configLarga.toleranciaMiligraus = 5000;  // 5,0°

  ControleGiro controleEstrito(configEstrita);
  ControleGiro controleLargo(configLarga);

  controleEstrito.iniciar(90000, 0);
  controleLargo.iniciar(90000, 0);

  // A 86° (4° de erro = 4000 m° restante):
  // O controlador largo (tolerância 5°) já deve considerar concluído após estabilização.
  // O controlador estrito (tolerância 0,5°) deve continuar girando.
  for (int i = 0; i < configLarga.ciclosEstabilizacao; ++i) {
    controleLargo.calcular(86000, 10);
    controleEstrito.calcular(86000, 10);
  }

  TEST_ASSERT_TRUE(controleLargo.concluido());
  TEST_ASSERT_FALSE(controleEstrito.concluido());
  TEST_ASSERT_EQUAL(EstadoGiro::Girando, controleEstrito.estado());
}

/// Critério 3: Deslizamento de rodas (derrapagem no piso) não engana o giro em malha fechada.
/// Em malha aberta (tempo ou encoder de roda), 50% de deslizamento resultaria em giro incompleto (~45°).
/// Com o giroscópio na malha fechada, o robô continua girando até atingir exatamente 90°.
void test_deslizamento_de_rodas_compensa_e_atinge_90_graus() {
  ControleGiro controle;
  MotoresSimulados motores;
  int32_t angulo = 0;

  controle.iniciarGiro90Esquerda(angulo);

  // Fator de deslizamento severo: apenas 40% da tração é transmitida ao piso (escorregamento)
  constexpr double FATOR_DESLIZAMENTO = 0.40;

  for (int ciclo = 0; ciclo < 400; ++ciclo) {
    const EstadoGiro est = controle.atualizar(angulo, motores, 10);
    if (est == EstadoGiro::Concluido) {
      break;
    }
    angulo = simularRotacao(motores.ultimaPotencia(), angulo, 10, FATOR_DESLIZAMENTO);
  }

  TEST_ASSERT_TRUE(controle.concluido());
  const int32_t erroFinal = std::abs(90000 - angulo);
  TEST_ASSERT_TRUE(erroFinal <= controle.configuracao().toleranciaMiligraus);
}

/// Critério 3: Giro integrado com giroscópio simulado (ImuSimulado) contendo viés calibrado.
void test_giro_com_giroscopio_simulado_vies_e_calibracao() {
  ImuSimulado imu;
  // Sensor com viés real de 1,5 °/s (15 décimos de °/s)
  imu.definirVies(15);

  AcompanhadorGiro acompanhador(imu);
  acompanhador.iniciarCalibracao();

  // Executa calibração com o robô parado (200 amostras a 10 ms = 2 s)
  for (int i = 0; i < 200; ++i) {
    acompanhador.ciclo(10);
  }
  TEST_ASSERT_TRUE(acompanhador.ultima().valida);
  TEST_ASSERT_EQUAL(EstadoCalibracao::Calibrado, acompanhador.ultima().calibracao);

  ControleGiro controle;
  MotoresSimulados motores;

  controle.iniciarGiro90Esquerda(acompanhador.ultima().anguloMiligraus);

  // Simula a rotação física do robô alimentando a taxa real na IMU
  for (int ciclo = 0; ciclo < 200; ++ciclo) {
    const LeituraGiro& leitura = acompanhador.ultima();
    const EstadoGiro est = controle.atualizar(leitura, motores, 10);
    if (est == EstadoGiro::Concluido) {
      break;
    }

    // Calcula a velocidade angular física decorrente da potência das rodas
    const PotenciaRodas pot = motores.ultimaPotencia();
    const double velDps = ((pot.direita - pot.esquerda) / 2.0) * 0.5;
    imu.definirTaxa(static_cast<int16_t>(std::round(velDps * 10.0)));

    // Avança a leitura da IMU
    acompanhador.ciclo(10);
  }

  TEST_ASSERT_TRUE(controle.concluido());
  const int32_t erroFinal = std::abs(90000 - acompanhador.ultima().anguloMiligraus);
  TEST_ASSERT_TRUE(erroFinal <= controle.configuracao().toleranciaMiligraus);
}

/// Rampa de desaceleração: potência diminui conforme o robô se aproxima do alvo.
void test_rampa_de_potencia_reduz_ao_aproximar_do_alvo() {
  ControleGiro controle;
  controle.iniciar(90000, 0);

  // Erro grande (90°): potência máxima
  const PotenciaRodas potLonge = controle.calcular(0, 10);
  TEST_ASSERT_EQUAL_INT16(-350, potLonge.esquerda);
  TEST_ASSERT_EQUAL_INT16(350, potLonge.direita);

  // Erro médio (10° = 10000 m°): potência proporcional 150 + (10000 * 40)/10000 = 190
  const PotenciaRodas potPerto = controle.calcular(80000, 10);
  TEST_ASSERT_EQUAL_INT16(-190, potPerto.esquerda);
  TEST_ASSERT_EQUAL_INT16(190, potPerto.direita);

  // Erro pequeno (2,5° = 2500 m°): potência mínima (150 + 10 = 160)
  const PotenciaRodas potMuitoPerto = controle.calcular(87500, 10);
  TEST_ASSERT_EQUAL_INT16(-160, potMuitoPerto.esquerda);
  TEST_ASSERT_EQUAL_INT16(160, potMuitoPerto.direita);
}

/// Ultrapassagem (overshoot): inverte a rotação para retornar à faixa de tolerância.
void test_overshoot_inverte_potencia_para_corrigir() {
  ControleGiro controle;
  controle.iniciar(90000, 0);

  // Robô ultrapassou e foi parar em 94° (erro = -4000 m°)
  const PotenciaRodas pot = controle.calcular(94000, 10);
  // Como erro < 0, deve aplicar potência no sentido horário (+esq, -dir) para voltar
  TEST_ASSERT_TRUE(pot.esquerda > 0);
  TEST_ASSERT_TRUE(pot.direita < 0);
}

/// Timeout: se o robô travar mecanicamente, para e acusa Timeout após tempo máximo.
void test_timeout_em_caso_de_bloqueio() {
  ConfiguracaoGiro config;
  config.tempoMaximoGiroMs = 500;
  ControleGiro controle(config);
  MotoresSimulados motores;

  controle.iniciarGiro90Esquerda(0);

  // Robô travado em 0° por mais de 500 ms
  for (int i = 0; i < 55; ++i) {
    controle.atualizar(0, motores, 10);
  }

  TEST_ASSERT_EQUAL(EstadoGiro::Timeout, controle.estado());
  TEST_ASSERT_FALSE(controle.concluido());
  TEST_ASSERT_TRUE(motores.freados());
}

/// Cancelar giro retorna estado para Inativo.
void test_cancelar_giro() {
  ControleGiro controle;
  controle.iniciar(90000, 0);
  TEST_ASSERT_TRUE(controle.emAndamento());

  controle.cancelar();
  TEST_ASSERT_EQUAL(EstadoGiro::Inativo, controle.estado());
  TEST_ASSERT_FALSE(controle.emAndamento());
}

/// Leitura do giroscópio inválida não envia potências ativas aos motores.
void test_leitura_giro_invalida_segura_motores() {
  ControleGiro controle;
  MotoresSimulados motores;
  controle.iniciarGiro90Esquerda(0);

  LeituraGiro leitura;
  leitura.valida = false;

  controle.atualizar(leitura, motores, 10);
  TEST_ASSERT_TRUE(motores.freados());
}

// ============================================================================
// PARTE 2: Correção do desvio de rumo durante avanço (ControleMovimento)
// ============================================================================

/// Critério 2: Robô em linha reta alinhado ao rumo (desvio 0) não sofre correção de rumo.
void test_avanco_sem_desvio_de_rumo() {
  ControleMovimento controle;
  controle.definirRumoAlvo(0);

  // Paredes dos dois lados perfeitamente centralizado e ângulo 0 m°
  const PotenciaRodas pot = controle.calcular(leitura(LONGE, 84, 84), 100, 0);
  TEST_ASSERT_EQUAL_INT16(400, pot.esquerda);
  TEST_ASSERT_EQUAL_INT16(400, pot.direita);
  TEST_ASSERT_EQUAL_INT32(0, controle.desvioRumoMiligraus());
  TEST_ASSERT_EQUAL_INT16(0, controle.correcaoRumo());
  TEST_ASSERT_TRUE(controle.temRumoValido());
}

/// Critério 2: Desvio de rumo para a direita (ângulo negativo) gera correção positiva (vira para esquerda).
void test_avanco_desvio_direita_vira_para_esquerda() {
  ControleMovimento controle;
  controle.definirRumoAlvo(0);

  // Robô apontando 2° para a direita (-2000 m°).
  // Ganho proporcional 80 (8,0 por grau) -> P = (2000 * 80) / 10000 = 16 de correção.
  const PotenciaRodas pot = controle.calcular(leitura(LONGE, 84, 84), 100, -2000);
  TEST_ASSERT_EQUAL_INT32(2000, controle.desvioRumoMiligraus());
  TEST_ASSERT_EQUAL_INT16(16, controle.correcaoRumo());

  // Correção positiva subtrai da roda esquerda e soma na direita (vira para esquerda)
  TEST_ASSERT_EQUAL_INT16(400 - 16, pot.esquerda);
  TEST_ASSERT_EQUAL_INT16(400 + 16, pot.direita);
}

/// Critério 2: Desvio de rumo para a esquerda (ângulo positivo) gera correção negativa (vira para direita).
void test_avanco_desvio_esquerda_vira_para_direita() {
  ControleMovimento controle;
  controle.definirRumoAlvo(0);

  // Robô apontando 3° para a esquerda (+3000 m°).
  // P = (-3000 * 80) / 10000 = -24.
  const PotenciaRodas pot = controle.calcular(leitura(LONGE, 84, 84), 100, 3000);
  TEST_ASSERT_EQUAL_INT32(-3000, controle.desvioRumoMiligraus());
  TEST_ASSERT_EQUAL_INT16(-24, controle.correcaoRumo());

  // Correção negativa soma na roda esquerda e subtrai na direita (vira para direita)
  TEST_ASSERT_EQUAL_INT16(400 + 24, pot.esquerda);
  TEST_ASSERT_EQUAL_INT16(400 - 24, pot.direita);
}

/// Critério 2: Correção cresce proporcionalmente ao ângulo de desvio.
void test_avanco_correcao_proporcional_ao_desvio() {
  ControleMovimento controle;
  controle.definirRumoAlvo(0);

  controle.calcular(leitura(LONGE, 84, 84), 100, -1000);
  const int16_t corr1Grau = controle.correcaoRumo();

  controle.reiniciar(0);
  controle.calcular(leitura(LONGE, 84, 84), 100, -3000);
  const int16_t corr3Graus = controle.correcaoRumo();

  TEST_ASSERT_EQUAL_INT16(8, corr1Grau);
  TEST_ASSERT_EQUAL_INT16(24, corr3Graus);
  TEST_ASSERT_EQUAL_INT16(corr1Grau * 3, corr3Graus);
}

/// Critério 2: Sem paredes laterais (SemReferencia), o giroscópio é a referência que mantém a linha reta.
void test_avanco_sem_paredes_laterais_mantem_linha_reta_por_giroscopio() {
  ControleMovimento controle;
  controle.definirRumoAlvo(0);

  // Sem paredes laterais e com desvio de 2,5° para a direita (-2500 m°)
  const PotenciaRodas pot = controle.calcular(leitura(LONGE, SEM_PAREDE, SEM_PAREDE), 100, -2500);

  TEST_ASSERT_EQUAL(ModoCentralizacao::SemReferencia, controle.centralizacao().modo());
  TEST_ASSERT_EQUAL_INT32(0, controle.centralizacao().erroMm());

  // A correção vem integralmente do giroscópio: P = (2500 * 80) / 10000 = 20
  TEST_ASSERT_EQUAL_INT16(20, controle.correcaoRumo());
  TEST_ASSERT_EQUAL_INT16(400 - 20, pot.esquerda);
  TEST_ASSERT_EQUAL_INT16(400 + 20, pot.direita);
}

/// Critério 3: Compensação de deslizamento assimétrico (uma roda desliza mais que a outra).
/// Simula avanço em linha reta num piso irregular (HU-11).
void test_avanco_compensa_deslizamento_assimetrico() {
  ControleMovimento controle;
  controle.definirRumoAlvo(0);

  // Simula 50 passos de 10 ms avançando.
  // Assimetria no piso: a roda direita tem 10% menos aderência e puxa o robô para a direita (ângulo fica negativo).
  int32_t anguloMiligraus = 0;
  for (int passo = 0; passo < 50; ++passo) {
    const PotenciaRodas pot = controle.calcular(leitura(LONGE, SEM_PAREDE, SEM_PAREDE), 10, anguloMiligraus);

    // Perturbação: a roda direita desenvolve menos empuxo efetivo
    const double velEsq = pot.esquerda * 0.3;
    const double velDir = pot.direita * 0.3 * 0.90;  // 10% de perda por deslizamento
    const double deltaGraus = (velDir - velEsq) * 0.01;
    anguloMiligraus += static_cast<int32_t>(std::round(deltaGraus * 1000.0));
  }

  // Graças à malha fechada de rumo, o erro angular é contido e não diverge indefinidamente
  TEST_ASSERT_TRUE(std::abs(controle.desvioRumoMiligraus()) < 3000);
}

/// Termo derivativo do rumo amortece variações bruscas de ângulo.
void test_avanco_derivada_amortece_rotacao_rapida() {
  ControleMovimento controle;
  controle.definirRumoAlvo(0);

  // Primeira leitura em 0°
  controle.calcular(leitura(LONGE, 84, 84), 100, 0);

  // Segunda leitura: salto rápido para -2° (-2000 m°) em 100 ms (-20 °/s)
  // Derivada negativa no desvio (indo em direção ao erro) reforça a correção
  const PotenciaRodas pot = controle.calcular(leitura(LONGE, 84, 84), 100, -2000);
  TEST_ASSERT_TRUE(controle.correcaoRumo() > 16);  // proporcional puro seria 16
}

/// Limite máximo da correção de rumo é respeitado.
void test_avanco_saturacao_da_correcao_de_rumo() {
  ControleMovimento controle;
  controle.definirRumoAlvo(0);

  // Desvio extremo de 45° (-45000 m°)
  controle.calcular(leitura(LONGE, 84, 84), 100, -45000);
  TEST_ASSERT_EQUAL_INT16(200, controle.correcaoRumo());
}

/// Respeito ao limite percentual da base para não inverter sentido da roda no cruzeiro.
void test_avanco_respeita_limite_percentual_da_base() {
  ConfiguracaoControle config;
  config.potenciaCruzeiro = 200;
  config.correcaoMaximaPercentualDaBase = 50;  // limite = 100
  ControleMovimento controle(config);
  controle.definirRumoAlvo(0);

  // Grande desvio angular gerando saturação em 200 de correção de rumo
  const PotenciaRodas pot = controle.calcular(leitura(LONGE, 84, 84), 100, -45000);
  // Correção total deve ser limitada a 50% de 200 = 100
  TEST_ASSERT_EQUAL_INT16(200 - 100, pot.esquerda);
  TEST_ASSERT_EQUAL_INT16(200 + 100, pot.direita);
}

/// `definirRumoAlvo` permite manter rumos em diferentes direções (ex.: após uma curva para o Leste).
void test_definir_rumo_alvo_diferentes_direcoes() {
  ControleMovimento controle;
  // Robô virou para a direita (Leste = -90° = -90000 m°)
  controle.definirRumoAlvo(-90000);

  // Estando em -90°, o desvio de rumo deve ser 0
  controle.calcular(leitura(LONGE, 84, 84), 100, -90000);
  TEST_ASSERT_EQUAL_INT32(0, controle.desvioRumoMiligraus());
  TEST_ASSERT_EQUAL_INT16(0, controle.correcaoRumo());

  // Se escapar para -92°, deve corrigir
  controle.calcular(leitura(LONGE, 84, 84), 100, -92000);
  TEST_ASSERT_EQUAL_INT32(2000, controle.desvioRumoMiligraus());
  TEST_ASSERT_TRUE(controle.correcaoRumo() > 0);
}

/// `reiniciar` zera o histórico de erro e derivada de rumo.
void test_reiniciar_zera_historico_de_rumo() {
  ControleMovimento controle;
  controle.definirRumoAlvo(0);
  controle.calcular(leitura(LONGE, 84, 84), 100, -2000);
  TEST_ASSERT_TRUE(controle.correcaoRumo() != 0);

  controle.reiniciar(0);
  TEST_ASSERT_EQUAL_INT32(0, controle.desvioRumoMiligraus());
  TEST_ASSERT_EQUAL_INT16(0, controle.correcaoRumo());
  TEST_ASSERT_FALSE(controle.temRumoValido());
}

/// Chamada com LeituraGiro integrada (com indicador de validade).
void test_atualizar_com_leitura_giro() {
  ControleMovimento controle;
  SensorDistanciaSimulado sensorSimulado;
  sensorSimulado.definirLeituraFixa(leitura(LONGE, 84, 84));
  MotoresSimulados motores;

  LeituraGiro giro;
  giro.valida = true;
  giro.anguloMiligraus = -2000;

  controle.atualizar(sensorSimulado, motores, 100, giro);
  TEST_ASSERT_TRUE(controle.temRumoValido());
  TEST_ASSERT_EQUAL_INT32(2000, controle.desvioRumoMiligraus());

  // Se a leitura for inválida, não aplica correção de rumo
  giro.valida = false;
  controle.atualizar(sensorSimulado, motores, 100, giro);
  TEST_ASSERT_FALSE(controle.temRumoValido());
  TEST_ASSERT_EQUAL_INT16(0, controle.correcaoRumo());
}

/// Teste de integração: sequência de avanço com desaceleração e parada na parede frontal.
void test_integracao_avanco_e_parada_na_parede_com_rumo() {
  ControleMovimento controle;
  controle.definirRumoAlvo(0);

  // Inicia distante da parede (3 células livres = 630 mm) e com desvio de 1° à direita
  PotenciaRodas pot = controle.calcular(leitura(630, 84, 84), 100, -1000);
  TEST_ASSERT_EQUAL(EstadoMovimento::Cruzeiro, controle.estado());
  TEST_ASSERT_TRUE(pot.direita > pot.esquerda);

  // Aproximando da parede (rampa de desaceleração)
  pot = controle.calcular(leitura(150, 84, 84), 100, 0);
  TEST_ASSERT_EQUAL(EstadoMovimento::Desacelerando, controle.estado());

  // Chegando à distância de parada (84 mm)
  pot = controle.calcular(leitura(84, 84, 84), 100, 0);
  TEST_ASSERT_EQUAL(EstadoMovimento::ParadoNaParede, controle.estado());
  TEST_ASSERT_EQUAL_INT16(0, pot.esquerda);
  TEST_ASSERT_EQUAL_INT16(0, pot.direita);
}

int main() {
  UNITY_BEGIN();

  // ControleGiro (giros de 90° e 180° em malha fechada)
  RUN_TEST(test_giro_90_esquerda_conclui_na_tolerancia);
  RUN_TEST(test_giro_90_direita_conclui_na_tolerancia);
  RUN_TEST(test_giro_180_esquerda_conclui_na_tolerancia);
  RUN_TEST(test_giro_180_direita_conclui_na_tolerancia);
  RUN_TEST(test_tolerancia_configuravel_afeta_precisao);
  RUN_TEST(test_deslizamento_de_rodas_compensa_e_atinge_90_graus);
  RUN_TEST(test_giro_com_giroscopio_simulado_vies_e_calibracao);
  RUN_TEST(test_rampa_de_potencia_reduz_ao_aproximar_do_alvo);
  RUN_TEST(test_overshoot_inverte_potencia_para_corrigir);
  RUN_TEST(test_timeout_em_caso_de_bloqueio);
  RUN_TEST(test_cancelar_giro);
  RUN_TEST(test_leitura_giro_invalida_segura_motores);

  // ControleMovimento (retidão do avanço e correção de rumo)
  RUN_TEST(test_avanco_sem_desvio_de_rumo);
  RUN_TEST(test_avanco_desvio_direita_vira_para_esquerda);
  RUN_TEST(test_avanco_desvio_esquerda_vira_para_direita);
  RUN_TEST(test_avanco_correcao_proporcional_ao_desvio);
  RUN_TEST(test_avanco_sem_paredes_laterais_mantem_linha_reta_por_giroscopio);
  RUN_TEST(test_avanco_compensa_deslizamento_assimetrico);
  RUN_TEST(test_avanco_derivada_amortece_rotacao_rapida);
  RUN_TEST(test_avanco_saturacao_da_correcao_de_rumo);
  RUN_TEST(test_avanco_respeita_limite_percentual_da_base);
  RUN_TEST(test_definir_rumo_alvo_diferentes_direcoes);
  RUN_TEST(test_reiniciar_zera_historico_de_rumo);
  RUN_TEST(test_atualizar_com_leitura_giro);
  RUN_TEST(test_integracao_avanco_e_parada_na_parede_com_rumo);

  return UNITY_END();
}
