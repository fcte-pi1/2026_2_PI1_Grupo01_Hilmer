#include "BarramentoI2c.h"

namespace micromouse {

bool BarramentoI2c::iniciar(int pinoSda, int pinoScl, uint32_t frequenciaHz) {
  if (mutex_ == nullptr) {
    mutex_ = xSemaphoreCreateMutex();
  }
  if (mutex_ == nullptr) {
    return false;
  }
  return wire_.begin(pinoSda, pinoScl, frequenciaHz);
}

bool BarramentoI2c::travar(uint32_t esperaMaximaMs) {
  if (mutex_ == nullptr) {
    return false;
  }
  if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(esperaMaximaMs)) == pdTRUE) {
    return true;
  }
  esperasEsgotadas_ = esperasEsgotadas_ + 1;
  return false;
}

void BarramentoI2c::destravar() {
  if (mutex_ != nullptr) {
    xSemaphoreGive(mutex_);
  }
}

}  // namespace micromouse
