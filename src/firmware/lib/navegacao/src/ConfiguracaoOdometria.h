#pragma once

/// @file ConfiguracaoOdometria.h
/// @brief Parâmetros da odometria diferencial: conversão de pulsos, giro e tamanho da célula.
///
/// `PULSOS_DIFERENCIAIS_POR_90_GRAUS_PROVISORIO` foi medido no robô: girando-o 90° no lugar, a diferença
/// `contagemEsquerda - contagemDireita` variou 1803 pulsos (bitola efetiva ~83 mm com 72,7 µm por pulso).
/// Fica provisório porque o giro foi feito à mão; refaça se a bitola ou as rodas mudarem.

#include <stdint.h>

#include "Configuracao.h"
#include "ConfiguracaoEncoder.h"

namespace micromouse {

/// Diferença de pulsos entre as rodas (esquerda menos direita) que corresponde a um giro de 90°
/// para a direita. É a bitola em pulsos: `bitola × π/2 / distância por pulso`.
constexpr uint32_t PULSOS_DIFERENCIAIS_POR_90_GRAUS_PROVISORIO = 1803;

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
