/// @file test_resolucao_mapa.cpp
/// @brief Testes da Issue #183 (HU-01): Algoritmo de resolução sobre o mapa (Flood Fill).
/// Roda no computador: `pio test -e native -f test_resolucao_mapa`.

#include <unity.h>

#include "AtualizadorMapa.h"
#include "ClassificadorParede.h"
#include "Direcao.h"
#include "LabirintoSimulado.h"
#include "SensorDistanciaSimulado.h"
#include "Mapa.h"
#include "Navegacao.h"
#include "../test_labirinto_simulado/LabirintosExemplo.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

PosicaoCelula pos(uint8_t linha, uint8_t coluna) {
  return PosicaoCelula{linha, coluna};
}

}  // namespace

/// @brief Verifica se as distâncias calculadas no mapa sem paredes internas correspondem à distância de Manhattan.
void test_flood_fill_mapa_vazio_4x4() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  Navegacao navegacao(mapa);  // Objetivo padrão: (3, 3)

  TEST_ASSERT_EQUAL_UINT8(3, navegacao.obterObjetivo().linha);
  TEST_ASSERT_EQUAL_UINT8(3, navegacao.obterObjetivo().coluna);

  // Distância do objetivo a si mesmo é 0
  TEST_ASSERT_EQUAL_UINT8(0, navegacao.obterDistancia(pos(3, 3)));

  // Em mapa sem paredes internas, a distância até (3, 3) é (3 - linha) + (3 - coluna)
  for (uint8_t l = 0; l < 4; ++l) {
    for (uint8_t c = 0; c < 4; ++c) {
      uint8_t esperado = (3 - l) + (3 - c);
      TEST_ASSERT_EQUAL_UINT8(esperado, navegacao.obterDistancia(pos(l, c)));
    }
  }

  // Da origem (0, 0) virado para o Norte, deve preferir Norte (continua em frente)
  Direcao dir;
  TEST_ASSERT_TRUE(navegacao.decidirProximaDirecao(dir));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Direcao::Norte), static_cast<uint8_t>(dir));
}

/// @brief Verifica se o algoritmo contorna uma parede conhecida.
void test_contorno_parede_conhecida() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);

  // Bloqueia a passagem de (0, 0) para o Norte (1, 0)
  mapa.registrarParede(pos(0, 0), Direcao::Norte, EstadoParede::Parede);

  Navegacao navegacao(mapa);
  navegacao.moverPara(0, 0, Direcao::Norte);

  // Como o Norte está bloqueado por parede, a próxima direção viável para (3, 3) deve ser Leste
  Direcao dir;
  TEST_ASSERT_TRUE(navegacao.decidirProximaDirecao(dir));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Direcao::Leste), static_cast<uint8_t>(dir));
}

/// @brief Verifica que, quando não há rota acessível até o objetivo, a decisão falha graciosamente.
void test_sem_caminho_ate_objetivo() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);

  // Enclausura a célula de partida (0, 0)
  mapa.registrarParede(pos(0, 0), Direcao::Norte, EstadoParede::Parede);
  mapa.registrarParede(pos(0, 0), Direcao::Leste, EstadoParede::Parede);

  Navegacao navegacao(mapa);
  navegacao.moverPara(0, 0, Direcao::Norte);

  TEST_ASSERT_EQUAL_UINT8(Navegacao::DISTANCIA_INFINITA, navegacao.obterDistancia(pos(0, 0)));

  Direcao dir;
  TEST_ASSERT_FALSE(navegacao.decidirProximaDirecao(dir));
}

/// @brief darPasso() só registra a célula esperada: a pose não muda até os sensores confirmarem.
void test_dar_passo_aguarda_confirmacao_dos_sensores() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  Navegacao navegacao(mapa);  // (0, 0) voltado para o Norte

  Direcao dir;
  TEST_ASSERT_TRUE(navegacao.darPasso(dir));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Direcao::Norte), static_cast<uint8_t>(dir));
  TEST_ASSERT_TRUE(navegacao.passoPendente());
  TEST_ASSERT_EQUAL_UINT8(1, navegacao.obterCelulaEsperada().linha);
  TEST_ASSERT_EQUAL_UINT8(0, navegacao.obterCelulaEsperada().coluna);

  // A pose continua na origem
  TEST_ASSERT_EQUAL_UINT8(0, navegacao.obterPosicao().linha);
  TEST_ASSERT_EQUAL_UINT8(0, navegacao.obterPosicao().coluna);

  // Não inicia outro passo enquanto o atual não for confirmado
  TEST_ASSERT_FALSE(navegacao.darPasso(dir));
}

