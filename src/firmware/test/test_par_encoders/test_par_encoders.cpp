#include <unity.h>

#include "EncoderSimulado.h"
#include "ParDeEncoders.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

void test_com_os_dois_contando_devolve_a_contagem_real() {
  EncoderSimulado esq, dir;
  ParDeEncoders par(esq, dir);
  esq.adicionarPulsos(500);
  dir.adicionarPulsos(520);
  par.atualizar(false);
  TEST_ASSERT_EQUAL_INT32(500, par.esquerdo().lerContagem());
  TEST_ASSERT_EQUAL_INT32(520, par.direito().lerContagem());
  TEST_ASSERT_FALSE(par.esquerdoSemSinal());
  TEST_ASSERT_FALSE(par.direitoSemSinal());
}

void test_encoder_esquerdo_mudo_passa_a_repetir_o_direito() {
  EncoderSimulado esq, dir;
  ParDeEncoders par(esq, dir);
  for (int i = 0; i < 20; i++) {
    dir.adicionarPulsos(100);  // o esquerdo não conta
    par.atualizar(false);
  }
  TEST_ASSERT_TRUE(par.esquerdoSemSinal());
  TEST_ASSERT_FALSE(par.direitoSemSinal());
  TEST_ASSERT_TRUE(par.consumirAvisoEsquerdo());
  TEST_ASSERT_FALSE(par.consumirAvisoEsquerdo());  // o aviso sai uma vez só
  // Depois de declarado, o esquerdo acompanha o direito.
  const int32_t antes = par.esquerdo().lerContagem();
  dir.adicionarPulsos(300);
  par.atualizar(false);
  TEST_ASSERT_EQUAL_INT32(antes + 300, par.esquerdo().lerContagem());
  TEST_ASSERT_EQUAL_INT32(2300, par.direito().lerContagem());
}

void test_encoder_direito_mudo_passa_a_repetir_o_esquerdo() {
  EncoderSimulado esq, dir;
  ParDeEncoders par(esq, dir);
  for (int i = 0; i < 20; i++) {
    esq.adicionarPulsos(100);
    par.atualizar(false);
  }
  TEST_ASSERT_TRUE(par.direitoSemSinal());
  TEST_ASSERT_FALSE(par.esquerdoSemSinal());
  TEST_ASSERT_TRUE(par.consumirAvisoDireito());
  const int32_t antes = par.direito().lerContagem();
  esq.adicionarPulsos(-250);  // ré
  par.atualizar(false);
  TEST_ASSERT_EQUAL_INT32(antes - 250, par.direito().lerContagem());
}

void test_no_giro_no_lugar_o_lado_mudo_repete_com_o_sinal_trocado() {
  EncoderSimulado esq, dir;
  ParDeEncoders par(esq, dir);
  for (int i = 0; i < 20; i++) {
    dir.adicionarPulsos(100);
    par.atualizar(false);
  }
  const int32_t esquerdoAntes = par.esquerdo().lerContagem();
  dir.adicionarPulsos(-400);  // giro: o direito recua e o esquerdo (mudo) deveria avançar
  par.atualizar(true);
  TEST_ASSERT_EQUAL_INT32(esquerdoAntes + 400, par.esquerdo().lerContagem());
}

void test_ruido_pequeno_nao_declara_falha() {
  EncoderSimulado esq, dir;
  ParDeEncoders par(esq, dir);
  esq.adicionarPulsos(30);
  dir.adicionarPulsos(40);
  par.atualizar(false);
  TEST_ASSERT_FALSE(par.esquerdoSemSinal());
  TEST_ASSERT_FALSE(par.direitoSemSinal());
}

void test_os_dois_parados_nao_declaram_falha() {
  EncoderSimulado esq, dir;
  ParDeEncoders par(esq, dir);
  for (int i = 0; i < 100; i++) {
    par.atualizar(false);
  }
  TEST_ASSERT_FALSE(par.esquerdoSemSinal());
  TEST_ASSERT_FALSE(par.direitoSemSinal());
}

void test_zerar_o_adaptador_zera_so_a_contagem_exposta() {
  EncoderSimulado esq, dir;
  ParDeEncoders par(esq, dir);
  esq.adicionarPulsos(300);
  dir.adicionarPulsos(300);
  par.atualizar(false);
  par.esquerdo().zerar();
  par.direito().zerar();
  TEST_ASSERT_EQUAL_INT32(0, par.esquerdo().lerContagem());
  dir.adicionarPulsos(50);
  esq.adicionarPulsos(50);
  par.atualizar(false);
  TEST_ASSERT_EQUAL_INT32(50, par.esquerdo().lerContagem());
  TEST_ASSERT_EQUAL_INT32(50, par.direito().lerContagem());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_com_os_dois_contando_devolve_a_contagem_real);
  RUN_TEST(test_encoder_esquerdo_mudo_passa_a_repetir_o_direito);
  RUN_TEST(test_encoder_direito_mudo_passa_a_repetir_o_esquerdo);
  RUN_TEST(test_no_giro_no_lugar_o_lado_mudo_repete_com_o_sinal_trocado);
  RUN_TEST(test_ruido_pequeno_nao_declara_falha);
  RUN_TEST(test_os_dois_parados_nao_declaram_falha);
  RUN_TEST(test_zerar_o_adaptador_zera_so_a_contagem_exposta);
  return UNITY_END();
}
