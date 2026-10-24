#pragma once

/// @file ConversorGiro.h
/// @brief Conversão da leitura bruta do giroscópio do MPU6050 para décimos de °/s.
///
/// Fica fora do driver para ser testada no computador: o driver só lê os dois bytes do eixo Z
/// e chama `juntarBytesGiro`, `converterBrutoParaDecimosDps` e `ajustarSinalGiro`.

#include <stdint.h>

#include "ConfiguracaoImu.h"

namespace micromouse {

/// Sensibilidade do giroscópio multiplicada por 10, em LSB por °/s (131 → 1310), para a conta
/// ficar em inteiros.
constexpr uint16_t sensibilidadeGiroDecimaLsb(FaixaGiro faixa) {
  return faixa == FaixaGiro::Dps250    ? 1310
         : faixa == FaixaGiro::Dps500  ? 655
         : faixa == FaixaGiro::Dps1000 ? 328
                                       : 164;
}

/// Converte a leitura bruta (complemento de dois, 16 bits) em décimos de °/s, arredondando para
/// o mais próximo. Exemplo: na faixa de ±250 °/s, 131 LSB = 1 °/s = 10 décimos.
constexpr int16_t converterBrutoParaDecimosDps(int16_t bruto, FaixaGiro faixa) {
  // décimos = bruto × 10 / (LSB por °/s) = bruto × 100 / (sensibilidade × 10)
  const int32_t numerador = static_cast<int32_t>(bruto) * 100;
  const int32_t divisor = sensibilidadeGiroDecimaLsb(faixa);
  const int32_t metade = divisor / 2;
  return static_cast<int16_t>(numerador >= 0 ? (numerador + metade) / divisor : (numerador - metade) / divisor);
}

/// Valor do registrador GYRO_CONFIG do MPU6050 para a faixa: o campo FS_SEL fica nos bits 4 e 3
/// (0x00, 0x08, 0x10 e 0x18), e os bits de autoteste ficam em zero.
constexpr uint8_t valorGyroConfig(FaixaGiro faixa) { return static_cast<uint8_t>(static_cast<uint8_t>(faixa) << 3); }

/// Junta os dois bytes lidos do MPU6050 (o mais significativo vem primeiro) num inteiro de 16 bits
/// com sinal, em complemento de dois.
constexpr int16_t juntarBytesGiro(uint8_t alto, uint8_t baixo) {
  return static_cast<int16_t>(static_cast<uint16_t>((static_cast<uint16_t>(alto) << 8) | baixo));
}

/// Troca o sinal da taxa quando o sensor está montado de cabeça para baixo, sem estourar o
/// `int16_t` (o menor valor vira o maior).
constexpr int16_t ajustarSinalGiro(int16_t taxaDecimosDps, bool inverter) {
  return !inverter ? taxaDecimosDps : taxaDecimosDps == INT16_MIN ? INT16_MAX : static_cast<int16_t>(-taxaDecimosDps);
}

}  // namespace micromouse
