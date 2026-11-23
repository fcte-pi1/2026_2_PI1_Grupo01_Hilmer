/// @file test_localizador_largada.cpp
/// @brief Testes do `LocalizadorLargada` (largada em qualquer célula): as hipóteses de posição e
/// tamanho, as três regras que as descartam e o mapa real montado a partir da exploração; e o Flood
/// Fill da `Navegacao` com vários objetivos, que leva o robô ao candidato mais próximo.
/// Roda no computador: `pio test -e native -f test_localizador_largada`.

#include <unity.h>

#include "LocalizadorLargada.h"
#include "Mapa.h"
#include "Navegacao.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

constexpr PosicaoCelula LARGADA = LocalizadorLargada::LARGADA_NA_EXPLORACAO;

/// Célula da exploração a `dl` linhas e `dc` colunas da largada.
PosicaoCelula aPartirDaLargada(int dl, int dc) {
  return PosicaoCelula{static_cast<uint8_t>(LARGADA.linha + dl), static_cast<uint8_t>(LARGADA.coluna + dc)};
}

bool viva(const LocalizadorLargada& loc, uint8_t linha, uint8_t coluna, uint8_t colunas) {
  for (uint8_t i = 0; i < loc.quantidade(); ++i) {
    const HipoteseLargada h = loc.hipotese(i);
    if (h.linha == linha && h.coluna == coluna && h.colunas == colunas) {
      return true;
    }
  }
  return false;
}

/// Grava no mapa de exploração um labirinto 4×4 todo aberto por dentro, com a largada na célula real
/// (`linha`, `coluna`): o perímetro dele vira parede e as paredes internas, livres.
void gravarQuatroPorQuatroAberto(Mapa& exploracao, uint8_t linha, uint8_t coluna) {
  const uint8_t l0 = static_cast<uint8_t>(LARGADA.linha - linha);
  const uint8_t c0 = static_cast<uint8_t>(LARGADA.coluna - coluna);
  for (uint8_t l = 0; l < 4; ++l) {
    for (uint8_t c = 0; c < 4; ++c) {
      const PosicaoCelula p = {static_cast<uint8_t>(l0 + l), static_cast<uint8_t>(c0 + c)};
      exploracao.registrarParede(p, Direcao::Norte, l == 3 ? EstadoParede::Parede : EstadoParede::Livre);
      exploracao.registrarParede(p, Direcao::Sul, l == 0 ? EstadoParede::Parede : EstadoParede::Livre);
      exploracao.registrarParede(p, Direcao::Leste, c == 3 ? EstadoParede::Parede : EstadoParede::Livre);
      exploracao.registrarParede(p, Direcao::Oeste, c == 0 ? EstadoParede::Parede : EstadoParede::Livre);
    }
  }
}

}  // namespace

void test_comeca_com_todas_as_hipoteses() {
  LocalizadorLargada loc;
  TEST_ASSERT_EQUAL_UINT8(96, loc.quantidade());  // 4 linhas x (4 + 8 + 12) colunas
  TEST_ASSERT_FALSE(loc.resolvido());
  TEST_ASSERT_TRUE(loc.visitada(LARGADA));
  // Objetivo de cada hipótese: o canto Nordeste, no referencial da exploração.
  const PosicaoCelula objetivo = LocalizadorLargada::objetivoNaExploracao(HipoteseLargada{0, 0, 4});
  TEST_ASSERT_EQUAL_UINT8(LARGADA.linha + 3, objetivo.linha);
  TEST_ASSERT_EQUAL_UINT8(LARGADA.coluna + 3, objetivo.coluna);
}

void test_celula_visitada_descarta_quem_nao_a_contem() {
  Mapa exploracao(DimensaoMapa::Exploracao);
  LocalizadorLargada loc;
  // Três linhas ao Norte da largada: a largada só pode estar na linha 0.
  loc.visitar(aPartirDaLargada(3, 0));
  loc.atualizar(exploracao);
  for (uint8_t i = 0; i < loc.quantidade(); ++i) {
    TEST_ASSERT_EQUAL_UINT8(0, loc.hipotese(i).linha);
  }
  TEST_ASSERT_TRUE(viva(loc, 0, 0, 4));
  TEST_ASSERT_FALSE(viva(loc, 1, 0, 4));
}

void test_parede_livre_no_perimetro_descarta() {
  Mapa exploracao(DimensaoMapa::Exploracao);
  LocalizadorLargada loc;
  // Livre ao Sul da largada: ela não está na linha 0.
  exploracao.registrarParede(LARGADA, Direcao::Sul, EstadoParede::Livre);
  loc.atualizar(exploracao);
  TEST_ASSERT_FALSE(viva(loc, 0, 2, 8));
  TEST_ASSERT_TRUE(viva(loc, 1, 2, 8));
}

void test_regiao_fechada_resolve_tamanho_e_posicao() {
  Mapa exploracao(DimensaoMapa::Exploracao);
  LocalizadorLargada loc;
  gravarQuatroPorQuatroAberto(exploracao, 1, 2);
  loc.atualizar(exploracao);
  // A região de 4x4 fechada: um labirinto maior teria células isoladas (regra 3), e a posição só
  // cabe de um jeito (regra 2: as paredes internas livres não podem ser perímetro).
  TEST_ASSERT_TRUE(loc.resolvido());
  TEST_ASSERT_TRUE(viva(loc, 1, 2, 4));
  TEST_ASSERT_FALSE(loc.conectividadeDescartada());
  TEST_ASSERT_TRUE(loc.objetivoDeTodas(LocalizadorLargada::objetivoNaExploracao(HipoteseLargada{1, 2, 4})));
}

