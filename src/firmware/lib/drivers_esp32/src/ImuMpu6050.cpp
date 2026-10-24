#include "ImuMpu6050.h"

#include "ConversorGiro.h"

namespace micromouse {

namespace {

/// Registradores do MPU6050 usados pelo driver (mapa de registradores RM-MPU-6000A, rev. 4.2).
/// Ficam só aqui, no ambiente `esp32c3`.
constexpr uint8_t REG_SMPLRT_DIV = 0x19;    ///< Divisor da taxa de amostragem.
constexpr uint8_t REG_CONFIG = 0x1A;        ///< Filtro passa-baixa (DLPF_CFG).
constexpr uint8_t REG_GYRO_CONFIG = 0x1B;   ///< Faixa do giroscópio (FS_SEL).
constexpr uint8_t REG_GYRO_ZOUT_H = 0x47;   ///< Eixo Z do giroscópio, byte alto (o baixo vem em 0x48).
constexpr uint8_t REG_PWR_MGMT_1 = 0x6B;    ///< Reinício, economia de energia e relógio.
constexpr uint8_t REG_WHO_AM_I = 0x75;      ///< Identificação do chip.

constexpr uint8_t PWR_MGMT_1_REINICIAR = 0x80;     ///< DEVICE_RESET.
constexpr uint8_t PWR_MGMT_1_RELOGIO_GIRO_X = 0x01; ///< Sai do sono e usa o PLL do giroscópio X.
constexpr uint8_t CONFIG_DLPF_42HZ = 0x03;         ///< Giroscópio a 42 Hz de banda, 1 kHz interno.
/// 1 kHz / (1 + 9) = 100 amostras por segundo, uma a cada `PERIODO_LEITURA_IMU_MS` (10 ms).
constexpr uint8_t SMPLRT_DIV_100HZ = 9;

constexpr uint32_t ESPERA_REINICIO_MS = 100;  ///< O datasheet pede até 100 ms depois do reinício.

}  // namespace

ImuMpu6050::ImuMpu6050(BarramentoI2c& barramento, uint8_t endereco, FaixaGiro faixa, bool inverterZ)
    : barramento_(barramento), endereco_(endereco), faixa_(faixa), inverterZ_(inverterZ) {}

bool ImuMpu6050::iniciar() {
  iniciado_ = false;
  falhasLeitura_ = 0;
  identificacao_ = 0;

  if (!escreverRegistrador(REG_PWR_MGMT_1, PWR_MGMT_1_REINICIAR)) {
    return false;
  }
  delay(ESPERA_REINICIO_MS);  // só no setup() ou numa recuperação: nunca no ciclo de leitura

  uint8_t quemSou = 0;
  if (!lerRegistradores(REG_WHO_AM_I, &quemSou, 1)) {
    return false;
  }
  identificacao_ = quemSou;

  const bool configurado = escreverRegistrador(REG_PWR_MGMT_1, PWR_MGMT_1_RELOGIO_GIRO_X) &&
                           escreverRegistrador(REG_CONFIG, CONFIG_DLPF_42HZ) &&
                           escreverRegistrador(REG_SMPLRT_DIV, SMPLRT_DIV_100HZ) &&
                           escreverRegistrador(REG_GYRO_CONFIG, valorGyroConfig(faixa_));
  if (!configurado) {
    return false;
  }

  // Confere que a faixa ficou gravada: um erro aqui mudaria a escala de todas as leituras.
  uint8_t gyroConfig = 0;
  if (!lerRegistradores(REG_GYRO_CONFIG, &gyroConfig, 1) || (gyroConfig & 0x18) != valorGyroConfig(faixa_)) {
    return false;
  }
  iniciado_ = true;
  return true;
}

bool ImuMpu6050::lerTaxaGiroZ(int16_t& taxaDecimosDps) {
  uint8_t bytes[2] = {0, 0};
  if (!iniciado_ || !lerRegistradores(REG_GYRO_ZOUT_H, bytes, 2)) {
    ++falhasLeitura_;
    return false;
  }
  const int16_t bruto = juntarBytesGiro(bytes[0], bytes[1]);
  taxaDecimosDps = ajustarSinalGiro(converterBrutoParaDecimosDps(bruto, faixa_), inverterZ_);
  return true;
}

bool ImuMpu6050::escreverRegistrador(uint8_t registrador, uint8_t valor) {
  BarramentoI2c::Uso uso(barramento_);
  if (!uso.conseguiu()) {
    return false;
  }
  TwoWire& wire = barramento_.wire();
  wire.beginTransmission(endereco_);
  wire.write(registrador);
  wire.write(valor);
  return wire.endTransmission(true) == 0;
}

bool ImuMpu6050::lerRegistradores(uint8_t registrador, uint8_t* destino, uint8_t quantidade) {
  BarramentoI2c::Uso uso(barramento_);
  if (!uso.conseguiu()) {
    return false;
  }
  TwoWire& wire = barramento_.wire();
  wire.beginTransmission(endereco_);
  wire.write(registrador);
  // Sem STOP entre a escrita do registrador e a leitura (início repetido), como pede o datasheet.
  if (wire.endTransmission(false) != 0) {
    return false;
  }
  const uint8_t recebidos = static_cast<uint8_t>(wire.requestFrom(endereco_, quantidade));
  if (recebidos != quantidade) {
    // Esvazia o que tiver chegado, para a próxima leitura não pegar bytes velhos.
    while (wire.available() > 0) {
      wire.read();
    }
    return false;
  }
  for (uint8_t i = 0; i < quantidade; ++i) {
    destino[i] = static_cast<uint8_t>(wire.read());
  }
  return true;
}

}  // namespace micromouse
