/// @file test_perfil_lateral.cpp
/// @brief Testes do `PerfilLateral`. Roda no computador: `pio test -e native -f test_perfil_lateral`.

#include <unity.h>

#include "PerfilLateral.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

static void registrar(PerfilLateral& p, uint16_t esq, uint16_t dir, int vezes) {
  for (int i = 0; i < vezes; i++) {
    p.registrar(DistanciasLaterais{500, esq, dir});
  }
}

void test_guarda_uma_amostra_a_cada_duas_leituras_ate_a_capacidade() {
  PerfilLateral p;
  registrar(p, 100, 100, 10);
  TEST_ASSERT_EQUAL_UINT8(5, p.quantidade());
  registrar(p, 100, 100, 200);
  TEST_ASSERT_EQUAL_UINT8(PerfilLateral::CAPACIDADE, p.quantidade());
  p.reiniciar();
  TEST_ASSERT_EQUAL_UINT8(0, p.quantidade());
}

void test_abertura_aparece_como_sequencia_longa() {
  PerfilLateral p;
  registrar(p, 110, 120, 12);   // parede dos dois lados
  registrar(p, 260, 120, 8);    // a esquerda abre
  registrar(p, 110, 120, 10);
  TEST_ASSERT_EQUAL_UINT8(4, p.maiorSequenciaAcima(true, 200));
  TEST_ASSERT_EQUAL_UINT8(0, p.maiorSequenciaAcima(false, 200));
}

void test_leitura_isolada_nao_forma_sequencia() {
  PerfilLateral p;
  registrar(p, 110, 120, 4);
  registrar(p, 300, 120, 2);  // uma amostra só
  registrar(p, 110, 120, 6);
  TEST_ASSERT_EQUAL_UINT8(1, p.maiorSequenciaAcima(true, 200));
}

void test_leitura_fora_de_alcance_e_limitada() {
  PerfilLateral p;
  registrar(p, 8190, 100, 2);
  TEST_ASSERT_EQUAL_UINT16(999, p.esquerda(0));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_guarda_uma_amostra_a_cada_duas_leituras_ate_a_capacidade);
  RUN_TEST(test_abertura_aparece_como_sequencia_longa);
  RUN_TEST(test_leitura_isolada_nao_forma_sequencia);
  RUN_TEST(test_leitura_fora_de_alcance_e_limitada);
  return UNITY_END();
}
