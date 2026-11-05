/// @file test_integracao_mapa.cpp
/// @brief Integração percepção → mapa: o robô simulado explora um labirinto inteiro e o mapa
/// final deve ser idêntico ao labirinto (critério de pronto da #177).
/// Roda no computador: `pio test -e native -f test_integracao_mapa`.
///
/// Cadeia testada, sem hardware: `LabirintoSimulado` → `SensorDistanciaSimulado` → `ClassificadorParede`
/// → `AtualizadorMapa` → `Mapa`.

#include <unity.h>

#include "AtualizadorMapa.h"
#include "ClassificadorParede.h"
#include "Direcao.h"
#include "LabirintoSimulado.h"
#include "SensorDistanciaSimulado.h"
#include "Mapa.h"
#include "../test_labirinto_simulado/LabirintosExemplo.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

/// Robô simulado que explora o labirinto em profundidade usando só o que o sensor mostra:
/// a cada célula nova ele lê os sensores na pose atual, atualiza o mapa e segue pelas direções
/// que o próprio mapa diz estarem livres.
class ExploradorSimulado {
 public:
  ExploradorSimulado(const LabirintoSimulado& labirinto, Mapa& mapa)
      : mapa_(mapa) {
    sensorSimulado_.usarLabirinto(labirinto);
  }

  /// Explora a partir da partida (0, 0), virado para o Norte.
  void explorar() { visitar({0, 0}, Direcao::Norte); }

  uint8_t celulasVisitadas() const { return visitadas_; }
  uint8_t registrosComConflito() const { return conflitos_; }
  uint8_t registrosInvalidos() const { return invalidos_; }
  uint8_t leiturasQueFalharam() const { return falhas_; }

 private:
  void visitar(PosicaoCelula celula, Direcao direcao) {
    visitada_[celula.linha][celula.coluna] = true;
    visitadas_++;

    const Pose pose = {celula, direcao};
    sensorSimulado_.posicionar(pose);
    DistanciasLaterais leitura = {};
    if (!sensorSimulado_.lerDistanciasLaterais(leitura)) {
      falhas_++;
      return;
    }
    const ResultadoAtualizacao r =
        AtualizadorMapa::atualizar(mapa_, pose, classificador_, &leitura, 1);
    conflitos_ += r.temConflito ? 1 : 0;
    invalidos_ += (r.frente == ResultadoRegistro::Invalida && r.esquerda == ResultadoRegistro::Invalida &&
                   r.direita == ResultadoRegistro::Invalida)
                      ? 1
                      : 0;

    const Direcao direcoes[] = {Direcao::Norte, Direcao::Leste, Direcao::Sul, Direcao::Oeste};
    for (Direcao d : direcoes) {
      PosicaoCelula proxima;
      if (mapa_.obterParede(celula, d) == EstadoParede::Livre && vizinha(celula, d, proxima) &&
          mapa_.posValida(proxima) && !visitada_[proxima.linha][proxima.coluna]) {
        visitar(proxima, d);
      }
    }
  }

  Mapa& mapa_;
  SensorDistanciaSimulado sensorSimulado_;
  ClassificadorParede classificador_;
  bool visitada_[MAX_LINHAS_LABIRINTO][MAX_COLUNAS_LABIRINTO] = {};
  uint8_t visitadas_ = 0;
  uint8_t conflitos_ = 0;
  uint8_t invalidos_ = 0;
  uint8_t falhas_ = 0;
};

/// Número de paredes em que o mapa discorda do labirinto (parede ou livre).
uint16_t paredesDiferentes(const Mapa& mapa, const LabirintoSimulado& labirinto) {
  const Direcao direcoes[] = {Direcao::Norte, Direcao::Leste, Direcao::Sul, Direcao::Oeste};
  uint16_t diferentes = 0;
  for (uint8_t linha = 0; linha < labirinto.obterLinhas(); linha++) {
    for (uint8_t coluna = 0; coluna < labirinto.obterColunas(); coluna++) {
      for (Direcao d : direcoes) {
        const EstadoParede esperado =
            labirinto.temParede({linha, coluna}, d) ? EstadoParede::Parede : EstadoParede::Livre;
        if (mapa.obterParede({linha, coluna}, d) != esperado) {
          diferentes++;
        }
      }
    }
  }
  return diferentes;
}

