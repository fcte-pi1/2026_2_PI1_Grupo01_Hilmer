/// @file test_motores.cpp
/// @brief Testes dos motores N20 pelo DRV8833 (#341): o duty máximo configurável e o decorador
/// `MotoresComProtecao`, que freia o robô travado e recusa comandos até ser liberado.
/// Roda no computador: `pio test -e native -f test_motores`.

#include <unity.h>

#include <cstdint>

#include "ConfiguracaoMotores.h"
#include "ConfiguracaoProtecaoMotor.h"
#include "ControleMovimento.h"
#include "IMotores.h"
#include "SensorDistanciaSimulado.h"
#include "MotoresComProtecao.h"
#include "MotoresSimulados.h"
#include "SinaisDrv8833.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

constexpr uint32_t DT_MS = 100;                 ///< Ritmo das leituras do sensor de distância.
constexpr PotenciaRodas CRUZEIRO = {400, 400};  ///< Acima de `POTENCIA_ALTA_MINIMA`.

MedidasProtecao parado(uint16_t frenteMm = 500) { return MedidasProtecao{true, frenteMm, 0}; }

/// Roda a proteção por `ciclos` ciclos de `DT_MS`, com as mesmas medidas, e devolve o estado final.
EstadoProtecao rodar(MotoresComProtecao& motores, const MedidasProtecao& medidas, int ciclos) {
  EstadoProtecao estado = motores.estado();
  for (int i = 0; i < ciclos; ++i) {
    estado = motores.atualizarProtecao(medidas, DT_MS);
  }
  return estado;
}

}  // namespace

// ---------------------------------------------------------------- duty máximo (ConfiguracaoMotores.h)

void test_duty_total_pela_resolucao() {
  TEST_ASSERT_EQUAL_UINT32(1023, dutyTotalPwm(10));
  TEST_ASSERT_EQUAL_UINT32(255, dutyTotalPwm(8));
  TEST_ASSERT_EQUAL_UINT32(1023, DUTY_TOTAL_MOTORES);
}

void test_duty_maximo_e_a_fracao_configurada_do_total() {
  TEST_ASSERT_EQUAL_UINT32(1023, dutyMaximoMotores(10, 1000));
  TEST_ASSERT_EQUAL_UINT32(716, dutyMaximoMotores(10, 700));   // 1023 × 0,7 = 716,1: arredonda para baixo
  TEST_ASSERT_EQUAL_UINT32(511, dutyMaximoMotores(10, 500));
  TEST_ASSERT_EQUAL_UINT32(0, dutyMaximoMotores(10, 0));
  TEST_ASSERT_EQUAL_UINT32(1023, dutyMaximoMotores(10, 1500));  // acima de 100% vale 100%
}

void test_duty_maximo_em_uso_vem_da_configuracao() {
  TEST_ASSERT_EQUAL_UINT32(dutyMaximoMotores(RESOLUCAO_PWM_MOTORES_BITS, LIMITE_DUTY_MOTORES_MILESIMOS),
                           DUTY_MAXIMO_MOTORES);
  TEST_ASSERT_TRUE(DUTY_MAXIMO_MOTORES <= DUTY_TOTAL_MOTORES);
}

void test_potencia_maxima_vira_o_duty_maximo_e_nao_100_por_cento() {
  const SinaisPonteH frente = calcularSinaisDrv8833(POTENCIA_MAXIMA, false, DUTY_MAXIMO_MOTORES);
  TEST_ASSERT_EQUAL_UINT32(DUTY_MAXIMO_MOTORES, frente.in1);
  TEST_ASSERT_EQUAL_UINT32(0, frente.in2);
  const SinaisPonteH re = calcularSinaisDrv8833(-POTENCIA_MAXIMA, false, DUTY_MAXIMO_MOTORES);
  TEST_ASSERT_EQUAL_UINT32(0, re.in1);
  TEST_ASSERT_EQUAL_UINT32(DUTY_MAXIMO_MOTORES, re.in2);
}

void test_potencias_menores_sao_proporcionais_ao_duty_maximo() {
  // Metade da potência = metade do duty máximo (com 70%: 358 de 1023, e não 511).
  TEST_ASSERT_EQUAL_UINT32(DUTY_MAXIMO_MOTORES / 2, calcularSinaisDrv8833(500, false, DUTY_MAXIMO_MOTORES).in1);
  TEST_ASSERT_EQUAL_UINT32(0, calcularSinaisDrv8833(0, false, DUTY_MAXIMO_MOTORES).in1);
}

