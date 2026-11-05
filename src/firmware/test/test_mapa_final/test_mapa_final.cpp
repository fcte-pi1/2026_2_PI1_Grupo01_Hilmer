/// @file test_mapa_final.cpp
/// @brief Testes da Issue #180 (HU-04): congelamento e serialização do mapa final ao alcançar o objetivo.
/// Roda no computador: `pio test -e native -f test_mapa_final`.

#include <unity.h>
#include <ArduinoJson.h>

#include "ArmazenamentoMapa.h"
#include "AtualizadorMapa.h"
#include "ClassificadorParede.h"
#include "LabirintoSimulado.h"
#include "SensorDistanciaSimulado.h"
#include "Mapa.h"
#include "MapaJson.h"
#include "../test_labirinto_simulado/LabirintosExemplo.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

constexpr size_t CAPACIDADE_BUFFER = TAMANHO_MAX_JSON_MAPA + 1;

PosicaoCelula pos(uint8_t linha, uint8_t coluna) {
  return PosicaoCelula{linha, coluna};
}

}  // namespace

/// @brief Critério de Aceite 1: "Ao sinalizar 'objetivo alcançado', o mapa final é congelado e armazenado."
///
/// Este teste verifica:
/// - O estado inicial: o mapa não está congelado e mapaFinal() retorna nullptr;
/// - O comportamento durante a exploração: paredes podem ser registradas normalmente;
/// - A sinalização de "objetivo alcançado": a chamada a congelarMapa() congela com sucesso;
/// - A recusa de novos congelamentos: chamadas adicionais retornam false (idempotência);
/// - A integridade do mapa armazenado: mapaFinal() retorna ponteiro válido e preserva exatamente
///   as paredes descobertas durante a corrida.
void test_congelamento_ao_sinalizar_objetivo() {
  ArmazenamentoMapa armazenamento;

  // 1. Verifica estado inicial antes de qualquer corrida
  TEST_ASSERT_FALSE(armazenamento.mapaCongelado());
  TEST_ASSERT_NULL(armazenamento.mapaFinal());

  // 2. Inicia uma corrida com labirinto 4x4
  armazenamento.iniciarCorrida(DimensaoMapa::Labirinto4x4);
  TEST_ASSERT_TRUE(armazenamento.corridaIniciada());
  TEST_ASSERT_FALSE(armazenamento.mapaCongelado());
  TEST_ASSERT_NULL(armazenamento.mapaFinal());

  // 3. Simula a exploração registrando paredes na célula de partida (0, 0)
  armazenamento.mapa().registrarParede(pos(0, 0), Direcao::Norte, EstadoParede::Livre);
  armazenamento.mapa().registrarParede(pos(0, 0), Direcao::Leste, EstadoParede::Parede);

  // 4. Robô detecta que alcançou o objetivo (célula de destino) e congela o mapa
  const bool congelou = armazenamento.congelarMapa();
  TEST_ASSERT_TRUE(congelou);
  TEST_ASSERT_TRUE(armazenamento.mapaCongelado());

  // 5. Tentativas subsequentes de congelar devem ser recusadas
  TEST_ASSERT_FALSE(armazenamento.congelarMapa());

  // 6. O mapa final congelado deve estar disponível e manter exatamente as paredes descobertas
  const Mapa* final = armazenamento.mapaFinal();
  TEST_ASSERT_NOT_NULL(final);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(EstadoParede::Livre),
                          static_cast<uint8_t>(final->obterParede(pos(0, 0), Direcao::Norte)));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(EstadoParede::Parede),
                          static_cast<uint8_t>(final->obterParede(pos(0, 0), Direcao::Leste)));
}

