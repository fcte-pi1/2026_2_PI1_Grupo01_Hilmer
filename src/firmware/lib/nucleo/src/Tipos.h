#pragma once

#include <stdint.h>

namespace micromouse {

enum class Direcao : uint8_t { Norte, Leste, Sul, Oeste };

enum class Lado : uint8_t { Frente, Esquerda, Direita };

enum class EstadoParede : uint8_t { Desconhecido, Livre, Parede };

struct PosicaoCelula {
  uint8_t linha;
  uint8_t coluna;
};

struct Pose {
  PosicaoCelula celula;
  Direcao direcao;
};

struct DistanciasLaterais {
  uint16_t frenteMm;
  uint16_t esquerdaMm;
  uint16_t direitaMm;
};

}  // namespace micromouse
