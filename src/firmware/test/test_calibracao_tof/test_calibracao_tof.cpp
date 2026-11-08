/// @file test_calibracao_tof.cpp
/// @brief Testes do `CalibradorTof`: desvio por sensor, aplicação e recusa por movimento (#240).
/// Roda no computador: `pio test -e native -f test_calibracao_tof`.

#include <stdint.h>
#include <unity.h>

#include "CalibradorTof.h"
#include "SensorDistanciaSimulado.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

ConfiguracaoCalibracaoTof configRapido() {
  ConfiguracaoCalibracaoTof c;
  c.amostras = 4;
  c.variacaoMaximaMm = 8;
  c.referencias = {54, 54, 54};
  return c;
}

void alimentar(CalibradorTof& calibrador, const DistanciasLaterais& leitura, uint16_t vezes) {
  for (uint16_t i = 0; i < vezes; ++i) {
    calibrador.adicionarAmostra(leitura);
  }
}

}  // namespace

void test_comeca_nao_calibrado_e_desvios_zero() {
  CalibradorTof calibrador(configRapido());
  TEST_ASSERT_EQUAL(static_cast<int>(EstadoCalibracaoTof::NaoCalibrado),
                    static_cast<int>(calibrador.estado()));
  TEST_ASSERT_EQUAL_INT16(0, calibrador.desvios().frenteMm);
  TEST_ASSERT_EQUAL_INT16(0, calibrador.desvios().esquerdaMm);
  TEST_ASSERT_EQUAL_INT16(0, calibrador.desvios().direitaMm);
}

void test_calibracao_guarda_desvio_por_sensor() {
  CalibradorTof calibrador(configRapido());
  calibrador.iniciar();
  // Sensores leem 60 / 50 / 58 mm; referência 54 → desvios +6 / −4 / +4.
  alimentar(calibrador, {60, 50, 58}, 4);

  TEST_ASSERT_EQUAL(static_cast<int>(EstadoCalibracaoTof::Calibrado),
                    static_cast<int>(calibrador.estado()));
  TEST_ASSERT_EQUAL_INT16(6, calibrador.desvios().frenteMm);
  TEST_ASSERT_EQUAL_INT16(-4, calibrador.desvios().esquerdaMm);
  TEST_ASSERT_EQUAL_INT16(4, calibrador.desvios().direitaMm);
}

void test_media_arredonda_para_o_inteiro_mais_proximo() {
  ConfiguracaoCalibracaoTof c = configRapido();
  c.amostras = 3;
  CalibradorTof calibrador(c);
  calibrador.iniciar({50, 50, 50});
  calibrador.adicionarAmostra({51, 50, 50});
  calibrador.adicionarAmostra({52, 50, 50});
  calibrador.adicionarAmostra({52, 50, 50});  // média frente = 51,67 → 52; desvio = 2

  TEST_ASSERT_EQUAL(static_cast<int>(EstadoCalibracaoTof::Calibrado),
                    static_cast<int>(calibrador.estado()));
  TEST_ASSERT_EQUAL_INT16(2, calibrador.desvios().frenteMm);
}

void test_corrigir_aplica_desvio_nas_leituras() {
  CalibradorTof calibrador(configRapido());
  calibrador.iniciar();
  alimentar(calibrador, {60, 50, 58}, 4);

  const DistanciasLaterais corrigida = calibrador.corrigir({120, 90, 100});
  TEST_ASSERT_EQUAL_UINT16(114, corrigida.frenteMm);    // 120 − 6
  TEST_ASSERT_EQUAL_UINT16(94, corrigida.esquerdaMm);   // 90 − (−4)
  TEST_ASSERT_EQUAL_UINT16(96, corrigida.direitaMm);    // 100 − 4
}

void test_corrigir_satura_em_zero_quando_desvio_e_maior_que_a_leitura() {
  CalibradorTof calibrador(configRapido());
  calibrador.iniciar();
  alimentar(calibrador, {70, 54, 54}, 4);  // desvio frente = +16

  const DistanciasLaterais corrigida = calibrador.corrigir({10, 54, 54});
  TEST_ASSERT_EQUAL_UINT16(0, corrigida.frenteMm);
}

