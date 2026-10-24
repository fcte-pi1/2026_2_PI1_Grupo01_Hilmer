#pragma once

/// @file IImu.h
/// @brief Interface de acesso à unidade de medição inercial, a IMU (camada `hal`).
///
/// A lógica de orientação depende só desta interface, nunca do driver concreto. Assim, ela pode
/// ser testada no computador com o `ImuSimulado` e, na placa, usar o driver do MPU6050 por I²C
/// (em `drivers_esp32`).

#include <stdint.h>

namespace micromouse {

/// Fonte da taxa de giro em torno do eixo vertical (Z) do robô.
///
/// Unidade: décimos de grau por segundo (10 = 1 °/s), a mesma de
/// `MedidasProtecao::taxaGiroDecimosDps`.
///
/// Convenção de sinal: **positivo quando o robô gira para a esquerda** (sentido anti-horário visto
/// de cima, regra da mão direita com o eixo Z para cima) e negativo quando gira para a direita.
/// O driver inverte o sinal se o sensor estiver montado de cabeça para baixo.
///
/// Implementações:
/// - `ImuSimulado` (`lib/simulacao`): taxa definida pelo teste, com viés e ruído, sem hardware.
/// - `ImuMpu6050` (`lib/drivers_esp32`): o MPU6050 pelo I²C, compartilhado com os ToF (#378).
class IImu {
 public:
  virtual ~IImu() = default;

  /// Lê a taxa de giro em Z.
  /// @param[out] taxaDecimosDps Recebe a taxa medida, já convertida para décimos de °/s. Não muda
  ///             se a função retorna `false`.
  /// @return `false` se o sensor não respondeu ou a leitura é inválida; `true` caso contrário.
  virtual bool lerTaxaGiroZ(int16_t& taxaDecimosDps) = 0;
};

}  // namespace micromouse
