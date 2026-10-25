/// @file test_mapa.cpp
/// @brief Testes unitários do `Mapa`: tamanhos, estado inicial, registro de paredes,
/// espelhamento na célula vizinha e igualdade. Roda no computador:
/// `pio test -e native -f test_mapeamento`.

#include <unity.h>

#include "Direcao.h"
#include "Mapa.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

static void verificarDimensoes(const Mapa& mapa, DimensaoMapa dimensao, uint8_t linhas, uint8_t colunas) {
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(dimensao), static_cast<uint8_t>(mapa.obterDimensao()));
  TEST_ASSERT_EQUAL_UINT8(linhas, mapa.obterLinhas());
  TEST_ASSERT_EQUAL_UINT8(colunas, mapa.obterColunas());
}

void test_constroi_labirinto_4x4() {
  verificarDimensoes(Mapa(DimensaoMapa::Labirinto4x4), DimensaoMapa::Labirinto4x4, 4, 4);
}

void test_constroi_labirinto_8x4_com_8_colunas_e_4_linhas() {
  verificarDimensoes(Mapa(DimensaoMapa::Labirinto8x4), DimensaoMapa::Labirinto8x4, 4, 8);
}

void test_constroi_labirinto_12x4_com_12_colunas_e_4_linhas() {
  verificarDimensoes(Mapa(DimensaoMapa::Labirinto12x4), DimensaoMapa::Labirinto12x4, 4, 12);
}

void test_construtor_padrao_cria_labirinto_4x4() {
  Mapa mapa;
  verificarDimensoes(mapa, DimensaoMapa::Labirinto4x4, 4, 4);
}

void test_pos_valida_aceita_apenas_celulas_dentro_do_mapa() {
  const Mapa mapa4(DimensaoMapa::Labirinto4x4);
  TEST_ASSERT_TRUE(mapa4.posValida({0, 0}));
  TEST_ASSERT_TRUE(mapa4.posValida({3, 3}));
  TEST_ASSERT_FALSE(mapa4.posValida({4, 0}));
  TEST_ASSERT_FALSE(mapa4.posValida({0, 4}));
  TEST_ASSERT_FALSE(mapa4.posValida({255, 255}));

  const Mapa mapa8(DimensaoMapa::Labirinto8x4);
  TEST_ASSERT_TRUE(mapa8.posValida({3, 7}));
  TEST_ASSERT_FALSE(mapa8.posValida({3, 8}));
  TEST_ASSERT_FALSE(mapa8.posValida({4, 0}));

  const Mapa mapa12(DimensaoMapa::Labirinto12x4);
  TEST_ASSERT_TRUE(mapa12.posValida({3, 11}));
  TEST_ASSERT_FALSE(mapa12.posValida({3, 12}));
  TEST_ASSERT_FALSE(mapa12.posValida({4, 0}));
}

/// Compara dois estados de parede pelo valor numérico, já que o Unity não conhece o enum.
#define ASSERT_PAREDE(esperado, obtido) \
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(esperado), static_cast<uint8_t>(obtido))

static EstadoParede esperadoNoInicio(bool noPerimetro) {
  return noPerimetro ? EstadoParede::Parede : EstadoParede::Desconhecido;
}

// Perímetro como Parede e interior como Desconhecido, por `obterCelula` e por `obterParede`.
static void verificarEstadoInicial(const Mapa& mapa) {
  for (uint8_t linha = 0; linha < mapa.obterLinhas(); linha++) {
    for (uint8_t coluna = 0; coluna < mapa.obterColunas(); coluna++) {
      const PosicaoCelula posicao = {linha, coluna};
      const Mapa::Celula celula = mapa.obterCelula(posicao);
      ASSERT_PAREDE(esperadoNoInicio(linha == 0), celula.sul);
      ASSERT_PAREDE(esperadoNoInicio(linha == mapa.obterLinhas() - 1), celula.norte);
      ASSERT_PAREDE(esperadoNoInicio(coluna == 0), celula.oeste);
      ASSERT_PAREDE(esperadoNoInicio(coluna == mapa.obterColunas() - 1), celula.leste);
      ASSERT_PAREDE(celula.norte, mapa.obterParede(posicao, Direcao::Norte));
      ASSERT_PAREDE(celula.leste, mapa.obterParede(posicao, Direcao::Leste));
      ASSERT_PAREDE(celula.sul, mapa.obterParede(posicao, Direcao::Sul));
      ASSERT_PAREDE(celula.oeste, mapa.obterParede(posicao, Direcao::Oeste));
    }
  }
}

