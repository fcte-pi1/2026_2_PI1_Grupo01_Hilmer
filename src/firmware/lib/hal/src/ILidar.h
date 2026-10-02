#pragma once

#include "Tipos.h"

namespace micromouse {

class ILidar {
 public:
  virtual ~ILidar() = default;

  // Retorna true e preenche `saida` quando há uma leitura válida das três direções.
  virtual bool lerDistanciasLaterais(DistanciasLaterais& saida) = 0;
};

}  // namespace micromouse
