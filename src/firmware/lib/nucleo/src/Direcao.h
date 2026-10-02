#pragma once

#include "Tipos.h"

namespace micromouse {

constexpr Direcao girarDireita(Direcao d) {
  return static_cast<Direcao>((static_cast<uint8_t>(d) + 1) & 3);
}

constexpr Direcao girarEsquerda(Direcao d) {
  return static_cast<Direcao>((static_cast<uint8_t>(d) + 3) & 3);
}

constexpr Direcao oposta(Direcao d) {
  return static_cast<Direcao>((static_cast<uint8_t>(d) + 2) & 3);
}

// Converte um lado relativo ao robô (frente, esquerda, direita) na direção absoluta do labirinto.
constexpr Direcao direcaoAbsoluta(Direcao direcaoDoRobo, Lado lado) {
  return lado == Lado::Frente     ? direcaoDoRobo
         : lado == Lado::Esquerda ? girarEsquerda(direcaoDoRobo)
                                  : girarDireita(direcaoDoRobo);
}

}  // namespace micromouse
