#include "ControladorCentralizacao.h"

namespace micromouse {

ControladorCentralizacao::ControladorCentralizacao(const ConfiguracaoControle& config)
    : config_(config) {}

int16_t ControladorCentralizacao::calcular(const DistanciasLaterais& leitura, uint32_t dtMs) {
  const bool paredeEsquerda = leitura.esquerdaMm <= config_.limiarParedeLateralMm;
  const bool paredeDireita = leitura.direitaMm <= config_.limiarParedeLateralMm;
  const int32_t alvo = config_.distanciaAlvoLateralMm;

  ModoCentralizacao modo;
  int32_t erro;
  if (paredeEsquerda && paredeDireita) {
    modo = ModoCentralizacao::DuasParedes;
    erro = (static_cast<int32_t>(leitura.esquerdaMm) - leitura.direitaMm) / 2;
  } else if (paredeEsquerda) {
    modo = ModoCentralizacao::SoEsquerda;
    erro = static_cast<int32_t>(leitura.esquerdaMm) - alvo;
  } else if (paredeDireita) {
    modo = ModoCentralizacao::SoDireita;
    erro = alvo - static_cast<int32_t>(leitura.direitaMm);
  } else {
    reiniciar();
    return 0;
  }

  if (temErroAnterior_ && modo == modo_) {
    if (dtMs > 0) {
      const int32_t derivadaBruta = (erro - erroMm_) * 1000 / static_cast<int32_t>(dtMs);
      derivadaMmPorS_ += (derivadaBruta - derivadaMmPorS_) * config_.filtroDerivadaPercentual / 100;
    }
  } else {
    derivadaMmPorS_ = 0;
  }

  modo_ = modo;
  erroMm_ = erro;
  temErroAnterior_ = true;

  int32_t correcao =
      (config_.ganhoProporcionalDecimos * erro + config_.ganhoDerivativoDecimos * derivadaMmPorS_) / 10;
  if (correcao > config_.correcaoMaxima) {
    correcao = config_.correcaoMaxima;
  } else if (correcao < -config_.correcaoMaxima) {
    correcao = -config_.correcaoMaxima;
  }
  return static_cast<int16_t>(correcao);
}

void ControladorCentralizacao::reiniciar() {
  modo_ = ModoCentralizacao::SemReferencia;
  temErroAnterior_ = false;
  erroMm_ = 0;
  derivadaMmPorS_ = 0;
}

}  // namespace micromouse
