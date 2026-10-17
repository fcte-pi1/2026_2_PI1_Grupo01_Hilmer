#include "MedidorDeslocamento.h"

namespace micromouse {

DeslocamentoRodas MedidorDeslocamento::medir() const {
  return DeslocamentoRodas{conversor_.pulsosParaMm(esquerdo_.lerContagem()),
                           conversor_.pulsosParaMm(direito_.lerContagem())};
}

int32_t MedidorDeslocamento::deslocamentoMedioMm() const {
  const DeslocamentoRodas rodas = medir();
  // Soma em 64 bits para não estourar com valores grandes.
  return static_cast<int32_t>((static_cast<int64_t>(rodas.esquerdaMm) + rodas.direitaMm) / 2);
}

void MedidorDeslocamento::zerar() {
  esquerdo_.zerar();
  direito_.zerar();
}

}  // namespace micromouse