void test_nenhuma_potencia_passa_do_duty_maximo() {
  for (int32_t p = -1500; p <= 1500; p += 25) {
    const SinaisPonteH s = calcularSinaisDrv8833(static_cast<int16_t>(p), p % 2 == 0, DUTY_MAXIMO_MOTORES);
    TEST_ASSERT_TRUE(s.in1 <= DUTY_MAXIMO_MOTORES);
    TEST_ASSERT_TRUE(s.in2 <= DUTY_MAXIMO_MOTORES);
  }
}

void test_freio_usa_o_duty_total() {
  // O freio põe as duas entradas em nível alto: o limite do duty não vale para ele.
  const SinaisPonteH freio = sinaisFreioDrv8833(DUTY_TOTAL_MOTORES);
  TEST_ASSERT_EQUAL_UINT32(1023, freio.in1);
  TEST_ASSERT_EQUAL_UINT32(1023, freio.in2);
}

// ---------------------------------------------------------------- MotoresComProtecao

void test_comandos_chegam_ao_driver_limitados() {
  MotoresSimulados driver;
  MotoresComProtecao motores(driver);
  motores.definirPotencia({400, -300});
  TEST_ASSERT_EQUAL_INT16(400, driver.ultimaPotencia().esquerda);
  TEST_ASSERT_EQUAL_INT16(-300, driver.ultimaPotencia().direita);
  motores.definirPotencia({1500, -2000});
  TEST_ASSERT_EQUAL_INT16(POTENCIA_MAXIMA, motores.ultimaPotencia().esquerda);
  TEST_ASSERT_EQUAL_INT16(-POTENCIA_MAXIMA, motores.ultimaPotencia().direita);
  motores.parar();
  TEST_ASSERT_TRUE(driver.freados());
  TEST_ASSERT_EQUAL_INT16(0, motores.ultimaPotencia().esquerda);
}

void test_robo_travado_e_freado_no_tempo_limite() {
  MotoresSimulados driver;
  MotoresComProtecao motores(driver);
  motores.definirPotencia(CRUZEIRO);
  TEST_ASSERT_EQUAL(EstadoProtecao::Monitorando, rodar(motores, parado(), 4));
  TEST_ASSERT_FALSE(driver.freados());
  TEST_ASSERT_EQUAL(EstadoProtecao::Travado, rodar(motores, parado(), 1));  // 500 ms
  TEST_ASSERT_TRUE(driver.freados());
  TEST_ASSERT_TRUE(motores.travado());
  TEST_ASSERT_EQUAL_UINT32(1, motores.cortes());
  TEST_ASSERT_EQUAL_INT16(0, motores.ultimaPotencia().esquerda);
}

void test_travado_recusa_comandos_ate_liberar() {
  MotoresSimulados driver;
  MotoresComProtecao motores(driver);
  motores.definirPotencia(CRUZEIRO);
  rodar(motores, parado(), 5);
  TEST_ASSERT_TRUE(motores.travado());

  // O controle tenta religar o motor preso: o comando vira freio.
  motores.definirPotencia(CRUZEIRO);
  TEST_ASSERT_TRUE(driver.freados());
  TEST_ASSERT_EQUAL_INT16(0, driver.ultimaPotencia().esquerda);
  TEST_ASSERT_EQUAL_UINT32(1, motores.comandosRecusados());
  // Mais ciclos travados não contam um corte novo.
  rodar(motores, parado(), 10);
  TEST_ASSERT_EQUAL_UINT32(1, motores.cortes());

  motores.liberar();
  TEST_ASSERT_FALSE(motores.travado());
  motores.definirPotencia({300, 320});
  TEST_ASSERT_FALSE(driver.freados());
  TEST_ASSERT_EQUAL_INT16(320, driver.ultimaPotencia().direita);
}

void test_robo_andando_ou_girando_nao_e_freado() {
  MotoresSimulados driver;
  MotoresComProtecao motores(driver);
  motores.definirPotencia(CRUZEIRO);
  uint16_t frente = 800;
  for (int i = 0; i < 20; ++i) {  // anda 20 mm a cada 100 ms
    TEST_ASSERT_NOT_EQUAL(EstadoProtecao::Travado, motores.atualizarProtecao(parado(frente), DT_MS));
    frente = static_cast<uint16_t>(frente - 20);
  }
  motores.definirPotencia({400, -400});  // giro no lugar: o ToF frontal quase não muda, o giroscópio sim
  TEST_ASSERT_NOT_EQUAL(EstadoProtecao::Travado, rodar(motores, MedidasProtecao{true, 500, 900}, 20));
  TEST_ASSERT_FALSE(driver.freados());
  TEST_ASSERT_EQUAL_UINT32(0, motores.cortes());
}

