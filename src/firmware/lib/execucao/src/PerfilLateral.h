#pragma once

/// @file PerfilLateral.h
/// @brief Leituras laterais do ToF durante o avanço de uma célula, para achar aberturas nas paredes.
///
/// Com os sensores laterais na diagonal, uma leitura tirada com o robô parado no centro da célula pode
/// bater na quina de uma parede vizinha e fazer uma abertura parecer parede. Durante o avanço, o raio varre
/// o corredor: uma abertura aparece como uma sequência de leituras longas. Não depende do Arduino.

#include <stdint.h>

#include "Tipos.h"

namespace micromouse {

class PerfilLateral {
 public:
  static constexpr uint8_t CAPACIDADE = 24;  ///< Amostras guardadas por lado (uma a cada `DECIMACAO` leituras).
  static constexpr uint8_t DECIMACAO = 2;
  static constexpr uint16_t LIMITE_REGISTRO_MM = 999;  ///< Leituras maiores são guardadas como 999.

  void reiniciar() {
    quantidade_ = 0;
    contador_ = 0;
  }

  /// Guarda uma leitura nova do avanço (uma a cada `DECIMACAO`, até `CAPACIDADE`).
  void registrar(const DistanciasLaterais& leitura) {
    if (contador_++ % DECIMACAO != 0 || quantidade_ >= CAPACIDADE) {
      return;
    }
    esquerda_[quantidade_] = limitar(leitura.esquerdaMm);
    direita_[quantidade_] = limitar(leitura.direitaMm);
    ++quantidade_;
  }

  uint8_t quantidade() const { return quantidade_; }
  uint16_t esquerda(uint8_t i) const { return esquerda_[i]; }
  uint16_t direita(uint8_t i) const { return direita_[i]; }

  /// Maior sequência de amostras seguidas com leitura de pelo menos `limiarMm` num lado.
  uint8_t maiorSequenciaAcima(bool ladoEsquerdo, uint16_t limiarMm) const {
    uint8_t melhor = 0;
    uint8_t atual = 0;
    for (uint8_t i = 0; i < quantidade_; i++) {
      const uint16_t v = ladoEsquerdo ? esquerda_[i] : direita_[i];
      atual = v >= limiarMm ? static_cast<uint8_t>(atual + 1) : 0;
      if (atual > melhor) {
        melhor = atual;
      }
    }
    return melhor;
  }

 private:
  static uint16_t limitar(uint16_t v) { return v > LIMITE_REGISTRO_MM ? LIMITE_REGISTRO_MM : v; }

  uint16_t esquerda_[CAPACIDADE] = {};
  uint16_t direita_[CAPACIDADE] = {};
  uint8_t quantidade_ = 0;
  uint8_t contador_ = 0;
};

}  // namespace micromouse
