#pragma once

/// @file ISensorDistancia.h
/// @brief Interface de acesso aos sensores de distância (camada `hal`).
///
/// Na Rev 2.0 do hardware, são os 3 sensores VL53L0X (até a Rev 1, era o LiDAR 360°).
/// A lógica do firmware depende só desta interface, nunca do driver concreto.
/// Assim, a percepção e o mapeamento podem ser testados no computador com o
/// `SensorDistanciaSimulado` e, na placa, usar o driver real (em `drivers_esp32`).

#include "Tipos.h"

namespace micromouse {

/// Fonte de leituras de distância nas três direções relativas ao robô.
///
/// Implementações:
/// - `SensorDistanciaSimulado` (`lib/simulacao`): leituras configuradas pelo teste, sem hardware.
/// - `TofVl53l0x` (`lib/drivers_esp32`): os 3 VL53L0X da Rev 2.0, pelo I²C.
class ISensorDistancia {
 public:
  virtual ~ISensorDistancia() = default;

  /// Lê as distâncias à frente, à esquerda e à direita do robô.
  ///
  /// @param[out] saida Recebe as distâncias, em milímetros, quando a leitura é válida.
  /// @return `true` se houve leitura válida das três direções; `false` se o sensor
  ///         não respondeu ou a leitura é inválida. Quando retorna `false`, o conteúdo
  ///         de `saida` não deve ser usado. Medida fora da faixa do sensor não é inválida:
  ///         sai limitada à faixa (no `TofVl53l0x`, "sem alvo" vira o alcance máximo, ou
  ///         seja, lado livre, e "perto demais" vira o mínimo, ou seja, parede).
  virtual bool lerDistanciasLaterais(DistanciasLaterais& saida) = 0;

  /// Idade, em ms, da amostra mais antiga que compõe a última leitura, medida agora.
  /// Serve para a navegação não reagir duas vezes ao mesmo dado: se a idade não diminuiu
  /// desde a chamada anterior, a leitura é a mesma.
  ///
  /// A implementação padrão devolve 0 (leitura sempre nova), o que vale para fontes sem
  /// relógio, como o `SensorDistanciaSimulado`.
  virtual uint32_t idadeUltimaLeituraMs() const { return 0; }
};

}  // namespace micromouse