void test_estado_inicial_perimetro_parede_e_interior_desconhecido_nos_tres_tamanhos() {
  verificarEstadoInicial(Mapa(DimensaoMapa::Labirinto4x4));
  verificarEstadoInicial(Mapa(DimensaoMapa::Labirinto8x4));
  verificarEstadoInicial(Mapa(DimensaoMapa::Labirinto12x4));
}

void test_consultas_fora_do_mapa_retornam_desconhecido() {
  const Mapa mapa(DimensaoMapa::Labirinto4x4);
  const PosicaoCelula foras[] = {{4, 0}, {0, 4}, {255, 255}};
  for (const PosicaoCelula& fora : foras) {
    const Mapa::Celula celula = mapa.obterCelula(fora);
    ASSERT_PAREDE(EstadoParede::Desconhecido, celula.norte);
    ASSERT_PAREDE(EstadoParede::Desconhecido, celula.leste);
    ASSERT_PAREDE(EstadoParede::Desconhecido, celula.sul);
    ASSERT_PAREDE(EstadoParede::Desconhecido, celula.oeste);
    ASSERT_PAREDE(EstadoParede::Desconhecido, mapa.obterParede(fora, Direcao::Norte));
  }
}

/// Compara dois resultados de registro pelo valor numérico, já que o Unity não conhece o enum.
#define ASSERT_RESULTADO(esperado, obtido) \
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(esperado), static_cast<uint8_t>(obtido))

void test_registrar_parede_nova_retorna_registrada_e_grava() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  const PosicaoCelula celula = {1, 1};
  ASSERT_RESULTADO(ResultadoRegistro::Registrada,
                   mapa.registrarParede(celula, Direcao::Norte, EstadoParede::Parede));
  ASSERT_PAREDE(EstadoParede::Parede, mapa.obterParede(celula, Direcao::Norte));
  ASSERT_PAREDE(EstadoParede::Desconhecido, mapa.obterParede(celula, Direcao::Leste));
}

void test_registrar_o_mesmo_estado_retorna_ja_conhecida() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  mapa.registrarParede({1, 1}, Direcao::Norte, EstadoParede::Parede);
  ASSERT_RESULTADO(ResultadoRegistro::JaConhecida,
                   mapa.registrarParede({1, 1}, Direcao::Norte, EstadoParede::Parede));
  ASSERT_RESULTADO(ResultadoRegistro::JaConhecida,
                   mapa.registrarParede({0, 0}, Direcao::Sul, EstadoParede::Parede));
}

void test_registrar_estado_diferente_retorna_conflito_e_mantem_o_original() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  mapa.registrarParede({1, 1}, Direcao::Norte, EstadoParede::Parede);
  ASSERT_RESULTADO(ResultadoRegistro::Conflito,
                   mapa.registrarParede({1, 1}, Direcao::Norte, EstadoParede::Livre));
  ASSERT_PAREDE(EstadoParede::Parede, mapa.obterParede({1, 1}, Direcao::Norte));
}

