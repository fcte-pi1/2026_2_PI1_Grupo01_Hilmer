/// @file test_envio_supabase.cpp
/// @brief Testes do lado do robô no envio ao Supabase (#236 e #238): o JSON do lote e das outras
/// chamadas, a leitura das respostas, as credenciais, o período da coleta e os tempos da rede, a
/// reconexão do Wi-Fi e a cadeia coleta → buffer → servidor simulado, com a rede caindo no meio e com
/// o servidor demorando a responder.
/// Roda no computador: `pio test -e native -f test_envio_supabase`.

#include <ArduinoJson.h>
#include <unity.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include "AmostraTelemetria.h"
#include "Configuracao.h"
#include "ConfiguracaoRede.h"
#include "ConfiguracaoReenvio.h"
#include "ContratoTelemetria.h"
#include "ControladorEnvio.h"
#include "CredenciaisSupabase.h"
#include "EnvioSimulado.h"
#include "IRedeWifi.h"
#include "ReconexaoWifi.h"
#include "TravaNula.h"

using namespace micromouse;

void setUp() {}
void tearDown() {}

namespace {

AmostraTelemetria amostraExemplo() {
  AmostraTelemetria a;
  a.numeroSequencia = 3;
  a.tempoDecorridoMs = 1500;
  a.velocidadeMmPorS = 120;
  a.tensaoBateriaMv = 7400;
  a.correnteBateriaMa = -250;
  a.posicaoLinha = 1;
  a.posicaoColuna = 2;
  a.orientacao = 'L';
  a.estado = EstadoCorrida::Explorando;
  a.evento = EventoTelemetria::Nenhum;
  return a;
}

/// A pior amostra possível para o tamanho do JSON: todos os números no maior comprimento.
AmostraTelemetria amostraMaisLonga() {
  AmostraTelemetria a;
  a.numeroSequencia = UINT32_MAX;
  a.tempoDecorridoMs = UINT32_MAX;
  a.velocidadeMmPorS = INT32_MIN;
  a.tensaoBateriaMv = UINT16_MAX;
  a.correnteBateriaMa = INT16_MIN;
  a.posicaoLinha = UINT8_MAX;
  a.posicaoColuna = UINT8_MAX;
  a.orientacao = 'O';
  a.estado = EstadoCorrida::Explorando;
  a.evento = EventoTelemetria::ObjetivoAlcancado;
  return a;
}

/// Rede Wi-Fi simulada: o teste decide quando ela conecta. Com `redes = 2`, simula a principal e a
/// reserva (#491) e guarda em qual rede foi cada tentativa.
class RedeWifiSimulada : public IRedeWifi {
 public:
  explicit RedeWifiSimulada(uint8_t redes = 1) : redes_(redes) {}
  bool conectada() override { return conectada_; }
  void iniciarConexao() override {
    if (inicios_ < static_cast<int>(sizeof(redeDoInicio_))) {
      redeDoInicio_[inicios_] = escolhida_;
    }
    ++inicios_;
  }
  void desconectar() override { ++desconexoes_; conectada_ = false; }
  uint8_t quantidadeRedes() const override { return redes_; }
  void escolherRede(uint8_t indice) override { escolhida_ = indice; }
  void definirConectada(bool conectada) { conectada_ = conectada; }
  int inicios() const { return inicios_; }
  int desconexoes() const { return desconexoes_; }
  uint8_t escolhida() const { return escolhida_; }
  /// Rede usada na tentativa `n` (0 é a primeira).
  uint8_t redeDoInicio(int n) const { return redeDoInicio_[n]; }

 private:
  uint8_t redes_;
  bool conectada_ = false;
  int inicios_ = 0;
  int desconexoes_ = 0;
  uint8_t escolhida_ = 0;
  uint8_t redeDoInicio_[16] = {};
};

/// Cliente "HTTP" para o teste de integração: transforma o lote em JSON com `serializarLote`, lê o
/// JSON de volta como o backend faria, grava no servidor simulado e devolve a resposta também em
/// JSON, lida por `lerRespostaLote`. Assim a cadeia inteira passa pelo contrato.
class EnvioJsonSimulado : public IEnvioTelemetria {
 public:
  explicit EnvioJsonSimulado(EnvioSimulado& servidor) : servidor_(servidor) {}

  ResultadoEnvio enviarLote(uint32_t corridaId, const AmostraTelemetria* amostras, uint16_t quantidade) override {
    static char json[TAMANHO_MAXIMO_LOTE_JSON];
    if (serializarLote(corridaId, amostras, quantidade, json, sizeof(json)) == 0) {
      return ResultadoEnvio{-1000, 0, 0};
    }
    JsonDocument doc;
    TEST_ASSERT_EQUAL(DeserializationError::Ok, deserializeJson(doc, json).code());
    static AmostraTelemetria lidas[TAMANHO_MAXIMO_LOTE];
    uint16_t n = 0;
    for (JsonObjectConst o : doc["amostras"].as<JsonArrayConst>()) {
      AmostraTelemetria a;
      a.numeroSequencia = o["numero_sequencia"].as<uint32_t>();
      a.posicaoLinha = o["posicao_linha"].as<uint8_t>();
      a.posicaoColuna = o["posicao_coluna"].as<uint8_t>();
      a.orientacao = o["orientacao"].as<const char*>()[0];
      a.velocidadeMmPorS = static_cast<int32_t>(std::lround(o["velocidade"].as<double>() * 1000.0));
      a.tempoDecorridoMs = static_cast<uint32_t>(std::lround(o["tempo_decorrido"].as<double>() * 1000.0));
      lidas[n++] = a;
    }
    const ResultadoEnvio r = servidor_.enviarLote(doc["corrida_id"].as<uint32_t>(), lidas, n);
    if (r.codigoHttp < 200 || r.codigoHttp >= 300) {
      return r;
    }
    char resposta[96];
    std::snprintf(resposta, sizeof(resposta), "{\"gravadas\":%u,\"existentes\":%u,\"reconsolidada\":false}",
                  static_cast<unsigned>(r.gravadas), static_cast<unsigned>(r.existentes));
    ResultadoEnvio lido{r.codigoHttp, 0, 0};
    TEST_ASSERT_TRUE(lerRespostaLote(resposta, lido.gravadas, lido.existentes));
    return lido;
  }

  ResultadoEnvio encerrarCorrida(uint32_t corridaId) override { return servidor_.encerrarCorrida(corridaId); }

