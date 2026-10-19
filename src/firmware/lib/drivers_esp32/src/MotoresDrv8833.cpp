#include "MotoresDrv8833.h"

#include <esp_arduino_version.h>

#include "SinaisDrv8833.h"

namespace micromouse {

namespace {

constexpr uint32_t DUTY_MAXIMO = (1u << RESOLUCAO_PWM_MOTORES_BITS) - 1;

}  // namespace

MotoresDrv8833::MotoresDrv8833(uint8_t esquerdaIn1, uint8_t esquerdaIn2, uint8_t direitaIn1,
                               uint8_t direitaIn2, bool inverterEsquerda, bool inverterDireita)
    : pinos_{esquerdaIn1, esquerdaIn2, direitaIn1, direitaIn2},
      inverterEsquerda_(inverterEsquerda),
      inverterDireita_(inverterDireita) {}

void MotoresDrv8833::iniciar() {
  for (uint8_t i = 0; i < 4; ++i) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    // Arduino-ESP32 3.x: o canal do LEDC é escolhido automaticamente para cada pino.
    ledcAttach(pinos_[i], FREQUENCIA_PWM_MOTORES_HZ, RESOLUCAO_PWM_MOTORES_BITS);
#else
    // Arduino-ESP32 2.x: um canal do LEDC por pino (o ESP32-C3 tem os canais 0 a 5).
    ledcSetup(i, FREQUENCIA_PWM_MOTORES_HZ, RESOLUCAO_PWM_MOTORES_BITS);
    ledcAttachPin(pinos_[i], i);
#endif
  }
  iniciado_ = true;
  parar();
}

void MotoresDrv8833::definirPotencia(const PotenciaRodas& potencia) {
  if (!iniciado_) {
    return;
  }
  const SinaisPonteH esquerda = calcularSinaisDrv8833(potencia.esquerda, inverterEsquerda_, DUTY_MAXIMO);
  const SinaisPonteH direita = calcularSinaisDrv8833(potencia.direita, inverterDireita_, DUTY_MAXIMO);
  escrever(0, esquerda.in1);
  escrever(1, esquerda.in2);
  escrever(2, direita.in1);
  escrever(3, direita.in2);
}

void MotoresDrv8833::parar() {
  if (!iniciado_) {
    return;
  }
  const SinaisPonteH freio = sinaisFreioDrv8833(DUTY_MAXIMO);
  escrever(0, freio.in1);
  escrever(1, freio.in2);
  escrever(2, freio.in1);
  escrever(3, freio.in2);
}

void MotoresDrv8833::escrever(uint8_t indice, uint32_t duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(pinos_[indice], duty);
#else
  ledcWrite(indice, duty);
#endif
}

}  // namespace micromouse
