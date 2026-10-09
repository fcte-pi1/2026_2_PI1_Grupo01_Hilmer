/// @file test_labirinto_simulado.cpp
/// @brief Testes unitários do `LabirintoSimulado` e do modo labirinto do `LidarSimulado`.
/// Roda no computador: `pio test -e native -f test_labirinto_simulado`.

#include <string.h>
#include <unity.h>

#include "Direcao.h"
#include "LabirintoSimulado.h"
#include "LabirintosExemplo.h"
#include "LidarSimulado.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

/// Largura de uma linha do `LABIRINTO_4X4` com o `\n`: 4 colunas × 4 + 1 + 1.
constexpr size_t LARGURA_4X4_COM_QUEBRA = 18;

/// Carrega um dos exemplos num labirinto novo e confere que foi aceito.
void carregarExemplo(LabirintoSimulado& labirinto, const char* texto) {
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ResultadoCarga::Carregado),
                          static_cast<uint8_t>(labirinto.carregar(texto)));
}

void verificarResultado(ResultadoCarga esperado, ResultadoCarga obtido) {
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(esperado), static_cast<uint8_t>(obtido));
}

/// Copia o `LABIRINTO_4X4` para `destino` trocando o caractere em `posicao`.
void copiarComAlteracao(char* destino, size_t posicao, char novo) {
  strcpy(destino, exemplos::LABIRINTO_4X4);
  destino[posicao] = novo;
}

void verificarDistancias(const LabirintoSimulado& labirinto, Pose pose, uint16_t frente,
                         uint16_t esquerda, uint16_t direita) {
  DistanciasLaterais d = {};
  TEST_ASSERT_TRUE(labirinto.distancias(pose, d));
  TEST_ASSERT_EQUAL_UINT16(frente, d.frenteMm);
  TEST_ASSERT_EQUAL_UINT16(esquerda, d.esquerdaMm);
  TEST_ASSERT_EQUAL_UINT16(direita, d.direitaMm);
}

void verificarLeitura(LidarSimulado& lidar, uint16_t frente, uint16_t esquerda, uint16_t direita) {
  DistanciasLaterais d = {};
  TEST_ASSERT_TRUE(lidar.lerDistanciasLaterais(d));
  TEST_ASSERT_EQUAL_UINT16(frente, d.frenteMm);
  TEST_ASSERT_EQUAL_UINT16(esquerda, d.esquerdaMm);
  TEST_ASSERT_EQUAL_UINT16(direita, d.direitaMm);
}

}  // namespace

// ---- Carga do texto e validação

void test_carrega_os_exemplos_dos_tres_tamanhos() {
  LabirintoSimulado labirinto;
  TEST_ASSERT_FALSE(labirinto.carregado());

  carregarExemplo(labirinto, exemplos::LABIRINTO_4X4);
  TEST_ASSERT_TRUE(labirinto.carregado());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DimensaoMapa::Labirinto4x4),
                          static_cast<uint8_t>(labirinto.obterDimensao()));
  TEST_ASSERT_EQUAL_UINT8(4, labirinto.obterLinhas());
  TEST_ASSERT_EQUAL_UINT8(4, labirinto.obterColunas());

  carregarExemplo(labirinto, exemplos::LABIRINTO_8X4);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DimensaoMapa::Labirinto8x4),
                          static_cast<uint8_t>(labirinto.obterDimensao()));
  TEST_ASSERT_EQUAL_UINT8(4, labirinto.obterLinhas());
  TEST_ASSERT_EQUAL_UINT8(8, labirinto.obterColunas());

  carregarExemplo(labirinto, exemplos::LABIRINTO_12X4);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DimensaoMapa::Labirinto12x4),
                          static_cast<uint8_t>(labirinto.obterDimensao()));
  TEST_ASSERT_EQUAL_UINT8(4, labirinto.obterLinhas());
  TEST_ASSERT_EQUAL_UINT8(12, labirinto.obterColunas());
}

void test_carrega_com_ou_sem_quebra_de_linha_final() {
  char texto[256];
  strcpy(texto, exemplos::LABIRINTO_4X4);
  strcat(texto, "\n");
  LabirintoSimulado labirinto;
  carregarExemplo(labirinto, texto);
  verificarDistancias(labirinto, {{0, 0}, Direcao::Norte}, 630, 90, 90);
}

