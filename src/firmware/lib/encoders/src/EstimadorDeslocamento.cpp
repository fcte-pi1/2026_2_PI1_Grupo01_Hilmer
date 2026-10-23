#include "EstimadorDeslocamento.h"

namespace micromouse {

EstimadorDeslocamento::EstimadorDeslocamento(const ConfiguracaoEstimador& config)
    : esquerdo_(config), direito_(config) {}

void EstimadorDeslocamento::atualizar(const PotenciaRodas& potencia, uint32_t dtMs) {
  esquerdo_.atualizar(potencia.esquerda, dtMs);
  direito_.atualizar(potencia.direita, dtMs);
}

void EstimadorDeslocamento::zerar() {
  esquerdo_.zerar();
  direito_.zerar();
}

void EstimadorDeslocamento::corrigirAvancoMicrometros(int32_t micrometros) {
  esquerdo_.corrigirMicrometros(micrometros);
  direito_.corrigirMicrometros(micrometros);
}

}  // namespace micromouse
