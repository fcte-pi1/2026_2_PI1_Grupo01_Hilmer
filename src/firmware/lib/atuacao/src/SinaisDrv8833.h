#pragma once

/// @file SinaisDrv8833.h
/// @brief Conversão da potência de uma roda nos sinais PWM das entradas IN1 e IN2 do DRV8833.
///
/// Fica separada do driver (`MotoresDrv8833`, em `drivers_esp32`) para ser testada no
/// computador, sem o Arduino.

#include <stdint.h>

#include "IMotores.h"

namespace micromouse {

/// Ciclo de trabalho (duty) das duas entradas de uma ponte H do DRV8833.
struct SinaisPonteH {
  uint32_t in1;  ///< Duty da entrada IN1 (0 a `dutyMaximo`).
  uint32_t in2;  ///< Duty da entrada IN2 (0 a `dutyMaximo`).
};

/// Sinais para uma potência, no modo *fast decay* da tabela do DRV8833 (datasheet SLVSAR1E,
/// tabela 2):
/// - potência positiva: PWM em IN1 e IN2 em 0 (avança);
/// - potência negativa: IN1 em 0 e PWM em IN2 (recua);
/// - potência zero: IN1 e IN2 em 0 (roda livre).
///
/// @param potencia De `-POTENCIA_MAXIMA` a `POTENCIA_MAXIMA` (valores fora são limitados).
/// @param inverter `true` para o motor montado espelhado: troca o sentido.
/// @param dutyMaximo Duty correspondente a 100% (por exemplo, 1023 com PWM de 10 bits).
constexpr SinaisPonteH calcularSinaisDrv8833(int16_t potencia, bool inverter, uint32_t dutyMaximo) {
  const int16_t limitada = limitarPotencia(inverter ? -static_cast<int32_t>(potencia) : potencia);
  const uint32_t modulo = static_cast<uint32_t>(limitada < 0 ? -limitada : limitada);
  const uint32_t duty = modulo * dutyMaximo / POTENCIA_MAXIMA;
  return limitada >= 0 ? SinaisPonteH{duty, 0} : SinaisPonteH{0, duty};
}

/// Sinais de freio: IN1 e IN2 em nível alto (as duas saídas em curto, a roda trava).
constexpr SinaisPonteH sinaisFreioDrv8833(uint32_t dutyMaximo) { return SinaisPonteH{dutyMaximo, dutyMaximo}; }

}  // namespace micromouse
