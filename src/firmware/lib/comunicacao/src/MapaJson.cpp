#include "MapaJson.h"

#include <ArduinoJson.h>

#include "ArmazenamentoMapa.h"

namespace micromouse {

namespace {

const char* nomeDimensao(DimensaoMapa dimensao) {
  switch (dimensao) {
    case DimensaoMapa::Labirinto4x4:
      return "4x4";
    case DimensaoMapa::Labirinto8x4:
      return "8x4";
    case DimensaoMapa::Labirinto12x4:
      return "12x4";
    case DimensaoMapa::Exploracao:
      return "";
  }
  return "";
}

/// `true` para parede, `false` para livre e `null` para desconhecida.
void escreverParede(JsonObject celula, const char* campo, EstadoParede estado) {
  if (estado == EstadoParede::Desconhecido) {
    celula[campo] = nullptr;
  } else {
    celula[campo] = (estado == EstadoParede::Parede);
  }
}

}  // namespace

size_t MapaJson::serializar(const Mapa& mapa, char* saida, size_t capacidade) {
  if (saida == nullptr || capacidade == 0) {
    return 0;
  }
  saida[0] = '\0';
  if (mapa.obterDimensao() == DimensaoMapa::Exploracao) {
    return 0;  // o mapa de exploração não é o labirinto: o dashboard só recebe o mapa já localizado
  }

  JsonDocument doc;
  doc["dimensao"] = nomeDimensao(mapa.obterDimensao());
  doc["linhas"] = mapa.obterLinhas();
  doc["colunas"] = mapa.obterColunas();
  JsonArray celulas = doc["celulas"].to<JsonArray>();
  for (uint8_t linha = 0; linha < mapa.obterLinhas(); linha++) {
    for (uint8_t coluna = 0; coluna < mapa.obterColunas(); coluna++) {
      const Mapa::Celula c = mapa.obterCelula({linha, coluna});
      JsonObject objeto = celulas.add<JsonObject>();
      objeto["linha"] = linha;
      objeto["coluna"] = coluna;
      escreverParede(objeto, "parede_norte", c.norte);
      escreverParede(objeto, "parede_sul", c.sul);
      escreverParede(objeto, "parede_leste", c.leste);
      escreverParede(objeto, "parede_oeste", c.oeste);
    }
  }

  if (measureJson(doc) >= capacidade) {
    return 0;
  }
  return serializeJson(doc, saida, capacidade);
}

size_t MapaJson::serializarFinal(const ArmazenamentoMapa& armazenamento, char* saida, size_t capacidade) {
  const Mapa* final = armazenamento.mapaFinal();
  if (final == nullptr) {
    if (saida != nullptr && capacidade > 0) {
      saida[0] = '\0';
    }
    return 0;
  }
  return serializar(*final, saida, capacidade);
}

}  // namespace micromouse
