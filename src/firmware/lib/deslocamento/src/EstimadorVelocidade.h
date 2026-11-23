#pragma once

/// @file EstimadorVelocidade.h
/// @brief Velocidade média do robô entre duas leituras do deslocamento (encoders), para a telemetria.

#include <stdint.h>

namespace micromouse {

/// Calcula a velocidade escalar (mm/s) a partir do deslocamento médio das rodas, lido a cada ciclo da
/// telemetria (1 s). É a distância percorrida no intervalo dividida pelo tempo: andar de ré também conta
/// (a velocidade é escalar), e o robô parado dá 0. Não depende do Arduino: testado no ambiente `native`.
class EstimadorVelocidade {
 public:
  /// @param deslocamentoMm Deslocamento médio acumulado das rodas, em mm (sobe ao andar para a frente).
  /// @param agoraMs Relógio em ms (o contador pode dar a volta).
  /// @return Velocidade em mm/s desde a chamada anterior; 0 na primeira chamada ou com tempo zerado.
  int32_t atualizar(int32_t deslocamentoMm, uint32_t agoraMs) {
    int32_t velocidade = 0;
    if (temAnterior_) {
      const uint32_t dt = agoraMs - anteriorMs_;
      if (dt > 0) {
        const int64_t delta = static_cast<int64_t>(deslocamentoMm) - anteriorMm_;
        const int64_t modulo = delta < 0 ? -delta : delta;
        velocidade = static_cast<int32_t>(modulo * 1000 / dt);
      }
    }
    temAnterior_ = true;
    anteriorMm_ = deslocamentoMm;
    anteriorMs_ = agoraMs;
    return velocidade;
  }

 private:
  bool temAnterior_ = false;
  int32_t anteriorMm_ = 0;
  uint32_t anteriorMs_ = 0;
};

}  // namespace micromouse
