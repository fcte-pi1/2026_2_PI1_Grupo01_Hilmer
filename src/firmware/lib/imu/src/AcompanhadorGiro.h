#pragma once

/// @file AcompanhadorGiro.h
/// @brief Um ciclo da tarefa da IMU: lê o giroscópio, calibra o viés na largada e integra o ângulo
/// de giro, contando as falhas de leitura (#378).
///
/// Fica fora do `main.cpp` para ser testado no computador com o `ImuSimulado`. Na placa, a tarefa
/// da IMU só chama `ciclo()` em intervalo fixo e publica o resultado para as outras tarefas.

#include <stdint.h>

#include "ConfiguracaoImu.h"
#include "IImu.h"
#include "IntegradorGiro.h"

namespace micromouse {

/// Situação do giroscópio depois do último ciclo, para as outras tarefas lerem.
struct LeituraGiro {
  /// Última taxa lida, em décimos de °/s, **sem** o viés (positivo = giro para a esquerda). Zero
  /// enquanto não houver leitura.
  int16_t taxaDecimosDps = 0;
  /// Ângulo de giro acumulado, em mili-graus, desde a calibração ou o último `zerarAngulo()`.
  int32_t anguloMiligraus = 0;
  /// `true` com o viés calibrado e sem `FALHAS_SEGUIDAS_GIRO_INVALIDO` falhas seguidas. Só use o
  /// ângulo para controle quando for `true`.
  bool valida = false;
  /// Situação da calibração do viés.
  EstadoCalibracao calibracao = EstadoCalibracao::NaoCalibrado;
  /// Leituras que falharam desde o início (sensor sem resposta ou barramento ocupado).
  uint32_t falhasLeitura = 0;
  /// Calibrações recusadas porque o robô se mexeu; cada uma recomeça sozinha.
  uint32_t calibracoesRecusadas = 0;
};

/// Junta a IMU e o `IntegradorGiro` no ciclo da tarefa da IMU.
///
/// 1. Depois de `iniciarCalibracao()` (robô parado na largada), cada ciclo vira uma amostra da
///    calibração. Se a calibração for recusada (o robô se mexeu), ela recomeça sozinha.
/// 2. Calibrado, cada ciclo tira o viés e soma a taxa ao ângulo, com o `dtMs` medido pela tarefa.
/// 3. Uma leitura com falha não soma nada ao ângulo. Depois de `falhasParaInvalidar` falhas
///    seguidas, `valida` fica falso até a próxima leitura boa.
///
/// Uso típico, na tarefa da IMU:
/// @code
/// AcompanhadorGiro giro(imu);
/// giro.iniciarCalibracao();
/// for (;;) {
///   vTaskDelayUntil(&ultimo, pdMS_TO_TICKS(PERIODO_LEITURA_IMU_MS));
///   const LeituraGiro leitura = giro.ciclo(PERIODO_LEITURA_IMU_MS);
///   // publica `leitura` para a navegação
/// }
/// @endcode
class AcompanhadorGiro {
 public:
  /// @param imu Fonte da taxa de giro (o driver do MPU6050 ou o `ImuSimulado`).
  /// @param config Parâmetros da calibração e da zona morta.
  /// @param falhasParaInvalidar Falhas seguidas que invalidam o ângulo.
  explicit AcompanhadorGiro(IImu& imu, const ConfiguracaoIntegradorGiro& config = ConfiguracaoIntegradorGiro{},
                            uint8_t falhasParaInvalidar = FALHAS_SEGUIDAS_GIRO_INVALIDO);

  /// Começa (ou recomeça) a calibração do viés. Chame com o robô parado. O ângulo volta a zero.
  void iniciarCalibracao();

  /// Lê a IMU uma vez e calibra ou integra.
  /// @param dtMs Tempo desde o ciclo anterior, em ms.
  /// @return A situação depois deste ciclo (a mesma de `ultima()`).
  const LeituraGiro& ciclo(uint32_t dtMs);

  /// Situação depois do último ciclo.
  const LeituraGiro& ultima() const { return leitura_; }

  /// Volta o ângulo a zero (por exemplo, no começo de um giro). Não muda o viés.
  void zerarAngulo();

 private:
  IImu& imu_;
  IntegradorGiro integrador_;
  uint8_t falhasParaInvalidar_;
  uint8_t falhasSeguidas_ = 0;
  LeituraGiro leitura_;
};

}  // namespace micromouse
