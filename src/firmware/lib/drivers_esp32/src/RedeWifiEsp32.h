#pragma once

/// @file RedeWifiEsp32.h
/// @brief `IRedeWifi` real: o Wi-Fi do ESP32 em modo estação (#238), com a rede reserva (#491).

#include <Arduino.h>
#include <WiFi.h>

#include "IRedeWifi.h"

namespace micromouse {

/// Wi-Fi do ESP32 em modo estação. A reconexão automática do próprio Wi-Fi fica desligada: quem
/// decide quando tentar de novo, com espera crescente, e quando passar para a rede reserva, é a
/// `ReconexaoWifi`.
class RedeWifiEsp32 : public IRedeWifi {
 public:
  /// @param ssid Rede Wi-Fi principal.
  /// @param senha Senha (vazia numa rede aberta).
  /// @param ssidReserva Rede reserva (#491); nula ou vazia, só a principal.
  /// @param senhaReserva Senha da rede reserva.
  RedeWifiEsp32(const char* ssid, const char* senha, const char* ssidReserva = nullptr,
                const char* senhaReserva = nullptr)
      : ssid_{ssid, ssidReserva}, senha_{senha, senhaReserva != nullptr ? senhaReserva : ""} {}

  /// Põe o Wi-Fi em modo estação, sem gravar a rede na flash. Chame uma vez, na tarefa de rede.
  void iniciar() {
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
  }

  bool conectada() override { return WiFi.status() == WL_CONNECTED; }
  void iniciarConexao() override { WiFi.begin(ssid_[atual_], senha_[atual_]); }
  void desconectar() override { WiFi.disconnect(); }
  uint8_t quantidadeRedes() const override {
    return ssid_[1] != nullptr && ssid_[1][0] != '\0' ? 2 : 1;
  }
  void escolherRede(uint8_t indice) override { atual_ = indice < quantidadeRedes() ? indice : 0; }

  /// Nome da rede da tentativa atual ou da conexão.
  const char* ssidAtual() const { return ssid_[atual_]; }
  /// `"principal"` ou `"reserva"`, para o log.
  const char* nomeRedeAtual() const { return atual_ == 0 ? "principal" : "reserva"; }

 private:
  const char* ssid_[2];
  const char* senha_[2];
  uint8_t atual_ = 0;
};

}  // namespace micromouse