 private:
  EnvioSimulado& servidor_;
};

/// Código que o `HTTPClient` devolve quando a resposta não chega no tempo limite
/// (`HTTPC_ERROR_READ_TIMEOUT`). Negativo: para a `PoliticaReenvio`, é "sem resposta".
constexpr int TEMPO_ESGOTADO_HTTP = -11;

/// Servidor simulado que demora para responder (#236). Cada chamada gasta `demoraMs` num relógio
/// simulado, e, enquanto ela não volta, a coleta continua registrando amostras, como a
/// `tarefaTelemetria` faz enquanto a `tarefaRede` espera a resposta. Se a demora chega ao tempo limite
/// da rede (`TEMPO_LIMITE_REDE_MS`), o robô desiste no limite e recebe tempo esgotado, mesmo que o
/// servidor tenha gravado o lote.
class EnvioLentoSimulado : public IEnvioTelemetria {
 public:
  /// Chamada a cada passo de 100 ms da espera, com o relógio já avançado.
  using Coleta = void (*)(uint32_t agoraMs, void* contexto);

  EnvioLentoSimulado(EnvioSimulado& servidor, uint32_t& relogioMs, Coleta coleta, void* contexto)
      : servidor_(servidor), relogioMs_(relogioMs), coleta_(coleta), contexto_(contexto) {}

  /// Tempo de resposta das próximas chamadas, em ms (múltiplo de 100).
  void definirDemora(uint32_t demoraMs) { demoraMs_ = demoraMs; }
  /// Chamadas em que o robô desistiu no tempo limite.
  uint32_t temposEsgotados() const { return temposEsgotados_; }

  ResultadoEnvio enviarLote(uint32_t corridaId, const AmostraTelemetria* amostras, uint16_t quantidade) override {
    const bool esgotou = esperar();
    const ResultadoEnvio r = servidor_.enviarLote(corridaId, amostras, quantidade);
    return esgotou ? tempoEsgotado() : r;
  }

  ResultadoEnvio encerrarCorrida(uint32_t corridaId) override {
    const bool esgotou = esperar();
    const ResultadoEnvio r = servidor_.encerrarCorrida(corridaId);
    return esgotou ? tempoEsgotado() : r;
  }

 private:
  static constexpr uint32_t PASSO_MS = 100;

  /// Passa o tempo da chamada em passos de 100 ms, deixando a coleta rodar a cada passo.
  /// @return `true` se a chamada chegou ao tempo limite da rede.
  bool esperar() {
    const bool esgotou = demoraMs_ >= TEMPO_LIMITE_REDE_MS;
    const uint32_t espera = esgotou ? TEMPO_LIMITE_REDE_MS : demoraMs_;
    for (uint32_t passado = 0; passado < espera; passado += PASSO_MS) {
      relogioMs_ += PASSO_MS;
      coleta_(relogioMs_, contexto_);
    }
    return esgotou;
  }

  ResultadoEnvio tempoEsgotado() {
    ++temposEsgotados_;
    return ResultadoEnvio{TEMPO_ESGOTADO_HTTP, 0, 0};
  }

  EnvioSimulado& servidor_;
  uint32_t& relogioMs_;
  Coleta coleta_;
  void* contexto_;
  uint32_t demoraMs_ = 0;
  uint32_t temposEsgotados_ = 0;
};

/// A `tarefaTelemetria` simulada: registra a amostra de cada segundo assim que o relógio chega nele,
/// tanto no laço da tarefa de rede quanto durante uma chamada lenta.
struct ColetaSimulada {
  ControladorEnvio* controlador = nullptr;
  const TravaNula* trava = nullptr;
  uint32_t total = 0;                   ///< Amostras da corrida (uma por segundo).
  uint32_t proxima = 1;                 ///< Segundo da próxima amostra.
  bool duranteRede = false;             ///< `true` enquanto uma chamada de rede está em curso.
  uint32_t registradasDuranteRede = 0;  ///< Amostras registradas com uma chamada de rede em curso.
  uint32_t comTravaSegura = 0;          ///< Vezes em que a coleta achou a trava segura por outra parte.
  uint32_t maiorAtrasoMs = 0;           ///< Maior atraso entre o segundo da amostra e o registro.
};

void coletar(uint32_t agoraMs, void* contexto) {
  ColetaSimulada& c = *static_cast<ColetaSimulada*>(contexto);
  while (c.proxima <= c.total && agoraMs >= c.proxima * 1000) {
    if (c.trava->profundidade() != 0) {
      ++c.comTravaSegura;
    }
    const uint32_t atraso = agoraMs - c.proxima * 1000;
    if (atraso > c.maiorAtrasoMs) {
      c.maiorAtrasoMs = atraso;
    }
    const bool ultima = c.proxima == c.total;
    const Pose pose{{static_cast<uint8_t>(c.proxima % 4), static_cast<uint8_t>((c.proxima / 4) % 4)}, Direcao::Leste};
    c.controlador->registrar(montarAmostra(pose, c.proxima * 1000, 150,
                                           ultima ? EstadoCorrida::Parado : EstadoCorrida::Explorando,
                                           ultima ? EventoTelemetria::ObjetivoAlcancado : EventoTelemetria::Nenhum));
    if (c.duranteRede) {
      ++c.registradasDuranteRede;
    }
    ++c.proxima;
  }
}

void coletarDuranteRede(uint32_t agoraMs, void* contexto) {
  ColetaSimulada& c = *static_cast<ColetaSimulada*>(contexto);
  c.duranteRede = true;
  coletar(agoraMs, contexto);
  c.duranteRede = false;
}

/// Valor numérico do JSON em milésimos, arredondado (0.12 → 120), para comparar sem ponto flutuante:
/// o `pio test -e native` não liga a precisão dupla do Unity.
long long milesimos(JsonVariantConst valor) { return std::llround(valor.as<double>() * 1000.0); }

/// Abre o contrato único do lote (`src/backend/tests/fixtures/lote-telemetria.json`). Tenta a partir
/// deste arquivo (`__FILE__` aponta para src/firmware/test/<teste>/...) e a partir da pasta do projeto
/// (`pio test` roda dentro de src/firmware). Devolve nullptr se não achar.
FILE* abrirContrato() {
  std::string base = __FILE__;
  for (int i = 0; i < 3 && base != "."; ++i) {
    const size_t barra = base.find_last_of("/\\");
    base = barra == std::string::npos ? std::string(".") : base.substr(0, barra);
  }
  const std::string candidatos[] = {base + "/../backend/tests/fixtures/lote-telemetria.json",
                                    "../backend/tests/fixtures/lote-telemetria.json"};
  for (const std::string& caminho : candidatos) {
    FILE* arquivo = std::fopen(caminho.c_str(), "rb");
    if (arquivo != nullptr) {
      return arquivo;
    }
  }
  return nullptr;
}

}  // namespace

