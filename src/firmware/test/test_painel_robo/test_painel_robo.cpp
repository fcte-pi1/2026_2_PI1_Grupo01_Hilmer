/// @file test_painel_robo.cpp
/// @brief Testes do canal painel ↔ robô, a parte sem Wi-Fi: a leitura do comando que o painel mandou, o
/// corpo do lote de logs (JSON com aspas escapadas), a fila de linhas e a regra de quais linhas do log
/// vão para o painel.
/// Roda no computador: `pio test -e native -f test_painel_robo`.

#include <ArduinoJson.h>
#include <string.h>
#include <unity.h>

#include "ContratoTelemetria.h"
#include "FilaLinhas.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

// ---------------------------------------------------------------- comando do painel

void test_comando_iniciar_parar_e_zerar() {
  ComandoRobo c = ComandoRobo::Nenhum;
  TEST_ASSERT_TRUE(lerComandoRobo("{\"id\":7,\"comando\":\"iniciar\"}", c));
  TEST_ASSERT_EQUAL(ComandoRobo::Iniciar, c);
  TEST_ASSERT_TRUE(lerComandoRobo("{\"id\":8,\"comando\":\"parar\"}", c));
  TEST_ASSERT_EQUAL(ComandoRobo::Parar, c);
  TEST_ASSERT_TRUE(lerComandoRobo("{\"id\":9,\"comando\":\"zerar\"}", c));
  TEST_ASSERT_EQUAL(ComandoRobo::Zerar, c);
}

void test_sem_comando_pendente_a_resposta_traz_nulos() {
  ComandoRobo c = ComandoRobo::Iniciar;
  TEST_ASSERT_TRUE(lerComandoRobo("{\"id\":null,\"comando\":null}", c));
  TEST_ASSERT_EQUAL(ComandoRobo::Nenhum, c);
  c = ComandoRobo::Iniciar;
  TEST_ASSERT_TRUE(lerComandoRobo("{}", c));  // sem o campo, o mesmo que nulo
  TEST_ASSERT_EQUAL(ComandoRobo::Nenhum, c);
}

void test_resposta_ilegivel_ou_comando_desconhecido_e_recusado() {
  ComandoRobo c = ComandoRobo::Nenhum;
  TEST_ASSERT_FALSE(lerComandoRobo(nullptr, c));
  TEST_ASSERT_FALSE(lerComandoRobo("", c));
  TEST_ASSERT_FALSE(lerComandoRobo("nao e json", c));
  TEST_ASSERT_FALSE(lerComandoRobo("[\"iniciar\"]", c));
  TEST_ASSERT_FALSE(lerComandoRobo("{\"comando\":\"apagar\"}", c));
  TEST_ASSERT_FALSE(lerComandoRobo("{\"comando\":1}", c));
  TEST_ASSERT_FALSE(lerComandoRobo("{\"comando\":\"INICIAR\"}", c));
  TEST_ASSERT_EQUAL(ComandoRobo::Nenhum, c);  // a saída não muda numa resposta recusada
}

// ---------------------------------------------------------------- lote de logs

void test_lote_de_logs_e_um_json_com_as_aspas_escapadas() {
  const char* linhas[2] = {"{\"status\":\"largada_liberada\"}", "segunda linha"};
  char saida[512];
  const size_t n = serializarLogs(7, linhas, 2, saida, sizeof(saida));
  TEST_ASSERT_GREATER_THAN_UINT32(0, n);
  TEST_ASSERT_EQUAL_UINT32(strlen(saida), n);
  JsonDocument doc;
  TEST_ASSERT_TRUE(deserializeJson(doc, saida) == DeserializationError::Ok);
  TEST_ASSERT_EQUAL_INT(7, doc["corrida_id"].as<int>());
  TEST_ASSERT_EQUAL_UINT32(2, doc["linhas"].size());
  // As aspas voltam iguais depois do escape: o painel mostra a linha como o robô a escreveu.
  TEST_ASSERT_EQUAL_STRING("{\"status\":\"largada_liberada\"}", doc["linhas"][0].as<const char*>());
  TEST_ASSERT_EQUAL_STRING("segunda linha", doc["linhas"][1].as<const char*>());
}