void test_recusa_tamanho_fora_da_competicao() {
  LabirintoSimulado labirinto;
  const char* tres_por_tres =
      "+---+---+---+\n"
      "|           |\n"
      "+   +   +   +\n"
      "|           |\n"
      "+   +   +   +\n"
      "|           |\n"
      "+---+---+---+";
  verificarResultado(ResultadoCarga::TamanhoInvalido, labirinto.carregar(tres_por_tres));

  // 4 colunas e 5 linhas: a largura é de um tamanho válido, mas as linhas não.
  const char* quatro_por_cinco =
      "+---+---+---+---+\n"
      "|               |\n"
      "+   +   +   +   +\n"
      "|               |\n"
      "+   +   +   +   +\n"
      "|               |\n"
      "+   +   +   +   +\n"
      "|               |\n"
      "+   +   +   +   +\n"
      "|               |\n"
      "+---+---+---+---+";
  verificarResultado(ResultadoCarga::TamanhoInvalido, labirinto.carregar(quatro_por_cinco));

  // 6 colunas e 4 linhas: não é nenhum dos três tamanhos.
  const char* seis_por_quatro =
      "+---+---+---+---+---+---+\n"
      "|                       |\n"
      "+   +   +   +   +   +   +\n"
      "|                       |\n"
      "+   +   +   +   +   +   +\n"
      "|                       |\n"
      "+   +   +   +   +   +   +\n"
      "|                       |\n"
      "+---+---+---+---+---+---+";
  verificarResultado(ResultadoCarga::TamanhoInvalido, labirinto.carregar(seis_por_quatro));

  // Uma linha com um caractere a menos.
  char larguras_diferentes[256];
  strcpy(larguras_diferentes, exemplos::LABIRINTO_4X4);
  larguras_diferentes[LARGURA_4X4_COM_QUEBRA - 2] = '\n';
  larguras_diferentes[LARGURA_4X4_COM_QUEBRA - 1] = '\n';
  verificarResultado(ResultadoCarga::TamanhoInvalido, labirinto.carregar(larguras_diferentes));

  // Mais linhas do que o maior labirinto comporta.
  char linhas_demais[512];
  strcpy(linhas_demais, exemplos::LABIRINTO_4X4);
  strcat(linhas_demais, "\n+---+---+---+---+\n+---+---+---+---+");
  verificarResultado(ResultadoCarga::TamanhoInvalido, labirinto.carregar(linhas_demais));

  // Linha mais larga do que o maior labirinto comporta.
  char larga[256];
  memset(larga, '-', sizeof(larga));
  larga[sizeof(larga) - 1] = '\0';
  verificarResultado(ResultadoCarga::TamanhoInvalido, labirinto.carregar(larga));

  verificarResultado(ResultadoCarga::TamanhoInvalido, labirinto.carregar(""));
  verificarResultado(ResultadoCarga::TamanhoInvalido, labirinto.carregar(nullptr));
  TEST_ASSERT_FALSE(labirinto.carregado());
}

void test_recusa_caractere_invalido() {
  LabirintoSimulado labirinto;
  char texto[256];

  // 'x' no lugar de '|' (primeira parede vertical da linha 1).
  copiarComAlteracao(texto, LARGURA_4X4_COM_QUEBRA, 'x');
  verificarResultado(ResultadoCarga::CaractereInvalido, labirinto.carregar(texto));

  // 'x' no lugar de um '-'.
  copiarComAlteracao(texto, 2, 'x');
  verificarResultado(ResultadoCarga::CaractereInvalido, labirinto.carregar(texto));

  // Canto que não é '+'.
  copiarComAlteracao(texto, 4, '-');
  verificarResultado(ResultadoCarga::CaractereInvalido, labirinto.carregar(texto));

  // '|' dentro de um trecho de parede horizontal.
  copiarComAlteracao(texto, 2, '|');
  verificarResultado(ResultadoCarga::CaractereInvalido, labirinto.carregar(texto));

  // Conteúdo dentro da célula.
  copiarComAlteracao(texto, LARGURA_4X4_COM_QUEBRA + 2, 'o');
  verificarResultado(ResultadoCarga::CaractereInvalido, labirinto.carregar(texto));

  // '+' no lugar de uma parede vertical.
  copiarComAlteracao(texto, LARGURA_4X4_COM_QUEBRA + 4, '+');
  verificarResultado(ResultadoCarga::CaractereInvalido, labirinto.carregar(texto));
  TEST_ASSERT_FALSE(labirinto.carregado());
}

