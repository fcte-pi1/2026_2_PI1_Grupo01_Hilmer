/// @file test_console_remoto.cpp
/// @brief Testes da fila de bytes e do filtro do telnet (`FilaBytes.h`), a parte do console remoto
/// que não depende do Wi-Fi: o texto do robô entra na fila sem bloquear e o que o cliente digita
/// chega limpo das negociações do telnet.
/// Roda no computador: `pio test -e native -f test_console_remoto`.

#include <string.h>
#include <unity.h>

#include "FilaBytes.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

bool empilharTexto(FilaBytes<16>& fila, const char* texto) {
  return fila.empilhar(reinterpret_cast<const uint8_t*>(texto), strlen(texto));
}

}  // namespace

void test_fila_devolve_os_bytes_na_ordem() {
  FilaBytes<16> fila;
  TEST_ASSERT_TRUE(fila.vazia());
  TEST_ASSERT_TRUE(empilharTexto(fila, "abc"));
  TEST_ASSERT_TRUE(empilharTexto(fila, "de"));
  uint8_t saida[8] = {};
  TEST_ASSERT_EQUAL_UINT32(5, fila.desempilhar(saida, sizeof(saida)));
  TEST_ASSERT_EQUAL_MEMORY("abcde", saida, 5);
  TEST_ASSERT_EQUAL_INT(-1, fila.ler());
}

void test_texto_que_nao_cabe_e_descartado_inteiro() {
  FilaBytes<16> fila;
  TEST_ASSERT_TRUE(empilharTexto(fila, "0123456789"));
  TEST_ASSERT_FALSE(empilharTexto(fila, "abcdefghij"));  // só 6 de espaço: nenhum byte entra
  TEST_ASSERT_EQUAL_UINT32(1, fila.descartados());
  TEST_ASSERT_EQUAL_UINT32(10, fila.tamanho());
  TEST_ASSERT_TRUE(empilharTexto(fila, "abcdef"));  // os 6 que cabem entram
  TEST_ASSERT_EQUAL_UINT32(16, fila.tamanho());
}

void test_fila_da_a_volta_sem_perder_bytes() {
  FilaBytes<16> fila;
  uint8_t saida[16];
  for (int rodada = 0; rodada < 10; ++rodada) {
    TEST_ASSERT_TRUE(empilharTexto(fila, "1234567890"));
    TEST_ASSERT_EQUAL_UINT32(10, fila.desempilhar(saida, 10));
    TEST_ASSERT_EQUAL_MEMORY("1234567890", saida, 10);
  }
  TEST_ASSERT_EQUAL_UINT32(0, fila.descartados());
}

void test_limpar_esvazia_e_mantem_o_contador() {
  FilaBytes<16> fila;
  empilharTexto(fila, "0123456789");
  empilharTexto(fila, "abcdefghij");  // descartado
  fila.limpar();
  TEST_ASSERT_TRUE(fila.vazia());
  TEST_ASSERT_EQUAL_UINT32(1, fila.descartados());
}

void test_filtro_descarta_as_negociacoes_do_telnet() {
  FiltroTelnet filtro;
  // O cliente telnet, ao conectar: IAC WILL 31, IAC DO 1, IAC SB... (comando de 2 bytes), e depois "L\r\n".
  const uint8_t fluxo[] = {255, 251, 31, 255, 253, 1, 255, 241, 'L', '\r', '\n'};
  char texto[8] = {};
  size_t n = 0;
  for (uint8_t c : fluxo) {
    const int f = filtro.filtrar(c);
    if (f >= 0) {
      texto[n++] = static_cast<char>(f);
    }
  }
  TEST_ASSERT_EQUAL_UINT32(3, n);
  TEST_ASSERT_EQUAL_STRING("L\r\n", texto);
}

void test_filtro_deixa_passar_o_byte_255_duplicado() {
  FiltroTelnet filtro;
  TEST_ASSERT_EQUAL_INT(-1, filtro.filtrar(255));
  TEST_ASSERT_EQUAL_INT(255, filtro.filtrar(255));  // IAC IAC = o byte 255 como texto
  TEST_ASSERT_EQUAL_INT('x', filtro.filtrar('x'));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_fila_devolve_os_bytes_na_ordem);
  RUN_TEST(test_texto_que_nao_cabe_e_descartado_inteiro);
  RUN_TEST(test_fila_da_a_volta_sem_perder_bytes);
  RUN_TEST(test_limpar_esvazia_e_mantem_o_contador);
  RUN_TEST(test_filtro_descarta_as_negociacoes_do_telnet);
  RUN_TEST(test_filtro_deixa_passar_o_byte_255_duplicado);
  return UNITY_END();
}