/// @brief Enquanto os sensores indicam a célula de origem o passo segue pendente; ao indicarem a
/// célula esperada, a pose é atualizada e o passo é concluído.
void test_confirmar_pose_em_andamento_e_chegada() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  Navegacao navegacao(mapa);

  Direcao dir;
  TEST_ASSERT_TRUE(navegacao.darPasso(dir));

  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ResultadoConfirmacao::EmAndamento),
                          static_cast<uint8_t>(navegacao.confirmarPose({pos(0, 0), Direcao::Norte})));
  TEST_ASSERT_TRUE(navegacao.passoPendente());

  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ResultadoConfirmacao::Confirmada),
                          static_cast<uint8_t>(navegacao.confirmarPose({pos(1, 0), Direcao::Norte})));
  TEST_ASSERT_FALSE(navegacao.passoPendente());
  TEST_ASSERT_EQUAL_UINT8(1, navegacao.obterPosicao().linha);
  TEST_ASSERT_EQUAL_UINT8(0, navegacao.obterPosicao().coluna);
}

/// @brief Se os sensores indicam uma célula diferente da esperada, a pose medida é adotada, o
/// passo é cancelado e o próximo passo replaneja a partir dela.
void test_confirmar_pose_divergente_replaneja() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  Navegacao navegacao(mapa);

  Direcao dir;
  TEST_ASSERT_TRUE(navegacao.darPasso(dir));  // espera (1, 0)

  // Os sensores indicam (0, 1): o robô derivou para Leste
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ResultadoConfirmacao::Divergente),
                          static_cast<uint8_t>(navegacao.confirmarPose({pos(0, 1), Direcao::Leste})));
  TEST_ASSERT_FALSE(navegacao.passoPendente());
  TEST_ASSERT_EQUAL_UINT8(0, navegacao.obterPosicao().linha);
  TEST_ASSERT_EQUAL_UINT8(1, navegacao.obterPosicao().coluna);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Direcao::Leste),
                          static_cast<uint8_t>(navegacao.obterDirecao()));

  // Replaneja de (0, 1) voltado para Leste: segue em frente
  TEST_ASSERT_TRUE(navegacao.darPasso(dir));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Direcao::Leste), static_cast<uint8_t>(dir));
  TEST_ASSERT_EQUAL_UINT8(0, navegacao.obterCelulaEsperada().linha);
  TEST_ASSERT_EQUAL_UINT8(2, navegacao.obterCelulaEsperada().coluna);
}

/// @brief Uma pose fora do mapa é ignorada: a pose atual e o passo pendente não mudam.
void test_confirmar_pose_invalida_ignorada() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  Navegacao navegacao(mapa);

  Direcao dir;
  TEST_ASSERT_TRUE(navegacao.darPasso(dir));

  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ResultadoConfirmacao::Invalida),
                          static_cast<uint8_t>(navegacao.confirmarPose({pos(9, 9), Direcao::Norte})));
  TEST_ASSERT_TRUE(navegacao.passoPendente());
  TEST_ASSERT_EQUAL_UINT8(0, navegacao.obterPosicao().linha);
  TEST_ASSERT_EQUAL_UINT8(0, navegacao.obterPosicao().coluna);
}

/// @brief Verifica se o robô atinge o objetivo e encerra a navegação.
void test_atingimento_de_objetivo() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  Navegacao navegacao(mapa);

  // Move diretamente para o objetivo
  navegacao.moverPara(3, 3, Direcao::Norte);

  TEST_ASSERT_TRUE(navegacao.atingiuObjetivo());

  Direcao dir;
  TEST_ASSERT_FALSE(navegacao.decidirProximaDirecao(dir));
  TEST_ASSERT_FALSE(navegacao.darPasso(dir));
}

