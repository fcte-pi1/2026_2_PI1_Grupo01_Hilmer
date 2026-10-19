/// @file test_controle_movimento.cpp
/// @brief Testes da centralização no corredor e da parada frontal (#191, HU-12): o controlador
/// lateral, o perfil de parada, a combinação dos dois e uma simulação do robô no corredor.
/// Roda no computador: `pio test -e native -f test_controle_movimento`.

#include <unity.h>

#include <cmath>
#include <cstdint>
#include <initializer_list>

#include "ConfiguracaoControle.h"
#include "ControladorCentralizacao.h"
#include "ControleMovimento.h"
#include "IMotores.h"
#include "LidarSimulado.h"
#include "MotoresSimulados.h"
#include "PerfilParadaFrontal.h"
#include "SinaisDrv8833.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

constexpr uint16_t LONGE = 1000;  ///< Leitura frontal de um caminho livre.
constexpr uint16_t SEM_PAREDE = 400;  ///< Leitura lateral de um lado sem parede.

/// Leitura com as distâncias, em mm, à frente, à esquerda e à direita.
DistanciasLaterais leitura(uint16_t frente, uint16_t esquerda, uint16_t direita) {
  return DistanciasLaterais{frente, esquerda, direita};
}

}  // namespace

// ---------------------------------------------------------------- ControladorCentralizacao

/// Critério 1: centralizado entre as duas paredes (84 mm de cada lado), não há correção.
void test_centralizado_nao_corrige() {
  ControladorCentralizacao controlador;
  TEST_ASSERT_EQUAL_INT16(0, controlador.calcular(leitura(LONGE, 84, 84), 100));
  TEST_ASSERT_EQUAL(ModoCentralizacao::DuasParedes, controlador.modo());
  TEST_ASSERT_EQUAL_INT32(0, controlador.erroMm());
}

/// Critério 1: 10 mm para a direita (94 mm à esquerda, 74 à direita), a correção é positiva
/// (virar para a esquerda) e proporcional: 2,0 × 10 mm = 20.
void test_deslocado_para_a_direita_vira_para_a_esquerda() {
  ControladorCentralizacao controlador;
  TEST_ASSERT_EQUAL_INT16(20, controlador.calcular(leitura(LONGE, 94, 74), 100));
  TEST_ASSERT_EQUAL_INT32(10, controlador.erroMm());
}

/// Critério 1: o lado oposto dá a correção oposta.
void test_deslocado_para_a_esquerda_vira_para_a_direita() {
  ControladorCentralizacao controlador;
  TEST_ASSERT_EQUAL_INT16(-20, controlador.calcular(leitura(LONGE, 74, 94), 100));
  TEST_ASSERT_EQUAL_INT32(-10, controlador.erroMm());
}

/// Só com a parede da esquerda, o erro é a leitura menos a distância alvo (84 mm).
void test_so_parede_esquerda_usa_a_distancia_alvo() {
  ControladorCentralizacao controlador;
  TEST_ASSERT_EQUAL_INT16(20, controlador.calcular(leitura(LONGE, 94, SEM_PAREDE), 100));
  TEST_ASSERT_EQUAL(ModoCentralizacao::SoEsquerda, controlador.modo());
  TEST_ASSERT_EQUAL_INT32(10, controlador.erroMm());
}

/// Só com a parede da direita, o erro é a distância alvo menos a leitura.
void test_so_parede_direita_usa_a_distancia_alvo() {
  ControladorCentralizacao controlador;
  TEST_ASSERT_EQUAL_INT16(20, controlador.calcular(leitura(LONGE, SEM_PAREDE, 74), 100));
  TEST_ASSERT_EQUAL(ModoCentralizacao::SoDireita, controlador.modo());
}

