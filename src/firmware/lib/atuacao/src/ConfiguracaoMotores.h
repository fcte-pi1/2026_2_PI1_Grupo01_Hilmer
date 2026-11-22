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
/// 80%: o ponto testado na bancada com o robô montado (potência 1000 com duty de 80%), em que as rodas
/// andam. Provisório até a eletrônica informar o VM e a corrente de travamento do N20.
constexpr uint16_t LIMITE_DUTY_MOTORES_MILESIMOS = 800;

/// Zona morta do motor N20, em milésimos do duty total: o menor duty que ainda vence o atrito da
/// roda. Qualquer comando de potência acima de zero parte desse duty e sobe até o limite acima, em vez
/// de começar em zero. Sem isso, as potências baixas do controle (cruzeiro 400, giro, realinhamento
/// 150) viram um duty de 10 a 28% que não gira a roda, e o robô fica parado achando que anda.
///
/// Medido na rampa da bancada, com a bateria ligada e as rodas fora do chão: a roda esquerda parte
/// em ~52% de duty e a direita em ~56%; a 60% as duas giram (650 a 730 pulsos por passo de 300 ms) e a
/// 80% chegam a ~1450. Por isso a zona morta é 60%: toda potência acima de zero entrega de 60% a 80%.
/// No chão, com a carga do robô, a partida pode pedir um pouco mais: se uma roda não arrancar, suba
/// este valor de 50 em 50; se o robô disparar, desça. Remeça com a rampa se o VM mudar.
constexpr uint16_t ZONA_MORTA_DUTY_MILESIMOS = 600;

/// Duty correspondente a 100% numa resolução de `bits` bits.
constexpr uint32_t dutyTotalPwm(uint8_t bits) { return (1UL << bits) - 1UL; }

/// Duty máximo dos motores em movimento: `limiteMilesimos` milésimos do duty total, arredondado
/// para baixo (nunca passa do limite). Acima de 1000, vale 1000.
constexpr uint32_t dutyMaximoMotores(uint8_t bits, uint16_t limiteMilesimos) {
  return dutyTotalPwm(bits) * (limiteMilesimos > 1000 ? 1000U : limiteMilesimos) / 1000U;
}

/// Duty mínimo de um motor em movimento (a zona morta), já na resolução do PWM.
constexpr uint32_t DUTY_MINIMO_MOTORES = dutyMaximoMotores(RESOLUCAO_PWM_MOTORES_BITS, ZONA_MORTA_DUTY_MILESIMOS);

/// Duty total (100%), usado no freio: as duas entradas da ponte em nível alto.
constexpr uint32_t DUTY_TOTAL_MOTORES = dutyTotalPwm(RESOLUCAO_PWM_MOTORES_BITS);

/// Duty máximo em movimento, já com o limite aplicado.
constexpr uint32_t DUTY_MAXIMO_MOTORES = dutyMaximoMotores(RESOLUCAO_PWM_MOTORES_BITS, LIMITE_DUTY_MOTORES_MILESIMOS);

static_assert(LIMITE_DUTY_MOTORES_MILESIMOS <= 1000, "o duty máximo não pode passar de 100%");
static_assert(ZONA_MORTA_DUTY_MILESIMOS < LIMITE_DUTY_MOTORES_MILESIMOS, "a zona morta tem de ficar abaixo do limite de duty");
// O LEDC do ESP32 precisa de frequência × 2^bits até 80 MHz (o relógio APB).
static_assert(static_cast<uint64_t>(FREQUENCIA_PWM_MOTORES_HZ) << RESOLUCAO_PWM_MOTORES_BITS <= 80000000ULL,
              "frequência e resolução do PWM fora do alcance do LEDC");

}  // namespace micromouse