void test_lote_sem_corrida_deixa_o_campo_de_fora() {
  const char* linhas[1] = {"antes da corrida"};
  char saida[256];
  TEST_ASSERT_GREATER_THAN_UINT32(0, serializarLogs(0, linhas, 1, saida, sizeof(saida)));
  JsonDocument doc;
  TEST_ASSERT_TRUE(deserializeJson(doc, saida) == DeserializationError::Ok);
  TEST_ASSERT_TRUE(doc["corrida_id"].isNull());
}

void test_lote_de_logs_invalido_ou_que_nao_cabe_devolve_zero() {
  char saida[64];
  const char* vazia[1] = {""};
  const char* nula[1] = {nullptr};
  const char* grande[1] = {"uma linha bem maior do que cabe no buffer de sessenta e quatro bytes"};
  TEST_ASSERT_EQUAL_UINT32(0, serializarLogs(1, vazia, 1, saida, sizeof(saida)));
  TEST_ASSERT_EQUAL_UINT32(0, serializarLogs(1, nula, 1, saida, sizeof(saida)));
  TEST_ASSERT_EQUAL_UINT32(0, serializarLogs(1, grande, 1, saida, sizeof(saida)));
  TEST_ASSERT_EQUAL_UINT32(0, serializarLogs(1, nullptr, 0, saida, sizeof(saida)));
  TEST_ASSERT_EQUAL_UINT32(0, serializarLogs(1, vazia, 1, nullptr, 0));
}

void test_o_pior_lote_de_logs_cabe_no_buffer_do_cliente() {
  // 10 linhas de 199 caracteres, todas de aspas (o pior escape: cada aspa vira duas).
  static char armazenamento[LINHAS_POR_LOTE_DE_LOGS][TAMANHO_MAX_LINHA_DE_LOG];
  const char* linhas[LINHAS_POR_LOTE_DE_LOGS];
  for (size_t i = 0; i < LINHAS_POR_LOTE_DE_LOGS; ++i) {
    memset(armazenamento[i], '"', TAMANHO_MAX_LINHA_DE_LOG - 1);
    armazenamento[i][TAMANHO_MAX_LINHA_DE_LOG - 1] = '\0';
    linhas[i] = armazenamento[i];
  }
  // O mesmo tamanho do buffer `logs_` do `EnvioHttpSupabase`.
  static char saida[LINHAS_POR_LOTE_DE_LOGS * TAMANHO_MAX_LINHA_DE_LOG * 2 + 64];
  TEST_ASSERT_GREATER_THAN_UINT32(0, serializarLogs(2147483647u, linhas, LINHAS_POR_LOTE_DE_LOGS, saida, sizeof(saida)));
}

// ---------------------------------------------------------------- labirinto descoberto

void test_o_nome_do_labirinto_e_o_do_banco() {
  TEST_ASSERT_EQUAL_STRING("4x4", nomeDoLabirinto(DimensaoMapa::Labirinto4x4));
  TEST_ASSERT_EQUAL_STRING("8x4", nomeDoLabirinto(DimensaoMapa::Labirinto8x4));
  TEST_ASSERT_EQUAL_STRING("12x4", nomeDoLabirinto(DimensaoMapa::Labirinto12x4));
  TEST_ASSERT_EQUAL_STRING("", nomeDoLabirinto(DimensaoMapa::Exploracao));  // não é um labirinto
}

// ---------------------------------------------------------------- fila de linhas e filtro

void test_fila_de_linhas_tira_na_ordem_e_sem_o_fim_de_linha() {
  FilaLinhas<4, 32> fila;
  TEST_ASSERT_TRUE(fila.empilhar("primeira\n"));
  TEST_ASSERT_TRUE(fila.empilhar("segunda\r\n"));
  char saida[32];
  TEST_ASSERT_EQUAL_UINT32(8, fila.desempilhar(saida, sizeof(saida)));
  TEST_ASSERT_EQUAL_STRING("primeira", saida);
  TEST_ASSERT_EQUAL_UINT32(7, fila.desempilhar(saida, sizeof(saida)));
  TEST_ASSERT_EQUAL_STRING("segunda", saida);
  TEST_ASSERT_EQUAL_UINT32(0, fila.desempilhar(saida, sizeof(saida)));
  TEST_ASSERT_TRUE(fila.vazia());
}

