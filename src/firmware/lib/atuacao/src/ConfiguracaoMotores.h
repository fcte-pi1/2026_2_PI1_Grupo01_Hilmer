#pragma once

/// @file ConfiguracaoMotores.h
/// @brief PWM dos motores N20 pelo DRV8833: frequência, resolução e duty máximo (#341).
///
/// ATENÇÃO: os valores abaixo são PROVISÓRIOS. A eletrônica ainda não informou a tensão do VM do
/// DRV8833 (bateria 2S ou 5 V) nem a tensão nominal, a redução e a corrente de travamento do N20.
/// Para mudar um valor, altere só este arquivo; o teste `test_motores` confere as contas.

#include <stdint.h>

namespace micromouse {

/// Frequência do PWM dos motores: 20 kHz, acima do audível e dentro do limite do DRV8833 (até
/// 250 kHz). Provisório: confirmar na bancada que o N20 não aquece nem perde torque nessa frequência.
constexpr uint32_t FREQUENCIA_PWM_MOTORES_HZ = 20000;

/// Resolução do PWM dos motores: 10 bits (duty de 0 a 1023).
constexpr uint8_t RESOLUCAO_PWM_MOTORES_BITS = 10;

/// Maior duty aplicado aos motores em movimento, em milésimos do total (1000 = 100%). A potência
/// máxima pedida pelo controle (`POTENCIA_MAXIMA`) vira este duty, e as menores são proporcionais.
///
/// Provisório: 70%, para um N20 de 6 V alimentado pela bateria 2S carregada (6 V / 8,4 V ≈ 71%).
/// Se o VM for 5 V, ou se o N20 for de 12 V, este valor pode subir até 1000.
constexpr uint16_t LIMITE_DUTY_MOTORES_MILESIMOS = 700;

/// Duty correspondente a 100% numa resolução de `bits` bits.
constexpr uint32_t dutyTotalPwm(uint8_t bits) { return (1UL << bits) - 1UL; }

/// Duty máximo dos motores em movimento: `limiteMilesimos` milésimos do duty total, arredondado
/// para baixo (nunca passa do limite). Acima de 1000, vale 1000.
constexpr uint32_t dutyMaximoMotores(uint8_t bits, uint16_t limiteMilesimos) {
  return dutyTotalPwm(bits) * (limiteMilesimos > 1000 ? 1000U : limiteMilesimos) / 1000U;
}

/// Duty total (100%), usado no freio: as duas entradas da ponte em nível alto.
constexpr uint32_t DUTY_TOTAL_MOTORES = dutyTotalPwm(RESOLUCAO_PWM_MOTORES_BITS);

/// Duty máximo em movimento, já com o limite aplicado.
constexpr uint32_t DUTY_MAXIMO_MOTORES = dutyMaximoMotores(RESOLUCAO_PWM_MOTORES_BITS, LIMITE_DUTY_MOTORES_MILESIMOS);

static_assert(LIMITE_DUTY_MOTORES_MILESIMOS <= 1000, "o duty máximo não pode passar de 100%");
// O LEDC do ESP32 precisa de frequência × 2^bits até 80 MHz (o relógio APB).
static_assert(static_cast<uint64_t>(FREQUENCIA_PWM_MOTORES_HZ) << RESOLUCAO_PWM_MOTORES_BITS <= 80000000ULL,
              "frequência e resolução do PWM fora do alcance do LEDC");

}  // namespace micromouse
