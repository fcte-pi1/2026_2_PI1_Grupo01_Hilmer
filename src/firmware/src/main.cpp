#include <Arduino.h>

#include "Configuracao.h"

static void tarefaNavegacao(void*) {
  for (;;) {
    // Ciclo percepção, decisão e atuação: percepção, mapeamento, navegação e atuação entram aqui.
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

static void tarefaTelemetria(void*) {
  for (;;) {
    // Coleta e envio de telemetria entram aqui.
    vTaskDelay(pdMS_TO_TICKS(micromouse::PERIODO_TELEMETRIA_MS));
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("micromouse: iniciando");

  xTaskCreate(tarefaNavegacao, "navegacao", 4096, nullptr, 2, nullptr);
  xTaskCreate(tarefaTelemetria, "telemetria", 4096, nullptr, 1, nullptr);
}

void loop() {
  delay(1000);
}
