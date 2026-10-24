#pragma once

/// @file ImuSimulado.h
/// @brief Implementação falsa de `IImu` para testes sem hardware.

#include <stdint.h>

#include "IImu.h"

namespace micromouse {

/// IMU falsa: devolve a taxa de giro que o teste definir, somada a um viés e a um ruído, sem
/// hardware.
///
/// A leitura é `taxa + viés + ruído`, limitada à faixa de `int16_t`. O ruído é alternado e
/// determinístico (+amplitude, −amplitude, +amplitude, …), para os testes serem reproduzíveis.
///
/// Exemplo:
/// @code
/// ImuSimulado imu;
/// imu.definirVies(12);        // sensor parado lê 1,2 °/s
/// imu.definirTaxa(900);       // o robô gira para a esquerda a 90 °/s
/// IImu& sensor = imu;         // a lógica recebe só a interface
/// @endcode
class ImuSimulado : public IImu {
 public:
  /// Taxa de giro "verdadeira" do robô, em décimos de °/s.
  void definirTaxa(int16_t taxaDecimosDps) { taxa_ = taxaDecimosDps; }

  /// Viés do sensor, somado a toda leitura (o que o sensor parado mede), em décimos de °/s.
  void definirVies(int16_t viesDecimosDps) { vies_ = viesDecimosDps; }

  /// Amplitude do ruído alternado, em décimos de °/s (0 desliga o ruído).
  void definirRuidoAlternado(int16_t amplitudeDecimosDps) {
    amplitude_ = amplitudeDecimosDps;
    proximoPositivo_ = true;
  }

  /// Liga ou desliga a simulação de sensor sem resposta.
  void simularFalha(bool falha) { falha_ = falha; }

  /// Quantas leituras bem-sucedidas foram feitas.
  uint32_t leiturasFeitas() const { return leituras_; }

  /// @copydoc IImu::lerTaxaGiroZ
  bool lerTaxaGiroZ(int16_t& taxaDecimosDps) override {
    if (falha_) {
      return false;
    }
    int32_t valor = static_cast<int32_t>(taxa_) + vies_;
    if (amplitude_ != 0) {
      valor += proximoPositivo_ ? amplitude_ : -amplitude_;
      proximoPositivo_ = !proximoPositivo_;
    }
    if (valor > INT16_MAX) {
      valor = INT16_MAX;
    } else if (valor < INT16_MIN) {
      valor = INT16_MIN;
    }
    taxaDecimosDps = static_cast<int16_t>(valor);
    ++leituras_;
    return true;
  }

 private:
  int16_t taxa_ = 0;
  int16_t vies_ = 0;
  int16_t amplitude_ = 0;
  bool proximoPositivo_ = true;
  bool falha_ = false;
  uint32_t leituras_ = 0;
};

}  // namespace micromouse