namespace {

/// Navega no labirinto simulado, partindo de (0, 0) voltada para o Norte com o mapa vazio,
/// até alcançar o objetivo padrão (canto oposto) ou esgotar `maxPassos`.
///
/// A cada passo confere que o robô não atravessou uma parede real do labirinto. Uma leitura
/// do sensor que falha não gera decisão: o robô fica parado e lê de novo.
/// @param falharACada Faz falhar uma a cada `falharACada` leituras (0 = nunca falha).
void resolverLabirintoSimulado(const char* texto, DimensaoMapa dimensao, uint16_t maxPassos,
                               uint16_t falharACada = 0) {
  LabirintoSimulado labirinto;
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ResultadoCarga::Carregado),
                          static_cast<uint8_t>(labirinto.carregar(texto)));

  SensorDistanciaSimulado sensorSimulado;
  sensorSimulado.usarLabirinto(labirinto);

  ClassificadorParede classificador;
  Mapa mapa(dimensao);
  Navegacao navegacao(mapa);

  uint16_t passos = 0;
  uint16_t leituras = 0;
  uint16_t falhas = 0;

  while (!navegacao.atingiuObjetivo() && passos < maxPassos && leituras < 2 * maxPassos) {
    const Pose poseAtual = navegacao.obterPose();
    sensorSimulado.posicionar(poseAtual);

    leituras++;
    sensorSimulado.simularFalha(falharACada > 0 && leituras % falharACada == 0);

    DistanciasLaterais leitura = {};
    if (!sensorSimulado.lerDistanciasLaterais(leitura)) {
      // Sem leitura, as paredes da célula seguem desconhecidas: não decide e lê de novo.
      falhas++;
      continue;
    }

    // Atualiza o mapa com as paredes percebidas na célula atual
    AtualizadorMapa::atualizar(mapa, poseAtual, classificador, &leitura, 1);

    // A parede à frente precisa estar conhecida antes de decidir
    TEST_ASSERT_NOT_EQUAL(static_cast<uint8_t>(EstadoParede::Desconhecido),
                          static_cast<uint8_t>(mapa.obterParede(poseAtual.celula, poseAtual.direcao)));

    // Decide a próxima direção; a pose só muda quando os sensores confirmarem
    Direcao proxDir;
    TEST_ASSERT_TRUE(navegacao.darPasso(proxDir));
    TEST_ASSERT_FALSE_MESSAGE(labirinto.temParede(poseAtual.celula, proxDir),
                              "O robô tentou atravessar uma parede real do labirinto");

    // Simula a odometria: primeiro o robô gira e ainda está na célula de origem...
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(ResultadoConfirmacao::EmAndamento),
        static_cast<uint8_t>(navegacao.confirmarPose({poseAtual.celula, proxDir})));

    // ...depois chega à célula vizinha (uma parede real o deixaria na origem)
    PosicaoCelula medida = poseAtual.celula;
    if (!labirinto.temParede(poseAtual.celula, proxDir)) {
      vizinha(poseAtual.celula, proxDir, medida);
    }
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ResultadoConfirmacao::Confirmada),
                            static_cast<uint8_t>(navegacao.confirmarPose({medida, proxDir})));

    passos++;
  }

  TEST_ASSERT_TRUE(navegacao.atingiuObjetivo());
  TEST_ASSERT_EQUAL_UINT8(mapa.obterLinhas() - 1, navegacao.obterPosicao().linha);
  TEST_ASSERT_EQUAL_UINT8(mapa.obterColunas() - 1, navegacao.obterPosicao().coluna);
  TEST_ASSERT_TRUE(passos < maxPassos);
  if (falharACada > 0) {
    TEST_ASSERT_TRUE(falhas > 0);
  }
}

}  // namespace

/// @brief Critério de Pronto da Issue #183:
/// Navega autonomamente no labirinto 4x4 simulado da competição (com paredes desconhecidas a priori),
/// atualizando o mapa sensorialmente a cada passo até alcançar a célula objetivo (3, 3).
void test_resolucao_labirinto_4x4_simulado() {
  resolverLabirintoSimulado(exemplos::LABIRINTO_4X4, DimensaoMapa::Labirinto4x4, 50);
}

/// @brief Navega no labirinto 8x4 simulado com paredes desconhecidas a priori até o objetivo (3, 7).
void test_resolucao_labirinto_8x4_simulado() {
  resolverLabirintoSimulado(exemplos::LABIRINTO_8X4, DimensaoMapa::Labirinto8x4, 100);
}

