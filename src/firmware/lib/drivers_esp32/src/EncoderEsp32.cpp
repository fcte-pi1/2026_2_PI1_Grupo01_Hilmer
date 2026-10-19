#include "EncoderEsp32.h"

namespace micromouse {

EncoderEsp32::EncoderEsp32(uint8_t pinoA, uint8_t pinoB, bool inverterSentido, uint8_t modoPino)
    : pinoA_(pinoA),
      pinoB_(pinoB),
      modoPino_(modoPino),
      decodificador_(inverterSentido) {}

EncoderEsp32::~EncoderEsp32() {
  if (inicializado_) {
    detachInterrupt(digitalPinToInterrupt(pinoA_));
    detachInterrupt(digitalPinToInterrupt(pinoB_));
  }
}

void EncoderEsp32::iniciar() {
  pinMode(pinoA_, modoPino_);
  pinMode(pinoB_, modoPino_);

  const bool canalA = digitalRead(pinoA_);
  const bool canalB = digitalRead(pinoB_);

  portENTER_CRITICAL(&mux_);
  decodificador_.iniciar(canalA, canalB);
  portEXIT_CRITICAL(&mux_);

  attachInterruptArg(digitalPinToInterrupt(pinoA_), tratarInterrupcao, this, CHANGE);
  attachInterruptArg(digitalPinToInterrupt(pinoB_), tratarInterrupcao, this, CHANGE);

  inicializado_ = true;
}

int32_t EncoderEsp32::lerContagem() const {
  portENTER_CRITICAL(&mux_);
  const int32_t c = decodificador_.contagem();
  portEXIT_CRITICAL(&mux_);
  return c;
}

void EncoderEsp32::zerar() {
  portENTER_CRITICAL(&mux_);
  decodificador_.zerar();
  portEXIT_CRITICAL(&mux_);
}

uint32_t EncoderEsp32::transicoesInvalidas() const {
  portENTER_CRITICAL(&mux_);
  const uint32_t inv = decodificador_.transicoesInvalidas();
  portEXIT_CRITICAL(&mux_);
  return inv;
}

void IRAM_ATTR EncoderEsp32::tratarInterrupcao(void* arg) {
  if (arg != nullptr) {
    static_cast<EncoderEsp32*>(arg)->processarInterrupcao();
  }
}

void IRAM_ATTR EncoderEsp32::processarInterrupcao() {
  const bool canalA = digitalRead(pinoA_);
  const bool canalB = digitalRead(pinoB_);

  portENTER_CRITICAL_ISR(&mux_);
  decodificador_.processar(canalA, canalB);
  portEXIT_CRITICAL_ISR(&mux_);
}

}  // namespace micromouse
