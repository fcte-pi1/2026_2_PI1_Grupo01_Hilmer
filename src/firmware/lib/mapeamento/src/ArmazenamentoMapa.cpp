#include "ArmazenamentoMapa.h"

namespace micromouse {

void ArmazenamentoMapa::iniciarCorrida(DimensaoMapa dimensao) {
  mapa_ = Mapa(dimensao);
  corridaIniciada_ = true;
}

}  // namespace micromouse
