#include "DecodificadorQuadratura.h"

namespace micromouse {

namespace {

/// Marca de transição inválida na tabela (os dois canais mudaram de uma vez).
constexpr int8_t INVALIDA = 2;

/// Passo de cada transição, indexada por `(estadoAnterior << 2) | estadoNovo`, com o estado
/// formado por `(A << 1) | B`. A sequência 00 → 10 → 11 → 01 → 00 é o sentido positivo.
constexpr int8_t TABELA[16] = {
    //  novo: 00  01  10  11
    /* 00 */ 0, -1, +1, INVALIDA,
    /* 01 */ +1, 0, INVALIDA, -1,
    /* 10 */ -1, INVALIDA, 0, +1,
    /* 11 */ INVALIDA, +1, -1, 0,
};

uint8_t montarEstado(bool canalA, bool canalB) {
  return static_cast<uint8_t>((canalA ? 2 : 0) | (canalB ? 1 : 0));
}

}  // namespace

DecodificadorQuadratura::DecodificadorQuadratura(bool inverterSentido)
    : inverter_(inverterSentido) {}

void DecodificadorQuadratura::iniciar(bool canalA, bool canalB) {
  estado_ = montarEstado(canalA, canalB);
}

int8_t DecodificadorQuadratura::processar(bool canalA, bool canalB) {
  const uint8_t novo = montarEstado(canalA, canalB);
  const int8_t passo = TABELA[(estado_ << 2) | novo];
  estado_ = novo;
  if (passo == INVALIDA) {
    invalidas_++;
    return 0;
  }
  const int8_t resultado = inverter_ ? static_cast<int8_t>(-passo) : passo;
  contagem_ += resultado;
  return resultado;
}

}  // namespace micromouse
