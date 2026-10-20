#pragma once

/// @file IMotores.h
/// @brief Interface de acionamento dos dois motores de tração (camada `hal`).
///
/// A lógica de controle (`lib/atuacao`) depende só desta interface, nunca do driver concreto.
/// Assim, ela pode ser testada no computador com os `MotoresSimulados` e, na placa, usar o
/// driver do DRV8833 (em `drivers_esp32`).

#include <stdint.h>

namespace micromouse {

/// Maior potência aceita por roda, em milésimos da potência total (1000 = 100%).
constexpr int16_t POTENCIA_MAXIMA = 1000;

/// Potência pedida para cada roda, de `-POTENCIA_MAXIMA` (ré máxima) a `POTENCIA_MAXIMA`
/// (frente máxima). Zero deixa a roda livre.
struct PotenciaRodas {
  int16_t esquerda;  ///< Roda esquerda (motor M1).
  int16_t direita;   ///< Roda direita (motor M2).
};

/// Par de motores de tração diferencial.
///
/// Convenção de sinal: potência **positiva faz a roda avançar** (robô indo para a frente), a
/// mesma do `IEncoder`. Como os motores ficam espelhados no chassi, o driver inverte o sentido
/// de um deles, para que as duas rodas sigam a mesma convenção.
///
/// Implementações:
/// - `MotoresSimulados` (`lib/simulacao`): guarda o último comando, sem hardware.
/// - `MotoresDrv8833` (`lib/drivers_esp32`): PWM nas entradas do DRV8833.
class IMotores {
 public:
  virtual ~IMotores() = default;

  /// Aplica a potência de cada roda. Valores fora de `[-POTENCIA_MAXIMA, POTENCIA_MAXIMA]`
  /// são limitados a essa faixa.
  virtual void definirPotencia(const PotenciaRodas& potencia) = 0;

  /// Para os dois motores com freio (as rodas ficam travadas, sem deslizar).
  virtual void parar() = 0;
};

/// Limita uma potência à faixa `[-POTENCIA_MAXIMA, POTENCIA_MAXIMA]`.
constexpr int16_t limitarPotencia(int32_t potencia) {
  return static_cast<int16_t>(potencia > POTENCIA_MAXIMA    ? POTENCIA_MAXIMA
                              : potencia < -POTENCIA_MAXIMA ? -POTENCIA_MAXIMA
                                                            : potencia);
}

}  // namespace micromouse
