#pragma once

/// @file MedidorDeslocamento.h
/// @brief Distância percorrida por cada roda, a partir dos dois encoders.

#include <stdint.h>

#include "ConversorDeslocamento.h"
#include "IEncoder.h"

namespace micromouse {

/// Distância percorrida por cada roda desde o último `zerar()`, em milímetros.
/// Positiva quando a roda avançou. Rodas com distâncias diferentes indicam curva: a base da
/// odometria diferencial usada para estimar a posição (HU-05).
struct DeslocamentoRodas {
  int32_t esquerdaMm;
  int32_t direitaMm;
};

/// Lê os dois encoders e converte os pulsos em distância por roda.
///
/// Guarda referências aos encoders e ao conversor, que precisam continuar existindo enquanto o
/// medidor for usado. Não aloca memória dinâmica.
class MedidorDeslocamento {
 public:
  MedidorDeslocamento(IEncoder& esquerdo, IEncoder& direito, const ConversorDeslocamento& conversor)
      : esquerdo_(esquerdo), direito_(direito), conversor_(conversor) {}

  /// Distância percorrida por cada roda desde o último `zerar()`.
  DeslocamentoRodas medir() const;

  /// Distância média das duas rodas, em mm: o avanço do centro do robô em linha reta.
  int32_t deslocamentoMedioMm() const;

  /// Zera os dois encoders, começando uma nova medição (por exemplo, ao entrar numa célula).
  void zerar();

 private:
  IEncoder& esquerdo_;
  IEncoder& direito_;
  const ConversorDeslocamento& conversor_;
};

}  // namespace micromouse
