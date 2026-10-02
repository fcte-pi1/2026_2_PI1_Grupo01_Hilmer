#include <unity.h>

#include "LidarSimulado.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

static DistanciasLaterais leitura(uint16_t frente, uint16_t esquerda, uint16_t direita) {
  return DistanciasLaterais{frente, esquerda, direita};
}

void test_sem_leitura_configurada_a_leitura_falha() {
  LidarSimulado lidar;
  DistanciasLaterais saida = {};
  TEST_ASSERT_FALSE(lidar.lerDistanciasLaterais(saida));
}

void test_leitura_fixa_se_repete_a_cada_chamada() {
  LidarSimulado lidar;
  lidar.definirLeituraFixa(leitura(90, 90, 270));
  DistanciasLaterais saida = {};
  for (int i = 0; i < 3; i++) {
    TEST_ASSERT_TRUE(lidar.lerDistanciasLaterais(saida));
    TEST_ASSERT_EQUAL_UINT16(90, saida.frenteMm);
    TEST_ASSERT_EQUAL_UINT16(90, saida.esquerdaMm);
    TEST_ASSERT_EQUAL_UINT16(270, saida.direitaMm);
  }
}

void test_fila_entrega_as_leituras_em_ordem_e_depois_volta_para_a_fixa() {
  LidarSimulado lidar;
  lidar.definirLeituraFixa(leitura(500, 500, 500));
  TEST_ASSERT_TRUE(lidar.enfileirarLeitura(leitura(100, 0, 0)));
  TEST_ASSERT_TRUE(lidar.enfileirarLeitura(leitura(200, 0, 0)));

  DistanciasLaterais saida = {};
  TEST_ASSERT_TRUE(lidar.lerDistanciasLaterais(saida));
  TEST_ASSERT_EQUAL_UINT16(100, saida.frenteMm);
  TEST_ASSERT_TRUE(lidar.lerDistanciasLaterais(saida));
  TEST_ASSERT_EQUAL_UINT16(200, saida.frenteMm);
  TEST_ASSERT_TRUE(lidar.lerDistanciasLaterais(saida));
  TEST_ASSERT_EQUAL_UINT16(500, saida.frenteMm);
}

void test_falha_simulada_retorna_false_e_nao_consome_a_fila() {
  LidarSimulado lidar;
  lidar.enfileirarLeitura(leitura(100, 100, 100));
  DistanciasLaterais saida = {};

  lidar.simularFalha(true);
  TEST_ASSERT_FALSE(lidar.lerDistanciasLaterais(saida));

  lidar.simularFalha(false);
  TEST_ASSERT_TRUE(lidar.lerDistanciasLaterais(saida));
  TEST_ASSERT_EQUAL_UINT16(100, saida.frenteMm);
}

void test_fila_cheia_recusa_novas_leituras() {
  LidarSimulado lidar;
  for (uint8_t i = 0; i < LidarSimulado::CAPACIDADE_FILA; i++) {
    TEST_ASSERT_TRUE(lidar.enfileirarLeitura(leitura(i, 0, 0)));
  }
  TEST_ASSERT_FALSE(lidar.enfileirarLeitura(leitura(999, 0, 0)));
}

void test_pode_ser_usado_pela_interface_ILidar() {
  LidarSimulado simulado;
  simulado.definirLeituraFixa(leitura(180, 180, 180));
  ILidar& lidar = simulado;
  DistanciasLaterais saida = {};
  TEST_ASSERT_TRUE(lidar.lerDistanciasLaterais(saida));
  TEST_ASSERT_EQUAL_UINT16(180, saida.frenteMm);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_sem_leitura_configurada_a_leitura_falha);
  RUN_TEST(test_leitura_fixa_se_repete_a_cada_chamada);
  RUN_TEST(test_fila_entrega_as_leituras_em_ordem_e_depois_volta_para_a_fixa);
  RUN_TEST(test_falha_simulada_retorna_false_e_nao_consome_a_fila);
  RUN_TEST(test_fila_cheia_recusa_novas_leituras);
  RUN_TEST(test_pode_ser_usado_pela_interface_ILidar);
  return UNITY_END();
}
