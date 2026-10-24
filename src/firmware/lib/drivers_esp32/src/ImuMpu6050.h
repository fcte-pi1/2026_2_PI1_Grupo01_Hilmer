#pragma once

/// @file ImuMpu6050.h
/// @brief Driver do giroscópio do MPU6050 pelo I²C (#378).
///
/// Implementa `IImu` (camada `hal`). A conversão da leitura e o ajuste do sinal ficam em
/// `ConversorGiro.h` (camada `imu`), testados no computador; aqui só se fala com os registradores,
/// sempre segurando o `BarramentoI2c`, que é compartilhado com os 3 VL53L0X.

#include <Arduino.h>

#include "BarramentoI2c.h"
#include "ConfiguracaoImu.h"
#include "IImu.h"

namespace micromouse {

/// MPU6050 no barramento I²C, lido só no eixo Z do giroscópio.
///
/// `iniciar()` reinicia o sensor, tira-o do modo de economia, escolhe o relógio do giroscópio,
/// liga o filtro passa-baixa (cerca de 42 Hz, abaixo da metade da taxa de 100 leituras por segundo)
/// e configura a faixa `FAIXA_GIRO`. Depois disso, `lerTaxaGiroZ()` lê os dois bytes do eixo Z e os
/// converte para décimos de °/s.
///
/// Exemplo (a montagem real fica em `main.cpp`):
/// @code
/// BarramentoI2c barramento(Wire);
/// barramento.iniciar(pinos::I2C_SDA, pinos::I2C_SCL);
/// ImuMpu6050 imu(barramento);
/// if (!imu.iniciar()) { /* sem resposta no endereço 0x68 */ }
/// IImu& giro = imu;
/// @endcode
class ImuMpu6050 : public IImu {
 public:
  /// @param barramento Barramento I²C já iniciado.
  /// @param endereco Endereço I²C (0x68 com AD0 no GND).
  /// @param faixa Faixa do giroscópio.
  /// @param inverterZ `true` se o sensor estiver montado de cabeça para baixo.
  explicit ImuMpu6050(BarramentoI2c& barramento, uint8_t endereco = ENDERECO_I2C_MPU6050,
                      FaixaGiro faixa = FAIXA_GIRO, bool inverterZ = INVERTER_GIRO_Z);

  /// Reinicia e configura o sensor. Chame no `setup()`, depois de `BarramentoI2c::iniciar()`.
  /// Pode ser chamado de novo para reiniciar o sensor depois de uma falha (#241).
  /// @return `false` se o sensor não respondeu ou recusou a configuração.
  bool iniciar();

  /// `true` se o último `iniciar()` deu certo.
  bool iniciado() const { return iniciado_; }

  /// Valor lido do registrador WHO_AM_I no último `iniciar()`: 0x68 no MPU6050. Módulos com outro
  /// chip da família (MPU6500, MPU9250) respondem outro valor e funcionam com os mesmos registradores
  /// do giroscópio; o `main.cpp` só registra o valor no log.
  uint8_t identificacao() const { return identificacao_; }

  /// @copydoc IImu::lerTaxaGiroZ
  /// Falha se o sensor não foi iniciado, se o barramento não foi conseguido a tempo ou se a
  /// transação I²C não foi confirmada.
  bool lerTaxaGiroZ(int16_t& taxaDecimosDps) override;

  /// Leituras que falharam desde o último `iniciar()`.
  uint32_t falhasLeitura() const { return falhasLeitura_; }

 private:
  bool escreverRegistrador(uint8_t registrador, uint8_t valor);
  bool lerRegistradores(uint8_t registrador, uint8_t* destino, uint8_t quantidade);

  BarramentoI2c& barramento_;
  uint8_t endereco_;
  FaixaGiro faixa_;
  bool inverterZ_;
  bool iniciado_ = false;
  uint8_t identificacao_ = 0;
  uint32_t falhasLeitura_ = 0;
};

}  // namespace micromouse
