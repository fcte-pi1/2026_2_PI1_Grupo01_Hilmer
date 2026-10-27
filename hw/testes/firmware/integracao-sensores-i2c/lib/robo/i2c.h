#pragma once

#include <stdint.h>

// Solta o barramento e inicia o Wire a 400 kHz. Se a ESP reiniciou no meio de uma leitura
// (upload, RESET), um sensor pode ter ficado segurando o SDA em LOW esperando mais clocks.
void iniciarI2C();
bool respondeEm(uint8_t endereco);
