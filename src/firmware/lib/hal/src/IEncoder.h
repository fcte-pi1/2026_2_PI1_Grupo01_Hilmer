#pragma once

/// @file IEncoder.h
/// @brief Interface de acesso a um encoder de roda (camada `hal`).
///
/// A lógica de odometria depende só desta interface, nunca do driver concreto. Assim, ela pode ser
/// testada no computador com o `EncoderSimulado` e, na placa, usar o driver por interrupção de
/// GPIO (em `drivers_esp32`).

#include <stdint.h>

namespace micromouse {

/// Contador de pulsos de uma roda.
///
/// Convenção de sinal: a contagem **aumenta quando a roda avança** (robô indo para a frente) e
/// diminui quando ela recua. Como os motores ficam espelhados, o driver de uma das rodas inverte
/// o sentido (veja `DecodificadorQuadratura`), para que as duas rodas sigam a mesma convenção.
///
/// Implementações:
/// - `EncoderSimulado` (`lib/simulacao`): contagem definida pelo teste, sem hardware.
/// - Driver por interrupção de GPIO (`lib/drivers_esp32`): a ser criado.
class IEncoder {
 public:
  virtual ~IEncoder() = default;

  /// Pulsos acumulados desde a última chamada de `zerar()` (ou desde o início), já com a
  /// decodificação em quadratura aplicada.
  virtual int32_t lerContagem() const = 0;

  /// Volta a contagem para zero.
  virtual void zerar() = 0;
};

}  // namespace micromouse
