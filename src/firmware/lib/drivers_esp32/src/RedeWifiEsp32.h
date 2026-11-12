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
  void iniciarConexao() override {
    if (preenchido(ssid_[atual_])) {  // sem rede configurada (bancada sem credenciais.h), não tenta
      WiFi.begin(ssid_[atual_], senha_[atual_]);
    }
  }
  void desconectar() override { WiFi.disconnect(); }
  uint8_t quantidadeRedes() const override { return preenchido(ssid_[1]) ? 2 : 1; }
  void escolherRede(uint8_t indice) override { atual_ = indice < quantidadeRedes() ? indice : 0; }

  /// Troca uma das redes em tempo de execução (a bancada usa para a rede escolhida pela serial). Os
  /// textos precisam continuar válidos depois da chamada. Depois, chame `ReconexaoWifi::reiniciar()`.
  /// @param indice 0 para a principal, 1 para a reserva.
  void definirRede(uint8_t indice, const char* ssid, const char* senha) {
    if (indice < 2) {
      ssid_[indice] = ssid;
      senha_[indice] = senha != nullptr ? senha : "";
    }
  }

  /// Nome da rede `indice` (0 principal, 1 reserva), ou `""` se não houver.
  const char* ssid(uint8_t indice) const { return indice < 2 && preenchido(ssid_[indice]) ? ssid_[indice] : ""; }

  /// Nome da rede da tentativa atual ou da conexão.
  const char* ssidAtual() const { return ssid(atual_); }
  /// `"principal"` ou `"reserva"`, para o log.
  const char* nomeRedeAtual() const { return atual_ == 0 ? "principal" : "reserva"; }

 private:
  static bool preenchido(const char* texto) { return texto != nullptr && texto[0] != '\0'; }

  const char* ssid_[2];
  const char* senha_[2];
  uint8_t atual_ = 0;
};

}  // namespace micromouse