/// Sem parede nenhuma ao lado não há referência: o robô segue reto.
void test_sem_parede_lateral_nao_corrige() {
  ControladorCentralizacao controlador;
  controlador.calcular(leitura(LONGE, 94, 74), 100);
  TEST_ASSERT_EQUAL_INT16(0, controlador.calcular(leitura(LONGE, SEM_PAREDE, SEM_PAREDE), 100));
  TEST_ASSERT_EQUAL(ModoCentralizacao::SemReferencia, controlador.modo());
  TEST_ASSERT_EQUAL_INT32(0, controlador.derivadaMmPorS());
}

/// A leitura no limiar (180 mm) ainda conta como parede; acima dele, não.
void test_limiar_de_parede_lateral() {
  ControladorCentralizacao controlador;
  controlador.calcular(leitura(LONGE, 180, 84), 100);
  TEST_ASSERT_EQUAL(ModoCentralizacao::DuasParedes, controlador.modo());
  controlador.calcular(leitura(LONGE, 181, 84), 100);
  TEST_ASSERT_EQUAL(ModoCentralizacao::SoDireita, controlador.modo());
}

/// A derivada reforça a correção quando o robô se aproxima da parede e a suaviza quando ele já
/// está voltando ao centro.
void test_derivada_amortece_a_aproximacao_e_o_retorno() {
  ControladorCentralizacao indo;
  indo.calcular(leitura(LONGE, 89, 79), 100);                     // 5 mm para a direita
  const int16_t aproximando = indo.calcular(leitura(LONGE, 94, 74), 100);  // 10 mm, indo para a parede
  TEST_ASSERT_GREATER_THAN_INT32(0, indo.derivadaMmPorS());
  TEST_ASSERT_GREATER_THAN_INT16(20, aproximando);

  ControladorCentralizacao voltando;
  voltando.calcular(leitura(LONGE, 99, 69), 100);                     // 15 mm para a direita
  const int16_t retornando = voltando.calcular(leitura(LONGE, 94, 74), 100);  // 10 mm, voltando
  TEST_ASSERT_LESS_THAN_INT32(0, voltando.derivadaMmPorS());
  TEST_ASSERT_LESS_THAN_INT16(20, retornando);
}

/// Quando a referência muda (uma parede lateral acaba), a derivada recomeça do zero, para não
/// reagir ao salto da leitura.
void test_troca_de_modo_zera_a_derivada() {
  ControladorCentralizacao controlador;
  controlador.calcular(leitura(LONGE, 84, 84), 100);
  controlador.calcular(leitura(LONGE, 94, 74), 100);
  TEST_ASSERT_NOT_EQUAL(0, controlador.derivadaMmPorS());
  controlador.calcular(leitura(LONGE, SEM_PAREDE, 74), 100);
  TEST_ASSERT_EQUAL(ModoCentralizacao::SoDireita, controlador.modo());
  TEST_ASSERT_EQUAL_INT32(0, controlador.derivadaMmPorS());
}

/// Com `dtMs` igual a zero a derivada não é atualizada (evita divisão por zero).
void test_dt_zero_nao_atualiza_a_derivada() {
  ControladorCentralizacao controlador;
  controlador.calcular(leitura(LONGE, 84, 84), 100);
  TEST_ASSERT_EQUAL_INT16(20, controlador.calcular(leitura(LONGE, 94, 74), 0));
  TEST_ASSERT_EQUAL_INT32(0, controlador.derivadaMmPorS());
}

/// A correção nunca passa de `correcaoMaxima`, nos dois sentidos.
void test_correcao_limitada() {
  ConfiguracaoControle config;
  config.correcaoMaxima = 30;
  ControladorCentralizacao controlador(config);
  TEST_ASSERT_EQUAL_INT16(30, controlador.calcular(leitura(LONGE, 140, 30), 100));
  controlador.reiniciar();
  TEST_ASSERT_EQUAL_INT16(-30, controlador.calcular(leitura(LONGE, 30, 140), 100));
}

// ---------------------------------------------------------------- PerfilParadaFrontal