void test_sem_conectividade_quando_ela_descarta_tudo() {
  Mapa exploracao(DimensaoMapa::Exploracao);
  LocalizadorLargada loc;
  // A largada fechada nos 4 lados: nenhum labirinto conectado a contém. A regra 3 sai, e sobram as
  // hipóteses das outras duas regras.
  const Direcao todas[4] = {Direcao::Norte, Direcao::Leste, Direcao::Sul, Direcao::Oeste};
  for (Direcao d : todas) {
    exploracao.registrarParede(LARGADA, d, EstadoParede::Parede);
  }
  loc.atualizar(exploracao);
  TEST_ASSERT_TRUE(loc.conectividadeDescartada());
  TEST_ASSERT_EQUAL_UINT8(96, loc.quantidade());
}

void test_objetivos_distintos_e_sem_repeticao() {
  LocalizadorLargada loc;
  PosicaoCelula objetivos[LocalizadorLargada::MAX_HIPOTESES];
  const uint16_t n = loc.objetivos(objetivos, LocalizadorLargada::MAX_HIPOTESES);
  // (0, 1) num 4x4 e (0, 5) num 8x4 levam ao mesmo canto: há menos objetivos que hipóteses.
  TEST_ASSERT_LESS_THAN_UINT16(96, n);
  for (uint16_t i = 0; i < n; ++i) {
    for (uint16_t j = i + 1; j < n; ++j) {
      TEST_ASSERT_FALSE(objetivos[i].linha == objetivos[j].linha && objetivos[i].coluna == objetivos[j].coluna);
    }
  }
}

void test_mapa_real_copia_as_paredes_vistas() {
  Mapa exploracao(DimensaoMapa::Exploracao);
  const HipoteseLargada h = {1, 2, 8};
  // Parede ao Leste e passagem ao Norte da largada, que é a célula real (1, 2).
  exploracao.registrarParede(LARGADA, Direcao::Leste, EstadoParede::Parede);
  exploracao.registrarParede(LARGADA, Direcao::Norte, EstadoParede::Livre);
  const Mapa real = LocalizadorLargada::mapaReal(exploracao, h);
  TEST_ASSERT_EQUAL(DimensaoMapa::Labirinto8x4, real.obterDimensao());
  TEST_ASSERT_EQUAL(EstadoParede::Parede, real.obterParede({1, 2}, Direcao::Leste));
  TEST_ASSERT_EQUAL(EstadoParede::Parede, real.obterParede({1, 3}, Direcao::Oeste));
  TEST_ASSERT_EQUAL(EstadoParede::Livre, real.obterParede({1, 2}, Direcao::Norte));
  TEST_ASSERT_EQUAL(EstadoParede::Desconhecido, real.obterParede({1, 2}, Direcao::Sul));
  TEST_ASSERT_EQUAL(EstadoParede::Parede, real.obterParede({0, 0}, Direcao::Sul));  // perímetro
}

void test_flood_fill_com_varios_objetivos_vai_ao_mais_proximo() {
  Mapa mapa(DimensaoMapa::Exploracao);
  Navegacao navegacao(mapa);
  navegacao.definirPose(Pose{LARGADA, Direcao::Norte});
  const PosicaoCelula objetivos[2] = {aPartirDaLargada(0, 5), aPartirDaLargada(0, -2)};
  navegacao.definirObjetivos(objetivos, 2);
  TEST_ASSERT_EQUAL_UINT8(2, navegacao.obterDistancia(LARGADA));
  Direcao direcao = Direcao::Norte;
  TEST_ASSERT_TRUE(navegacao.decidirProximaDirecao(direcao));
  TEST_ASSERT_EQUAL(Direcao::Oeste, direcao);
  TEST_ASSERT_FALSE(navegacao.atingiuObjetivo());
  navegacao.definirPose(Pose{aPartirDaLargada(0, 5), Direcao::Leste});
  TEST_ASSERT_TRUE(navegacao.atingiuObjetivo());
  // Sem objetivos: sem caminho.
  navegacao.definirObjetivos(nullptr, 0);
  navegacao.definirPose(Pose{LARGADA, Direcao::Norte});
  TEST_ASSERT_FALSE(navegacao.decidirProximaDirecao(direcao));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_comeca_com_todas_as_hipoteses);
  RUN_TEST(test_celula_visitada_descarta_quem_nao_a_contem);
  RUN_TEST(test_parede_livre_no_perimetro_descarta);
  RUN_TEST(test_regiao_fechada_resolve_tamanho_e_posicao);
  RUN_TEST(test_sem_conectividade_quando_ela_descarta_tudo);
  RUN_TEST(test_objetivos_distintos_e_sem_repeticao);
  RUN_TEST(test_mapa_real_copia_as_paredes_vistas);
  RUN_TEST(test_flood_fill_com_varios_objetivos_vai_ao_mais_proximo);
  return UNITY_END();
}
