#include "ServicoOta.h"

#include <ArduinoOTA.h>

namespace micromouse {

namespace {

void (*aoIniciarGravacao)() = nullptr;

}  // namespace

void ServicoOta::iniciar(void (*aoIniciar)(), const char* nome) {
  if (ativo_) {
    return;
  }
  aoIniciarGravacao = aoIniciar;
  ArduinoOTA.setHostname(nome);
  ArduinoOTA.setPassword(MICROMOUSE_SENHA_OTA);
  ArduinoOTA.onStart([]() {
    if (aoIniciarGravacao != nullptr) {
      aoIniciarGravacao();
    }
    Serial.println("{\"status\":\"ota_gravando\"}");
  });
  ArduinoOTA.onEnd([]() { Serial.println("{\"status\":\"ota_gravado\"}"); });
  ArduinoOTA.onError([](ota_error_t erro) {
    Serial.printf("{\"status\":\"ota_erro\",\"codigo\":%u}\n", static_cast<unsigned>(erro));
  });
  ArduinoOTA.begin();
  ativo_ = true;
}

void ServicoOta::ciclo() {
  if (ativo_) {
    ArduinoOTA.handle();
  }
}

}  // namespace micromouse