// ---------------------------------------------------------------- JSON do lote

void test_lote_com_uma_amostra_tem_os_nomes_das_colunas_e_as_unidades_do_banco() {
  const AmostraTelemetria a = amostraExemplo();
  char json[512];
  const size_t n = serializarLote(7, &a, 1, json, sizeof(json));
  TEST_ASSERT_EQUAL_STRING(
      "{\"corrida_id\":7,\"amostras\":[{\"numero_sequencia\":3,\"posicao_linha\":1,\"posicao_coluna\":2,"
      "\"orientacao\":\"L\",\"velocidade\":0.120,\"tensao_bateria\":7.400,\"corrente_bateria\":-0.250,"
      "\"tempo_decorrido\":1.500,\"estado_corrida\":\"explorando\"}]}",
      json);
  TEST_ASSERT_EQUAL_size_t(std::strlen(json), n);
}

void test_evento_so_aparece_quando_existe() {
  AmostraTelemetria a = amostraExemplo();
  char json[512];
  serializarLote(1, &a, 1, json, sizeof(json));
  TEST_ASSERT_NULL(std::strstr(json, "evento"));
  a.evento = EventoTelemetria::Colisao;
  serializarLote(1, &a, 1, json, sizeof(json));
  TEST_ASSERT_NOT_NULL(std::strstr(json, ",\"evento\":\"colisao\"}"));
  a.evento = EventoTelemetria::ObjetivoAlcancado;
  serializarLote(1, &a, 1, json, sizeof(json));
  TEST_ASSERT_NOT_NULL(std::strstr(json, ",\"evento\":\"objetivo_alcancado\"}"));
}

void test_lote_com_varias_amostras_em_ordem_e_json_valido() {
  AmostraTelemetria amostras[3] = {amostraExemplo(), amostraExemplo(), amostraExemplo()};
  for (uint32_t i = 0; i < 3; ++i) {
    amostras[i].numeroSequencia = 10 + i;
  }
  char json[2048];
  TEST_ASSERT_TRUE(serializarLote(42, amostras, 3, json, sizeof(json)) > 0);
  JsonDocument doc;
  TEST_ASSERT_EQUAL(DeserializationError::Ok, deserializeJson(doc, json).code());
  TEST_ASSERT_EQUAL_UINT32(42, doc["corrida_id"].as<uint32_t>());
  JsonArrayConst lista = doc["amostras"].as<JsonArrayConst>();
  TEST_ASSERT_EQUAL_size_t(3, lista.size());
  for (uint32_t i = 0; i < 3; ++i) {
    TEST_ASSERT_EQUAL_UINT32(10 + i, lista[i]["numero_sequencia"].as<uint32_t>());
    TEST_ASSERT_EQUAL_INT32(120, static_cast<int32_t>(milesimos(lista[i]["velocidade"])));
  }
}

void test_numeros_negativos_e_pequenos_com_tres_casas() {
  AmostraTelemetria a = amostraExemplo();
  a.velocidadeMmPorS = -5;      // −0,005 m/s
  a.correnteBateriaMa = -1000;  // −1,000 A
  a.tempoDecorridoMs = 0;
  char json[512];
  serializarLote(1, &a, 1, json, sizeof(json));
  TEST_ASSERT_NOT_NULL(std::strstr(json, "\"velocidade\":-0.005,"));
  TEST_ASSERT_NOT_NULL(std::strstr(json, "\"corrente_bateria\":-1.000,"));
  TEST_ASSERT_NOT_NULL(std::strstr(json, "\"tempo_decorrido\":0.000,"));
}

void test_lote_vazio_e_orientacao_invalida_nao_quebram_o_json() {
  char json[128];
  TEST_ASSERT_TRUE(serializarLote(5, nullptr, 0, json, sizeof(json)) > 0);
  TEST_ASSERT_EQUAL_STRING("{\"corrida_id\":5,\"amostras\":[]}", json);

  AmostraTelemetria a = amostraExemplo();
  a.orientacao = '"';  // um caractere que quebraria o JSON
  char outro[512];
  serializarLote(1, &a, 1, outro, sizeof(outro));
  JsonDocument doc;
  TEST_ASSERT_EQUAL(DeserializationError::Ok, deserializeJson(doc, outro).code());
  TEST_ASSERT_EQUAL_STRING("?", doc["amostras"][0]["orientacao"].as<const char*>());  // o servidor recusa
}

void test_lote_que_nao_cabe_devolve_zero_e_texto_vazio() {
  const AmostraTelemetria a = amostraExemplo();
  char json[512];
  const size_t cheio = serializarLote(7, &a, 1, json, sizeof(json));
  char justo[512];
  TEST_ASSERT_EQUAL_size_t(cheio, serializarLote(7, &a, 1, justo, cheio + 1));  // cabe com o '\0'
  TEST_ASSERT_EQUAL_size_t(0, serializarLote(7, &a, 1, justo, cheio));          // falta 1 byte
  TEST_ASSERT_EQUAL_STRING("", justo);
  TEST_ASSERT_EQUAL_size_t(0, serializarLote(7, &a, 1, nullptr, 100));
  TEST_ASSERT_EQUAL_size_t(0, serializarLote(7, nullptr, 1, justo, sizeof(justo)));
}

void test_pior_amostra_e_pior_lote_cabem_no_espaco_reservado() {
  const AmostraTelemetria a = amostraMaisLonga();
  static char json[TAMANHO_MAXIMO_LOTE_JSON];
  const size_t uma = serializarLote(UINT32_MAX, &a, 1, json, sizeof(json));
  TEST_ASSERT_TRUE(uma > 0);
  const size_t envelope = std::strlen("{\"corrida_id\":4294967295,\"amostras\":[]}");
  TEST_ASSERT_TRUE(uma - envelope <= TAMANHO_MAXIMO_AMOSTRA_JSON);

  static AmostraTelemetria lote[TAMANHO_MAXIMO_LOTE];
  for (uint16_t i = 0; i < TAMANHO_MAXIMO_LOTE; ++i) {
    lote[i] = a;
  }
  TEST_ASSERT_TRUE(serializarLote(UINT32_MAX, lote, TAMANHO_MAXIMO_LOTE, json, sizeof(json)) > 0);
  JsonDocument doc;
  TEST_ASSERT_EQUAL(DeserializationError::Ok, deserializeJson(doc, json).code());
  TEST_ASSERT_EQUAL_size_t(TAMANHO_MAXIMO_LOTE, doc["amostras"].as<JsonArrayConst>().size());
  TEST_ASSERT_NOT_NULL(std::strstr(json, "\"velocidade\":-2147483.648,"));
}

