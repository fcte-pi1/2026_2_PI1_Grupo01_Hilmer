#pragma once

/// @file ConversorDeslocamento.h
/// @brief Conversão de pulsos do encoder em distância percorrida pela roda.

#include <stdint.h>

#include "ConfiguracaoEncoder.h"

namespace micromouse {

/// Converte pulsos em distância usando uma constante de calibração, em micrômetros por pulso.
///
/// Só usa aritmética de inteiros (o ESP32-C3 não tem unidade de ponto flutuante). Os resultados
/// são saturados nos limites de `int32_t`, então uma contagem muito grande nunca estoura.
class ConversorDeslocamento {
 public:
  /// @param micrometrosPorPulso Distância da roda por pulso, em µm (padrão: o valor provisório).
  explicit ConversorDeslocamento(uint32_t micrometrosPorPulso = MICROMETROS_POR_PULSO_PROVISORIO)
      : micrometrosPorPulso_(micrometrosPorPulso) {}

  /// Distância correspondente aos pulsos, em micrômetros (negativa se a contagem for negativa).
  int32_t pulsosParaMicrometros(int32_t pulsos) const;

  /// Distância correspondente aos pulsos, em milímetros, arredondada para o mais próximo.
  int32_t pulsosParaMm(int32_t pulsos) const;

  /// Troca a constante de calibração.
  void definirMicrometrosPorPulso(uint32_t micrometrosPorPulso) {
    micrometrosPorPulso_ = micrometrosPorPulso;
  }

  /// Constante de calibração atual, em µm por pulso.
  uint32_t obterMicrometrosPorPulso() const { return micrometrosPorPulso_; }

  /// Calcula a constante de calibração a partir de um percurso medido na bancada.
  /// @param distanciaRealMm Distância que a roda realmente andou, medida com régua, em mm.
  /// @param pulsos Pulsos contados nesse percurso (valor positivo).
  /// @return µm por pulso, arredondado; `0` se `pulsos` for zero (calibração impossível).
  static uint32_t calcularMicrometrosPorPulso(uint32_t distanciaRealMm, uint32_t pulsos);

 private:
  uint32_t micrometrosPorPulso_;
};

}  // namespace micromouse
