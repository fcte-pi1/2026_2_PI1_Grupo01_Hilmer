#include "EncoderEstimado.h"

namespace micromouse {

EncoderEstimado::EncoderEstimado(const ConfiguracaoEstimador& config) : config_(config) {}

void EncoderEstimado::atualizar(int16_t potencia, uint32_t dtMs) {
  if (dtMs == 0) {
    return;
  }
  const int64_t alvoUmPorS =
      static_cast<int64_t>(config_.tabela.velocidadeMmPorS(potencia)) * 1000 * config_.fatorEscalaPercentual / 100;
  // Resposta de primeira ordem: a velocidade caminha para o alvo com a constante de tempo do motor.
  velocidadeUmPorS_ += (alvoUmPorS - velocidadeUmPorS_) * static_cast<int64_t>(dtMs) /
                       (static_cast<int64_t>(config_.constanteTempoMs) + static_cast<int64_t>(dtMs));
  acumuladoNm_ += velocidadeUmPorS_ * static_cast<int64_t>(dtMs);
}

int32_t EncoderEstimado::lerContagem() const {
  const int64_t pulsos = acumuladoNm_ / (1000LL * static_cast<int64_t>(config_.micrometrosPorPulso));
  if (pulsos > INT32_MAX) {
    return INT32_MAX;
  }
  if (pulsos < INT32_MIN) {
    return INT32_MIN;
  }
  return static_cast<int32_t>(pulsos);
}

void EncoderEstimado::zerar() {
  velocidadeUmPorS_ = 0;
  acumuladoNm_ = 0;
}

void EncoderEstimado::corrigirMicrometros(int32_t micrometros) { acumuladoNm_ += static_cast<int64_t>(micrometros) * 1000; }

}  // namespace micromouse