/// Contrato único: o lote que o firmware gera para as amostras do exemplo do backend é igual, campo a
/// campo, ao `src/backend/tests/fixtures/lote-telemetria.json`, que os testes do backend aceitam.
void test_contrato_lote_igual_ao_exemplo_do_backend() {
  FILE* arquivo = abrirContrato();
  if (arquivo == nullptr) {
    TEST_IGNORE_MESSAGE("lote-telemetria.json ainda não está nesta branch (chega com a #229, PR #398)");
  }
  std::string texto;
  char pedaco[512];
  size_t lidos = 0;
  while ((lidos = std::fread(pedaco, 1, sizeof(pedaco), arquivo)) > 0) {
    texto.append(pedaco, lidos);
  }
  std::fclose(arquivo);

  JsonDocument esperado;
  TEST_ASSERT_EQUAL(DeserializationError::Ok, deserializeJson(esperado, texto).code());
  static AmostraTelemetria amostras[16];
  uint16_t n = 0;
  for (JsonObjectConst o : esperado["amostras"].as<JsonArrayConst>()) {
    AmostraTelemetria a;
    a.numeroSequencia = o["numero_sequencia"].as<uint32_t>();
    a.posicaoLinha = o["posicao_linha"].as<uint8_t>();
    a.posicaoColuna = o["posicao_coluna"].as<uint8_t>();
    a.orientacao = o["orientacao"].as<const char*>()[0];
    a.velocidadeMmPorS = static_cast<int32_t>(std::lround(o["velocidade"].as<double>() * 1000.0));
    a.tensaoBateriaMv = static_cast<uint16_t>(std::lround(o["tensao_bateria"].as<double>() * 1000.0));
    a.correnteBateriaMa = static_cast<int16_t>(std::lround(o["corrente_bateria"].as<double>() * 1000.0));
    a.tempoDecorridoMs = static_cast<uint32_t>(std::lround(o["tempo_decorrido"].as<double>() * 1000.0));
    const char* estado = o["estado_corrida"].as<const char*>();
    a.estado = std::strcmp(estado, "aguardando") == 0   ? EstadoCorrida::Aguardando
               : std::strcmp(estado, "explorando") == 0 ? EstadoCorrida::Explorando
               : std::strcmp(estado, "retornando") == 0 ? EstadoCorrida::Retornando
                                                        : EstadoCorrida::Parado;
    const char* evento = o["evento"] | "";
    a.evento = std::strcmp(evento, "colisao") == 0              ? EventoTelemetria::Colisao
               : std::strcmp(evento, "objetivo_alcancado") == 0 ? EventoTelemetria::ObjetivoAlcancado
                                                                : EventoTelemetria::Nenhum;
    amostras[n++] = a;
  }

  static char json[8192];
  TEST_ASSERT_TRUE(serializarLote(esperado["corrida_id"].as<uint32_t>(), amostras, n, json, sizeof(json)) > 0);
  JsonDocument gerado;
  TEST_ASSERT_EQUAL(DeserializationError::Ok, deserializeJson(gerado, json).code());
  TEST_ASSERT_EQUAL_UINT32(esperado["corrida_id"].as<uint32_t>(), gerado["corrida_id"].as<uint32_t>());
  JsonArrayConst a = esperado["amostras"].as<JsonArrayConst>();
  JsonArrayConst b = gerado["amostras"].as<JsonArrayConst>();
  TEST_ASSERT_EQUAL_size_t(a.size(), b.size());
  for (size_t i = 0; i < a.size(); ++i) {
    JsonObjectConst ea = a[i];
    JsonObjectConst gb = b[i];
    TEST_ASSERT_EQUAL_size_t(ea.size(), gb.size());  // os mesmos campos, nem mais nem menos
    for (JsonPairConst campo : ea) {
      JsonVariantConst valor = gb[campo.key()];
      TEST_ASSERT_FALSE_MESSAGE(valor.isNull(), campo.key().c_str());
      if (campo.value().is<const char*>()) {
        TEST_ASSERT_EQUAL_STRING(campo.value().as<const char*>(), valor.as<const char*>());
      } else {
        TEST_ASSERT_EQUAL_INT32_MESSAGE(static_cast<int32_t>(milesimos(campo.value())),
                                        static_cast<int32_t>(milesimos(valor)), campo.key().c_str());
      }
    }
  }
}

// ---------------------------------------------------------------- respostas do servidor

void test_le_as_contagens_da_resposta_do_lote() {
  uint16_t g = 99;
  uint16_t e = 99;
  TEST_ASSERT_TRUE(lerRespostaLote("{\"gravadas\":3,\"existentes\":2,\"reconsolidada\":false}", g, e));
  TEST_ASSERT_EQUAL_UINT16(3, g);
  TEST_ASSERT_EQUAL_UINT16(2, e);
  TEST_ASSERT_TRUE(lerRespostaLote("{\"existentes\":0,\"gravadas\":100}", g, e));
  TEST_ASSERT_EQUAL_UINT16(100, g);
}

void test_resposta_do_lote_sem_as_contagens_e_recusada() {
  uint16_t g = 7;
  uint16_t e = 7;
  const char* invalidas[] = {
      "{\"gravadas\":3}",                         // falta existentes
      "{\"gravadas\":-1,\"existentes\":0}",       // negativo
      "{\"gravadas\":1.5,\"existentes\":0}",      // não inteiro
      "{\"gravadas\":\"3\",\"existentes\":0}",    // texto
      "{\"gravadas\":70000,\"existentes\":0}",    // maior que 65535
      "[3,2]",                                    // não é objeto
      "{\"erro\":\"nao_autorizado\"}",            // corpo de erro
      "isto não é JSON",
      "",
  };
  for (const char* corpo : invalidas) {
    TEST_ASSERT_FALSE_MESSAGE(lerRespostaLote(corpo, g, e), corpo);
  }
  TEST_ASSERT_FALSE(lerRespostaLote(nullptr, g, e));
  TEST_ASSERT_EQUAL_UINT16(7, g);  // nada muda quando a leitura falha
  TEST_ASSERT_EQUAL_UINT16(7, e);
}

