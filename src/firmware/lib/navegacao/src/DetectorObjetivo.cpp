#include "DetectorObjetivo.h"

#include "ArmazenamentoMapa.h"
#include "IMotores.h"

namespace micromouse {

PosicaoCelula DetectorObjetivo::calcularObjetivo(uint8_t linhas, uint8_t colunas) {
  if (linhas == 0 || colunas == 0) {
    return PosicaoCelula{0, 0};
  }
  return PosicaoCelula{static_cast<uint8_t>(linhas - 1), static_cast<uint8_t>(colunas - 1)};
}

PosicaoCelula DetectorObjetivo::calcularObjetivo(DimensaoMapa dimensao) {
  switch (dimensao) {
    case DimensaoMapa::Labirinto4x4:
      return PosicaoCelula{3, 3};
    case DimensaoMapa::Labirinto8x4:
      return PosicaoCelula{3, 7};
    case DimensaoMapa::Labirinto12x4:
      return PosicaoCelula{3, 11};
    case DimensaoMapa::Exploracao:  // sem objetivo único: quem explora define os candidatos
    default:
      return PosicaoCelula{3, 3};
  }
}

bool DetectorObjetivo::alcancouObjetivo(const PosicaoCelula& atual, const PosicaoCelula& objetivo) {
  return atual.linha == objetivo.linha && atual.coluna == objetivo.coluna;
}

DetectorObjetivo::DetectorObjetivo(PosicaoCelula objetivo) : objetivo_(objetivo) {}

DetectorObjetivo::DetectorObjetivo(DimensaoMapa dimensao) : objetivo_(calcularObjetivo(dimensao)) {}

DetectorObjetivo::DetectorObjetivo(uint8_t linhas, uint8_t colunas) : objetivo_(calcularObjetivo(linhas, colunas)) {}

bool DetectorObjetivo::alcancouObjetivo(const PosicaoCelula& atual) const {
  return alcancouObjetivo(atual, objetivo_);
}

bool DetectorObjetivo::processarChegada(const Pose& pose, IMotores& motores, ArmazenamentoMapa* armazenamento) {
  if (alcancado_) {
    motores.parar();
    return true;
  }
  if (alcancouObjetivo(pose)) {
    alcancado_ = true;
    eventoChegadaPendente_ = true;
    motores.parar();
    if (armazenamento != nullptr) {
      armazenamento->congelarMapa();
    }
    return true;
  }
  return false;
}

bool DetectorObjetivo::consumirEventoChegada() {
  if (eventoChegadaPendente_) {
    eventoChegadaPendente_ = false;
    return true;
  }
  return false;
}

}  // namespace micromouse
