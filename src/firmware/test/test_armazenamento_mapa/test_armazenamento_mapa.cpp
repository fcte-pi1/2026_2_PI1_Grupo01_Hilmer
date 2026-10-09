/// @file test_armazenamento_mapa.cpp
/// @brief Testes do `ArmazenamentoMapa`: o mapa sobrevive ao reinício do estado de navegação,
/// só é zerado por uma nova corrida e comporta o maior labirinto (12×4). Roda no computador:
/// `pio test -e native -f test_armazenamento_mapa`.

#include <unity.h>

#include <type_traits>
#include <utility>

#include "ArmazenamentoMapa.h"
#include "Mapa.h"

using namespace micromouse;

// Regras garantidas pelo compilador: se alguma for quebrada, este arquivo não compila.
static_assert(!std::is_copy_constructible<ArmazenamentoMapa>::value,
              "ArmazenamentoMapa não pode ser copiado: duas cópias seriam dois mapas divergentes");
static_assert(!std::is_copy_assignable<ArmazenamentoMapa>::value,
              "ArmazenamentoMapa não pode ser atribuído: duas cópias seriam dois mapas divergentes");
static_assert(std::is_same<decltype(std::declval<const ArmazenamentoMapa&>().mapa()), const Mapa&>::value,
              "Pelo acesso const (telemetria), o mapa deve ser somente leitura");

void setUp() {}
void tearDown() {}

/// Substituto mínimo da navegação, que ainda não existe. Como a real, recebe o mapa emprestado
/// e tem um estado próprio (a posição). Destruir e recriar este objeto simula o reinício do
/// estado de navegação.
struct NavegacaoFalsa {
  Mapa& mapa;
  PosicaoCelula posicao{0, 0};

  void moverPara(uint8_t linha, uint8_t coluna) { posicao = PosicaoCelula{linha, coluna}; }

  ResultadoRegistro anotar(Direcao direcao, EstadoParede estado) {
    return mapa.registrarParede(posicao, direcao, estado);
  }

  EstadoParede consultar(Direcao direcao) const { return mapa.obterParede(posicao, direcao); }
};

static PosicaoCelula pos(uint8_t linha, uint8_t coluna) { return PosicaoCelula{linha, coluna}; }

static void verificarParede(EstadoParede esperado, EstadoParede obtido) {
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(esperado), static_cast<uint8_t>(obtido));
}

/// Confere que o mapa está como recém-criado: perímetro `Parede` e interior `Desconhecido`.
static void verificarMapaRecemCriado(const Mapa& mapa) {
  for (uint8_t linha = 0; linha < mapa.obterLinhas(); linha++) {
    for (uint8_t coluna = 0; coluna < mapa.obterColunas(); coluna++) {
      const Mapa::Celula celula = mapa.obterCelula(pos(linha, coluna));
      verificarParede(linha == 0 ? EstadoParede::Parede : EstadoParede::Desconhecido, celula.sul);
      verificarParede(linha == mapa.obterLinhas() - 1 ? EstadoParede::Parede : EstadoParede::Desconhecido,
                      celula.norte);
      verificarParede(coluna == 0 ? EstadoParede::Parede : EstadoParede::Desconhecido, celula.oeste);
      verificarParede(coluna == mapa.obterColunas() - 1 ? EstadoParede::Parede : EstadoParede::Desconhecido,
                      celula.leste);
    }
  }
}

void test_comeca_sem_corrida_iniciada() {
  ArmazenamentoMapa armazenamento;
  TEST_ASSERT_FALSE(armazenamento.corridaIniciada());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DimensaoMapa::Labirinto4x4),
                          static_cast<uint8_t>(armazenamento.mapa().obterDimensao()));
  verificarMapaRecemCriado(armazenamento.mapa());

  armazenamento.iniciarCorrida(DimensaoMapa::Labirinto8x4);
  TEST_ASSERT_TRUE(armazenamento.corridaIniciada());
}