void test_recusa_parede_incompleta() {
  LabirintoSimulado labirinto;
  char texto[256];

  // "- -" no lugar de "---" na borda Norte.
  copiarComAlteracao(texto, 2, ' ');
  verificarResultado(ResultadoCarga::ParedeIncompleta, labirinto.carregar(texto));

  // "-  " no lugar de "---".
  copiarComAlteracao(texto, 2, ' ');
  texto[3] = ' ';
  verificarResultado(ResultadoCarga::ParedeIncompleta, labirinto.carregar(texto));

  // "  -" numa borda interna que já tinha parede (linha 6, coluna 1).
  copiarComAlteracao(texto, 6 * LARGURA_4X4_COM_QUEBRA + 5, ' ');
  verificarResultado(ResultadoCarga::ParedeIncompleta, labirinto.carregar(texto));
  TEST_ASSERT_FALSE(labirinto.carregado());
}

void test_recusa_perimetro_aberto() {
  LabirintoSimulado labirinto;
  char texto[256];

  // Borda Norte sem parede na primeira célula.
  copiarComAlteracao(texto, 1, ' ');
  texto[2] = ' ';
  texto[3] = ' ';
  verificarResultado(ResultadoCarga::PerimetroAberto, labirinto.carregar(texto));

  // Borda Sul sem parede na última célula.
  copiarComAlteracao(texto, 8 * LARGURA_4X4_COM_QUEBRA + 13, ' ');
  texto[8 * LARGURA_4X4_COM_QUEBRA + 14] = ' ';
  texto[8 * LARGURA_4X4_COM_QUEBRA + 15] = ' ';
  verificarResultado(ResultadoCarga::PerimetroAberto, labirinto.carregar(texto));

  // Borda Oeste aberta na linha 3.
  copiarComAlteracao(texto, LARGURA_4X4_COM_QUEBRA, ' ');
  verificarResultado(ResultadoCarga::PerimetroAberto, labirinto.carregar(texto));

  // Borda Leste aberta na linha 0.
  copiarComAlteracao(texto, 7 * LARGURA_4X4_COM_QUEBRA + 16, ' ');
  verificarResultado(ResultadoCarga::PerimetroAberto, labirinto.carregar(texto));
  TEST_ASSERT_FALSE(labirinto.carregado());
}

void test_texto_invalido_mantem_o_labirinto_anterior() {
  LabirintoSimulado labirinto;
  carregarExemplo(labirinto, exemplos::LABIRINTO_4X4);

  char texto[256];
  copiarComAlteracao(texto, 1, ' ');
  texto[2] = ' ';
  texto[3] = ' ';
  verificarResultado(ResultadoCarga::PerimetroAberto, labirinto.carregar(texto));
  verificarResultado(ResultadoCarga::TamanhoInvalido, labirinto.carregar("lixo"));

  TEST_ASSERT_TRUE(labirinto.carregado());
  TEST_ASSERT_EQUAL_UINT8(4, labirinto.obterColunas());
  TEST_ASSERT_TRUE(labirinto.temParede({0, 0}, Direcao::Leste));
  verificarDistancias(labirinto, {{0, 0}, Direcao::Norte}, 630, 90, 90);
}

// ---- Consulta de paredes

void test_tem_parede_concorda_entre_celulas_vizinhas() {
  const char* textos[] = {exemplos::LABIRINTO_4X4, exemplos::LABIRINTO_8X4,
                          exemplos::LABIRINTO_12X4};
  const Direcao direcoes[] = {Direcao::Norte, Direcao::Leste, Direcao::Sul, Direcao::Oeste};
  for (const char* texto : textos) {
    LabirintoSimulado labirinto;
    carregarExemplo(labirinto, texto);
    for (uint8_t linha = 0; linha < labirinto.obterLinhas(); linha++) {
      for (uint8_t coluna = 0; coluna < labirinto.obterColunas(); coluna++) {
        for (Direcao d : direcoes) {
          PosicaoCelula vizinhaDe;
          if (vizinha({linha, coluna}, d, vizinhaDe) && labirinto.posValida(vizinhaDe)) {
            TEST_ASSERT_EQUAL(labirinto.temParede({linha, coluna}, d),
                              labirinto.temParede(vizinhaDe, oposta(d)));
          }
        }
      }
    }
  }
}