/// Critério 2: com o caminho livre (inclusive no início da rampa, 264 mm), potência de cruzeiro.
void test_caminho_livre_usa_a_potencia_de_cruzeiro() {
  PerfilParadaFrontal perfil;
  TEST_ASSERT_EQUAL_INT16(400, perfil.potenciaBase(LONGE));
  TEST_ASSERT_EQUAL_INT16(400, perfil.potenciaBase(264));
  TEST_ASSERT_FALSE(perfil.deveParar(264));
}

/// Critério 2: entre 264 e 84 mm, a potência cai em rampa até a mínima (150).
void test_rampa_reduz_a_potencia_ate_a_minima() {
  PerfilParadaFrontal perfil;
  TEST_ASSERT_EQUAL_INT16(275, perfil.potenciaBase(174));  // metade da rampa
  TEST_ASSERT_EQUAL_INT16(151, perfil.potenciaBase(85));   // logo antes de parar
  int16_t anterior = perfil.potenciaBase(264);
  for (uint16_t frente = 263; frente > 84; --frente) {
    const int16_t atual = perfil.potenciaBase(frente);
    TEST_ASSERT_LESS_OR_EQUAL_INT16(anterior, atual);
    TEST_ASSERT_GREATER_OR_EQUAL_INT16(150, atual);
    anterior = atual;
  }
}

/// Critério 2: na distância de parada (84 mm, o centro da célula) ou menos, a potência é zero.
void test_para_na_distancia_de_parada() {
  PerfilParadaFrontal perfil;
  TEST_ASSERT_EQUAL_INT16(0, perfil.potenciaBase(84));
  TEST_ASSERT_EQUAL_INT16(0, perfil.potenciaBase(30));
  TEST_ASSERT_TRUE(perfil.deveParar(84));
}

/// Se o início da rampa não fica antes da parada, não há rampa: cruzeiro até parar.
void test_configuracao_sem_rampa() {
  ConfiguracaoControle config;
  config.distanciaInicioDesaceleracaoMm = config.distanciaParadaFrontalMm;
  PerfilParadaFrontal perfil(config);
  TEST_ASSERT_EQUAL_INT16(400, perfil.potenciaBase(85));
  TEST_ASSERT_EQUAL_INT16(0, perfil.potenciaBase(84));
}

// ---------------------------------------------------------------- ControleMovimento

/// A potência base vem da frente e a correção é tirada da roda esquerda e somada à direita.
void test_movimento_combina_base_e_correcao() {
  ControleMovimento controle;
  const DistanciasLaterais l = leitura(LONGE, 94, 74);
  const PotenciaRodas p = controle.calcular(&l, 100);
  TEST_ASSERT_EQUAL(EstadoMovimento::Cruzeiro, controle.estado());
  TEST_ASSERT_EQUAL_INT16(380, p.esquerda);
  TEST_ASSERT_EQUAL_INT16(420, p.direita);
}

/// Perto da parede, com pouca potência, a correção é limitada a 60% da base: nenhuma roda gira
/// para trás.
void test_correcao_limitada_pela_potencia_base() {
  ControleMovimento controle;
  const DistanciasLaterais l = leitura(90, 140, 30);
  const PotenciaRodas p = controle.calcular(&l, 100);
  TEST_ASSERT_EQUAL(EstadoMovimento::Desacelerando, controle.estado());
  TEST_ASSERT_GREATER_THAN_INT16(0, p.esquerda);
  TEST_ASSERT_GREATER_THAN_INT16(p.esquerda, p.direita);
  TEST_ASSERT_LESS_OR_EQUAL_INT16(158 * 160 / 100, p.direita);  // base 158 + 60%
}

