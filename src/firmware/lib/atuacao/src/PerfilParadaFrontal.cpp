#include "PerfilParadaFrontal.h"

namespace micromouse {

PerfilParadaFrontal::PerfilParadaFrontal(const ConfiguracaoControle& config) : config_(config) {}

int16_t PerfilParadaFrontal::potenciaBase(uint16_t frenteMm) const {
  if (deveParar(frenteMm)) {
    return 0;
  }

  const uint16_t parada = config_.distanciaParadaFrontalMm;
  const uint16_t inicio = config_.distanciaInicioDesaceleracaoMm;
  if (frenteMm >= inicio || inicio <= parada) {
    return config_.potenciaCruzeiro;
  }

  const int32_t faixaPotencia = config_.potenciaCruzeiro - config_.potenciaMinima;
  const int32_t percorrido = static_cast<int32_t>(frenteMm) - parada;
  const int32_t faixaDistancia = static_cast<int32_t>(inicio) - parada;
  return static_cast<int16_t>(config_.potenciaMinima + faixaPotencia * percorrido / faixaDistancia);
}

}  // namespace micromouse