/// @brief Navega no labirinto 12x4 simulado com paredes desconhecidas a priori até o objetivo (3, 11).
void test_resolucao_labirinto_12x4_simulado() {
  resolverLabirintoSimulado(exemplos::LABIRINTO_12X4, DimensaoMapa::Labirinto12x4, 150);
}

/// @brief Com o sensor falhando uma a cada 3 leituras, o robô não decide sem leitura e
/// ainda alcança o objetivo do labirinto 4x4 sem atravessar paredes.
void test_resolucao_com_falhas_de_leitura() {
  resolverLabirintoSimulado(exemplos::LABIRINTO_4X4, DimensaoMapa::Labirinto4x4, 50, 3);
}

// ---- #357: validação da pose e passo pendente

namespace {

void verificarPose(const Navegacao& navegacao, uint8_t linha, uint8_t coluna, Direcao direcao) {
  TEST_ASSERT_EQUAL_UINT8(linha, navegacao.obterPosicao().linha);
  TEST_ASSERT_EQUAL_UINT8(coluna, navegacao.obterPosicao().coluna);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(direcao), static_cast<uint8_t>(navegacao.obterDirecao()));
}

void verificarResultado(ResultadoConfirmacao esperado, ResultadoConfirmacao obtido) {
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(esperado), static_cast<uint8_t>(obtido));
}

}  // namespace

/// @brief definirPose, moverPara e o construtor recusam célula fora do mapa nos três tamanhos,
/// sem mudar a pose nem o passo pendente.
void test_pose_fora_do_mapa_e_recusada_nos_tres_tamanhos() {
  const DimensaoMapa dimensoes[] = {DimensaoMapa::Labirinto4x4, DimensaoMapa::Labirinto8x4,
                                    DimensaoMapa::Labirinto12x4};
  for (DimensaoMapa dimensao : dimensoes) {
    Mapa mapa(dimensao);
    const uint8_t linhas = mapa.obterLinhas();
    const uint8_t colunas = mapa.obterColunas();

    Navegacao navegacao(mapa);
    TEST_ASSERT_TRUE(navegacao.moverPara(1, 1, Direcao::Leste));
    Direcao dir;
    TEST_ASSERT_TRUE(navegacao.darPasso(dir));

    // definirPose: linha fora do mapa
    TEST_ASSERT_FALSE(navegacao.definirPose({pos(linhas, 0), Direcao::Norte}));
    verificarPose(navegacao, 1, 1, Direcao::Leste);
    TEST_ASSERT_TRUE(navegacao.passoPendente());

    // moverPara: coluna fora do mapa e limite de uint8_t (como a Odometria pode entregar)
    TEST_ASSERT_FALSE(navegacao.moverPara(0, colunas, Direcao::Sul));
    TEST_ASSERT_FALSE(navegacao.moverPara(255, 255, Direcao::Sul));
    verificarPose(navegacao, 1, 1, Direcao::Leste);
    TEST_ASSERT_TRUE(navegacao.passoPendente());

    // Construtor: pose inicial fora do mapa começa na partida (0, 0), mantendo a direção
    Navegacao comPoseInvalida(mapa, pos(linhas - 1, colunas - 1), {pos(linhas, colunas), Direcao::Oeste});
    verificarPose(comPoseInvalida, 0, 0, Direcao::Oeste);
    TEST_ASSERT_TRUE(comPoseInvalida.decidirProximaDirecao(dir));
  }
}

/// @brief Com o mapa trocado por um menor (nova corrida), a pose antiga fica fora do mapa:
/// decidirProximaDirecao e darPasso devolvem false sem ler o vetor de distâncias.
void test_mapa_trocado_deixa_pose_fora_e_nao_le_o_vetor() {
  Mapa mapa(DimensaoMapa::Labirinto12x4);
  Navegacao navegacao(mapa, pos(3, 11), {pos(3, 10), Direcao::Norte});

  mapa = Mapa(DimensaoMapa::Labirinto4x4);  // como ArmazenamentoMapa::iniciarCorrida faz

  Direcao dir = Direcao::Sul;
  TEST_ASSERT_FALSE(navegacao.decidirProximaDirecao(dir));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Direcao::Sul), static_cast<uint8_t>(dir));
  TEST_ASSERT_FALSE(navegacao.darPasso(dir));

  // Depois de redefinir objetivo e pose para o mapa novo, a navegação volta a funcionar
  navegacao.definirObjetivo(pos(3, 3));
  TEST_ASSERT_TRUE(navegacao.definirPose({pos(0, 0), Direcao::Norte}));
  TEST_ASSERT_TRUE(navegacao.darPasso(dir));
}