/// Critério 2: ao chegar à distância de parada, para e continua parado até `reiniciar()`.
void test_parada_trava_ate_reiniciar() {
  ControleMovimento controle;
  const DistanciasLaterais perto = leitura(80, 84, 84);
  const DistanciasLaterais livre = leitura(LONGE, 84, 84);
  PotenciaRodas p = controle.calcular(&perto, 100);
  TEST_ASSERT_EQUAL(EstadoMovimento::ParadoNaParede, controle.estado());
  TEST_ASSERT_TRUE(controle.parado());
  TEST_ASSERT_EQUAL_INT16(0, p.esquerda);
  TEST_ASSERT_EQUAL_INT16(0, p.direita);

  p = controle.calcular(&livre, 100);
  TEST_ASSERT_EQUAL_INT16(0, p.direita);
  TEST_ASSERT_TRUE(controle.parado());

  controle.reiniciar();
  p = controle.calcular(&livre, 100);
  TEST_ASSERT_EQUAL(EstadoMovimento::Cruzeiro, controle.estado());
  TEST_ASSERT_EQUAL_INT16(400, p.direita);
}

/// Fail-safe: sem leitura do LiDAR, segue reto na potência mínima; depois de 500 ms, freia e
/// continua parado mesmo que a leitura volte.
void test_sem_leitura_segue_reto_e_depois_para() {
  ControleMovimento controle;
  for (int i = 0; i < 4; ++i) {
    const PotenciaRodas p = controle.calcular(nullptr, 100);
    TEST_ASSERT_EQUAL(EstadoMovimento::SemLeitura, controle.estado());
    TEST_ASSERT_EQUAL_INT16(150, p.esquerda);
    TEST_ASSERT_EQUAL_INT16(150, p.direita);
  }
  const PotenciaRodas p = controle.calcular(nullptr, 100);
  TEST_ASSERT_EQUAL(EstadoMovimento::ParadoSemLeitura, controle.estado());
  TEST_ASSERT_EQUAL_INT16(0, p.esquerda);

  const DistanciasLaterais livre = leitura(LONGE, 84, 84);
  controle.calcular(&livre, 100);
  TEST_ASSERT_TRUE(controle.parado());
}

/// Uma leitura válida no meio zera a contagem do tempo sem leitura.
void test_leitura_valida_zera_o_tempo_sem_leitura() {
  ControleMovimento controle;
  const DistanciasLaterais livre = leitura(LONGE, 84, 84);
  for (int i = 0; i < 3; ++i) controle.calcular(nullptr, 100);
  controle.calcular(&livre, 100);
  for (int i = 0; i < 3; ++i) controle.calcular(nullptr, 100);
  TEST_ASSERT_EQUAL(EstadoMovimento::SemLeitura, controle.estado());
}

/// `atualizar` lê o LiDAR pela interface e comanda os motores: potência enquanto anda, freio
/// quando para.
void test_atualizar_le_o_lidar_e_comanda_os_motores() {
  LidarSimulado lidar;
  MotoresSimulados motores;
  ControleMovimento controle;

  lidar.definirLeituraFixa(leitura(LONGE, 94, 74));
  TEST_ASSERT_EQUAL(EstadoMovimento::Cruzeiro, controle.atualizar(lidar, motores, 100));
  TEST_ASSERT_FALSE(motores.freados());
  TEST_ASSERT_EQUAL_INT16(380, motores.ultimaPotencia().esquerda);
  TEST_ASSERT_EQUAL_INT16(420, motores.ultimaPotencia().direita);

  lidar.definirLeituraFixa(leitura(80, 84, 84));
  TEST_ASSERT_EQUAL(EstadoMovimento::ParadoNaParede, controle.atualizar(lidar, motores, 100));
  TEST_ASSERT_TRUE(motores.freados());

  lidar.simularFalha(true);
  controle.reiniciar();
  TEST_ASSERT_EQUAL(EstadoMovimento::SemLeitura, controle.atualizar(lidar, motores, 100));
  TEST_ASSERT_EQUAL_INT16(150, motores.ultimaPotencia().direita);
  TEST_ASSERT_EQUAL_UINT32(3, motores.comandos());
}