void test_corrigir_parede_sobrescreve_nos_dois_lados() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  mapa.registrarParede({1, 1}, Direcao::Norte, EstadoParede::Livre);
  ASSERT_RESULTADO(ResultadoRegistro::Registrada, mapa.corrigirParede({1, 1}, Direcao::Norte, EstadoParede::Parede));
  ASSERT_PAREDE(EstadoParede::Parede, mapa.obterParede({1, 1}, Direcao::Norte));
  ASSERT_PAREDE(EstadoParede::Parede, mapa.obterParede({2, 1}, Direcao::Sul));
  ASSERT_RESULTADO(ResultadoRegistro::JaConhecida, mapa.corrigirParede({2, 1}, Direcao::Sul, EstadoParede::Parede));
  // Também grava uma parede ainda desconhecida.
  ASSERT_RESULTADO(ResultadoRegistro::Registrada, mapa.corrigirParede({0, 0}, Direcao::Leste, EstadoParede::Livre));
  ASSERT_PAREDE(EstadoParede::Livre, mapa.obterParede({0, 1}, Direcao::Oeste));
}

void test_corrigir_parede_recusa_entradas_invalidas_e_o_perimetro_livre() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  ASSERT_RESULTADO(ResultadoRegistro::Invalida, mapa.corrigirParede({1, 1}, Direcao::Norte, EstadoParede::Desconhecido));
  ASSERT_RESULTADO(ResultadoRegistro::Invalida, mapa.corrigirParede({4, 0}, Direcao::Norte, EstadoParede::Parede));
  ASSERT_RESULTADO(ResultadoRegistro::Invalida, mapa.corrigirParede({0, 0}, Direcao::Sul, EstadoParede::Livre));
  ASSERT_PAREDE(EstadoParede::Parede, mapa.obterParede({0, 0}, Direcao::Sul));
  ASSERT_RESULTADO(ResultadoRegistro::JaConhecida, mapa.corrigirParede({3, 3}, Direcao::Norte, EstadoParede::Parede));
}

void test_registrar_entradas_invalidas_retorna_invalida() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  ASSERT_RESULTADO(ResultadoRegistro::Invalida,
                   mapa.registrarParede({1, 1}, Direcao::Norte, EstadoParede::Desconhecido));
  ASSERT_RESULTADO(ResultadoRegistro::Invalida,
                   mapa.registrarParede({4, 0}, Direcao::Norte, EstadoParede::Parede));
  ASSERT_RESULTADO(ResultadoRegistro::Invalida,
                   mapa.registrarParede({0, 4}, Direcao::Leste, EstadoParede::Parede));
  ASSERT_PAREDE(EstadoParede::Desconhecido, mapa.obterParede({1, 1}, Direcao::Norte));
}

void test_registrar_espelha_na_celula_vizinha_nas_quatro_direcoes() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  const PosicaoCelula centro = {1, 1};
  mapa.registrarParede(centro, Direcao::Norte, EstadoParede::Parede);
  mapa.registrarParede(centro, Direcao::Leste, EstadoParede::Livre);
  mapa.registrarParede(centro, Direcao::Sul, EstadoParede::Parede);
  mapa.registrarParede(centro, Direcao::Oeste, EstadoParede::Livre);
  ASSERT_PAREDE(EstadoParede::Parede, mapa.obterParede({2, 1}, Direcao::Sul));
  ASSERT_PAREDE(EstadoParede::Livre, mapa.obterParede({1, 2}, Direcao::Oeste));
  ASSERT_PAREDE(EstadoParede::Parede, mapa.obterParede({0, 1}, Direcao::Norte));
  ASSERT_PAREDE(EstadoParede::Livre, mapa.obterParede({1, 0}, Direcao::Leste));
}

static PosicaoCelula pos(int linha, int coluna) {
  return PosicaoCelula{static_cast<uint8_t>(linha), static_cast<uint8_t>(coluna)};
}

static const Direcao TODAS_AS_DIRECOES[] = {Direcao::Norte, Direcao::Leste, Direcao::Sul, Direcao::Oeste};

// Toda parede entre duas células deve ter o mesmo estado vista dos dois lados.
static void verificarConsistencia(const Mapa& mapa) {
  for (int linha = 0; linha < mapa.obterLinhas(); linha++) {
    for (int coluna = 0; coluna < mapa.obterColunas(); coluna++) {
      const PosicaoCelula celula = pos(linha, coluna);
      for (Direcao direcao : TODAS_AS_DIRECOES) {
        PosicaoCelula vizinhaPosicao = {0, 0};
        if (vizinha(celula, direcao, vizinhaPosicao) && mapa.posValida(vizinhaPosicao)) {
          ASSERT_PAREDE(mapa.obterParede(celula, direcao),
                        mapa.obterParede(vizinhaPosicao, oposta(direcao)));
        }
      }
    }
  }
}

