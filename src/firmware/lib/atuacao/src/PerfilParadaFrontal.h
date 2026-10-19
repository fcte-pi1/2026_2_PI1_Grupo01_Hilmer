#pragma once

/// @file PerfilParadaFrontal.h
/// @brief Redução de velocidade e parada antes de uma parede frontal (#191, HU-12).

#include <stdint.h>

#include "ConfiguracaoControle.h"

namespace micromouse {

/// Potência base das rodas em função da distância até a parede da frente.
///
/// - Acima de `distanciaInicioDesaceleracaoMm`: `potenciaCruzeiro`.
/// - Entre o início da desaceleração e a parada: rampa linear de `potenciaCruzeiro` até
///   `potenciaMinima`.
/// - Em `distanciaParadaFrontalMm` ou menos: 0 (parar).
///
/// Sem estado: a mesma leitura sempre dá a mesma potência.
class PerfilParadaFrontal {
 public:
  explicit PerfilParadaFrontal(const ConfiguracaoControle& config = ConfiguracaoControle{});

  /// Potência base para a leitura frontal, igual nas duas rodas. 0 significa parar.
  int16_t potenciaBase(uint16_t frenteMm) const;

  /// `true` se a leitura frontal já pede a parada.
  bool deveParar(uint16_t frenteMm) const { return frenteMm <= config_.distanciaParadaFrontalMm; }

 private:
  ConfiguracaoControle config_;
};

}  // namespace micromouse
