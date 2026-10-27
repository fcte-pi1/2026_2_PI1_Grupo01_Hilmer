// Pinagem do robo (ESP32-C3 Super Mini). Detalhes em PINAGEM.md.
#pragma once

#define SDA_PIN 6
#define SCL_PIN 7
#define XSHUT_ESQ 9  // o pull-up da placa do sensor mantem o BOOT alto ao ligar
#define XSHUT_DIR 8

// DRV8833: um par de entradas por motor
#define ESQ_IN1 0
#define ESQ_IN2 1
#define DIR_IN1 10
#define DIR_IN2 21

// Encoder hall do N20 (canais A e B), alimentado em 3V3 para a saida nao passar de 3.3 V.
// Se o contador diminuir com o motor indo para frente, troque A e B aqui.
#define ENC_ESQ_A 3
#define ENC_ESQ_B 4