void test_recusa_se_algum_lado_variar_demais() {
  ConfiguracaoCalibracaoTof c = configRapido();
  c.variacaoMaximaMm = 5;
  CalibradorTof calibrador(c);
  calibrador.iniciar();
  calibrador.adicionarAmostra({54, 54, 54});
  calibrador.adicionarAmostra({54, 54, 54});
  calibrador.adicionarAmostra({54, 54, 54});
  calibrador.adicionarAmostra({62, 54, 54});  // frente variou 8 mm > 5

  TEST_ASSERT_EQUAL(static_cast<int>(EstadoCalibracaoTof::Recusada),
                    static_cast<int>(calibrador.estado()));
  TEST_ASSERT_EQUAL_INT16(0, calibrador.desvios().frenteMm);  // desvio anterior (zero) mantido
}

void test_recusa_mantem_desvios_da_calibracao_anterior() {
  CalibradorTof calibrador(configRapido());
  calibrador.iniciar();
  alimentar(calibrador, {60, 50, 58}, 4);
  TEST_ASSERT_EQUAL_INT16(6, calibrador.desvios().frenteMm);

  // Reinicia e força variação acima do limite (54→63 = 9 mm > 8 mm): desvios anteriores ficam.
  calibrador.iniciar();
  calibrador.adicionarAmostra({54, 54, 54});
  calibrador.adicionarAmostra({54, 54, 54});
  calibrador.adicionarAmostra({54, 54, 54});
  calibrador.adicionarAmostra({63, 54, 54});

  TEST_ASSERT_EQUAL(static_cast<int>(EstadoCalibracaoTof::Recusada),
                    static_cast<int>(calibrador.estado()));
  TEST_ASSERT_EQUAL_INT16(6, calibrador.desvios().frenteMm);
  TEST_ASSERT_EQUAL_INT16(-4, calibrador.desvios().esquerdaMm);
  TEST_ASSERT_EQUAL_INT16(4, calibrador.desvios().direitaMm);
}

void test_amostra_fora_de_calibrando_e_ignorada() {
  CalibradorTof calibrador(configRapido());
  TEST_ASSERT_EQUAL(static_cast<int>(EstadoCalibracaoTof::NaoCalibrado),
                    static_cast<int>(calibrador.adicionarAmostra({60, 60, 60})));
  TEST_ASSERT_EQUAL_INT16(0, calibrador.desvios().frenteMm);
}

void test_orcamento_tof_cabe_no_rnf08() {
  // 10 amostras × 33 ms ≈ 330 ms < 400 ms de fatia ToF < 1000 ms total.
  TEST_ASSERT_TRUE(AMOSTRAS_CALIBRACAO_TOF * 33U <= ORCAMENTO_CALIBRACAO_TOF_MS);
  TEST_ASSERT_TRUE(ORCAMENTO_CALIBRACAO_TOF_MS < ORCAMENTO_CALIBRACAO_MS);
}

void test_com_leituras_simuladas_do_sensor() {
  SensorDistanciaSimulado sensorSimulado;
  sensorSimulado.definirLeituraFixa({60, 50, 58});

  CalibradorTof calibrador(configRapido());
  calibrador.iniciar({54, 54, 54});

  DistanciasLaterais leitura;
  for (uint16_t i = 0; i < 4; ++i) {
    TEST_ASSERT_TRUE(sensorSimulado.lerDistanciasLaterais(leitura));
    calibrador.adicionarAmostra(leitura);
  }

  TEST_ASSERT_EQUAL(static_cast<int>(EstadoCalibracaoTof::Calibrado),
                    static_cast<int>(calibrador.estado()));
  const DistanciasLaterais corrigida = calibrador.corrigir(leitura);
  TEST_ASSERT_EQUAL_UINT16(54, corrigida.frenteMm);
  TEST_ASSERT_EQUAL_UINT16(54, corrigida.esquerdaMm);
  TEST_ASSERT_EQUAL_UINT16(54, corrigida.direitaMm);
}

void test_lado_sem_referencia_fica_fora_da_calibracao() {
  // Célula de partida: parede dos dois lados e a frente aberta (sem distância conhecida).
  ConfiguracaoCalibracaoTof c = configRapido();
  c.referencias = REFERENCIAS_TOF_NA_LARGADA;
  CalibradorTof calibrador(c);
  calibrador.iniciar();
  alimentar(calibrador, {600, 60, 50}, 4);  // frente aberta; esquerda +6, direita −4

  TEST_ASSERT_EQUAL(static_cast<int>(EstadoCalibracaoTof::Calibrado), static_cast<int>(calibrador.estado()));
  TEST_ASSERT_EQUAL_INT16(0, calibrador.desvios().frenteMm);
  TEST_ASSERT_EQUAL_INT16(6, calibrador.desvios().esquerdaMm);
  TEST_ASSERT_EQUAL_INT16(-4, calibrador.desvios().direitaMm);
  // A frente passa como veio: sem referência, nada a corrigir.
  TEST_ASSERT_EQUAL_UINT16(600, calibrador.corrigir({600, 60, 50}).frenteMm);
}