/// @brief Critério de Aceite 2: "Mapa final serializado no formato da tarefa 5.3.3."
///
/// Este teste verifica:
/// - Recusa antes do congelamento: MapaJson::serializarFinal rejeita se o mapa não estiver congelado;
/// - Serialização após congelar: gera uma cadeia JSON válida no formato especificado (dimensao,
///   linhas, colunas, e lista de celulas com parede_norte, parede_sul, etc.);
/// - Validação dos campos gerados: confere as chaves e tipos esperados pelo banco/telemetria;
/// - Proteção de buffer insuficiente: retorna 0 quando a capacidade informada é menor que o necessário.
void test_serializacao_mapa_final_formato_5_3_3() {
  ArmazenamentoMapa armazenamento;
  armazenamento.iniciarCorrida(DimensaoMapa::Labirinto4x4);

  char buffer[CAPACIDADE_BUFFER];

  // 1. Antes de congelar, a serialização do mapa final deve falhar (retorna 0 e buffer limpo)
  const size_t bytesAntes = MapaJson::serializarFinal(armazenamento, buffer, sizeof(buffer));
  TEST_ASSERT_EQUAL_UINT32(0, bytesAntes);
  TEST_ASSERT_EQUAL_CHAR('\0', buffer[0]);

  // 2. Simula registro de paredes conhecidas e congela o mapa
  armazenamento.mapa().registrarParede(pos(0, 0), Direcao::Norte, EstadoParede::Livre);
  armazenamento.mapa().registrarParede(pos(1, 1), Direcao::Leste, EstadoParede::Parede);
  TEST_ASSERT_TRUE(armazenamento.congelarMapa());

  // 3. Após congelar, a serialização deve ter sucesso e preencher o buffer
  const size_t bytesDepois = MapaJson::serializarFinal(armazenamento, buffer, sizeof(buffer));
  TEST_ASSERT_GREATER_THAN_UINT32(0, bytesDepois);
  TEST_ASSERT_EQUAL_UINT32(strlen(buffer), bytesDepois);

  // 4. Faz o parse do JSON gerado para checar conformidade com o formato da tarefa 5.3.3
  JsonDocument doc;
  const DeserializationError erro = deserializeJson(doc, buffer);
  TEST_ASSERT_TRUE(erro == DeserializationError::Ok);
  TEST_ASSERT_EQUAL_STRING("4x4", doc["dimensao"].as<const char*>());
  TEST_ASSERT_EQUAL_UINT8(4, doc["linhas"].as<uint8_t>());
  TEST_ASSERT_EQUAL_UINT8(4, doc["colunas"].as<uint8_t>());

  // 5. Valida o array de células: num labirinto 4x4, são exatamente 16 células
  JsonArrayConst celulas = doc["celulas"].as<JsonArrayConst>();
  TEST_ASSERT_EQUAL_UINT32(16, celulas.size());

  // 6. Confere campos da primeira célula (0, 0): linha, coluna, parede_norte (false) e oeste (perímetro = true)
  JsonObjectConst c00 = celulas[0];
  TEST_ASSERT_EQUAL_UINT8(0, c00["linha"].as<uint8_t>());
  TEST_ASSERT_EQUAL_UINT8(0, c00["coluna"].as<uint8_t>());
  TEST_ASSERT_FALSE(c00["parede_norte"].as<bool>());
  TEST_ASSERT_TRUE(c00["parede_oeste"].as<bool>());

  // 7. Valida proteção contra estouro de buffer: se a capacidade for insuficiente, retorna 0
  char bufferPequeno[32];
  TEST_ASSERT_EQUAL_UINT32(0, MapaJson::serializarFinal(armazenamento, bufferPequeno, sizeof(bufferPequeno)));
}

