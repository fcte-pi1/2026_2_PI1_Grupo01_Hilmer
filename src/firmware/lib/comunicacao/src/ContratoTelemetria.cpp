#include "ContratoTelemetria.h"

#include <ArduinoJson.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "Direcao.h"

namespace micromouse {

namespace {

/// Acrescenta texto formatado em `saida` a partir de `usado`. Devolve `false` se não couber.
bool acrescentar(char* saida, size_t capacidade, size_t& usado, const char* formato, ...)
    __attribute__((format(printf, 4, 5)));

bool acrescentar(char* saida, size_t capacidade, size_t& usado, const char* formato, ...) {
  if (usado >= capacidade) {
    return false;
  }
  va_list argumentos;
  va_start(argumentos, formato);
  const int escritos = vsnprintf(saida + usado, capacidade - usado, formato, argumentos);
  va_end(argumentos);
  if (escritos < 0 || static_cast<size_t>(escritos) >= capacidade - usado) {
    return false;
  }
  usado += static_cast<size_t>(escritos);
  return true;
}

/// Escreve um valor em milésimos como número decimal com três casas: 1234 → "1.234", −5 → "-0.005".
bool acrescentarMilesimos(char* saida, size_t capacidade, size_t& usado, int64_t milesimos) {
  const bool negativo = milesimos < 0;
  const uint64_t modulo = negativo ? static_cast<uint64_t>(-(milesimos + 1)) + 1 : static_cast<uint64_t>(milesimos);
  return acrescentar(saida, capacidade, usado, "%s%llu.%03llu", negativo ? "-" : "",
                     static_cast<unsigned long long>(modulo / 1000), static_cast<unsigned long long>(modulo % 1000));
}

/// Letra da orientação segura para o JSON: só letras maiúsculas passam; o resto vira `?` (o servidor
/// recusa o lote, em vez de o firmware gerar um JSON quebrado).
char letraSegura(char orientacao) { return (orientacao >= 'A' && orientacao <= 'Z') ? orientacao : '?'; }

bool acrescentarAmostra(char* saida, size_t capacidade, size_t& usado, const AmostraTelemetria& a) {
  bool ok = acrescentar(saida, capacidade, usado,
                        "{\"numero_sequencia\":%lu,\"posicao_linha\":%u,\"posicao_coluna\":%u,\"orientacao\":\"%c\","
                        "\"velocidade\":",
                        static_cast<unsigned long>(a.numeroSequencia), static_cast<unsigned>(a.posicaoLinha),
                        static_cast<unsigned>(a.posicaoColuna), letraSegura(a.orientacao));
  ok = ok && acrescentarMilesimos(saida, capacidade, usado, a.velocidadeMmPorS);
  ok = ok && acrescentar(saida, capacidade, usado, ",\"tensao_bateria\":");
  ok = ok && acrescentarMilesimos(saida, capacidade, usado, a.tensaoBateriaMv);
  ok = ok && acrescentar(saida, capacidade, usado, ",\"corrente_bateria\":");
  ok = ok && acrescentarMilesimos(saida, capacidade, usado, a.correnteBateriaMa);
  ok = ok && acrescentar(saida, capacidade, usado, ",\"tempo_decorrido\":");
  ok = ok && acrescentarMilesimos(saida, capacidade, usado, a.tempoDecorridoMs);
  ok = ok && acrescentar(saida, capacidade, usado, ",\"estado_corrida\":\"%s\"", nomeDoEstado(a.estado));
  const char* evento = nomeDoEvento(a.evento);
  if (evento != nullptr) {
    ok = ok && acrescentar(saida, capacidade, usado, ",\"evento\":\"%s\"", evento);
  }
  return ok && acrescentar(saida, capacidade, usado, "}");
}

/// Lê um inteiro de `valor` dentro de [minimo, maximo].
bool lerInteiro(JsonVariantConst valor, long long minimo, long long maximo, long long& saida) {
  if (!valor.is<long long>()) {
    return false;
  }
  const long long lido = valor.as<long long>();
  if (lido < minimo || lido > maximo) {
    return false;
  }
  saida = lido;
  return true;
}

}  // namespace

size_t serializarLote(uint32_t corridaId, const AmostraTelemetria* amostras, uint16_t quantidade, char* saida,
                      size_t capacidade) {
  if (saida == nullptr || capacidade == 0 || (amostras == nullptr && quantidade > 0)) {
    return 0;
  }
  size_t usado = 0;
  bool ok = acrescentar(saida, capacidade, usado, "{\"corrida_id\":%lu,\"amostras\":[",
                        static_cast<unsigned long>(corridaId));
  for (uint16_t i = 0; ok && i < quantidade; ++i) {
    ok = (i == 0 || acrescentar(saida, capacidade, usado, ",")) && acrescentarAmostra(saida, capacidade, usado, amostras[i]);
  }
  ok = ok && acrescentar(saida, capacidade, usado, "]}");
  if (!ok) {
    saida[0] = '\0';
    return 0;
  }
  return usado;
}

bool lerRespostaLote(const char* corpo, uint16_t& gravadas, uint16_t& existentes) {
  if (corpo == nullptr) {
    return false;
  }
  JsonDocument doc;
  if (deserializeJson(doc, corpo) != DeserializationError::Ok || !doc.is<JsonObjectConst>()) {
    return false;
  }
  long long g = 0;
  long long e = 0;
  if (!lerInteiro(doc["gravadas"], 0, UINT16_MAX, g) || !lerInteiro(doc["existentes"], 0, UINT16_MAX, e)) {
    return false;
  }
  gravadas = static_cast<uint16_t>(g);
  existentes = static_cast<uint16_t>(e);
  return true;
}

bool lerIdCorrida(const char* corpo, uint32_t& id) {
  if (corpo == nullptr) {
    return false;
  }
  JsonDocument doc;
  if (deserializeJson(doc, corpo) != DeserializationError::Ok || !doc.is<JsonObjectConst>()) {
    return false;
  }
  long long lido = 0;
  if (!lerInteiro(doc["id"], 1, INT32_MAX, lido)) {  // `corridas.id` é um inteiro de 32 bits no banco
    return false;
  }
  id = static_cast<uint32_t>(lido);
  return true;
}

const char* nomeDoLabirinto(DimensaoMapa dimensao) {
  switch (dimensao) {
    case DimensaoMapa::Labirinto4x4:
      return "4x4";
    case DimensaoMapa::Labirinto8x4:
      return "8x4";
    case DimensaoMapa::Labirinto12x4:
      return "12x4";
    case DimensaoMapa::Exploracao:
      break;
  }
  return "";
}

bool lerComandoRobo(const char* corpo, ComandoRobo& comando) {
  if (corpo == nullptr) {
    return false;
  }
  JsonDocument doc;
  if (deserializeJson(doc, corpo) != DeserializationError::Ok || !doc.is<JsonObjectConst>()) {
    return false;
  }
  JsonVariantConst lido = doc["comando"];
  if (lido.isNull()) {
    comando = ComandoRobo::Nenhum;
    return true;
  }
  if (!lido.is<const char*>()) {
    return false;
  }
  const char* texto = lido.as<const char*>();
  if (strcmp(texto, "iniciar") == 0) {
    comando = ComandoRobo::Iniciar;
  } else if (strcmp(texto, "parar") == 0) {
    comando = ComandoRobo::Parar;
  } else if (strcmp(texto, "zerar") == 0) {
    comando = ComandoRobo::Zerar;
  } else {
    return false;
  }
  return true;
}

size_t serializarLogs(uint32_t corridaId, const char* const* linhas, size_t quantidade, char* saida, size_t capacidade) {
  if (linhas == nullptr || quantidade == 0 || saida == nullptr || capacidade == 0) {
    return 0;
  }
  JsonDocument doc;
  if (corridaId > 0) {
    doc["corrida_id"] = corridaId;
  }
  JsonArray lista = doc["linhas"].to<JsonArray>();
  for (size_t i = 0; i < quantidade; ++i) {
    if (linhas[i] == nullptr || linhas[i][0] == '\0') {
      return 0;
    }
    lista.add(linhas[i]);
  }
  if (measureJson(doc) >= capacidade) {
    return 0;
  }
  return serializeJson(doc, saida, capacidade);
}

size_t serializarCriacaoCorrida(const char* labirinto, char* saida, size_t capacidade) {
  if (labirinto == nullptr || saida == nullptr || capacidade == 0) {
    return 0;
  }
  size_t usado = 0;
  if (!acrescentar(saida, capacidade, usado, "{\"labirinto\":\"%s\"}", labirinto)) {
    saida[0] = '\0';
    return 0;
  }
  return usado;
}

size_t serializarEncerramento(bool concluida, char* saida, size_t capacidade) {
  if (saida == nullptr || capacidade == 0) {
    return 0;
  }
  size_t usado = 0;
  if (!acrescentar(saida, capacidade, usado, "{\"status\":\"%s\"}", concluida ? "concluida" : "nao_concluida")) {
    saida[0] = '\0';
    return 0;
  }
  return usado;
}

AmostraTelemetria montarAmostra(const Pose& pose, uint32_t tempoDecorridoMs, int32_t velocidadeMmPorS,
                                EstadoCorrida estado, EventoTelemetria evento) {
  AmostraTelemetria a;
  a.numeroSequencia = 0;
  a.tempoDecorridoMs = tempoDecorridoMs;
  a.velocidadeMmPorS = velocidadeMmPorS;
  a.tensaoBateriaMv = 0;
  a.correnteBateriaMa = 0;
  a.posicaoLinha = pose.celula.linha;
  a.posicaoColuna = pose.celula.coluna;
  a.orientacao = letraDaDirecao(pose.direcao);
  a.estado = estado;
  a.evento = evento;
  return a;
}

}  // namespace micromouse