void test_corrida_12x4_comporta_todas_as_paredes() {
  ArmazenamentoMapa armazenamento;
  armazenamento.iniciarCorrida(DimensaoMapa::Labirinto12x4);
  Mapa& mapa = armazenamento.mapa();
  TEST_ASSERT_EQUAL_UINT8(4, mapa.obterLinhas());
  TEST_ASSERT_EQUAL_UINT8(12, mapa.obterColunas());

  // Registra cada parede interna (norte e leste de cada célula), alternando Parede e Livre.
  for (uint8_t linha = 0; linha < 4; linha++) {
    for (uint8_t coluna = 0; coluna < 12; coluna++) {
      const EstadoParede estado = (linha + coluna) % 2 == 0 ? EstadoParede::Parede : EstadoParede::Livre;
      if (linha < 3) {
        TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ResultadoRegistro::Registrada),
                                static_cast<uint8_t>(mapa.registrarParede(pos(linha, coluna), Direcao::Norte, estado)));
      }
      if (coluna < 11) {
        TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ResultadoRegistro::Registrada),
                                static_cast<uint8_t>(mapa.registrarParede(pos(linha, coluna), Direcao::Leste, estado)));
      }
    }
  }

  // Lê tudo de volta, pelos dois lados de cada parede, pelo acesso somente leitura.
  const ArmazenamentoMapa& leitura = armazenamento;
  for (uint8_t linha = 0; linha < 4; linha++) {
    for (uint8_t coluna = 0; coluna < 12; coluna++) {
      const EstadoParede estado = (linha + coluna) % 2 == 0 ? EstadoParede::Parede : EstadoParede::Livre;
      if (linha < 3) {
        verificarParede(estado, leitura.mapa().obterParede(pos(linha, coluna), Direcao::Norte));
        verificarParede(estado, leitura.mapa().obterParede(pos(linha + 1, coluna), Direcao::Sul));
      }
      if (coluna < 11) {
        verificarParede(estado, leitura.mapa().obterParede(pos(linha, coluna), Direcao::Leste));
        verificarParede(estado, leitura.mapa().obterParede(pos(linha, coluna + 1), Direcao::Oeste));
      }
    }
  }
}

void test_mapa_preservado_apos_reiniciar_navegacao() {
  ArmazenamentoMapa armazenamento;
  armazenamento.iniciarCorrida(DimensaoMapa::Labirinto12x4);

  {
    NavegacaoFalsa navegacao{armazenamento.mapa()};
    navegacao.moverPara(0, 0);
    navegacao.anotar(Direcao::Norte, EstadoParede::Livre);
    navegacao.anotar(Direcao::Leste, EstadoParede::Parede);
    navegacao.moverPara(1, 0);
    navegacao.anotar(Direcao::Leste, EstadoParede::Livre);
    navegacao.moverPara(1, 1);
    navegacao.anotar(Direcao::Norte, EstadoParede::Parede);
  }
  const Mapa copiaAntesDoReinicio = armazenamento.mapa();

  // Reinício: a navegação antiga foi destruída ao sair do bloco; uma nova começa do zero.
  NavegacaoFalsa navegacaoReiniciada{armazenamento.mapa()};
  TEST_ASSERT_EQUAL_UINT8(0, navegacaoReiniciada.posicao.linha);
  TEST_ASSERT_EQUAL_UINT8(0, navegacaoReiniciada.posicao.coluna);

  TEST_ASSERT_TRUE(armazenamento.mapa() == copiaAntesDoReinicio);
  TEST_ASSERT_TRUE(armazenamento.corridaIniciada());
}

void test_mapa_preservado_apos_varios_reinicios() {
  ArmazenamentoMapa armazenamento;
  armazenamento.iniciarCorrida(DimensaoMapa::Labirinto12x4);
  constexpr uint8_t CICLOS = 10;

  for (uint8_t ciclo = 0; ciclo < CICLOS; ciclo++) {
    {
      // Cada ciclo é uma navegação nova, que anota uma parede diferente e é destruída.
      NavegacaoFalsa navegacao{armazenamento.mapa()};
      navegacao.moverPara(ciclo % 3, ciclo);
      TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ResultadoRegistro::Registrada),
                              static_cast<uint8_t>(navegacao.anotar(Direcao::Norte, EstadoParede::Parede)));
    }
    const Mapa copia = armazenamento.mapa();

    // Depois do reinício, todas as paredes dos ciclos anteriores continuam lá.
    for (uint8_t anterior = 0; anterior <= ciclo; anterior++) {
      verificarParede(EstadoParede::Parede,
                      armazenamento.mapa().obterParede(pos(anterior % 3, anterior), Direcao::Norte));
    }
    TEST_ASSERT_TRUE(armazenamento.mapa() == copia);
  }
}

