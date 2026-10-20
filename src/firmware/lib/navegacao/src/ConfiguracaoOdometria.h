#pragma once

/// @file ConfiguracaoOdometria.h
/// @brief Parâmetros da odometria diferencial: conversão de pulsos, giro e tamanho da célula.
///
/// ATENÇÃO: `PULSOS_DIFERENCIAIS_POR_90_GRAUS_PROVISORIO` é PROVISÓRIO e ilustrativo. Depende da
/// distância entre as rodas (bitola), que ainda não consta em nenhum documento: com 100 mm, o giro
/// de 90° pede uma diferença de π/2 × 100 mm = 157 mm entre as rodas, ou 625 pulsos. Meça na
/// bancada (issue #187): gire o robô 90° no lugar e leia `contagemEsquerda - contagemDireita`.

#include <stdint.h>

#include "Configuracao.h"
#include "ConfiguracaoEncoder.h"

namespace micromouse {

/// Diferença de pulsos entre as rodas (esquerda menos direita) que corresponde a um giro de 90°
/// para a direita. É a bitola em pulsos: `bitola × π/2 / distância por pulso`.
constexpr uint32_t PULSOS_DIFERENCIAIS_POR_90_GRAUS_PROVISORIO = 625;

/// Parâmetros usados pela `Odometria`.
struct ConfiguracaoOdometria {
  /// Distância da roda por pulso, em µm (veja `ConfiguracaoEncoder.h`).
  uint32_t micrometrosPorPulso = MICROMETROS_POR_PULSO_PROVISORIO;
  /// Diferença de pulsos entre as rodas num giro de 90° (veja acima).
  uint32_t pulsosDiferenciaisPor90Graus = PULSOS_DIFERENCIAIS_POR_90_GRAUS_PROVISORIO;
  /// Lado da célula, em µm.
  uint32_t tamanhoCelulaMicrometros = static_cast<uint32_t>(TAMANHO_CELULA_MM) * 1000;
};

}  // namespace micromouse