/// Carrega o texto, explora e confere que o mapa final é idêntico ao labirinto.
void explorarEConferir(const char* texto, DimensaoMapa dimensao) {
  LabirintoSimulado labirinto;
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ResultadoCarga::Carregado),
                          static_cast<uint8_t>(labirinto.carregar(texto)));
  Mapa mapa(dimensao);
  ExploradorSimulado explorador(labirinto, mapa);

  explorador.explorar();

  TEST_ASSERT_EQUAL_UINT8(labirinto.obterLinhas() * labirinto.obterColunas(),
                          explorador.celulasVisitadas());
  TEST_ASSERT_EQUAL_UINT8(0, explorador.leiturasQueFalharam());
  TEST_ASSERT_EQUAL_UINT8(0, explorador.registrosComConflito());
  TEST_ASSERT_EQUAL_UINT8(0, explorador.registrosInvalidos());
  TEST_ASSERT_EQUAL_UINT16(0, paredesDiferentes(mapa, labirinto));
}

}  // namespace

void test_mapa_final_identico_ao_labirinto_4x4() {
  explorarEConferir(exemplos::LABIRINTO_4X4, DimensaoMapa::Labirinto4x4);
}

void test_mapa_final_identico_ao_labirinto_8x4() {
  explorarEConferir(exemplos::LABIRINTO_8X4, DimensaoMapa::Labirinto8x4);
}

void test_mapa_final_identico_ao_labirinto_12x4() {
  explorarEConferir(exemplos::LABIRINTO_12X4, DimensaoMapa::Labirinto12x4);
}

void test_mapa_nao_e_identico_antes_de_explorar() {
  // Garante que a comparação detecta diferença: o mapa vazio só conhece o perímetro.
  LabirintoSimulado labirinto;
  labirinto.carregar(exemplos::LABIRINTO_4X4);
  const Mapa vazio(DimensaoMapa::Labirinto4x4);
  TEST_ASSERT_GREATER_THAN_UINT16(0, paredesDiferentes(vazio, labirinto));
}

void test_uma_celula_registra_so_as_tres_paredes_que_o_sensor_ve() {
  LabirintoSimulado labirinto;
  labirinto.carregar(exemplos::LABIRINTO_4X4);
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  SensorDistanciaSimulado sensorSimulado;
  sensorSimulado.usarLabirinto(labirinto);
  ClassificadorParede classificador;

  // Em (1, 0) virado para Leste: frente = (1, 1) livre, esquerda (Norte) livre, direita (Sul) livre.
  const Pose pose = {{1, 0}, Direcao::Leste};
  sensorSimulado.posicionar(pose);
  DistanciasLaterais leitura = {};
  TEST_ASSERT_TRUE(sensorSimulado.lerDistanciasLaterais(leitura));
  AtualizadorMapa::atualizar(mapa, pose, classificador, &leitura, 1);

  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(EstadoParede::Livre),
                          static_cast<uint8_t>(mapa.obterParede({1, 0}, Direcao::Leste)));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(EstadoParede::Livre),
                          static_cast<uint8_t>(mapa.obterParede({1, 0}, Direcao::Norte)));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(EstadoParede::Livre),
                          static_cast<uint8_t>(mapa.obterParede({1, 0}, Direcao::Sul)));
  // O lado de trás (Oeste) é o perímetro: já era Parede.
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(EstadoParede::Parede),
                          static_cast<uint8_t>(mapa.obterParede({1, 0}, Direcao::Oeste)));
  // A vizinha a Leste recebeu o espelho da parede registrada.
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(EstadoParede::Livre),
                          static_cast<uint8_t>(mapa.obterParede({1, 1}, Direcao::Oeste)));
  // Nada além disso foi visto: o resto da vizinha continua desconhecido.
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(EstadoParede::Desconhecido),
                          static_cast<uint8_t>(mapa.obterParede({1, 1}, Direcao::Norte)));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_mapa_final_identico_ao_labirinto_4x4);
  RUN_TEST(test_mapa_final_identico_ao_labirinto_8x4);
  RUN_TEST(test_mapa_final_identico_ao_labirinto_12x4);
  RUN_TEST(test_mapa_nao_e_identico_antes_de_explorar);
  RUN_TEST(test_uma_celula_registra_so_as_tres_paredes_que_o_sensor_ve);
  return UNITY_END();
}
