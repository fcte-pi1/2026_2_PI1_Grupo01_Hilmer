#pragma once

/// @file IRedeWifi.h
/// @brief Interface da conexão Wi-Fi (camada `hal`).
///
/// A reconexão com espera crescente (`ReconexaoWifi`, em `lib/comunicacao`) depende só desta
/// interface. Assim, ela é testada no computador com uma rede falsa e, na placa, usa o `WiFi` do
/// ESP32 (`RedeWifiEsp32`, em `drivers_esp32`).

#include <stdint.h>

namespace micromouse {

/// Conexão Wi-Fi em modo estação. Nenhuma chamada espera a conexão terminar.
class IRedeWifi {
 public:
  virtual ~IRedeWifi() = default;

  /// `true` com a conexão feita e um endereço IP recebido.
  virtual bool conectada() = 0;

  /// Começa uma tentativa de conexão e volta na hora (a conexão segue sozinha em segundo plano).
  virtual void iniciarConexao() = 0;

  /// Desfaz a conexão ou a tentativa em andamento.
  virtual void desconectar() = 0;

  /// Quantas redes a placa conhece: 1 (só a principal) ou 2 (a principal e a reserva, #491).
  virtual uint8_t quantidadeRedes() const { return 1; }

  /// Escolhe a rede da próxima `iniciarConexao()`: 0 é a principal, 1 a reserva.
  virtual void escolherRede(uint8_t indice) { (void)indice; }
};

}  // namespace micromouse