void test_le_o_id_da_corrida_criada() {
  uint32_t id = 0;
  TEST_ASSERT_TRUE(lerIdCorrida(
      "{\"id\":12,\"labirinto_id\":1,\"numero_tentativa\":3,\"status\":\"em_andamento\",\"tempo_final\":null}", id));
  TEST_ASSERT_EQUAL_UINT32(12, id);
  TEST_ASSERT_FALSE(lerIdCorrida("{\"labirinto_id\":1}", id));
  TEST_ASSERT_FALSE(lerIdCorrida("{\"id\":0}", id));
  TEST_ASSERT_FALSE(lerIdCorrida("{\"id\":\"12\"}", id));
  TEST_ASSERT_FALSE(lerIdCorrida("{\"id\":3000000000}", id));  // passa do inteiro de 32 bits do banco
  TEST_ASSERT_FALSE(lerIdCorrida("nada", id));
  TEST_ASSERT_FALSE(lerIdCorrida(nullptr, id));
  TEST_ASSERT_EQUAL_UINT32(12, id);
}

void test_corpos_da_criacao_e_do_encerramento() {
  char corpo[48];
  TEST_ASSERT_EQUAL_size_t(19, serializarCriacaoCorrida("4x4", corpo, sizeof(corpo)));
  TEST_ASSERT_EQUAL_STRING("{\"labirinto\":\"4x4\"}", corpo);
  TEST_ASSERT_TRUE(serializarEncerramento(true, corpo, sizeof(corpo)) > 0);
  TEST_ASSERT_EQUAL_STRING("{\"status\":\"concluida\"}", corpo);
  TEST_ASSERT_TRUE(serializarEncerramento(false, corpo, sizeof(corpo)) > 0);
  TEST_ASSERT_EQUAL_STRING("{\"status\":\"nao_concluida\"}", corpo);
  TEST_ASSERT_EQUAL_size_t(0, serializarCriacaoCorrida("12x4", corpo, 8));
  TEST_ASSERT_EQUAL_size_t(0, serializarEncerramento(true, corpo, 8));
  TEST_ASSERT_EQUAL_size_t(0, serializarCriacaoCorrida(nullptr, corpo, sizeof(corpo)));
  TEST_ASSERT_EQUAL_size_t(0, serializarEncerramento(true, nullptr, 0));
}

void test_amostra_montada_a_partir_da_pose() {
  const Pose pose{{2, 3}, Direcao::Leste};
  const AmostraTelemetria a =
      montarAmostra(pose, 4200, 150, EstadoCorrida::Parado, EventoTelemetria::ObjetivoAlcancado);
  TEST_ASSERT_EQUAL_UINT8(2, a.posicaoLinha);
  TEST_ASSERT_EQUAL_UINT8(3, a.posicaoColuna);
  TEST_ASSERT_EQUAL_CHAR('L', a.orientacao);
  TEST_ASSERT_EQUAL_UINT32(4200, a.tempoDecorridoMs);
  TEST_ASSERT_EQUAL_INT32(150, a.velocidadeMmPorS);
  TEST_ASSERT_EQUAL_UINT32(0, a.numeroSequencia);  // quem numera é o registrar()
  TEST_ASSERT_EQUAL_UINT16(0, a.tensaoBateriaMv);  // a Rev 2.0 não mede a bateria
  TEST_ASSERT_EQUAL_INT16(0, a.correnteBateriaMa);
  TEST_ASSERT_TRUE(a.estado == EstadoCorrida::Parado);
  TEST_ASSERT_TRUE(a.evento == EventoTelemetria::ObjetivoAlcancado);
}

// ---------------------------------------------------------------- credenciais e URLs

namespace {

CredenciaisSupabase credenciaisCompletas() {
  CredenciaisSupabase c;
  c.ssidWifi = "lab";
  c.senhaWifi = "senha";
  c.urlProjeto = "https://abc.supabase.co";
  c.chaveAnon = "anon";
  c.tokenDispositivo = "token";
  c.certificadoRaiz = "-----BEGIN CERTIFICATE-----";
  return c;
}

}  // namespace

void test_sem_credenciais_o_envio_fica_desligado() {
  TEST_ASSERT_FALSE(credenciaisValidas(CredenciaisSupabase{}));  // sem include/credenciais.h
  TEST_ASSERT_TRUE(credenciaisValidas(credenciaisCompletas()));

  CredenciaisSupabase c = credenciaisCompletas();
  c.tokenDispositivo = "";
  TEST_ASSERT_FALSE(credenciaisValidas(c));
  c = credenciaisCompletas();
  c.chaveAnon = nullptr;
  TEST_ASSERT_FALSE(credenciaisValidas(c));
  c = credenciaisCompletas();
  c.ssidWifi = "";
  TEST_ASSERT_FALSE(credenciaisValidas(c));
  c = credenciaisCompletas();
  c.senhaWifi = "";  // rede aberta: vale
  TEST_ASSERT_TRUE(credenciaisValidas(c));
}

void test_so_https_e_com_certificado_ou_excecao_de_bancada() {
  CredenciaisSupabase c = credenciaisCompletas();
  c.urlProjeto = "http://abc.supabase.co";
  TEST_ASSERT_FALSE(credenciaisValidas(c));
  c.urlProjeto = "https://";
  TEST_ASSERT_FALSE(credenciaisValidas(c));
  c = credenciaisCompletas();
  c.certificadoRaiz = "";
  TEST_ASSERT_FALSE(credenciaisValidas(c));
  c.semValidarCertificado = true;  // só na bancada
  TEST_ASSERT_TRUE(credenciaisValidas(c));
}

void test_url_das_funcoes() {
  char url[96];
  TEST_ASSERT_TRUE(montarUrlFuncao("https://abc.supabase.co", "telemetria", url, sizeof(url)) > 0);
  TEST_ASSERT_EQUAL_STRING("https://abc.supabase.co/functions/v1/telemetria", url);
  montarUrlFuncao("https://abc.supabase.co/", "/corridas/12/encerrar", url, sizeof(url));
  TEST_ASSERT_EQUAL_STRING("https://abc.supabase.co/functions/v1/corridas/12/encerrar", url);
  TEST_ASSERT_EQUAL_size_t(0, montarUrlFuncao("https://abc.supabase.co", "corridas", url, 20));
  TEST_ASSERT_EQUAL_STRING("", url);
  TEST_ASSERT_EQUAL_size_t(0, montarUrlFuncao(nullptr, "corridas", url, sizeof(url)));
}