// Registra uma vez cada aresta interna, alternando Parede e Livre, e confere o lado da vizinha.
static void registrarTodasAsArestasInternas(Mapa& mapa) {
  for (int linha = 0; linha < mapa.obterLinhas(); linha++) {
    for (int coluna = 0; coluna < mapa.obterColunas(); coluna++) {
      const EstadoParede estado =
          ((linha + coluna) % 2 == 0) ? EstadoParede::Parede : EstadoParede::Livre;
      const PosicaoCelula celula = pos(linha, coluna);
      if (linha + 1 < mapa.obterLinhas()) {
        ASSERT_RESULTADO(ResultadoRegistro::Registrada, mapa.registrarParede(celula, Direcao::Norte, estado));
        ASSERT_PAREDE(estado, mapa.obterParede(pos(linha + 1, coluna), Direcao::Sul));
      }
      if (coluna + 1 < mapa.obterColunas()) {
        ASSERT_RESULTADO(ResultadoRegistro::Registrada, mapa.registrarParede(celula, Direcao::Leste, estado));
        ASSERT_PAREDE(estado, mapa.obterParede(pos(linha, coluna + 1), Direcao::Oeste));
      }
    }
  }
}

void test_espelhamento_em_todas_as_arestas_internas_dos_tres_tamanhos() {
  Mapa mapas[] = {Mapa(DimensaoMapa::Labirinto4x4), Mapa(DimensaoMapa::Labirinto8x4),
                  Mapa(DimensaoMapa::Labirinto12x4)};
  for (Mapa& mapa : mapas) {
    registrarTodasAsArestasInternas(mapa);
    verificarConsistencia(mapa);
  }
}

void test_perimetro_recusa_livre_e_aceita_parede_repetida() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  ASSERT_RESULTADO(ResultadoRegistro::Conflito, mapa.registrarParede(pos(0, 2), Direcao::Sul, EstadoParede::Livre));
  ASSERT_RESULTADO(ResultadoRegistro::Conflito, mapa.registrarParede(pos(3, 0), Direcao::Oeste, EstadoParede::Livre));
  ASSERT_RESULTADO(ResultadoRegistro::Conflito, mapa.registrarParede(pos(3, 3), Direcao::Norte, EstadoParede::Livre));
  ASSERT_RESULTADO(ResultadoRegistro::Conflito, mapa.registrarParede(pos(1, 3), Direcao::Leste, EstadoParede::Livre));
  ASSERT_PAREDE(EstadoParede::Parede, mapa.obterParede(pos(0, 2), Direcao::Sul));
  ASSERT_RESULTADO(ResultadoRegistro::JaConhecida, mapa.registrarParede(pos(1, 3), Direcao::Leste, EstadoParede::Parede));
}

void test_mapas_com_as_mesmas_paredes_sao_iguais() {
  Mapa a(DimensaoMapa::Labirinto4x4);
  Mapa b(DimensaoMapa::Labirinto4x4);
  TEST_ASSERT_TRUE(a == b);
  TEST_ASSERT_FALSE(a != b);

  a.registrarParede({1, 1}, Direcao::Norte, EstadoParede::Parede);
  TEST_ASSERT_TRUE(a != b);

  b.registrarParede({1, 1}, Direcao::Norte, EstadoParede::Parede);
  TEST_ASSERT_TRUE(a == b);
}

void test_registrar_pelo_lado_da_vizinha_gera_o_mesmo_mapa() {
  Mapa a(DimensaoMapa::Labirinto4x4);
  Mapa b(DimensaoMapa::Labirinto4x4);
  a.registrarParede({1, 1}, Direcao::Norte, EstadoParede::Parede);
  b.registrarParede({2, 1}, Direcao::Sul, EstadoParede::Parede);
  TEST_ASSERT_TRUE(a == b);
}