/// Os motores simulados limitam a potência à faixa permitida, como o driver real.
void test_potencia_limitada_a_faixa_permitida() {
  MotoresSimulados motores;
  IMotores& tracao = motores;
  tracao.definirPotencia({1500, -1500});
  TEST_ASSERT_EQUAL_INT16(POTENCIA_MAXIMA, motores.ultimaPotencia().esquerda);
  TEST_ASSERT_EQUAL_INT16(-POTENCIA_MAXIMA, motores.ultimaPotencia().direita);
  TEST_ASSERT_EQUAL_INT16(123, limitarPotencia(123));
}

// ---------------------------------------------------------------- Sinais do DRV8833

/// Potência positiva: PWM em IN1 e IN2 em zero (avança). 500 de 1000 dá metade de 1023.
void test_drv8833_potencia_positiva_avanca() {
  const SinaisPonteH s = calcularSinaisDrv8833(500, false, 1023);
  TEST_ASSERT_EQUAL_UINT32(511, s.in1);
  TEST_ASSERT_EQUAL_UINT32(0, s.in2);
}

/// Potência negativa: IN1 em zero e PWM em IN2 (recua).
void test_drv8833_potencia_negativa_recua() {
  const SinaisPonteH s = calcularSinaisDrv8833(-500, false, 1023);
  TEST_ASSERT_EQUAL_UINT32(0, s.in1);
  TEST_ASSERT_EQUAL_UINT32(511, s.in2);
}

/// Potência zero: as duas entradas em zero (roda livre).
void test_drv8833_potencia_zero_deixa_a_roda_livre() {
  const SinaisPonteH s = calcularSinaisDrv8833(0, false, 1023);
  TEST_ASSERT_EQUAL_UINT32(0, s.in1);
  TEST_ASSERT_EQUAL_UINT32(0, s.in2);
}

/// O motor espelhado é invertido: a mesma potência positiva sai em IN2.
void test_drv8833_motor_invertido_troca_o_sentido() {
  const SinaisPonteH s = calcularSinaisDrv8833(500, true, 1023);
  TEST_ASSERT_EQUAL_UINT32(0, s.in1);
  TEST_ASSERT_EQUAL_UINT32(511, s.in2);
}

/// Potência acima do máximo é limitada a 100%, nos dois sentidos.
void test_drv8833_potencia_limitada_a_100_por_cento() {
  TEST_ASSERT_EQUAL_UINT32(1023, calcularSinaisDrv8833(1500, false, 1023).in1);
  TEST_ASSERT_EQUAL_UINT32(1023, calcularSinaisDrv8833(-1500, false, 1023).in2);
  TEST_ASSERT_EQUAL_UINT32(1023, calcularSinaisDrv8833(POTENCIA_MAXIMA, false, 1023).in1);
}

/// Freio: as duas entradas em nível alto.
void test_drv8833_freio_liga_as_duas_entradas() {
  const SinaisPonteH s = sinaisFreioDrv8833(1023);
  TEST_ASSERT_EQUAL_UINT32(1023, s.in1);
  TEST_ASSERT_EQUAL_UINT32(1023, s.in2);
}

// ---------------------------------------------------------------- Simulação do corredor

