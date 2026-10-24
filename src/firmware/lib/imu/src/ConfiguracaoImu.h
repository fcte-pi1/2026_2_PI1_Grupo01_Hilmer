#pragma once

/// @file ConfiguracaoImu.h
/// @brief Parâmetros da IMU MPU6050 e da integração do ângulo de giro (#378).
///
/// Valores **provisórios**: a faixa e os limites da calibração precisam ser confirmados na
/// bancada, com o sensor montado no chassi. Para mudar um valor, altere só este arquivo.

#include <stdint.h>

namespace micromouse {

/// Endereço I²C do MPU6050 com o pino AD0 ligado ao GND (esquemático Rev 2.0).
constexpr uint8_t ENDERECO_I2C_MPU6050 = 0x68;

/// Faixa de medição do giroscópio do MPU6050 (registrador GYRO_CONFIG).
enum class FaixaGiro : uint8_t {
  Dps250,   ///< ±250 °/s: 131 LSB por °/s.
  Dps500,   ///< ±500 °/s: 65,5 LSB por °/s.
  Dps1000,  ///< ±1000 °/s: 32,8 LSB por °/s.
  Dps2000   ///< ±2000 °/s: 16,4 LSB por °/s.
};

/// Faixa usada pelo robô. Um giro de 90° em cerca de 0,3 s passa de 300 °/s, acima da faixa de
/// ±250 °/s; ±500 °/s cobre o giro com folga e mantém boa resolução (0,015 °/s por LSB).
constexpr FaixaGiro FAIXA_GIRO = FaixaGiro::Dps500;

/// Intervalo entre leituras do giroscópio, em ms: o mesmo ciclo da tarefa de navegação.
constexpr uint32_t PERIODO_LEITURA_IMU_MS = 10;

/// Amostras da calibração do viés, com o robô parado: 200 × 10 ms = 2 s.
constexpr uint16_t AMOSTRAS_CALIBRACAO_GIRO = 200;

/// Maior diferença entre a maior e a menor amostra da calibração, em décimos de °/s. Acima dela,
/// o robô provavelmente se mexeu e a calibração é recusada.
constexpr uint16_t VARIACAO_MAXIMA_CALIBRACAO_DECIMOS_DPS = 50;

/// Zona morta, em décimos de °/s: depois de tirar o viés, taxas de até 0,3 °/s contam como zero,
/// para o ruído do sensor parado não virar deriva do ângulo.
constexpr uint16_t ZONA_MORTA_GIRO_DECIMOS_DPS = 3;

/// `true` se o MPU6050 estiver montado de cabeça para baixo no chassi: o driver inverte o sinal da
/// taxa para manter a convenção de `IImu` (positivo = giro para a esquerda). Confirme na bancada:
/// girando o robô para a esquerda com a mão, a taxa lida tem que ser positiva.
constexpr bool INVERTER_GIRO_Z = false;

/// Falhas de leitura seguidas a partir das quais o ângulo de giro deixa de ser confiável: 5 × 10 ms
/// = 50 ms sem leitura, um giro de 300 °/s já andaria 15° sem ser contado.
constexpr uint8_t FALHAS_SEGUIDAS_GIRO_INVALIDO = 5;

/// Parâmetros do `IntegradorGiro`, com os valores acima como padrão.
struct ConfiguracaoIntegradorGiro {
  uint16_t amostrasCalibracao = AMOSTRAS_CALIBRACAO_GIRO;
  uint16_t variacaoMaximaCalibracaoDecimosDps = VARIACAO_MAXIMA_CALIBRACAO_DECIMOS_DPS;
  uint16_t zonaMortaDecimosDps = ZONA_MORTA_GIRO_DECIMOS_DPS;
};

}  // namespace micromouse