// ---------------------------------------------------------------- período da coleta e tempos da rede

/// #236: a coleta sai a cerca de 1 s, dentro dos 2 s do RNF05; a tarefa de rede roda várias vezes por
/// período; um buffer cheio (10 minutos) se esvazia em menos de um período depois que a rede volta; e
/// uma chamada que esgota o tempo limite ainda cabe no prazo do encerramento.
void test_periodo_da_coleta_e_tempos_da_rede() {
  TEST_ASSERT_EQUAL_UINT32(1000, PERIODO_TELEMETRIA_MS);
  TEST_ASSERT_TRUE(PERIODO_TELEMETRIA_MS <= 2000);
  TEST_ASSERT_TRUE(PERIODO_CICLO_REDE_MS * 5 <= PERIODO_TELEMETRIA_MS);
  const uint32_t ciclosParaEsvaziar = CAPACIDADE_BUFFER_AMOSTRAS / TAMANHO_MAXIMO_LOTE;  // 6 lotes de 100
  TEST_ASSERT_TRUE(ciclosParaEsvaziar * PERIODO_CICLO_REDE_MS < PERIODO_TELEMETRIA_MS);
  TEST_ASSERT_TRUE(TEMPO_LIMITE_REDE_MS < TEMPO_LIMITE_ENCERRAMENTO_MS);
}

// ---------------------------------------------------------------- reconexão do Wi-Fi

void test_wifi_conecta_na_primeira_tentativa() {
  RedeWifiSimulada rede;
  ReconexaoWifi wifi(rede);
  TEST_ASSERT_FALSE(wifi.ciclo(0));
  TEST_ASSERT_EQUAL_INT(1, rede.inicios());
  TEST_ASSERT_TRUE(wifi.estado() == EstadoWifi::Conectando);
  rede.definirConectada(true);
  TEST_ASSERT_TRUE(wifi.ciclo(100));
  TEST_ASSERT_TRUE(wifi.estado() == EstadoWifi::Conectado);
  TEST_ASSERT_TRUE(wifi.ciclo(200));
  TEST_ASSERT_EQUAL_INT(1, rede.inicios());  // conectado: não tenta de novo
}

void test_wifi_espera_cada_vez_mais_entre_tentativas_e_volta_a_1s() {
  RedeWifiSimulada rede;
  ReconexaoWifi wifi(rede, 1000);  // tentativa de 1 s, para o teste
  uint32_t t = 0;
  wifi.ciclo(t);                                   // 1ª tentativa
  wifi.ciclo(t += 1000);                           // esgotou: espera 1 s
  TEST_ASSERT_EQUAL_INT(1, rede.desconexoes());
  TEST_ASSERT_TRUE(wifi.estado() == EstadoWifi::Aguardando);
  wifi.ciclo(t += 900);
  TEST_ASSERT_EQUAL_INT(1, rede.inicios());        // ainda não deu 1 s
  wifi.ciclo(t += 100);
  TEST_ASSERT_EQUAL_INT(2, rede.inicios());        // 2ª tentativa
  wifi.ciclo(t += 1000);                           // esgotou: espera 2 s
  wifi.ciclo(t += 1900);
  TEST_ASSERT_EQUAL_INT(2, rede.inicios());
  wifi.ciclo(t += 100);
  TEST_ASSERT_EQUAL_INT(3, rede.inicios());        // 3ª tentativa, depois de 2 s
  TEST_ASSERT_EQUAL_UINT32(4000, wifi.proximaEsperaMs());

  rede.definirConectada(true);
  TEST_ASSERT_TRUE(wifi.ciclo(t += 100));
  TEST_ASSERT_EQUAL_UINT32(ESPERA_MINIMA_WIFI_MS, wifi.proximaEsperaMs());  // sucesso: volta a 1 s
}

void test_wifi_que_cai_tenta_de_novo_na_hora_e_conta_a_queda() {
  RedeWifiSimulada rede;
  ReconexaoWifi wifi(rede);
  wifi.ciclo(0);
  rede.definirConectada(true);
  wifi.ciclo(100);
  rede.definirConectada(false);  // o roteador caiu
  TEST_ASSERT_FALSE(wifi.ciclo(200));
  TEST_ASSERT_EQUAL_UINT32(1, wifi.quedas());
  TEST_ASSERT_EQUAL_INT(2, rede.inicios());
  TEST_ASSERT_EQUAL_UINT32(2, wifi.tentativas());
}

void test_wifi_que_volta_sozinho_durante_a_espera_e_aceito() {
  RedeWifiSimulada rede;
  ReconexaoWifi wifi(rede, 1000);
  wifi.ciclo(0);
  wifi.ciclo(1000);  // esgotou: aguardando
  rede.definirConectada(true);
  TEST_ASSERT_TRUE(wifi.ciclo(1100));
  TEST_ASSERT_TRUE(wifi.estado() == EstadoWifi::Conectado);
}

void test_wifi_com_o_relogio_dando_a_volta() {
  RedeWifiSimulada rede;
  ReconexaoWifi wifi(rede, 1000);
  const uint32_t quaseNoFim = UINT32_MAX - 500;
  wifi.ciclo(quaseNoFim);
  wifi.ciclo(quaseNoFim + 1000);  // passou do fim do contador de ms
  TEST_ASSERT_TRUE(wifi.estado() == EstadoWifi::Aguardando);
  wifi.ciclo(quaseNoFim + 1999);
  TEST_ASSERT_EQUAL_INT(1, rede.inicios());
  wifi.ciclo(quaseNoFim + 2000);
  TEST_ASSERT_EQUAL_INT(2, rede.inicios());
}

void test_wifi_com_uma_rede_nunca_troca_de_rede() {
  RedeWifiSimulada rede;
  ReconexaoWifi wifi(rede, 1000);
  wifi.ciclo(0);
  wifi.ciclo(1000);  // esgotou
  TEST_ASSERT_EQUAL_UINT8(0, wifi.redeAtual());
  TEST_ASSERT_EQUAL_UINT32(0, wifi.trocasDeRede());
  TEST_ASSERT_EQUAL_UINT8(0, rede.escolhida());
}

