#pragma once

/// @file MotoresDrv8833.h
/// @brief Driver dos dois motores de tração pelo DRV8833, com PWM (LEDC) do ESP32-C3.
///
/// Implementa `IMotores` (camada `hal`). O cálculo dos sinais fica em `SinaisDrv8833.h`
/// (camada `atuacao`), testado no computador; aqui só se escreve o PWM nos pinos.

#include <Arduino.h>

#include "ConfiguracaoMotores.h"
#include "IMotores.h"

namespace micromouse {

/// Driver dos motores M1 (roda esquerda) e M2 (roda direita) pelo DRV8833.
///
/// Cada motor usa duas entradas com PWM (IN1 e IN2). Até `iniciar()` ser chamado, os comandos
/// são ignorados. `iniciar()` deixa os motores freados: primeiro põe os quatro pinos em nível alto
/// como saídas comuns (freio) e só depois liga o PWM, já com duty total (freio também), para a roda
/// não dar tranco nem girar solta no boot.
///
/// A potência máxima (`POTENCIA_MAXIMA`) vira o duty `DUTY_MAXIMO_MOTORES` (`ConfiguracaoMotores.h`),
/// e não 100%: o limite protege o N20 enquanto a tensão do VM não é conhecida. O freio sempre usa
/// o duty total.
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

  /// Freia os motores e configura o PWM dos quatro pinos. Chame na **primeira linha** do `setup()`:
  /// até lá, GPIO20/21 (UART0) ficam com o que a ROM deixou (ver README, "Tranco no boot").
  void iniciar();

  /// @copydoc IMotores::definirPotencia
  void definirPotencia(const PotenciaRodas& potencia) override;

  /// @copydoc IMotores::parar
  void parar() override;

  /// Troca o limite de duty em tempo de execução (milésimos do total; acima de 1000 vale 1000).
  /// Só para a bancada: o robô usa `LIMITE_DUTY_MOTORES_MILESIMOS` (`ConfiguracaoMotores.h`), que é
  /// o valor inicial. Vale a partir do próximo `definirPotencia()`.
  void definirLimiteDuty(uint16_t milesimos) {
    dutyMaximo_ = dutyMaximoMotores(RESOLUCAO_PWM_MOTORES_BITS, milesimos);
  }

 private:
  /// Escreve o duty no pino `indice` (0: esquerda IN1, 1: esquerda IN2, 2: direita IN1,
  /// 3: direita IN2).
  void escrever(uint8_t indice, uint32_t duty);

  uint8_t pinos_[4];
  bool inverterEsquerda_;
  bool inverterDireita_;
  bool iniciado_ = false;
  uint32_t dutyMaximo_ = DUTY_MAXIMO_MOTORES;
};

}  // namespace micromouse
