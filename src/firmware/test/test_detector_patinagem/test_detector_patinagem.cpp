#include <unity.h>

#include "DetectorPatinagem.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

// Avanço normal a 0,3 m/s: a odometria sobe e o ToF frontal cai na mesma proporção.
void test_avanco_normal_nao_e_patinagem() {
  DetectorPatinagem d;
  int64_t pos = 0;
  uint16_t frente = 600;
  bool detectou = false;
  for (int i = 0; i < 300; i++) {  // 3 s a 10 ms
    pos += 3000;                   // 3 mm por ciclo (0,3 m/s)
    if (i % 10 == 0 && frente > 100) frente = static_cast<uint16_t>(frente - 30);
    detectou = detectou || d.atualizar(true, pos, true, frente, 10);
  }
  TEST_ASSERT_FALSE(detectou);
  TEST_ASSERT_EQUAL_UINT32(0, d.deteccoes());
}

// Roda no buraco: a odometria anda, o ToF fica em ~136 mm.
void test_odometria_andando_com_o_tof_parado_e_patinagem() {
  DetectorPatinagem d;
  int64_t pos = 0;
  bool detectou = false;
  uint32_t cicloDaDeteccao = 0;
  for (uint32_t i = 0; i < 400 && !detectou; i++) {
    pos += 3000;
    detectou = d.atualizar(true, pos, true, static_cast<uint16_t>(136 + (i % 3)), 10);
    cicloDaDeteccao = i;
  }
  TEST_ASSERT_TRUE(detectou);
  TEST_ASSERT_TRUE(cicloDaDeteccao >= 140 && cicloDaDeteccao <= 160);  // ~1,5 s
  TEST_ASSERT_EQUAL_UINT32(1, d.deteccoes());
}

void test_robo_parado_nao_e_patinagem() {
  DetectorPatinagem d;
  bool detectou = false;
  for (int i = 0; i < 400; i++) {
    detectou = detectou || d.atualizar(true, 0, true, 136, 10);  // a odometria também não anda
  }
  TEST_ASSERT_FALSE(detectou);
}

void test_tof_longe_demais_nao_vale() {
  DetectorPatinagem d;
  int64_t pos = 0;
  bool detectou = false;
  for (int i = 0; i < 400; i++) {
    pos += 3000;
    detectou = detectou || d.atualizar(true, pos, true, 1200, 10);  // fora do alcance: não varia com o avanço
  }
  TEST_ASSERT_FALSE(detectou);
}

void test_fora_do_avanco_reinicia_a_janela() {
  DetectorPatinagem d;
  int64_t pos = 0;
  bool detectou = false;
  for (int i = 0; i < 400; i++) {
    pos += 3000;
    const bool ativo = (i % 100) != 0;  // a cada 1 s, a janela é descartada
    detectou = detectou || d.atualizar(ativo, pos, true, 136, 10);
  }
  TEST_ASSERT_FALSE(detectou);
}

void test_sem_leitura_valida_nao_detecta() {
  DetectorPatinagem d;
  int64_t pos = 0;
  bool detectou = false;
  for (int i = 0; i < 400; i++) {
    pos += 3000;
    detectou = detectou || d.atualizar(true, pos, false, 136, 10);
  }
  TEST_ASSERT_FALSE(detectou);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_avanco_normal_nao_e_patinagem);
  RUN_TEST(test_odometria_andando_com_o_tof_parado_e_patinagem);
  RUN_TEST(test_robo_parado_nao_e_patinagem);
  RUN_TEST(test_tof_longe_demais_nao_vale);
  RUN_TEST(test_fora_do_avanco_reinicia_a_janela);
  RUN_TEST(test_sem_leitura_valida_nao_detecta);
  return UNITY_END();
}
