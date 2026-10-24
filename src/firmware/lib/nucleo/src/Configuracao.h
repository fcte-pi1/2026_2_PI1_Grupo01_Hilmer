#pragma once

/// @file Configuracao.h
/// @brief Constantes do desafio e do firmware, com a origem de cada valor.

#include <stdint.h>

namespace micromouse {

/// Maior número de linhas de um labirinto da competição (labirinto 12×4).
/// Dimensiona as estruturas de tamanho fixo do mapa, sem alocação dinâmica.
constexpr uint8_t MAX_LINHAS_LABIRINTO = 4;

/// Maior número de colunas de um labirinto da competição (labirinto 12×4).
constexpr uint8_t MAX_COLUNAS_LABIRINTO = 12;

/// Lado de uma célula do labirinto, em milímetros (18 × 18 cm).
constexpr uint16_t TAMANHO_CELULA_MM = 180;

/// Intervalo entre envios de telemetria, em milissegundos.
/// A arquitetura prevê envio a cerca de 1 s, o que atende a latência máxima de 2 s do RNF05.
constexpr uint32_t PERIODO_TELEMETRIA_MS = 1000;

/// Tempo máximo de uma tentativa, em milissegundos: 10 minutos (RNF03).
constexpr uint32_t LIMITE_TEMPO_CORRIDA_MS = 10UL * 60UL * 1000UL;

/// Frequência do barramento I²C dos 3 VL53L0X e do MPU6050, em Hz. Os dois aceitam 400 kHz, mas
/// fica em 100 kHz (o padrão do `Wire`) até a bancada confirmar os resistores de pull-up dos módulos.
/// Provisório.
constexpr uint32_t FREQUENCIA_I2C_HZ = 100000;

/// Maior espera, em ms, para uma tarefa conseguir o barramento I²C quando outra o está usando. Uma
/// leitura do MPU6050 leva menos de 1 ms a 100 kHz, e a dos 3 ToF, poucos ms. Passado esse tempo,
/// a leitura falha (e é contada) em vez de atrasar a tarefa além do seu ciclo de 10 ms. Provisório.
constexpr uint32_t ESPERA_MAXIMA_I2C_MS = 8;

}  // namespace micromouse