void test_fila_cheia_descarta_a_linha_nova_e_conta() {
  FilaLinhas<2, 16> fila;
  TEST_ASSERT_TRUE(fila.empilhar("a"));
  TEST_ASSERT_TRUE(fila.empilhar("b"));
  TEST_ASSERT_FALSE(fila.empilhar("c"));
  TEST_ASSERT_EQUAL_UINT32(1, fila.descartadas());
  char saida[16];
  fila.desempilhar(saida, sizeof(saida));
  TEST_ASSERT_TRUE(fila.empilhar("d"));  // abriu espaço, e a fila deu a volta
  fila.desempilhar(saida, sizeof(saida));
  TEST_ASSERT_EQUAL_STRING("b", saida);
  fila.desempilhar(saida, sizeof(saida));
  TEST_ASSERT_EQUAL_STRING("d", saida);
}

void test_linha_vazia_ou_grande_demais_e_descartada() {
  FilaLinhas<4, 8> fila;
  TEST_ASSERT_FALSE(fila.empilhar(""));
  TEST_ASSERT_FALSE(fila.empilhar("\r\n"));
  TEST_ASSERT_FALSE(fila.empilhar(nullptr));
  TEST_ASSERT_FALSE(fila.empilhar("12345678"));  // 8 caracteres: sem lugar para o '\0'
  TEST_ASSERT_TRUE(fila.empilhar("1234567"));
  TEST_ASSERT_EQUAL_UINT32(4, fila.descartadas());
}

void test_saida_pequena_demais_deixa_a_linha_na_fila() {
  FilaLinhas<2, 16> fila;
  fila.empilhar("uma linha longa");
  char pequena[4];
  TEST_ASSERT_EQUAL_UINT32(0, fila.desempilhar(pequena, sizeof(pequena)));
  TEST_ASSERT_EQUAL_UINT32(1, fila.tamanho());
}

void test_so_os_eventos_vao_para_o_painel() {
  TEST_ASSERT_TRUE(ehLinhaDeEventoParaOPainel("{\"status\":\"largada_liberada\"}\n"));
  TEST_ASSERT_TRUE(ehLinhaDeEventoParaOPainel("{\"status\":\"colisao\",\"tipo\":\"impacto\"}"));
  TEST_ASSERT_TRUE(ehLinhaDeEventoParaOPainel("{\"status\":\"bateria\",\"situacao\":\"fraca\",\"mv\":6400}"));
  // `envio_pausado` é um evento; os contadores `envio` (a cada 10 s) não.
  TEST_ASSERT_TRUE(ehLinhaDeEventoParaOPainel("{\"status\":\"envio_pausado\",\"motivo\":\"credenciais_recusadas\"}"));
  TEST_ASSERT_FALSE(ehLinhaDeEventoParaOPainel("{\"status\":\"envio\",\"ocupacao\":0}"));
  // A telemetria de 1 s e o relatório do boot não são eventos.
  TEST_ASSERT_FALSE(ehLinhaDeEventoParaOPainel("{\"seq\":12,\"pose\":{}}"));
  TEST_ASSERT_FALSE(ehLinhaDeEventoParaOPainel("{\"tipo\":\"boot\",\"ok\":true}"));
  TEST_ASSERT_FALSE(ehLinhaDeEventoParaOPainel("micromouse: iniciando"));
  TEST_ASSERT_FALSE(ehLinhaDeEventoParaOPainel(""));
  TEST_ASSERT_FALSE(ehLinhaDeEventoParaOPainel(nullptr));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_comando_iniciar_parar_e_zerar);
  RUN_TEST(test_sem_comando_pendente_a_resposta_traz_nulos);
  RUN_TEST(test_resposta_ilegivel_ou_comando_desconhecido_e_recusado);
  RUN_TEST(test_lote_de_logs_e_um_json_com_as_aspas_escapadas);
  RUN_TEST(test_lote_sem_corrida_deixa_o_campo_de_fora);
  RUN_TEST(test_lote_de_logs_invalido_ou_que_nao_cabe_devolve_zero);
  RUN_TEST(test_o_pior_lote_de_logs_cabe_no_buffer_do_cliente);
  RUN_TEST(test_o_nome_do_labirinto_e_o_do_banco);
  RUN_TEST(test_fila_de_linhas_tira_na_ordem_e_sem_o_fim_de_linha);
  RUN_TEST(test_fila_cheia_descarta_a_linha_nova_e_conta);
  RUN_TEST(test_linha_vazia_ou_grande_demais_e_descartada);
  RUN_TEST(test_saida_pequena_demais_deixa_a_linha_na_fila);
  RUN_TEST(test_so_os_eventos_vao_para_o_painel);
  return UNITY_END();
}