void test_wifi_com_a_principal_fora_passa_para_a_reserva_na_hora() {
  RedeWifiSimulada rede(2);
  ReconexaoWifi wifi(rede, 1000);
  wifi.ciclo(0);  // 1ª tentativa, na principal
  TEST_ASSERT_EQUAL_UINT8(0, rede.redeDoInicio(0));
  wifi.ciclo(1000);  // principal esgotou: troca para a reserva, sem esperar
  TEST_ASSERT_EQUAL_UINT8(1, wifi.redeAtual());
  TEST_ASSERT_EQUAL_UINT32(1, wifi.trocasDeRede());
  wifi.ciclo(1001);
  TEST_ASSERT_EQUAL_INT(2, rede.inicios());
  TEST_ASSERT_EQUAL_UINT8(1, rede.redeDoInicio(1));
  TEST_ASSERT_EQUAL_UINT32(ESPERA_MINIMA_WIFI_MS, wifi.proximaEsperaMs());  // a espera não cresceu

  rede.definirConectada(true);  // a reserva conectou
  TEST_ASSERT_TRUE(wifi.ciclo(1100));
  TEST_ASSERT_EQUAL_UINT8(1, wifi.redeAtual());
}

void test_wifi_com_as_duas_redes_fora_espera_cada_vez_mais_a_cada_rodada() {
  RedeWifiSimulada rede(2);
  ReconexaoWifi wifi(rede, 1000);
  uint32_t t = 0;
  wifi.ciclo(t);          // principal
  wifi.ciclo(t += 1000);  // esgotou: reserva já
  wifi.ciclo(t += 1);     // reserva
  wifi.ciclo(t += 1000);  // esgotou: as duas falharam, espera 1 s
  TEST_ASSERT_TRUE(wifi.estado() == EstadoWifi::Aguardando);
  wifi.ciclo(t += 900);
  TEST_ASSERT_EQUAL_INT(2, rede.inicios());
  wifi.ciclo(t += 100);
  TEST_ASSERT_EQUAL_INT(3, rede.inicios());
  TEST_ASSERT_EQUAL_UINT8(0, rede.redeDoInicio(2));  // nova rodada começa na principal
  wifi.ciclo(t += 1000);  // esgotou: reserva já
  wifi.ciclo(t += 1);
  TEST_ASSERT_EQUAL_UINT8(1, rede.redeDoInicio(3));
  wifi.ciclo(t += 1000);  // esgotou de novo: espera 2 s
  wifi.ciclo(t += 1900);
  TEST_ASSERT_EQUAL_INT(4, rede.inicios());
  wifi.ciclo(t += 100);
  TEST_ASSERT_EQUAL_INT(5, rede.inicios());
}

void test_wifi_que_cai_na_reserva_tenta_a_reserva_antes_da_principal() {
  RedeWifiSimulada rede(2);
  ReconexaoWifi wifi(rede, 1000);
  wifi.ciclo(0);
  wifi.ciclo(1000);  // principal fora: reserva
  wifi.ciclo(1001);
  rede.definirConectada(true);
  wifi.ciclo(1100);  // conectado na reserva
  rede.definirConectada(false);  // a reserva caiu
  wifi.ciclo(1200);
  TEST_ASSERT_EQUAL_UINT32(1, wifi.quedas());
  TEST_ASSERT_EQUAL_UINT8(1, rede.redeDoInicio(2));  // tenta de novo a mesma rede
  wifi.ciclo(2200);  // esgotou: volta à principal, na hora (a rodada começou de novo depois de conectar)
  wifi.ciclo(2201);
  TEST_ASSERT_EQUAL_UINT8(0, rede.redeDoInicio(3));
  TEST_ASSERT_EQUAL_UINT32(2, wifi.trocasDeRede());
}

// ---------------------------------------------------------------- integração: coleta → buffer → JSON → servidor

/// Uma corrida de 120 amostras pelo caminho real do contrato (JSON nos dois sentidos), com a rede fora
/// do ar no meio e uma resposta perdida. O servidor termina com as 120, em ordem e sem duplicata, e a
/// corrida só é encerrada depois do último lote.
void test_integracao_corrida_pelo_json_com_rede_caindo_no_meio() {
  EnvioSimulado servidor;
  EnvioJsonSimulado cliente(servidor);
  TravaNula trava;
  ControladorEnvio controlador(cliente, trava);
  controlador.iniciarCorrida(9);

  uint32_t t = 0;
  for (uint32_t s = 1; s <= 120; ++s) {
    const bool ultima = s == 120;
    const Pose pose{{static_cast<uint8_t>(s % 4), static_cast<uint8_t>((s / 4) % 4)}, Direcao::Norte};
    controlador.registrar(montarAmostra(pose, s * 1000, 100,
                                        ultima ? EstadoCorrida::Parado : EstadoCorrida::Explorando,
                                        ultima ? EventoTelemetria::ObjetivoAlcancado : EventoTelemetria::Nenhum));
    if (s == 40) {
      servidor.definirOffline(true);  // o Wi-Fi cai por 30 amostras
    }
    if (s == 70) {
      servidor.definirOffline(false);
      servidor.perderProximasRespostas(1);  // grava, mas a resposta não chega
    }
    for (int c = 0; c < 10; ++c) {  // 10 ciclos de rede de 100 ms por amostra
      controlador.ciclo(t);
      t += 100;
    }
  }
  controlador.solicitarEncerramento(t);
  for (int c = 0; c < 2000 && !controlador.encerrada(); ++c) {
    controlador.ciclo(t);
    t += 100;
  }

  TEST_ASSERT_TRUE(controlador.encerrada());
  TEST_ASSERT_EQUAL_UINT16(120, servidor.amostrasArmazenadas());
  for (uint16_t i = 0; i < 120; ++i) {
    TEST_ASSERT_EQUAL_UINT32(i + 1, servidor.armazenada(i).numeroSequencia);
    TEST_ASSERT_EQUAL_UINT32((i + 1) * 1000, servidor.armazenada(i).tempoDecorridoMs);
  }
  TEST_ASSERT_EQUAL_UINT16(120, servidor.amostrasAoEncerrar());  // encerrou com tudo entregue
  TEST_ASSERT_TRUE(servidor.duplicatasIgnoradas() >= 1);         // o reenvio da resposta perdida
  TEST_ASSERT_EQUAL_UINT32(0, controlador.amostrasPerdidas());
  TEST_ASSERT_EQUAL_UINT16(0, controlador.ocupacao());
}