/// @brief Critério de Aceite 3: "Testado no labirinto simulado."
///
/// Este teste simula o robô percorrendo um labirinto virtual 4x4 com sensor de distância simulado:
/// - Carrega o labirinto 4x4 de referência (LabirintoSimulado);
/// - Acopla o sensor virtual (SensorDistanciaSimulado);
/// - Percorre passo a passo as células até a célula de destino (3, 3);
/// - A cada célula, atualiza o mapa via AtualizadorMapa a partir das distâncias medidas;
/// - Ao atingir o objetivo (3, 3), dispara o evento de "objetivo alcançado" e congela o mapa;
/// - Valida que o mapa final foi congelado e serializa corretamente o resultado da exploração.
void test_mapa_final_testado_no_labirinto_simulado() {
  // 1. Carrega o labirinto 4x4 de exemplo no simulador
  LabirintoSimulado labirinto;
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ResultadoCarga::Carregado),
                          static_cast<uint8_t>(labirinto.carregar(exemplos::LABIRINTO_4X4)));

  // 2. Conecta o sensor simulado ao labirinto simulado
  SensorDistanciaSimulado sensorSimulado;
  sensorSimulado.usarLabirinto(labirinto);

  // 3. Inicializa o armazenamento de mapa para a corrida
  ArmazenamentoMapa armazenamento;
  armazenamento.iniciarCorrida(DimensaoMapa::Labirinto4x4);

  ClassificadorParede classificador;

  // 4. Define o trajeto do robô partindo de (0, 0) até a célula objetivo (3, 3)
  const Pose trajeto[] = {
      {pos(0, 0), Direcao::Norte},
      {pos(1, 0), Direcao::Norte},
      {pos(2, 0), Direcao::Norte},
      {pos(3, 0), Direcao::Leste},
      {pos(3, 1), Direcao::Leste},
      {pos(3, 2), Direcao::Leste},
      {pos(3, 3), Direcao::Leste}  // Célula objetivo
  };

  const PosicaoCelula objetivo = pos(3, 3);

  // 5. Executa a navegação passo a passo
  for (const Pose& pose : trajeto) {
    sensorSimulado.posicionar(pose);
    DistanciasLaterais leitura = {};
    TEST_ASSERT_TRUE(sensorSimulado.lerDistanciasLaterais(leitura));

    // Atualiza o mapa com as leituras dos sensores
    const ResultadoAtualizacao res =
        AtualizadorMapa::atualizar(armazenamento.mapa(), pose, classificador, &leitura, 1);
    TEST_ASSERT_FALSE(res.temConflito);

    // Verifica se alcançou a célula objetivo
    if (pose.celula.linha == objetivo.linha && pose.celula.coluna == objetivo.coluna) {
      // Sinaliza "objetivo alcançado" e congela o mapa final
      TEST_ASSERT_TRUE(armazenamento.congelarMapa());
      break;
    }
  }

  // 6. Valida que o mapa foi congelado ao término do percurso
  TEST_ASSERT_TRUE(armazenamento.mapaCongelado());
  TEST_ASSERT_NOT_NULL(armazenamento.mapaFinal());

  // 7. Serializa o mapa final resultante da exploração completa
  char buffer[CAPACIDADE_BUFFER];
  const size_t tamanho = MapaJson::serializarFinal(armazenamento, buffer, sizeof(buffer));
  TEST_ASSERT_GREATER_THAN_UINT32(0, tamanho);

  // 8. Faz o parse e verifica a consistência dos dados do labirinto explorado
  JsonDocument doc;
  TEST_ASSERT_TRUE(deserializeJson(doc, buffer) == DeserializationError::Ok);
  TEST_ASSERT_EQUAL_STRING("4x4", doc["dimensao"].as<const char*>());

  // Na célula de partida (0, 0): Sul e Oeste são perímetro (true), Norte é corredor livre (false)
  JsonArrayConst celulas = doc["celulas"].as<JsonArrayConst>();
  JsonObjectConst c0 = celulas[0];
  TEST_ASSERT_TRUE(c0["parede_sul"].as<bool>());
  TEST_ASSERT_TRUE(c0["parede_oeste"].as<bool>());
  TEST_ASSERT_FALSE(c0["parede_norte"].as<bool>());

  // Na célula objetivo (3, 3): Norte e Leste são perímetro (true)
  JsonObjectConst cFinal = celulas[15];
  TEST_ASSERT_EQUAL_UINT8(3, cFinal["linha"].as<uint8_t>());
  TEST_ASSERT_EQUAL_UINT8(3, cFinal["coluna"].as<uint8_t>());
  TEST_ASSERT_TRUE(cFinal["parede_norte"].as<bool>());
  TEST_ASSERT_TRUE(cFinal["parede_leste"].as<bool>());
}

