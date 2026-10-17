#pragma once

/// @file EncoderEsp32.h
/// @brief Driver de leitura de encoder em quadratura para ESP32-C3 via interrupções de GPIO.
///
/// Implementa `IEncoder` (camada `hal`) conectando as interrupções nos pinos A e B
/// à máquina de estados do `DecodificadorQuadratura` (camada `encoders`).

#include <Arduino.h>

#include "DecodificadorQuadratura.h"
#include "IEncoder.h"

namespace micromouse {

/// Driver de encoder físico no ESP32-C3.
///
/// Configura interrupções em ambos os pinos (CHANGE) para decodificação em quadratura x4.
/// As rotinas de interrupção executam a partir da IRAM e utilizam seções críticas do FreeRTOS
/// para garantir a consistência das leituras de contagem.
class EncoderEsp32 : public IEncoder {
 public:
  /// @param pinoA Pino GPIO conectado ao canal A do encoder.
  /// @param pinoB Pino GPIO conectado ao canal B do encoder.
  /// @param inverterSentido Se true, inverte a contagem (usado para motor espelhado).
  /// @param modoPino Modo do pino GPIO (padrão: INPUT_PULLUP).
  EncoderEsp32(uint8_t pinoA, uint8_t pinoB, bool inverterSentido = false,
               uint8_t modoPino = INPUT_PULLUP);

  ~EncoderEsp32() override;

  /// Inicializa os pinos GPIO e anexa as interrupções de hardware.
  /// Deve ser chamado no setup().
  void iniciar();

  /// @copydoc IEncoder::lerContagem
  int32_t lerContagem() const override;

  /// @copydoc IEncoder::zerar
  void zerar() override;

  /// Retorna o número de transições inválidas detectadas (indicativo de ruído ou perda de pulsos).
  uint32_t transicoesInvalidas() const;

 private:
  static void IRAM_ATTR tratarInterrupcao(void* arg);
  void IRAM_ATTR processarInterrupcao();

  uint8_t pinoA_;
  uint8_t pinoB_;
  uint8_t modoPino_;
  bool inicializado_ = false;
  DecodificadorQuadratura decodificador_;
  mutable portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
};

}  // namespace micromouse
