/// @file test_estimador_velocidade.cpp
/// @brief Testes do `EstimadorVelocidade`. Roda no computador: `pio test -e native -f test_estimador_velocidade`.

#include <unity.h>

#include "EstimadorVelocidade.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

void test_primeira_chamada_da_zero() {
  EstimadorVelocidade e;
  TEST_ASSERT_EQUAL_INT32(0, e.atualizar(500, 1000));
}

void test_distancia_sobre_o_tempo() {
  EstimadorVelocidade e;
  e.atualizar(0, 1000);
  TEST_ASSERT_EQUAL_INT32(180, e.atualizar(180, 2000));  // 180 mm em 1 s
  TEST_ASSERT_EQUAL_INT32(90, e.atualizar(225, 2500));   // 45 mm em 0,5 s
}

void test_parado_da_zero_e_re_conta_como_velocidade_escalar() {
  EstimadorVelocidade e;
  e.atualizar(100, 0);
  TEST_ASSERT_EQUAL_INT32(0, e.atualizar(100, 1000));
  TEST_ASSERT_EQUAL_INT32(50, e.atualizar(50, 2000));  // andou 50 mm de ré
}

void test_tempo_zerado_nao_divide_por_zero_e_o_relogio_pode_dar_a_volta() {
  EstimadorVelocidade e;
  e.atualizar(0, 1000);
  TEST_ASSERT_EQUAL_INT32(0, e.atualizar(10, 1000));
  e.atualizar(0, 0xFFFFFF00u);
  TEST_ASSERT_EQUAL_INT32(1000, e.atualizar(512, 0x00000100u));  // 512 mm em 512 ms, com o relógio dando a volta
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_primeira_chamada_da_zero);
  RUN_TEST(test_distancia_sobre_o_tempo);
  RUN_TEST(test_parado_da_zero_e_re_conta_como_velocidade_escalar);
  RUN_TEST(test_tempo_zerado_nao_divide_por_zero_e_o_relogio_pode_dar_a_volta);
  return UNITY_END();
}
