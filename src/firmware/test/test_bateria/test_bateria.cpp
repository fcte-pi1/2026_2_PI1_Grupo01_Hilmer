/// @file test_bateria.cpp
/// @brief Testes da leitura da bateria (`Bateria.h`): divisor, carga em % e o monitor de bateria fraca.
/// Roda no computador: `pio test -e native -f test_bateria`.

#include <unity.h>

#include "Bateria.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

void test_divisor_devolve_a_tensao_da_bateria() {
  // 8,4 V na bateria: 8400 x 33 / 133 = 2084 mV no pino (a conta do divisor 100 k / 33 k).
  TEST_ASSERT_UINT32_WITHIN(10, 8400, bateriaMvDoPino(2084));
  TEST_ASSERT_UINT32_WITHIN(10, 6600, bateriaMvDoPino(1639));
  TEST_ASSERT_EQUAL_UINT32(0, bateriaMvDoPino(0));
}

void test_percentual_entre_o_corte_e_a_carga_plena() {
  TEST_ASSERT_EQUAL_UINT8(0, bateriaPercentual(6000));
  TEST_ASSERT_EQUAL_UINT8(0, bateriaPercentual(BATERIA_CORTE_MV));
  TEST_ASSERT_EQUAL_UINT8(50, bateriaPercentual(7500));
  TEST_ASSERT_EQUAL_UINT8(100, bateriaPercentual(BATERIA_PLENA_MV));
  TEST_ASSERT_EQUAL_UINT8(100, bateriaPercentual(8600));
}

void test_monitor_comeca_sem_medida_e_avisa_cada_mudanca() {
  MonitorBateria monitor;
  TEST_ASSERT_EQUAL(SituacaoBateria::SemMedida, monitor.situacao());
  TEST_ASSERT_FALSE(monitor.atualizar(0));  // só USB: continua sem medida, sem aviso
  TEST_ASSERT_TRUE(monitor.atualizar(7800));
  TEST_ASSERT_EQUAL(SituacaoBateria::Ok, monitor.situacao());
  TEST_ASSERT_FALSE(monitor.atualizar(7700));  // mesma situação: sem aviso
  TEST_ASSERT_TRUE(monitor.atualizar(6500));
  TEST_ASSERT_EQUAL(SituacaoBateria::Fraca, monitor.situacao());
  TEST_ASSERT_EQUAL_STRING("fraca", nomeDaSituacaoBateria(monitor.situacao()));
}

void test_histerese_evita_o_aviso_piscar() {
  MonitorBateria monitor;
  monitor.atualizar(7800);
  monitor.atualizar(6500);  // fraca
  // O motor para, a tensão sobe um pouco acima do corte, mas dentro da histerese: continua fraca.
  TEST_ASSERT_FALSE(monitor.atualizar(BATERIA_CORTE_MV + 50));
  TEST_ASSERT_EQUAL(SituacaoBateria::Fraca, monitor.situacao());
  TEST_ASSERT_TRUE(monitor.atualizar(BATERIA_CORTE_MV + BATERIA_HISTERESE_MV));
  TEST_ASSERT_EQUAL(SituacaoBateria::Ok, monitor.situacao());
}

void test_chave_desligada_volta_a_sem_medida() {
  MonitorBateria monitor;
  monitor.atualizar(7800);
  TEST_ASSERT_TRUE(monitor.atualizar(30));
  TEST_ASSERT_EQUAL(SituacaoBateria::SemMedida, monitor.situacao());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_divisor_devolve_a_tensao_da_bateria);
  RUN_TEST(test_percentual_entre_o_corte_e_a_carga_plena);
  RUN_TEST(test_monitor_comeca_sem_medida_e_avisa_cada_mudanca);
  RUN_TEST(test_histerese_evita_o_aviso_piscar);
  RUN_TEST(test_chave_desligada_volta_a_sem_medida);
  return UNITY_END();
}
