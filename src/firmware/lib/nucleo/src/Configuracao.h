#pragma once

#include <stdint.h>

namespace micromouse {

constexpr uint8_t MAX_LINHAS_LABIRINTO = 4;
constexpr uint8_t MAX_COLUNAS_LABIRINTO = 12;
constexpr uint16_t TAMANHO_CELULA_MM = 180;
constexpr uint32_t PERIODO_TELEMETRIA_MS = 1000;
constexpr uint32_t LIMITE_TEMPO_CORRIDA_MS = 10UL * 60UL * 1000UL;

}  // namespace micromouse
