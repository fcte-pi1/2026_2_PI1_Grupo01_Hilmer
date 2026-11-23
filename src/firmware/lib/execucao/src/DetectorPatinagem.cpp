#include "DetectorPatinagem.h"

namespace micromouse {

DetectorPatinagem::DetectorPatinagem(ConfiguracaoPatinagem configuracao) : configuracao_(configuracao) {}

void DetectorPatinagem::reiniciar() {
  temBase_ = false;
  decorridoMs_ = 0;
}

bool DetectorPatinagem::atualizar(bool ativo, int64_t posicaoAvancoUm, bool frenteValida, uint16_t frenteMm,
                                  uint32_t dtMs) {
  if (!ativo || !frenteValida || frenteMm > configuracao_.frenteMaximaValidaMm) {
    reiniciar();
    return false;
  }
  if (!temBase_) {
    temBase_ = true;
    posicaoBaseUm_ = posicaoAvancoUm;
    frenteBaseMm_ = frenteMm;
    decorridoMs_ = 0;
    return false;
  }
  decorridoMs_ += dtMs;
  if (decorridoMs_ < configuracao_.janelaMs) {
    return false;
  }
  const int64_t percurso = posicaoAvancoUm > posicaoBaseUm_ ? posicaoAvancoUm - posicaoBaseUm_ : posicaoBaseUm_ - posicaoAvancoUm;
  const uint16_t variacao = frenteMm > frenteBaseMm_ ? static_cast<uint16_t>(frenteMm - frenteBaseMm_)
                                                    : static_cast<uint16_t>(frenteBaseMm_ - frenteMm);
  const bool patinando = percurso >= configuracao_.percursoMinimoUm && variacao <= configuracao_.variacaoMaximaFrenteMm;
  // Nova janela a partir daqui, patinando ou não.
  posicaoBaseUm_ = posicaoAvancoUm;
  frenteBaseMm_ = frenteMm;
  decorridoMs_ = 0;
  if (patinando) {
    ++deteccoes_;
  }
  return patinando;
}

}  // namespace micromouse
