#pragma once

/// @file MotoresDrv8833.h
/// @brief Driver dos dois motores de tração pelo DRV8833, com PWM (LEDC) do ESP32-C3.
///
/// Implementa `IMotores` (camada `hal`). O cálculo dos sinais fica em `SinaisDrv8833.h`
/// (camada `atuacao`), testado no computador; aqui só se escreve o PWM nos pinos.

#include <Arduino.h>

#include "IMotores.h"

namespace micromouse {

/// Frequência do PWM dos motores: 20 kHz, acima do audível e dentro do limite do DRV8833.
constexpr uint32_t FREQUENCIA_PWM_MOTORES_HZ = 20000;

/// Resolução do PWM dos motores: 10 bits (duty de 0 a 1023).
constexpr uint8_t RESOLUCAO_PWM_MOTORES_BITS = 10;

/// Driver dos motores M1 (roda esquerda) e M2 (roda direita) pelo DRV8833.
///
/// Cada motor usa duas entradas com PWM (IN1 e IN2). Até `iniciar()` ser chamado, os comandos
/// são ignorados. `iniciar()` deixa os motores freados.
///
/// O motor montado espelhado deve ser invertido no construtor. Confira na bancada: com
/// `definirPotencia({300, 300})`, as duas rodas precisam girar para a frente.
class MotoresDrv8833 : public IMotores {
 public:
  /// @param esquerdaIn1 Pino ligado ao AIN1 (motor M1, roda esquerda).
  /// @param esquerdaIn2 Pino ligado ao AIN2.
  /// @param direitaIn1 Pino ligado ao BIN1 (motor M2, roda direita).
  /// @param direitaIn2 Pino ligado ao BIN2.
  /// @param inverterEsquerda `true` se o motor esquerdo estiver montado espelhado.
  /// @param inverterDireita `true` se o motor direito estiver montado espelhado.
  MotoresDrv8833(uint8_t esquerdaIn1, uint8_t esquerdaIn2, uint8_t direitaIn1, uint8_t direitaIn2,
                 bool inverterEsquerda, bool inverterDireita);

  /// Configura o PWM dos quatro pinos e freia os motores. Chame no `setup()`.
  void iniciar();

  /// @copydoc IMotores::definirPotencia
  void definirPotencia(const PotenciaRodas& potencia) override;

  /// @copydoc IMotores::parar
  void parar() override;

 private:
  /// Escreve o duty no pino `indice` (0: esquerda IN1, 1: esquerda IN2, 2: direita IN1,
  /// 3: direita IN2).
  void escrever(uint8_t indice, uint32_t duty);

  uint8_t pinos_[4];
  bool inverterEsquerda_;
  bool inverterDireita_;
  bool iniciado_ = false;
};

}  // namespace micromouse
