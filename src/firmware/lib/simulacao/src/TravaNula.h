#pragma once

/// @file TravaNula.h
/// @brief `ITrava` falsa para os testes: não trava nada, só confere que os pares fecham.

#include <stdint.h>

#include "ITrava.h"

namespace micromouse {

/// Conta as travas e confere o equilíbrio: cada `travar()` precisa ter o seu `destravar()`.
class TravaNula : public ITrava {
 public:
  void travar() override {
    ++profundidade_;
    ++travadas_;
    if (profundidade_ > maiorProfundidade_) {
      maiorProfundidade_ = profundidade_;
    }
  }

  void destravar() override { --profundidade_; }

  /// Travas seguradas agora. Zero quando tudo foi liberado; negativo seria um `destravar` a mais.
  int32_t profundidade() const { return profundidade_; }
  /// Quantas vezes `travar()` foi chamada.
  uint32_t travadas() const { return travadas_; }
  /// Maior número de travas seguradas ao mesmo tempo (deve ser 1: nada de trava dentro de trava).
  int32_t maiorProfundidade() const { return maiorProfundidade_; }

 private:
  int32_t profundidade_ = 0;
  int32_t maiorProfundidade_ = 0;
  uint32_t travadas_ = 0;
};

}  // namespace micromouse