void test_mapas_de_tamanhos_diferentes_sao_diferentes() {
  TEST_ASSERT_TRUE(Mapa(DimensaoMapa::Labirinto4x4) != Mapa(DimensaoMapa::Labirinto8x4));
}

static uint32_t proximoAleatorio(uint32_t& estado) {
  estado = estado * 1664525u + 1013904223u;
  return estado >> 16;
}

// Registros pseudoaleatórios com semente fixa: o teste é reproduzível.
static void registrarAleatoriamente(Mapa& mapa, uint32_t semente, int quantidade) {
  uint32_t estado = semente;
  for (int i = 0; i < quantidade; i++) {
    const int linha = proximoAleatorio(estado) % mapa.obterLinhas();
    const int coluna = proximoAleatorio(estado) % mapa.obterColunas();
    const Direcao direcao = static_cast<Direcao>(proximoAleatorio(estado) % 4);
    const EstadoParede estadoParede =
        (proximoAleatorio(estado) % 2 == 0) ? EstadoParede::Parede : EstadoParede::Livre;
    mapa.registrarParede(pos(linha, coluna), direcao, estadoParede);
  }
}

void test_consistencia_se_mantem_apos_registros_pseudoaleatorios() {
  Mapa mapas[] = {Mapa(DimensaoMapa::Labirinto4x4), Mapa(DimensaoMapa::Labirinto8x4),
                  Mapa(DimensaoMapa::Labirinto12x4)};
  for (Mapa& mapa : mapas) {
    registrarAleatoriamente(mapa, 12345u, 500);
    verificarConsistencia(mapa);
  }
}

void test_mapa_ocupa_pouca_memoria() {
  // 12 colunas x 4 linhas de 4 bytes, mais o cabeçalho: bem abaixo dos 320 KB de RAM do ESP32-C3.
  TEST_ASSERT_LESS_OR_EQUAL_UINT32(256, sizeof(Mapa));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_constroi_labirinto_4x4);
  RUN_TEST(test_constroi_labirinto_8x4_com_8_colunas_e_4_linhas);
  RUN_TEST(test_constroi_labirinto_12x4_com_12_colunas_e_4_linhas);
  RUN_TEST(test_construtor_padrao_cria_labirinto_4x4);
  RUN_TEST(test_pos_valida_aceita_apenas_celulas_dentro_do_mapa);
  RUN_TEST(test_estado_inicial_perimetro_parede_e_interior_desconhecido_nos_tres_tamanhos);
  RUN_TEST(test_consultas_fora_do_mapa_retornam_desconhecido);
  RUN_TEST(test_registrar_parede_nova_retorna_registrada_e_grava);
  RUN_TEST(test_registrar_o_mesmo_estado_retorna_ja_conhecida);
  RUN_TEST(test_registrar_estado_diferente_retorna_conflito_e_mantem_o_original);
  RUN_TEST(test_registrar_entradas_invalidas_retorna_invalida);
  RUN_TEST(test_registrar_espelha_na_celula_vizinha_nas_quatro_direcoes);
  RUN_TEST(test_espelhamento_em_todas_as_arestas_internas_dos_tres_tamanhos);
  RUN_TEST(test_perimetro_recusa_livre_e_aceita_parede_repetida);
  RUN_TEST(test_mapas_com_as_mesmas_paredes_sao_iguais);
  RUN_TEST(test_registrar_pelo_lado_da_vizinha_gera_o_mesmo_mapa);
  RUN_TEST(test_mapas_de_tamanhos_diferentes_sao_diferentes);
  RUN_TEST(test_consistencia_se_mantem_apos_registros_pseudoaleatorios);
  RUN_TEST(test_mapa_ocupa_pouca_memoria);
  RUN_TEST(test_corrigir_parede_sobrescreve_nos_dois_lados);
  RUN_TEST(test_corrigir_parede_recusa_entradas_invalidas_e_o_perimetro_livre);
  return UNITY_END();
}