void test_variacao_do_lado_sem_referencia_nao_recusa_a_calibracao() {
  ConfiguracaoCalibracaoTof c = configRapido();
  c.referencias = REFERENCIAS_TOF_NA_LARGADA;
  CalibradorTof calibrador(c);
  calibrador.iniciar();
  const uint16_t frentes[4] = {300, 700, 450, 900};  // a frente oscila muito, mas não conta
  for (uint16_t frente : frentes) {
    calibrador.adicionarAmostra({frente, 54, 54});
  }
  TEST_ASSERT_EQUAL(static_cast<int>(EstadoCalibracaoTof::Calibrado), static_cast<int>(calibrador.estado()));
}

void test_desvio_acima_do_maximo_descarta_a_calibracao() {
  CalibradorTof calibrador(configRapido());
  calibrador.iniciar();
  alimentar(calibrador, {54, 54, 54}, 4);  // primeira calibração boa: desvios zero
  TEST_ASSERT_EQUAL(static_cast<int>(EstadoCalibracaoTof::Calibrado), static_cast<int>(calibrador.estado()));

  calibrador.iniciar();
  alimentar(calibrador, {54, 54 + DESVIO_MAXIMO_TOF_MM + 1, 54}, 4);  // referência errada: +31 mm

  TEST_ASSERT_EQUAL(static_cast<int>(EstadoCalibracaoTof::ForaDaFaixa), static_cast<int>(calibrador.estado()));
  TEST_ASSERT_EQUAL_INT16(0, calibrador.desvios().esquerdaMm);  // os desvios anteriores continuam
}

void test_desvio_exatamente_no_maximo_e_aceito_nos_dois_sinais() {
  CalibradorTof calibrador(configRapido());
  calibrador.iniciar();
  alimentar(calibrador, {54 + DESVIO_MAXIMO_TOF_MM, 54 - DESVIO_MAXIMO_TOF_MM, 54}, 4);

  TEST_ASSERT_EQUAL(static_cast<int>(EstadoCalibracaoTof::Calibrado), static_cast<int>(calibrador.estado()));
  TEST_ASSERT_EQUAL_INT16(static_cast<int16_t>(DESVIO_MAXIMO_TOF_MM), calibrador.desvios().frenteMm);
  TEST_ASSERT_EQUAL_INT16(-static_cast<int16_t>(DESVIO_MAXIMO_TOF_MM), calibrador.desvios().esquerdaMm);
}

void test_desvio_negativo_acima_do_maximo_tambem_descarta() {
  CalibradorTof calibrador(configRapido());
  calibrador.iniciar();
  alimentar(calibrador, {54, 54, 54 - DESVIO_MAXIMO_TOF_MM - 1}, 4);
  TEST_ASSERT_EQUAL(static_cast<int>(EstadoCalibracaoTof::ForaDaFaixa), static_cast<int>(calibrador.estado()));
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_comeca_nao_calibrado_e_desvios_zero);
  RUN_TEST(test_calibracao_guarda_desvio_por_sensor);
  RUN_TEST(test_media_arredonda_para_o_inteiro_mais_proximo);
  RUN_TEST(test_corrigir_aplica_desvio_nas_leituras);
  RUN_TEST(test_corrigir_satura_em_zero_quando_desvio_e_maior_que_a_leitura);
  RUN_TEST(test_recusa_se_algum_lado_variar_demais);
  RUN_TEST(test_recusa_mantem_desvios_da_calibracao_anterior);
  RUN_TEST(test_amostra_fora_de_calibrando_e_ignorada);
  RUN_TEST(test_orcamento_tof_cabe_no_rnf08);
  RUN_TEST(test_com_leituras_simuladas_do_sensor);
  RUN_TEST(test_lado_sem_referencia_fica_fora_da_calibracao);
  RUN_TEST(test_variacao_do_lado_sem_referencia_nao_recusa_a_calibracao);
  RUN_TEST(test_desvio_acima_do_maximo_descarta_a_calibracao);
  RUN_TEST(test_desvio_exatamente_no_maximo_e_aceito_nos_dois_sinais);
  RUN_TEST(test_desvio_negativo_acima_do_maximo_tambem_descarta);
  return UNITY_END();
}
