#pragma once

/// @file IEnvioTelemetria.h
/// @brief Interface de envio da telemetria ao servidor (camada `hal`).
///
/// A lógica de buffer e reenvio (`ControladorEnvio`, em `lib/comunicacao`) depende só desta
/// interface. Assim, ela é testada no computador com o `EnvioSimulado` (um servidor falso que imita
/// o backend) e, na placa, usa o cliente HTTP do Supabase (#236, em `drivers_esp32`).

#include <stdint.h>

#include "AmostraTelemetria.h"

namespace micromouse {

/// Resposta de uma chamada ao servidor.
struct ResultadoEnvio {
  /// Código HTTP. Zero ou negativo: sem resposta (Wi-Fi fora, tempo esgotado, falha de TLS).
  int codigoHttp = 0;
  /// Só valem com resposta `2xx` do envio de um lote: quantas amostras foram gravadas e quantas já
  /// existiam no servidor. Somadas, devem ser iguais ao tamanho do lote enviado.
  uint16_t gravadas = 0;
  uint16_t existentes = 0;
};

/// Chamadas ao servidor. Elas **bloqueiam** até a resposta ou o tempo limite da rede, então só podem
/// ser feitas pela tarefa de rede, nunca pela navegação.
class IEnvioTelemetria {
 public:
  virtual ~IEnvioTelemetria() = default;

  /// `POST /functions/v1/telemetria`: grava um lote de amostras da corrida, em ordem.
  /// @param corridaId Corrida criada no início (`POST /functions/v1/corridas`).
  /// @param amostras Amostras do lote, da mais antiga para a mais nova.
  /// @param quantidade Quantas amostras (no máximo `TAMANHO_MAXIMO_LOTE`).
  virtual ResultadoEnvio enviarLote(uint32_t corridaId, const AmostraTelemetria* amostras,
                                    uint16_t quantidade) = 0;

  /// `POST /functions/v1/corridas/{id}/encerrar`: encerra a corrida. É idempotente no servidor.
  virtual ResultadoEnvio encerrarCorrida(uint32_t corridaId) = 0;
};

}  // namespace micromouse