void test_potencia_baixa_ou_sem_leitura_frontal_nao_freia() {
  MotoresSimulados driver;
  MotoresComProtecao motores(driver);
  motores.definirPotencia({200, 200});  // abaixo do duty alto: pode ser só a zona morta do N20
  TEST_ASSERT_EQUAL(EstadoProtecao::Normal, rodar(motores, parado(), 50));
  motores.definirPotencia(CRUZEIRO);    // sem ToF frontal válido não dá para concluir nada
  TEST_ASSERT_EQUAL(EstadoProtecao::Normal, rodar(motores, MedidasProtecao{false, 0, 0}, 50));
  TEST_ASSERT_FALSE(driver.freados());
}

void test_freio_comandado_zera_a_contagem() {
  MotoresSimulados driver;
  MotoresComProtecao motores(driver);
  motores.definirPotencia(CRUZEIRO);
  rodar(motores, parado(), 4);  // 400 ms em duty alto
  motores.parar();              // o controle parou de propósito
  TEST_ASSERT_EQUAL(EstadoProtecao::Normal, motores.atualizarProtecao(parado(), DT_MS));
  motores.definirPotencia(CRUZEIRO);  // religou: a contagem recomeça do zero
  TEST_ASSERT_EQUAL(EstadoProtecao::Monitorando, rodar(motores, parado(), 4));
}

void test_configuracao_da_protecao_passa_pelo_decorador() {
  ConfiguracaoProtecaoMotor rapida;
  rapida.tempoLimiteMs = 200;
  MotoresSimulados driver;
  MotoresComProtecao motores(driver, rapida);
  motores.definirPotencia(CRUZEIRO);
  TEST_ASSERT_EQUAL(EstadoProtecao::Travado, rodar(motores, parado(), 2));
}

// ---------------------------------------------------------------- integração com o controle de movimento

/// Robô preso numa parede falsa: o controle manda potência de cruzeiro pelo decorador, a leitura não
/// muda, e o decorador freia e segura o freio mesmo com o controle mandando andar de novo.
void test_integracao_controle_com_robo_preso() {
  SensorDistanciaSimulado sensorSimulado;
  MotoresSimulados driver;
  MotoresComProtecao motores(driver);
  ControleMovimento controle;
  sensorSimulado.definirLeituraFixa(DistanciasLaterais{600, 84, 84});  // centralizado, caminho livre

  int ciclos = 0;
  while (!motores.travado() && ciclos < 20) {
    controle.atualizar(sensorSimulado, motores, DT_MS);
    DistanciasLaterais leitura = {0, 0, 0};
    const bool valida = sensorSimulado.lerDistanciasLaterais(leitura);
    motores.atualizarProtecao(MedidasProtecao{valida, leitura.frenteMm, 0}, DT_MS);
    ++ciclos;
  }
  TEST_ASSERT_TRUE(motores.travado());
  TEST_ASSERT_EQUAL_INT(5, ciclos);  // 500 ms em ciclos de 100 ms
  TEST_ASSERT_TRUE(driver.freados());

  for (int i = 0; i < 5; ++i) {  // o controle continua mandando andar
    controle.atualizar(sensorSimulado, motores, DT_MS);
  }
  TEST_ASSERT_TRUE(driver.freados());
  TEST_ASSERT_EQUAL_UINT32(5, motores.comandosRecusados());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_duty_total_pela_resolucao);
  RUN_TEST(test_duty_maximo_e_a_fracao_configurada_do_total);
  RUN_TEST(test_duty_maximo_em_uso_vem_da_configuracao);
  RUN_TEST(test_potencia_maxima_vira_o_duty_maximo_e_nao_100_por_cento);
  RUN_TEST(test_potencias_menores_sao_proporcionais_ao_duty_maximo);
  RUN_TEST(test_nenhuma_potencia_passa_do_duty_maximo);
  RUN_TEST(test_freio_usa_o_duty_total);
  RUN_TEST(test_comandos_chegam_ao_driver_limitados);
  RUN_TEST(test_robo_travado_e_freado_no_tempo_limite);
  RUN_TEST(test_travado_recusa_comandos_ate_liberar);
  RUN_TEST(test_robo_andando_ou_girando_nao_e_freado);
  RUN_TEST(test_potencia_baixa_ou_sem_leitura_frontal_nao_freia);
  RUN_TEST(test_freio_comandado_zera_a_contagem);
  RUN_TEST(test_configuracao_da_protecao_passa_pelo_decorador);
  RUN_TEST(test_integracao_controle_com_robo_preso);
  return UNITY_END();
}
