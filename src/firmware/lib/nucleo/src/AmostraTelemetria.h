#pragma once

/// @file AmostraTelemetria.h
/// @brief Uma amostra da telemetria, como o robô a guarda e a envia ao Supabase (#238, #236).
///
/// Os campos correspondem, um a um, às colunas da tabela `telemetria` (veja o contrato do lote em
/// `src/backend/tests/fixtures/lote-telemetria.json`). Os números ficam em inteiros porque o ESP32-C3 não
/// tem unidade de ponto flutuante; a conversão para as unidades do banco (m/s, V, A e s) acontece
/// ao montar o JSON do lote.

#include <stdint.h>

namespace micromouse {

/// Valores de `telemetria.estado_corrida`. Provisório: a #242 (Aguardando Largada) e a #231
/// (dashboard) podem acrescentar estados.
enum class EstadoCorrida : uint8_t {
  Aguardando,  ///< Calibrado, na largada, esperando começar.
  Explorando,  ///< Percorrendo o labirinto à procura do objetivo.
  Retornando,  ///< Voltando para a largada.
  Parado       ///< Motores parados (chegou ao objetivo ou parou por falha).
};

/// Valores de `telemetria.evento` (a coluna só aceita estes dois, ou nulo).
enum class EventoTelemetria : uint8_t {
  Nenhum,
  Colisao,
  ObjetivoAlcancado
};

/// Texto de `estado_corrida` no banco.
constexpr const char* nomeDoEstado(EstadoCorrida estado) {
  return estado == EstadoCorrida::Aguardando   ? "aguardando"
         : estado == EstadoCorrida::Explorando ? "explorando"
         : estado == EstadoCorrida::Retornando ? "retornando"
                                               : "parado";
}

/// Texto de `evento` no banco, ou `nullptr` para `Nenhum` (a coluna fica nula).
constexpr const char* nomeDoEvento(EventoTelemetria evento) {
  return evento == EventoTelemetria::Colisao             ? "colisao"
         : evento == EventoTelemetria::ObjetivoAlcancado ? "objetivo_alcancado"
                                                         : nullptr;
}

/// Uma amostra. O `numeroSequencia` é dado quando a amostra é gerada (a coleta) e **nunca muda**
/// num reenvio: é ele que o servidor usa para ignorar duplicatas.
struct AmostraTelemetria {
  uint32_t numeroSequencia = 0;    ///< `numero_sequencia`: 1, 2, 3... por corrida.
  uint32_t tempoDecorridoMs = 0;   ///< `tempo_decorrido`, em ms desde o início da corrida.
  int32_t velocidadeMmPorS = 0;    ///< `velocidade`, em mm/s (estimada, sem encoder).
  uint16_t tensaoBateriaMv = 0;    ///< `tensao_bateria`, em mV (0 enquanto a bateria não é medida).
  int16_t correnteBateriaMa = 0;   ///< `corrente_bateria`, em mA (0: sem sensor de corrente).
  uint8_t posicaoLinha = 0;        ///< `posicao_linha`.
  uint8_t posicaoColuna = 0;       ///< `posicao_coluna`.
  char orientacao = 'N';           ///< `orientacao`: 'N', 'S', 'L' ou 'O'.
  EstadoCorrida estado = EstadoCorrida::Aguardando;     ///< `estado_corrida`.
  EventoTelemetria evento = EventoTelemetria::Nenhum;   ///< `evento`.
};

}  // namespace micromouse
