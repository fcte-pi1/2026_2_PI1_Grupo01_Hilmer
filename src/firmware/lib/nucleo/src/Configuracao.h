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

/// Capacidade do mapa de exploração da largada em qualquer célula (`LARGADA_EM_QUALQUER_CELULA`):
/// sem saber onde foi colocado, o robô começa no meio de um mapa com o dobro (menos uma célula) do
/// maior labirinto em cada eixo, 7 × 23. Qualquer posição real da largada cabe nele.
constexpr uint8_t MAX_LINHAS_EXPLORACAO = 2 * MAX_LINHAS_LABIRINTO - 1;
constexpr uint8_t MAX_COLUNAS_EXPLORACAO = 2 * MAX_COLUNAS_LABIRINTO - 1;

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

/// Se o robô pode ser colocado em qualquer célula do labirinto, sem saber qual nem o tamanho do
/// labirinto. Ele só precisa estar virado para o Norte do labirinto (o lado das linhas maiores, o
/// mesmo da largada oficial em (0, 0)). Com `true`, a navegação explora num mapa de exploração,
/// descarta as hipóteses de posição e tamanho que contradizem as paredes vistas e, quando sobra uma,
/// passa para o labirinto real e segue até o objetivo (veja `LocalizadorLargada` e `CicloNavegacao`).
/// Com `false`, a largada é sempre a célula (0, 0) virada para o Norte, no 4x4.
///
/// Vem da macro `MICROMOUSE_LARGADA_EM_QUALQUER_CELULA` (0 ou 1), passada pelo `platformio.ini` nos
/// ambientes do robô. Sem a macro, vale a largada fixa, como nos testes `native` que não a pedem.
#ifndef MICROMOUSE_LARGADA_EM_QUALQUER_CELULA
#define MICROMOUSE_LARGADA_EM_QUALQUER_CELULA 0
#endif
constexpr bool LARGADA_EM_QUALQUER_CELULA = MICROMOUSE_LARGADA_EM_QUALQUER_CELULA != 0;

}  // namespace micromouse