namespace {

/// Robô de tração diferencial num corredor reto com parede frontal, para conferir o
/// critério 3 (nenhum contato do chassi com as paredes) antes da bancada.
///
/// Modelo: cada roda anda `mmPorSPorPotencia` × potência, a `bitolaMm` uma da outra. O LiDAR
/// fica no centro do robô e lê a 10 Hz (a cada 100 ms); o movimento é integrado a cada 10 ms.
struct CenarioCorredor {
  double deslocamentoInicialMm;  ///< Para a direita do centro (negativo: esquerda).
  double anguloInicialGraus;     ///< Positivo: virado para a direita.
  double ruidoMm;                ///< Amplitude do ruído somado a cada leitura.
  double mmPorSPorPotencia;
  double bitolaMm;
  double fimParedeEsquerdaMm;    ///< A parede da esquerda acaba neste ponto do corredor.
};

struct ResultadoCorredor {
  bool parou;
  double menorFolgaLateralMm;  ///< Menor distância entre o chassi e uma parede lateral.
  double folgaFrontalMm;       ///< Distância entre o chassi e a parede da frente, ao parar.
  double deslocamentoFinalMm;
};

uint32_t semente = 1;

double ruido(double amplitude) {
  semente = semente * 1103515245u + 12345u;
  return amplitude * (((semente >> 8) & 0xFFFF) / 32767.5 - 1.0);
}

ResultadoCorredor simularCorredor(const CenarioCorredor& c) {
  const double meio = DISTANCIA_CENTRO_A_PAREDE_MM;
  const double paredeFrontalMm = 3 * TAMANHO_CELULA_MM + meio;  // 3 células à frente
  ControleMovimento controle;
  semente = 2024;

  double y = c.deslocamentoInicialMm;
  double angulo = c.anguloInicialGraus * M_PI / 180.0;
  double x = 0;
  PotenciaRodas potencia = {0, 0};
  ResultadoCorredor r = {false, 1e9, 0, 0};

  for (int passo = 0; passo < 6000; ++passo) {
    if (passo % 10 == 0) {
      const double cosseno = std::cos(angulo);
      const double esquerda = x < c.fimParedeEsquerdaMm ? (meio + y) / cosseno : SEM_PAREDE;
      const double frente = (paredeFrontalMm - x) / cosseno;
      const DistanciasLaterais l = {
          static_cast<uint16_t>(std::lround(std::fmax(0.0, frente + ruido(c.ruidoMm)))),
          static_cast<uint16_t>(std::lround(esquerda + ruido(c.ruidoMm))),
          static_cast<uint16_t>(std::lround((meio - y) / cosseno + ruido(c.ruidoMm)))};
      potencia = controle.calcular(&l, 100);
      if (controle.parado()) {
        r.parou = true;
        break;
      }
    }
    const double ve = potencia.esquerda * c.mmPorSPorPotencia;
    const double vd = potencia.direita * c.mmPorSPorPotencia;
    const double dt = 0.01;
    angulo += (ve - vd) / c.bitolaMm * dt;
    x += (ve + vd) / 2 * std::cos(angulo) * dt;
    y += (ve + vd) / 2 * std::sin(angulo) * dt;

    const double alcanceLateral = std::fabs(y) + MEIO_COMPRIMENTO_CHASSI_MM * std::fabs(std::sin(angulo)) +
                                  MEIA_LARGURA_CHASSI_MM * std::cos(angulo);
    r.menorFolgaLateralMm = std::fmin(r.menorFolgaLateralMm, meio - alcanceLateral);
  }

  r.folgaFrontalMm = paredeFrontalMm - (x + MEIO_COMPRIMENTO_CHASSI_MM * std::cos(angulo) +
                                        MEIA_LARGURA_CHASSI_MM * std::fabs(std::sin(angulo)));
  r.deslocamentoFinalMm = y;
  return r;
}

void conferirCorredor(const CenarioCorredor& c) {
  const ResultadoCorredor r = simularCorredor(c);
  TEST_ASSERT_TRUE_MESSAGE(r.parou, "o robô não parou antes da parede da frente");
  TEST_ASSERT_TRUE_MESSAGE(r.menorFolgaLateralMm > 3.0, "o chassi chegou a menos de 3 mm da parede lateral");
  TEST_ASSERT_TRUE_MESSAGE(r.folgaFrontalMm > 20.0, "o chassi chegou a menos de 20 mm da parede da frente");
  TEST_ASSERT_TRUE_MESSAGE(std::fabs(r.deslocamentoFinalMm) < 6.0, "o robô não voltou para o centro");
}

}  // namespace

/// Critério 3 (simulado): saindo 15 mm fora do centro, para cada lado, o robô volta ao centro
/// sem encostar e para no centro da última célula.
void test_corredor_saindo_fora_do_centro_nunca_encosta() {
  for (double deslocamento : {-15.0, -10.0, 10.0, 15.0}) {
    conferirCorredor({deslocamento, 0, 0, 0.3, 100, 1e9});
  }
}

