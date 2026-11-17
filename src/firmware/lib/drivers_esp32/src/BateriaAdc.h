#pragma once

/// @file BateriaAdc.h
/// @brief Leitura da tensão da bateria pelo ADC1 (GPIO34), no divisor R1/R2 (`Bateria.h`).

#include <Arduino.h>

#include "Bateria.h"
#include "Pinos.h"

namespace micromouse {

/// Lê a bateria pela média de `BATERIA_LEITURAS` leituras calibradas do ADC. O GPIO34 é do ADC1, que
/// continua funcionando com o Wi-Fi ligado (o ADC2 não). Só tem tensão com a chave geral ligada.
class BateriaAdc {
 public:
  /// Configura o pino (atenuação de 11 dB: faixa até cerca de 3,1 V no pino). Chame no `setup()`.
  void iniciar() {
    pinMode(pinos::BATERIA_ADC, INPUT);
    analogSetPinAttenuation(pinos::BATERIA_ADC, ADC_11db);
  }

  /// Tensão da bateria, em mV. Leva cerca de 2 ms; 0 com a chave geral desligada (só USB).
  uint32_t lerMv() const {
    uint32_t somaPinoMv = 0;
    for (uint32_t i = 0; i < BATERIA_LEITURAS; i++) {
      somaPinoMv += analogReadMilliVolts(pinos::BATERIA_ADC);
    }
    return bateriaMvDoPino(somaPinoMv / BATERIA_LEITURAS);
  }
};

}  // namespace micromouse