/// @brief Valida que o mapa final permanece congelado e inalterado mesmo que a navegação
/// continue escrevendo novas paredes após o congelamento.
///
/// Este teste verifica:
/// - O robô alcança o objetivo e o mapa é congelado;
/// - O JSON do mapa final é serializado e armazenado;
/// - Novas paredes são registradas no mapa da navegação (mapa ativo);
/// - O mapaFinal() e o JSON serializado por serializarFinal() permanecem estritamente idênticos;
/// - O JSON da telemetria ativa (serializar()) reflete as alterações, confirmando que apenas o
///   mapa final está isolado/congelado.
void test_mapa_final_permanece_inalterado_apos_novos_registros() {
  ArmazenamentoMapa armazenamento;
  armazenamento.iniciarCorrida(DimensaoMapa::Labirinto4x4);

  // 1. Registra paredes iniciais e congela o mapa
  armazenamento.mapa().registrarParede(pos(0, 0), Direcao::Norte, EstadoParede::Livre);
  armazenamento.mapa().registrarParede(pos(0, 0), Direcao::Leste, EstadoParede::Parede);
  TEST_ASSERT_TRUE(armazenamento.congelarMapa());

  // 2. Serializa o mapa final no momento do congelamento
  char jsonCongelado[CAPACIDADE_BUFFER];
  const size_t bytesCongelado = MapaJson::serializarFinal(armazenamento, jsonCongelado, sizeof(jsonCongelado));
  TEST_ASSERT_GREATER_THAN_UINT32(0, bytesCongelado);

  // 3. Simula navegação continuando a escrever no mapa após o congelamento
  const ResultadoRegistro res =
      armazenamento.mapa().registrarParede(pos(1, 1), Direcao::Leste, EstadoParede::Parede);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ResultadoRegistro::Registrada), static_cast<uint8_t>(res));

  // 4. Confere que a escrita afetou o mapa ativo, mas não o mapa final congelado
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(EstadoParede::Parede),
                          static_cast<uint8_t>(armazenamento.mapa().obterParede(pos(1, 1), Direcao::Leste)));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(EstadoParede::Desconhecido),
                          static_cast<uint8_t>(armazenamento.mapaFinal()->obterParede(pos(1, 1), Direcao::Leste)));

  // 5. Serializa novamente o mapa final e confere que o JSON gerado é estritamente idêntico
  char jsonAposEscrita[CAPACIDADE_BUFFER];
  const size_t bytesApos = MapaJson::serializarFinal(armazenamento, jsonAposEscrita, sizeof(jsonAposEscrita));
  TEST_ASSERT_EQUAL_UINT32(bytesCongelado, bytesApos);
  TEST_ASSERT_EQUAL_STRING(jsonCongelado, jsonAposEscrita);

  // 6. Confere que o JSON ativo (serializar()) reflete a nova parede e difere do JSON final congelado
  char jsonAtivo[CAPACIDADE_BUFFER];
  const size_t bytesAtivo = MapaJson::serializar(armazenamento.mapa(), jsonAtivo, sizeof(jsonAtivo));
  TEST_ASSERT_GREATER_THAN_UINT32(0, bytesAtivo);
  TEST_ASSERT_TRUE(strcmp(jsonCongelado, jsonAtivo) != 0);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_congelamento_ao_sinalizar_objetivo);
  RUN_TEST(test_serializacao_mapa_final_formato_5_3_3);
  RUN_TEST(test_mapa_final_testado_no_labirinto_simulado);
  RUN_TEST(test_mapa_final_permanece_inalterado_apos_novos_registros);
  return UNITY_END();
}