/// Critério 3 (simulado): saindo torto (até 3° para cada lado), também não encosta.
void test_corredor_saindo_torto_nunca_encosta() {
  for (double angulo : {-3.0, 3.0}) {
    conferirCorredor({0, angulo, 0, 0.3, 100, 1e9});
    conferirCorredor({12, angulo, 0, 0.3, 100, 1e9});
  }
}

/// Critério 3 (simulado): com ruído de ±6 mm no LiDAR e com motores mais lentos ou mais rápidos
/// e bitolas diferentes das provisórias, o controle continua sem encostar.
void test_corredor_com_ruido_e_parametros_diferentes_nunca_encosta() {
  for (double velocidade : {0.2, 0.3, 0.45}) {
    for (double bitola : {80.0, 100.0, 120.0}) {
      conferirCorredor({15, 3, 6, velocidade, bitola, 1e9});
    }
  }
}

/// Critério 3 (simulado): a parede da esquerda acaba depois da primeira célula; o robô passa a
/// se guiar só pela direita sem desviar nem encostar.
void test_parede_lateral_que_termina_no_meio_do_corredor() {
  conferirCorredor({8, 0, 3, 0.3, 100, TAMANHO_CELULA_MM});
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_centralizado_nao_corrige);
  RUN_TEST(test_deslocado_para_a_direita_vira_para_a_esquerda);
  RUN_TEST(test_deslocado_para_a_esquerda_vira_para_a_direita);
  RUN_TEST(test_so_parede_esquerda_usa_a_distancia_alvo);
  RUN_TEST(test_so_parede_direita_usa_a_distancia_alvo);
  RUN_TEST(test_sem_parede_lateral_nao_corrige);
  RUN_TEST(test_limiar_de_parede_lateral);
  RUN_TEST(test_derivada_amortece_a_aproximacao_e_o_retorno);
  RUN_TEST(test_troca_de_modo_zera_a_derivada);
  RUN_TEST(test_dt_zero_nao_atualiza_a_derivada);
  RUN_TEST(test_correcao_limitada);
  RUN_TEST(test_caminho_livre_usa_a_potencia_de_cruzeiro);
  RUN_TEST(test_rampa_reduz_a_potencia_ate_a_minima);
  RUN_TEST(test_para_na_distancia_de_parada);
  RUN_TEST(test_configuracao_sem_rampa);
  RUN_TEST(test_movimento_combina_base_e_correcao);
  RUN_TEST(test_correcao_limitada_pela_potencia_base);
  RUN_TEST(test_parada_trava_ate_reiniciar);
  RUN_TEST(test_sem_leitura_segue_reto_e_depois_para);
  RUN_TEST(test_leitura_valida_zera_o_tempo_sem_leitura);
  RUN_TEST(test_atualizar_le_o_lidar_e_comanda_os_motores);
  RUN_TEST(test_potencia_limitada_a_faixa_permitida);
  RUN_TEST(test_drv8833_potencia_positiva_avanca);
  RUN_TEST(test_drv8833_potencia_negativa_recua);
  RUN_TEST(test_drv8833_potencia_zero_deixa_a_roda_livre);
  RUN_TEST(test_drv8833_motor_invertido_troca_o_sentido);
  RUN_TEST(test_drv8833_potencia_limitada_a_100_por_cento);
  RUN_TEST(test_drv8833_freio_liga_as_duas_entradas);
  RUN_TEST(test_corredor_saindo_fora_do_centro_nunca_encosta);
  RUN_TEST(test_corredor_saindo_torto_nunca_encosta);
  RUN_TEST(test_corredor_com_ruido_e_parametros_diferentes_nunca_encosta);
  RUN_TEST(test_parede_lateral_que_termina_no_meio_do_corredor);
  return UNITY_END();
}