void test_navegacao_recriada_enxerga_paredes_anteriores() {
  ArmazenamentoMapa armazenamento;
  armazenamento.iniciarCorrida(DimensaoMapa::Labirinto8x4);

  {
    NavegacaoFalsa antiga{armazenamento.mapa()};
    antiga.moverPara(1, 2);
    antiga.anotar(Direcao::Leste, EstadoParede::Parede);
    antiga.anotar(Direcao::Norte, EstadoParede::Livre);
  }

  NavegacaoFalsa nova{armazenamento.mapa()};
  nova.moverPara(1, 3);
  verificarParede(EstadoParede::Parede, nova.consultar(Direcao::Oeste));
  nova.moverPara(2, 2);
  verificarParede(EstadoParede::Livre, nova.consultar(Direcao::Sul));
}

void test_mapa_e_sempre_o_mesmo_objeto() {
  ArmazenamentoMapa armazenamento;
  const ArmazenamentoMapa& leitura = armazenamento;
  const Mapa* antes = &armazenamento.mapa();
  TEST_ASSERT_EQUAL_PTR(antes, &leitura.mapa());

  // Uma referência guardada antes de uma corrida continua válida depois dela.
  armazenamento.iniciarCorrida(DimensaoMapa::Labirinto4x4);
  TEST_ASSERT_EQUAL_PTR(antes, &armazenamento.mapa());
  armazenamento.iniciarCorrida(DimensaoMapa::Labirinto12x4);
  TEST_ASSERT_EQUAL_PTR(antes, &armazenamento.mapa());
  TEST_ASSERT_EQUAL_UINT8(12, antes->obterColunas());
}

void test_nova_corrida_zera_o_mapa() {
  ArmazenamentoMapa armazenamento;
  armazenamento.iniciarCorrida(DimensaoMapa::Labirinto12x4);
  {
    NavegacaoFalsa navegacao{armazenamento.mapa()};
    navegacao.moverPara(2, 5);
    navegacao.anotar(Direcao::Norte, EstadoParede::Parede);
    navegacao.anotar(Direcao::Leste, EstadoParede::Livre);
  }
  TEST_ASSERT_TRUE(armazenamento.mapa() != Mapa(DimensaoMapa::Labirinto12x4));

  armazenamento.iniciarCorrida(DimensaoMapa::Labirinto12x4);
  TEST_ASSERT_TRUE(armazenamento.mapa() == Mapa(DimensaoMapa::Labirinto12x4));
  verificarMapaRecemCriado(armazenamento.mapa());
}

void test_nova_corrida_pode_trocar_o_tamanho() {
  ArmazenamentoMapa armazenamento;
  armazenamento.iniciarCorrida(DimensaoMapa::Labirinto4x4);
  armazenamento.mapa().registrarParede(pos(1, 1), Direcao::Leste, EstadoParede::Parede);

  armazenamento.iniciarCorrida(DimensaoMapa::Labirinto12x4);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DimensaoMapa::Labirinto12x4),
                          static_cast<uint8_t>(armazenamento.mapa().obterDimensao()));
  TEST_ASSERT_EQUAL_UINT8(4, armazenamento.mapa().obterLinhas());
  TEST_ASSERT_EQUAL_UINT8(12, armazenamento.mapa().obterColunas());
  verificarMapaRecemCriado(armazenamento.mapa());
}

void test_armazenamento_ocupa_pouca_memoria() {
  // O mapa (195 bytes) mais o indicador de corrida: bem abaixo dos 320 KB de RAM do ESP32-C3.
  TEST_ASSERT_LESS_OR_EQUAL_UINT32(256, sizeof(ArmazenamentoMapa));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_comeca_sem_corrida_iniciada);
  RUN_TEST(test_corrida_12x4_comporta_todas_as_paredes);
  RUN_TEST(test_mapa_preservado_apos_reiniciar_navegacao);
  RUN_TEST(test_mapa_preservado_apos_varios_reinicios);
  RUN_TEST(test_navegacao_recriada_enxerga_paredes_anteriores);
  RUN_TEST(test_mapa_e_sempre_o_mesmo_objeto);
  RUN_TEST(test_nova_corrida_zera_o_mapa);
  RUN_TEST(test_nova_corrida_pode_trocar_o_tamanho);
  RUN_TEST(test_armazenamento_ocupa_pouca_memoria);
  return UNITY_END();
}