void test_todas_as_celulas_dos_exemplos_sao_alcancaveis_da_partida() {
  const char* textos[] = {exemplos::LABIRINTO_4X4, exemplos::LABIRINTO_8X4,
                          exemplos::LABIRINTO_12X4};
  const Direcao direcoes[] = {Direcao::Norte, Direcao::Leste, Direcao::Sul, Direcao::Oeste};
  for (const char* texto : textos) {
    LabirintoSimulado labirinto;
    carregarExemplo(labirinto, texto);
    // Busca em largura a partir de (0, 0), com fila e marcas de tamanho fixo.
    bool visitada[MAX_LINHAS_LABIRINTO][MAX_COLUNAS_LABIRINTO] = {};
    PosicaoCelula fila[MAX_LINHAS_LABIRINTO * MAX_COLUNAS_LABIRINTO];
    uint8_t inicio = 0;
    uint8_t fim = 0;
    visitada[0][0] = true;
    fila[fim++] = {0, 0};
    while (inicio < fim) {
      const PosicaoCelula atual = fila[inicio++];
      for (Direcao d : direcoes) {
        PosicaoCelula proxima;
        if (!labirinto.temParede(atual, d) && vizinha(atual, d, proxima) &&
            !visitada[proxima.linha][proxima.coluna]) {
          visitada[proxima.linha][proxima.coluna] = true;
          fila[fim++] = proxima;
        }
      }
    }
    TEST_ASSERT_EQUAL_UINT8(labirinto.obterLinhas() * labirinto.obterColunas(), fim);
  }
}

void test_tem_parede_le_os_quatro_lados_da_celula() {
  LabirintoSimulado labirinto;
  carregarExemplo(labirinto, exemplos::LABIRINTO_4X4);
  // Célula (1, 0): só tem parede a Oeste (borda); está aberta ao Norte, ao Sul e ao Leste.
  TEST_ASSERT_TRUE(labirinto.temParede({1, 0}, Direcao::Oeste));
  TEST_ASSERT_FALSE(labirinto.temParede({1, 0}, Direcao::Leste));
  TEST_ASSERT_FALSE(labirinto.temParede({1, 0}, Direcao::Norte));
  TEST_ASSERT_FALSE(labirinto.temParede({1, 0}, Direcao::Sul));
  // Célula (3, 1): parede ao Norte (borda) e ao Sul (interna).
  TEST_ASSERT_TRUE(labirinto.temParede({3, 1}, Direcao::Norte));
  TEST_ASSERT_TRUE(labirinto.temParede({3, 1}, Direcao::Sul));
  // Célula (0, 0): paredes Sul e Oeste (borda) e Leste (interna).
  TEST_ASSERT_TRUE(labirinto.temParede({0, 0}, Direcao::Sul));
  TEST_ASSERT_TRUE(labirinto.temParede({0, 0}, Direcao::Oeste));
  TEST_ASSERT_TRUE(labirinto.temParede({0, 0}, Direcao::Leste));
}

void test_tem_parede_fora_do_labirinto_e_verdadeiro() {
  LabirintoSimulado labirinto;
  // Sem carga, tudo conta como parede.
  TEST_ASSERT_TRUE(labirinto.temParede({0, 0}, Direcao::Norte));
  TEST_ASSERT_FALSE(labirinto.posValida({0, 0}));

  carregarExemplo(labirinto, exemplos::LABIRINTO_4X4);
  TEST_ASSERT_TRUE(labirinto.posValida({3, 3}));
  TEST_ASSERT_FALSE(labirinto.posValida({4, 0}));
  TEST_ASSERT_FALSE(labirinto.posValida({0, 4}));
  TEST_ASSERT_TRUE(labirinto.temParede({4, 0}, Direcao::Sul));
  TEST_ASSERT_TRUE(labirinto.temParede({0, 4}, Direcao::Oeste));
  TEST_ASSERT_TRUE(labirinto.temParede({255, 255}, Direcao::Norte));
}