/// @brief O sensor registra a parede depois que o passo foi planejado: o passo é cancelado
/// (Bloqueado) e o próximo darPasso replaneja, em vez de deixar o robô parado para sempre.
void test_parede_registrada_depois_do_passo_nao_prende_o_robo() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  Navegacao navegacao(mapa);  // (0, 0) voltado para o Norte

  Direcao dir;
  TEST_ASSERT_TRUE(navegacao.darPasso(dir));  // planeja ir ao Norte, para (1, 0)
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Direcao::Norte), static_cast<uint8_t>(dir));

  mapa.registrarParede(pos(0, 0), Direcao::Norte, EstadoParede::Parede);

  verificarResultado(ResultadoConfirmacao::Bloqueado, navegacao.confirmarPose({pos(0, 0), Direcao::Norte}));
  TEST_ASSERT_FALSE(navegacao.passoPendente());

  // Replaneja a partir da origem: com o Norte fechado, segue para Leste
  TEST_ASSERT_TRUE(navegacao.darPasso(dir));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Direcao::Leste), static_cast<uint8_t>(dir));
  TEST_ASSERT_EQUAL_UINT8(0, navegacao.obterCelulaEsperada().linha);
  TEST_ASSERT_EQUAL_UINT8(1, navegacao.obterCelulaEsperada().coluna);
}

/// @brief Sem parede no caminho, o robô parado na origem continua EmAndamento (caso normal).
void test_sem_parede_no_caminho_passo_segue_em_andamento() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  Navegacao navegacao(mapa);

  Direcao dir;
  TEST_ASSERT_TRUE(navegacao.darPasso(dir));
  mapa.registrarParede(pos(0, 0), Direcao::Leste, EstadoParede::Parede);  // parede fora do caminho

  verificarResultado(ResultadoConfirmacao::EmAndamento, navegacao.confirmarPose({pos(0, 0), Direcao::Norte}));
  TEST_ASSERT_TRUE(navegacao.passoPendente());
  verificarResultado(ResultadoConfirmacao::Confirmada, navegacao.confirmarPose({pos(1, 0), Direcao::Norte}));
}

/// @brief cancelarPasso limpa o passo pendente e libera um novo darPasso.
void test_cancelar_passo_libera_novo_passo() {
  Mapa mapa(DimensaoMapa::Labirinto4x4);
  Navegacao navegacao(mapa);

  Direcao dir;
  TEST_ASSERT_TRUE(navegacao.darPasso(dir));
  TEST_ASSERT_FALSE(navegacao.darPasso(dir));

  navegacao.cancelarPasso();
  TEST_ASSERT_FALSE(navegacao.passoPendente());
  TEST_ASSERT_TRUE(navegacao.darPasso(dir));
  verificarPose(navegacao, 0, 0, Direcao::Norte);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_flood_fill_mapa_vazio_4x4);
  RUN_TEST(test_contorno_parede_conhecida);
  RUN_TEST(test_sem_caminho_ate_objetivo);
  RUN_TEST(test_dar_passo_aguarda_confirmacao_dos_sensores);
  RUN_TEST(test_confirmar_pose_em_andamento_e_chegada);
  RUN_TEST(test_confirmar_pose_divergente_replaneja);
  RUN_TEST(test_confirmar_pose_invalida_ignorada);
  RUN_TEST(test_atingimento_de_objetivo);
  RUN_TEST(test_resolucao_labirinto_4x4_simulado);
  RUN_TEST(test_resolucao_labirinto_8x4_simulado);
  RUN_TEST(test_resolucao_labirinto_12x4_simulado);
  RUN_TEST(test_resolucao_com_falhas_de_leitura);
  RUN_TEST(test_pose_fora_do_mapa_e_recusada_nos_tres_tamanhos);
  RUN_TEST(test_mapa_trocado_deixa_pose_fora_e_nao_le_o_vetor);
  RUN_TEST(test_parede_registrada_depois_do_passo_nao_prende_o_robo);
  RUN_TEST(test_sem_parede_no_caminho_passo_segue_em_andamento);
  RUN_TEST(test_cancelar_passo_libera_novo_passo);
  return UNITY_END();
}