/// #236: corrida de 60 amostras com o servidor respondendo em 200 ms, depois lento (3 s, dentro do
/// tempo limite), depois passando do tempo limite (o robô desiste em 5 s, mas o servidor grava), com
/// `503` no meio. A coleta continua registrando durante as chamadas lentas, sem achar a trava segura e
/// sem atrasar mais que um ciclo; o servidor termina com as 60 amostras, em ordem e sem duplicata; e a
/// corrida só é encerrada com tudo entregue.
void test_integracao_servidor_lento_nao_atrasa_a_coleta_nem_duplica() {
  EnvioSimulado servidor;
  TravaNula trava;
  uint32_t agora = 0;
  ColetaSimulada coleta;
  EnvioLentoSimulado cliente(servidor, agora, coletarDuranteRede, &coleta);
  ControladorEnvio controlador(cliente, trava);
  coleta.controlador = &controlador;
  coleta.trava = &trava;
  coleta.total = 60;
  servidor.vigiarTrava(&trava);
  controlador.iniciarCorrida(5);

  bool erroForcado = false;
  bool encerramentoPedido = false;
  for (int ciclo = 0; ciclo < 5000 && !controlador.encerrada(); ++ciclo) {
    coletar(agora, &coleta);
    const uint32_t registradas = coleta.proxima - 1;
    if (registradas < 15) {
      cliente.definirDemora(200);
    } else if (registradas < 30) {
      cliente.definirDemora(3000);
    } else if (registradas < 40) {
      cliente.definirDemora(TEMPO_LIMITE_REDE_MS + 2000);
    } else {
      cliente.definirDemora(200);
    }
    if (registradas >= 45 && !erroForcado) {
      servidor.responderProximasCom(503, 2);
      erroForcado = true;
    }
    if (registradas == coleta.total && !encerramentoPedido) {
      controlador.solicitarEncerramento(agora);  // como a tarefaRede faz depois da última amostra
      encerramentoPedido = true;
    }
    controlador.ciclo(agora);
    agora += PERIODO_CICLO_REDE_MS;
  }

  TEST_ASSERT_TRUE(controlador.encerrada());
  TEST_ASSERT_EQUAL_UINT16(60, servidor.amostrasArmazenadas());
  for (uint16_t i = 0; i < 60; ++i) {
    TEST_ASSERT_EQUAL_UINT32(i + 1, servidor.armazenada(i).numeroSequencia);
    TEST_ASSERT_EQUAL_UINT32((i + 1) * 1000, servidor.armazenada(i).tempoDecorridoMs);
  }
  TEST_ASSERT_EQUAL_UINT16(60, servidor.amostrasAoEncerrar());
  TEST_ASSERT_TRUE(cliente.temposEsgotados() >= 1);       // o caso do tempo limite aconteceu
  TEST_ASSERT_TRUE(servidor.duplicatasIgnoradas() >= 1);  // e o lote gravado nele voltou sem duplicar
  TEST_ASSERT_TRUE(coleta.registradasDuranteRede >= 10);  // a coleta seguiu durante as chamadas lentas
  TEST_ASSERT_TRUE(coleta.maiorAtrasoMs < PERIODO_CICLO_REDE_MS);
  TEST_ASSERT_EQUAL_UINT32(0, coleta.comTravaSegura);
  TEST_ASSERT_EQUAL_UINT32(0, servidor.chamadasComTravaSegura());
  TEST_ASSERT_EQUAL_INT32(1, trava.maiorProfundidade());
  TEST_ASSERT_EQUAL_UINT32(0, controlador.amostrasPerdidas());
  TEST_ASSERT_EQUAL_UINT16(0, controlador.ocupacao());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_lote_com_uma_amostra_tem_os_nomes_das_colunas_e_as_unidades_do_banco);
  RUN_TEST(test_evento_so_aparece_quando_existe);
  RUN_TEST(test_lote_com_varias_amostras_em_ordem_e_json_valido);
  RUN_TEST(test_numeros_negativos_e_pequenos_com_tres_casas);
  RUN_TEST(test_lote_vazio_e_orientacao_invalida_nao_quebram_o_json);
  RUN_TEST(test_lote_que_nao_cabe_devolve_zero_e_texto_vazio);
  RUN_TEST(test_pior_amostra_e_pior_lote_cabem_no_espaco_reservado);
  RUN_TEST(test_contrato_lote_igual_ao_exemplo_do_backend);
  RUN_TEST(test_le_as_contagens_da_resposta_do_lote);
  RUN_TEST(test_resposta_do_lote_sem_as_contagens_e_recusada);
  RUN_TEST(test_le_o_id_da_corrida_criada);
  RUN_TEST(test_corpos_da_criacao_e_do_encerramento);
  RUN_TEST(test_amostra_montada_a_partir_da_pose);
  RUN_TEST(test_sem_credenciais_o_envio_fica_desligado);
  RUN_TEST(test_so_https_e_com_certificado_ou_excecao_de_bancada);
  RUN_TEST(test_url_das_funcoes);
  RUN_TEST(test_periodo_da_coleta_e_tempos_da_rede);
  RUN_TEST(test_wifi_conecta_na_primeira_tentativa);
  RUN_TEST(test_wifi_espera_cada_vez_mais_entre_tentativas_e_volta_a_1s);
  RUN_TEST(test_wifi_que_cai_tenta_de_novo_na_hora_e_conta_a_queda);
  RUN_TEST(test_wifi_que_volta_sozinho_durante_a_espera_e_aceito);
  RUN_TEST(test_wifi_com_o_relogio_dando_a_volta);
  RUN_TEST(test_wifi_com_uma_rede_nunca_troca_de_rede);
  RUN_TEST(test_wifi_com_a_principal_fora_passa_para_a_reserva_na_hora);
  RUN_TEST(test_wifi_com_as_duas_redes_fora_espera_cada_vez_mais_a_cada_rodada);
  RUN_TEST(test_wifi_que_cai_na_reserva_tenta_a_reserva_antes_da_principal);
  RUN_TEST(test_integracao_corrida_pelo_json_com_rede_caindo_no_meio);
  RUN_TEST(test_integracao_servidor_lento_nao_atrasa_a_coleta_nem_duplica);
  return UNITY_END();
}
