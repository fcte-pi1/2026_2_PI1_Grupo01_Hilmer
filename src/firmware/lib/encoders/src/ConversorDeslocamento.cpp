#include "ConversorDeslocamento.h"

namespace micromouse {

namespace {

constexpr int64_t MICROMETROS_POR_MM = 1000;

int32_t saturar(int64_t valor) {
  if (valor > INT32_MAX) {
    return INT32_MAX;
  }
  if (valor < INT32_MIN) {
    return INT32_MIN;
  }
  return static_cast<int32_t>(valor);
}

}  // namespace

int32_t ConversorDeslocamento::pulsosParaMicrometros(int32_t pulsos) const {
  return saturar(static_cast<int64_t>(pulsos) * micrometrosPorPulso_);
}

int32_t ConversorDeslocamento::pulsosParaMm(int32_t pulsos) const {
  const int64_t micrometros = static_cast<int64_t>(pulsos) * micrometrosPorPulso_;
  // Arredonda para o mm mais próximo, com a metade indo para longe do zero.
  const int64_t meio = MICROMETROS_POR_MM / 2;
  const int64_t mm = micrometros >= 0 ? (micrometros + meio) / MICROMETROS_POR_MM
                                      : -((-micrometros + meio) / MICROMETROS_POR_MM);
  return saturar(mm);
}

uint32_t ConversorDeslocamento::calcularMicrometrosPorPulso(uint32_t distanciaRealMm,
                                                            uint32_t pulsos) {
  if (pulsos == 0) {
    return 0;
  }
  const uint64_t micrometros = static_cast<uint64_t>(distanciaRealMm) * MICROMETROS_POR_MM;
  return static_cast<uint32_t>((micrometros + pulsos / 2) / pulsos);
}

}  // namespace micromouse