// ---- Distâncias (robô no centro da célula: 90 mm + 180 mm por célula livre)

void test_corredor() {
  LabirintoSimulado labirinto;
  carregarExemplo(labirinto, exemplos::LABIRINTO_4X4);
  // Linha 0 de leste a oeste a partir da coluna 1: paredes nos dois lados e 2 células livres à frente.
  verificarDistancias(labirinto, {{0, 1}, Direcao::Leste}, 450, 90, 90);
  // Visto no sentido contrário, a frente é a parede da coluna 1 e o corredor fica para trás.
  verificarDistancias(labirinto, {{0, 1}, Direcao::Oeste}, 90, 90, 90);
  // Corredor vertical da coluna 0, de baixo para cima.
  verificarDistancias(labirinto, {{2, 0}, Direcao::Norte}, 270, 90, 90);
}

void test_curva() {
  LabirintoSimulado labirinto;
  carregarExemplo(labirinto, exemplos::LABIRINTO_4X4);
  // Parede à frente e à esquerda; passagem para a direita (2 células livres, depois parede).
  verificarDistancias(labirinto, {{3, 0}, Direcao::Norte}, 90, 90, 270);
  // Mesma célula virada para Leste: o corredor segue à frente.
  verificarDistancias(labirinto, {{3, 0}, Direcao::Leste}, 270, 90, 630);
}

void test_beco_sem_saida() {
  LabirintoSimulado labirinto;
  carregarExemplo(labirinto, exemplos::LABIRINTO_4X4);
  // Célula (0, 3): fechada ao Norte, ao Sul e ao Leste; a única saída é a Oeste.
  verificarDistancias(labirinto, {{0, 3}, Direcao::Leste}, 90, 90, 90);
}

void test_labirinto_4x4_completo() {
  LabirintoSimulado labirinto;
  carregarExemplo(labirinto, exemplos::LABIRINTO_4X4);
  // Tabela de leituras esperadas, conferida à mão em LabirintosExemplo.h.
  verificarDistancias(labirinto, {{0, 0}, Direcao::Norte}, 630, 90, 90);
  verificarDistancias(labirinto, {{1, 0}, Direcao::Norte}, 450, 90, 270);
  verificarDistancias(labirinto, {{3, 0}, Direcao::Leste}, 270, 90, 630);
  verificarDistancias(labirinto, {{2, 2}, Direcao::Norte}, 270, 270, 90);
  verificarDistancias(labirinto, {{0, 1}, Direcao::Leste}, 450, 90, 90);
}

void test_labirintos_8x4_e_12x4_medem_corredores_longos() {
  LabirintoSimulado labirinto;
  // 12x4, célula (3, 0) virada para Leste: 5 células livres à frente até a parede da coluna 5,
  // borda Norte à esquerda e 3 células livres ao Sul (linhas 2, 1 e 0) à direita.
  carregarExemplo(labirinto, exemplos::LABIRINTO_12X4);
  verificarDistancias(labirinto, {{3, 0}, Direcao::Leste}, 90 + 5 * 180, 90, 90 + 3 * 180);
  // 8x4, célula (0, 1) virada para Leste: parede à frente, 2 células livres ao Norte e parede ao Sul.
  carregarExemplo(labirinto, exemplos::LABIRINTO_8X4);
  verificarDistancias(labirinto, {{0, 1}, Direcao::Leste}, 90, 90 + 2 * 180, 90);
}

void test_distancias_fora_do_labirinto_ou_sem_carga_falham() {
  LabirintoSimulado labirinto;
  DistanciasLaterais d = {1, 2, 3};
  TEST_ASSERT_FALSE(labirinto.distancias({{0, 0}, Direcao::Norte}, d));

  carregarExemplo(labirinto, exemplos::LABIRINTO_4X4);
  TEST_ASSERT_FALSE(labirinto.distancias({{4, 0}, Direcao::Norte}, d));
  TEST_ASSERT_FALSE(labirinto.distancias({{0, 4}, Direcao::Norte}, d));
  TEST_ASSERT_EQUAL_UINT16(1, d.frenteMm);
  TEST_ASSERT_EQUAL_UINT16(2, d.esquerdaMm);
  TEST_ASSERT_EQUAL_UINT16(3, d.direitaMm);
}

