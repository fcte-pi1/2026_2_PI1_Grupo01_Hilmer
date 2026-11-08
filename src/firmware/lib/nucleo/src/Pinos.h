#pragma once

/// @file Pinos.h
/// @brief Pinagem do ESP32-C3 Super Mini, conforme o esquemático elétrico Rev 2.0 (21/10/2026).
///
/// Fonte: esquemático e lista de componentes Rev 2.0 da frente de eletrônica
/// (`hw/Micromouse.pdf`, `docs/04-projeto-conceitual/hardware.md`). Os números são GPIOs
/// lógicos, não posições de terminal da placa: confira o pinout físico da Super Mini na montagem.
///
/// Qualquer mudança no esquemático elétrico deve ser refletida aqui, e só aqui: os drivers recebem
/// os pinos por parâmetro, então nenhum outro arquivo guarda número de GPIO.
///
/// **Placa ESP32 DevKit V1 (bancada).** Com `-DPLACA_ESP32_DEVKIT` (ambientes `*_devkit` do
/// `platformio.ini`), vale a pinagem do ESP32 DevKit V1, conforme a tabela de fiação da bancada
/// integrada. Sem a opção, vale a da Super Mini (Rev 2.0). Quando a troca de placa for definitiva,
/// a pinagem antiga sai e fica só uma.
///
/// Mudanças em relação à versão anterior: o LiDAR por UART saiu (entraram 3 sensores VL53L0X
/// por I²C), entrou o MPU6050 no mesmo barramento e todos os outros pinos mudaram de lugar.

#include <stddef.h>
#include <stdint.h>

namespace micromouse {
namespace pinos {

#if defined(PLACA_ESP32_DEVKIT)

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

#else

/// @name Barramento I²C (3 VL53L0X e o MPU6050 compartilham SDA e SCL)
/// GPIO8 e GPIO9 são pinos de strapping: valide os níveis no reset com os módulos ligados.
/// @{
constexpr int I2C_SDA = 9;  ///< GPIO9 ↔ SDA. O botão BOOT da Super Mini também usa este pino.
constexpr int I2C_SCL = 8;  ///< GPIO8 → SCL.
/// @}

/// @name XSHUT dos sensores VL53L0X (um por sensor, para atribuir endereços distintos no boot)
/// @{
constexpr int XSHUT_ESQ = 0;      ///< GPIO0 → XSHUT do sensor esquerdo (J3).
constexpr int XSHUT_FRONTAL = 1;  ///< GPIO1 → XSHUT do sensor frontal (J4).
constexpr int XSHUT_DIR = 2;      ///< GPIO2 → XSHUT do sensor direito (J5). GPIO2 é pino de strapping.
/// @}

#endif

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

#if defined(PLACA_ESP32_DEVKIT)

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

#else

/// @name Driver de motores DRV8833 (ponte H dupla, sinais PWM)
/// Os nomes seguem os do esquemático: IN1/IN2 acionam o motor esquerdo (OUT1/OUT2, J1) e
/// IN3/IN4 o direito (OUT3/OUT4, J2).
/// GPIO20 e GPIO21 são a UART0 do ESP32-C3: o log não pode usá-la (ver `platformio.ini`), e a
/// ROM ainda escreve no GPIO21 durante o reset, então o driver precisa ficar desabilitado no boot.
/// @{
constexpr int MOTOR_ESQ_IN1 = 10;  ///< GPIO10 → IN1 (motor esquerdo).
constexpr int MOTOR_ESQ_IN2 = 3;   ///< GPIO3 → IN2 (motor esquerdo).
constexpr int MOTOR_DIR_IN3 = 20;  ///< GPIO20 → IN3 (motor direito).
constexpr int MOTOR_DIR_IN4 = 21;  ///< GPIO21 → IN4 (motor direito).
/// @}

/// @name Encoders de quadratura (canais A e B de cada roda)
/// Reservados na Rev 2.0: os encoders ainda não estão instalados, mas os pinos já estão definidos
/// para o `EncoderEsp32` entrar sem mexer na pinagem.
/// @{
constexpr int ENCODER_ESQ_A = 4;  ///< GPIO4 ← canal A da roda esquerda.
constexpr int ENCODER_ESQ_B = 5;  ///< GPIO5 ← canal B da roda esquerda.
constexpr int ENCODER_DIR_A = 6;  ///< GPIO6 ← canal A da roda direita.
constexpr int ENCODER_DIR_B = 7;  ///< GPIO7 ← canal B da roda direita.
/// @}

/// @name USB nativa (log serial e gravação)
/// Reservados: nenhum outro periférico pode usar esses pinos. O `Serial` sai por aqui (USB CDC).
/// @{
constexpr int USB_DM = 18;  ///< GPIO18 = D−.
constexpr int USB_DP = 19;  ///< GPIO19 = D+.
/// @}

#endif  // PLACA_ESP32_DEVKIT

/// Todos os GPIOs em uso, para conferir repetição e faixa. Mantenha em sincronia com as constantes
/// acima: o teste `test_pinos` e o `static_assert` abaixo reprovam a compilação se houver repetição.
constexpr int TODOS_OS_PINOS[] = {
    I2C_SDA,       I2C_SCL,       XSHUT_ESQ,     XSHUT_FRONTAL, XSHUT_DIR,
    MOTOR_ESQ_IN1, MOTOR_ESQ_IN2, MOTOR_DIR_IN3, MOTOR_DIR_IN4, ENCODER_ESQ_A,
    ENCODER_ESQ_B, ENCODER_DIR_A, ENCODER_DIR_B,
#if defined(PLACA_ESP32_DEVKIT)
    BOTAO_START,   BATERIA_ADC,
#else
    USB_DM,        USB_DP,
#endif
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

#if defined(PLACA_ESP32_DEVKIT)

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

#else

/// `true` se todo GPIO existe no ESP32-C3 (GPIO0 a GPIO10 e GPIO18 a GPIO21).
constexpr bool pinosExistemNoEsp32C3() {
  for (size_t i = 0; i < QUANTIDADE_DE_PINOS; i++) {
    const int pino = TODOS_OS_PINOS[i];
    const bool valido = (pino >= 0 && pino <= 10) || (pino >= 18 && pino <= 21);
    if (!valido) {
      return false;
    }
  }
  return true;
}

static_assert(pinosSemRepeticao(), "Pinos.h: ha GPIO repetido na pinagem");
static_assert(pinosExistemNoEsp32C3(), "Pinos.h: ha GPIO que nao existe no ESP32-C3");

#endif

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
