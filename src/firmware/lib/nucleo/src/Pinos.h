#pragma once

/// @file Pinos.h
/// @brief Pinagem do ESP32 DevKit V1, conforme a tabela de fiação da bancada integrada.
///
/// A placa do robô é o ESP32 DevKit V1 (ESP32 clássico): a ESP32-C3 Super Mini do esquemático
/// Rev 2.0 saiu do projeto. Os números são GPIOs lógicos, não posições de terminal da placa:
/// confira o pinout físico do DevKit na montagem.
///
/// Qualquer mudança na fiação deve ser refletida aqui, e só aqui: os drivers recebem os pinos por
/// parâmetro, então nenhum outro arquivo guarda número de GPIO. O log (`Serial`) sai pela UART0
/// (GPIO1/GPIO3), pelo conversor USB da placa; esses dois pinos ficam livres de periféricos.

#include <stddef.h>
#include <stdint.h>

namespace micromouse {
namespace pinos {

/// @name Barramento I²C (3 VL53L0X e o MPU6050 compartilham SDA e SCL)
/// @{
constexpr int I2C_SDA = 21;  ///< GPIO21 ↔ SDA.
constexpr int I2C_SCL = 22;  ///< GPIO22 → SCL.
/// @}

/// @name XSHUT dos sensores VL53L0X (um por sensor, para atribuir endereços distintos no boot)
/// @{
constexpr int XSHUT_ESQ = 16;      ///< GPIO16 → XSHUT do sensor esquerdo (J3).
constexpr int XSHUT_FRONTAL = 17;  ///< GPIO17 → XSHUT do sensor frontal (J4).
constexpr int XSHUT_DIR = 4;       ///< GPIO4 → XSHUT do sensor direito (J5).
/// @}

/// @name Endereços I²C dos VL53L0X, atribuídos no boot (#344)
/// Todos ligam no endereço de fábrica (0x29). O driver solta um XSHUT por vez e muda o endereço
/// do sensor recém-ligado, então os três precisam ser distintos entre si, do 0x29 e do MPU6050.
/// @{
constexpr uint8_t ENDERECO_FABRICA_TOF = 0x29;  ///< Endereço do VL53L0X ao sair do reset.
constexpr uint8_t ENDERECO_MPU6050 = 0x68;      ///< MPU6050 com o pino AD0 em nível baixo.
constexpr uint8_t ENDERECO_TOF_ESQ = 0x30;      ///< Sensor esquerdo (J3).
constexpr uint8_t ENDERECO_TOF_FRONTAL = 0x31;  ///< Sensor frontal (J4).
constexpr uint8_t ENDERECO_TOF_DIR = 0x32;      ///< Sensor direito (J5).
/// @}

/// @name Driver de motores DRV8833 (ponte H dupla, sinais PWM)
/// IN1/IN2 acionam o motor esquerdo (OUT1/OUT2) e IN3/IN4 o direito (OUT3/OUT4). O GPIO14 emite um
/// pulso PWM curto no reset do ESP32: o motor direito pode dar um tranco no boot.
/// @{
constexpr int MOTOR_ESQ_IN1 = 25;  ///< GPIO25 → IN1 (motor esquerdo).
constexpr int MOTOR_ESQ_IN2 = 26;  ///< GPIO26 → IN2 (motor esquerdo).
constexpr int MOTOR_DIR_IN3 = 27;  ///< GPIO27 → IN3 (motor direito).
constexpr int MOTOR_DIR_IN4 = 14;  ///< GPIO14 → IN4 (motor direito).
/// @}

/// @name Encoders de quadratura (canais A e B de cada roda)
/// @{
constexpr int ENCODER_ESQ_A = 32;  ///< GPIO32 ← canal A (C1) da roda esquerda.
constexpr int ENCODER_ESQ_B = 33;  ///< GPIO33 ← canal B (C2) da roda esquerda.
constexpr int ENCODER_DIR_A = 18;  ///< GPIO18 ← canal A (C1) da roda direita.
constexpr int ENCODER_DIR_B = 19;  ///< GPIO19 ← canal B (C2) da roda direita.
/// @}

/// @name Botão de largada e leitura da bateria
/// @{
constexpr int BOTAO_START = 23;  ///< GPIO23 ← botão BTN_START para o GND (`INPUT_PULLUP`).
constexpr int BATERIA_ADC = 34;  ///< GPIO34 (ADC1, só entrada) ← divisor R1/R2 da bateria.
/// @}

/// Todos os GPIOs em uso, para conferir repetição e faixa. Mantenha em sincronia com as constantes
/// acima: o teste `test_pinos` e o `static_assert` abaixo reprovam a compilação se houver repetição.
constexpr int TODOS_OS_PINOS[] = {
    I2C_SDA,       I2C_SCL,       XSHUT_ESQ,     XSHUT_FRONTAL, XSHUT_DIR,
    MOTOR_ESQ_IN1, MOTOR_ESQ_IN2, MOTOR_DIR_IN3, MOTOR_DIR_IN4, ENCODER_ESQ_A,
    ENCODER_ESQ_B, ENCODER_DIR_A, ENCODER_DIR_B,
    BOTAO_START,   BATERIA_ADC,
};
constexpr size_t QUANTIDADE_DE_PINOS = sizeof(TODOS_OS_PINOS) / sizeof(TODOS_OS_PINOS[0]);

/// `true` se nenhum valor aparece duas vezes em `pinos[0..quantidade)`.
constexpr bool semRepeticao(const int* pinos, size_t quantidade) {
  for (size_t i = 0; i < quantidade; i++) {
    for (size_t j = i + 1; j < quantidade; j++) {
      if (pinos[i] == pinos[j]) {
        return false;
      }
    }
  }
  return true;
}

/// `true` se nenhum GPIO aparece duas vezes em `TODOS_OS_PINOS`.
constexpr bool pinosSemRepeticao() {
  return semRepeticao(TODOS_OS_PINOS, QUANTIDADE_DE_PINOS);
}

/// `true` se todo GPIO existe e está livre no ESP32 DevKit V1: GPIO0 a GPIO5, GPIO12 a GPIO19,
/// GPIO21 a GPIO23, GPIO25 a GPIO27 e GPIO32 a GPIO39. Ficam de fora o GPIO6 a GPIO11 (flash) e o
/// GPIO20, que o DevKit não expõe. Os GPIO34 a GPIO39 só têm entrada e nenhum pull-up interno.
constexpr bool pinosExistemNoEsp32DevKit() {
  for (size_t i = 0; i < QUANTIDADE_DE_PINOS; i++) {
    const int pino = TODOS_OS_PINOS[i];
    const bool valido = (pino >= 0 && pino <= 5) || (pino >= 12 && pino <= 19) || (pino >= 21 && pino <= 23) ||
                        (pino >= 25 && pino <= 27) || (pino >= 32 && pino <= 39);
    if (!valido) {
      return false;
    }
  }
  return true;
}

static_assert(pinosSemRepeticao(), "Pinos.h: ha GPIO repetido na pinagem");
static_assert(pinosExistemNoEsp32DevKit(), "Pinos.h: ha GPIO que nao existe ou esta ocupado no ESP32 DevKit");

/// `true` se os endereços I²C dos 3 VL53L0X são distintos entre si, do endereço de fábrica e do
/// MPU6050, e cabem em 7 bits.
constexpr bool enderecosTofDistintos() {
  const int enderecos[] = {ENDERECO_TOF_ESQ, ENDERECO_TOF_FRONTAL, ENDERECO_TOF_DIR,
                           ENDERECO_FABRICA_TOF, ENDERECO_MPU6050};
  for (int endereco : enderecos) {
    if (endereco > 0x7F) {
      return false;
    }
  }
  return semRepeticao(enderecos, sizeof(enderecos) / sizeof(enderecos[0]));
}

static_assert(enderecosTofDistintos(), "Pinos.h: enderecos I2C dos VL53L0X em conflito");

}  // namespace pinos
}  // namespace micromouse