// ---- LidarSimulado no modo labirinto

void test_lidar_posicionado_le_o_que_o_labirinto_mostra() {
  LabirintoSimulado labirinto;
  carregarExemplo(labirinto, exemplos::LABIRINTO_4X4);
  LidarSimulado lidar;
  lidar.usarLabirinto(labirinto);

  lidar.posicionar({{0, 0}, Direcao::Norte});
  verificarLeitura(lidar, 630, 90, 90);
  // A leitura não é consumida: repetir a chamada na mesma pose dá o mesmo resultado.
  verificarLeitura(lidar, 630, 90, 90);

  lidar.posicionar({{1, 0}, Direcao::Norte});
  verificarLeitura(lidar, 450, 90, 270);
  lidar.posicionar({{2, 2}, Direcao::Norte});
  verificarLeitura(lidar, 270, 270, 90);

  // Pela interface, como a lógica do firmware usa.
  ILidar& sensor = lidar;
  DistanciasLaterais d = {};
  TEST_ASSERT_TRUE(sensor.lerDistanciasLaterais(d));
  TEST_ASSERT_EQUAL_UINT16(270, d.frenteMm);
}

void test_lidar_no_labirinto_respeita_falha_e_fila() {
  LabirintoSimulado labirinto;
  carregarExemplo(labirinto, exemplos::LABIRINTO_4X4);
  LidarSimulado lidar;
  lidar.definirLeituraFixa({500, 500, 500});
  lidar.usarLabirinto(labirinto);
  DistanciasLaterais d = {};

  // Sem posicionar, o modo labirinto não age e vale a leitura fixa.
  verificarLeitura(lidar, 500, 500, 500);

  lidar.posicionar({{0, 0}, Direcao::Norte});
  verificarLeitura(lidar, 630, 90, 90);

  // A fila vem antes do labirinto e é consumida uma vez.
  lidar.enfileirarLeitura({1, 2, 3});
  verificarLeitura(lidar, 1, 2, 3);
  verificarLeitura(lidar, 630, 90, 90);

  // A falha vem antes de tudo e não consome a fila.
  lidar.enfileirarLeitura({4, 5, 6});
  lidar.simularFalha(true);
  TEST_ASSERT_FALSE(lidar.lerDistanciasLaterais(d));
  lidar.simularFalha(false);
  verificarLeitura(lidar, 4, 5, 6);
  verificarLeitura(lidar, 630, 90, 90);

  // Pose fora do labirinto: o sensor não responde, em vez de cair na leitura fixa.
  lidar.posicionar({{9, 9}, Direcao::Norte});
  TEST_ASSERT_FALSE(lidar.lerDistanciasLaterais(d));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_carrega_os_exemplos_dos_tres_tamanhos);
  RUN_TEST(test_carrega_com_ou_sem_quebra_de_linha_final);
  RUN_TEST(test_recusa_tamanho_fora_da_competicao);
  RUN_TEST(test_recusa_caractere_invalido);
  RUN_TEST(test_recusa_parede_incompleta);
  RUN_TEST(test_recusa_perimetro_aberto);
  RUN_TEST(test_texto_invalido_mantem_o_labirinto_anterior);
  RUN_TEST(test_tem_parede_concorda_entre_celulas_vizinhas);
  RUN_TEST(test_todas_as_celulas_dos_exemplos_sao_alcancaveis_da_partida);
  RUN_TEST(test_tem_parede_le_os_quatro_lados_da_celula);
  RUN_TEST(test_tem_parede_fora_do_labirinto_e_verdadeiro);
  RUN_TEST(test_corredor);
  RUN_TEST(test_curva);
  RUN_TEST(test_beco_sem_saida);
  RUN_TEST(test_labirinto_4x4_completo);
  RUN_TEST(test_labirintos_8x4_e_12x4_medem_corredores_longos);
  RUN_TEST(test_distancias_fora_do_labirinto_ou_sem_carga_falham);
  RUN_TEST(test_lidar_posicionado_le_o_que_o_labirinto_mostra);
  RUN_TEST(test_lidar_no_labirinto_respeita_falha_e_fila);
  return UNITY_END();
}
