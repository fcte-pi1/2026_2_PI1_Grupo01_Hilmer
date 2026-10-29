/// @file bancada_geral.cpp
/// @brief Firmware de bancada geral: menu de uma tecla pela serial para conferir, no hardware
/// montado, os 3 VL53L0X, o MPU6050, os dois motores e os encoders, com os mesmos drivers do robô.
///
/// Não é o firmware do robô: só entra no ambiente `bancada_geral` do `platformio.ini`. Grave e abra
/// o monitor com `pio run -e bancada_geral -t upload && pio device monitor -e bancada_geral`.
/// Digite só a tecla do menu (não precisa de Enter; Enter mostra o menu de novo).
///
/// SEGURANÇA: os motores ficam freados no boot e só giram dentro do submenu de motores, por
/// `DURACAO_TESTE_MOTOR_MS` no máximo. Qualquer tecla (menos Enter) freia os motores na hora.
/// Para o primeiro teste, deixe as rodas fora do chão.

#include <Arduino.h>
#include <Wire.h>
#include <stdarg.h>
#include <stdlib.h>

#if !defined(ARDUINO_USB_CDC_ON_BOOT) || !ARDUINO_USB_CDC_ON_BOOT
#error "Serial pela USB nativa: defina ARDUINO_USB_MODE=1 e ARDUINO_USB_CDC_ON_BOOT=1 no platformio.ini (Pinos.h)"
#endif

#include "BarramentoI2c.h"
#include "ConfiguracaoMotores.h"
#include "ConversorDeslocamento.h"
#include "EncoderEsp32.h"
#include "ImuMpu6050.h"
#include "MotoresDrv8833.h"
#include "Pinos.h"
#include "TofVl53l0x.h"

using namespace micromouse;

namespace {

/// Tempo máximo de cada teste de motor, em ms. Passado esse tempo, os motores freiam sozinhos.
constexpr uint32_t DURACAO_TESTE_MOTOR_MS = 2000;

/// Potência dos testes de motor, em milésimos da potência total (a mesma escala do robô). O driver
/// já limita o duty a `LIMITE_DUTY_MOTORES_MILESIMOS` (`ConfiguracaoMotores.h`).
constexpr int16_t POTENCIA_INICIAL = 300;
constexpr int16_t POTENCIA_PASSO = 100;
constexpr int16_t POTENCIA_MINIMA = 100;

/// Intervalo entre leituras dos ToF: o mesmo da tarefa de navegação.
constexpr uint32_t PERIODO_LEITURA_TOF_MS = 10;

/// Tempo de leitura contínua antes de mostrar uma leitura única do ToF, para o sensor ter amostra.
constexpr uint32_t ESPERA_LEITURA_TOF_MS = 150;

/// Intervalo entre linhas impressas nos modos de fluxo.
constexpr uint32_t PERIODO_FLUXO_MS = 200;

/// Autoteste: duração da espera com o robô parado e limite do giro parado, em décimos de °/s.
constexpr uint32_t AUTOTESTE_ESPERA_TOF_MS = 300;
constexpr uint32_t AUTOTESTE_ESPERA_ENCODER_MS = 1000;
constexpr uint8_t AUTOTESTE_LEITURAS_GIRO = 50;
constexpr int32_t AUTOTESTE_GIRO_PARADO_MAX_DECIMOS = 50;

/// Menos pulsos que isso num teste de motor contam como "encoder sem sinal".
constexpr int32_t PULSOS_MINIMOS_NO_TESTE = 10;

/// Rampa de potência: de `RAMPA_INICIO` a `POTENCIA_MAXIMA`, um degrau a cada `RAMPA_DEGRAU_MS`.
constexpr int16_t RAMPA_INICIO = 100;
constexpr int16_t RAMPA_PASSO = 50;
constexpr uint32_t RAMPA_DEGRAU_MS = 300;

/// Pulsos por degrau da rampa a partir dos quais a roda conta como girando.
constexpr int32_t RAMPA_PULSOS_PARA_GIRAR = 3;

/// Duração máxima do modo contínuo, em ms. Passado esse tempo, os motores freiam sozinhos.
constexpr uint32_t TEMPO_MAXIMO_CONTINUO_MS = 60000;

/// Duty efetivo, em %, de uma potência (o driver limita o duty a `LIMITE_DUTY_MOTORES_MILESIMOS`).
constexpr int dutyPercentual(int16_t potencia) {
  return (potencia < 0 ? -potencia : potencia) * static_cast<int>(LIMITE_DUTY_MOTORES_MILESIMOS) / 10000;
}

constexpr Lado LADOS[3] = {Lado::Frente, Lado::Esquerda, Lado::Direita};
constexpr const char* NOMES[3] = {"frente", "esquerda", "direita"};

enum class Tela : uint8_t { Principal, Motores };

/// Motor(es) acionados no modo contínuo.
enum class Alvo : uint8_t { Esquerdo, Direito, Ambos };

// Os motores do robô estão montados espelhados: o direito é invertido, como no `main.cpp`.
MotoresDrv8833 motores(pinos::MOTOR_ESQ_IN1, pinos::MOTOR_ESQ_IN2, pinos::MOTOR_DIR_IN3,
                       pinos::MOTOR_DIR_IN4, false, true);
// Encoders das rodas, com a mesma inversão do `main.cpp` (a roda direita é espelhada).
EncoderEsp32 encoderEsq(pinos::ENCODER_ESQ_A, pinos::ENCODER_ESQ_B, false);
EncoderEsp32 encoderDir(pinos::ENCODER_DIR_A, pinos::ENCODER_DIR_B, true);
const ConversorDeslocamento conversor;
BarramentoI2c barramento(Wire);
ImuMpu6050 imu(barramento);
TofVl53l0x tof(Wire, {pinos::XSHUT_FRONTAL, pinos::ENDERECO_TOF_FRONTAL},
               {pinos::XSHUT_ESQ, pinos::ENDERECO_TOF_ESQ}, {pinos::XSHUT_DIR, pinos::ENDERECO_TOF_DIR});

Tela tela = Tela::Principal;
int16_t potencia = POTENCIA_INICIAL;

const char* nomeEstado(EstadoAmostraTof estado) {
  switch (estado) {
    case EstadoAmostraTof::SemAmostra:
      return "sem amostra";
    case EstadoAmostraTof::Valida:
      return "valida";
    case EstadoAmostraTof::AbaixoDoMinimo:
      return "abaixo do minimo";
    case EstadoAmostraTof::ForaDeAlcance:
      return "fora de alcance";
    case EstadoAmostraTof::Falha:
      return "falha";
  }
  return "?";
}

/// `true` se chegou alguma tecla pela serial, exceto Enter. Descarta o que chegou.
bool teclaPressionada() {
  bool pressionada = false;
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    if (c != '\r' && c != '\n') {
      pressionada = true;
    }
  }
  return pressionada;
}

void imprimirMenuPrincipal() {
  Serial.println();
  Serial.println("=== BANCADA GERAL: sensores, motores e encoders ===");
  Serial.println(" Build: " __DATE__ " " __TIME__);
  Serial.println(" t  AUTOTESTE: confere sensores e encoders parados (nao gira motor)");
  Serial.println(" 1  varredura I2C (quem responde no barramento)");
  Serial.println(" 2  reiniciar os ToF e o MPU6050");
  Serial.println(" 3  ToF: uma leitura dos 3 sensores");
  Serial.println(" 4  ToF: leitura continua (qualquer tecla para)");
  Serial.println(" 5  MPU6050: uma leitura (giro Z e aceleracao X/Y)");
  Serial.println(" 6  MPU6050: leitura continua (qualquer tecla para)");
  Serial.println(" 7  motores (abre o menu de motores)");
  Serial.println(" 8  pinagem (GPIO de cada sinal)");
  Serial.println(" 9  encoders: contagem continua, gire a roda a mao (qualquer tecla para)");
  Serial.println(" 0  zerar a contagem dos encoders");
  Serial.println(" r  reiniciar a placa");
  Serial.println(" Enter  mostra este menu");
}

void imprimirMenuMotores() {
  Serial.println();
  Serial.println("=== MOTORES (cada teste dura no maximo 2 s; qualquer tecla freia) ===");
  Serial.printf(" Potencia atual: %d de 1000 (duty efetivo %d%%, limite do driver %u%%)\n", potencia,
                dutyPercentual(potencia), static_cast<unsigned>(LIMITE_DUTY_MOTORES_MILESIMOS / 10));
  Serial.println(" 1  motor esquerdo para a frente     2  motor esquerdo para tras");
  Serial.println(" 3  motor direito para a frente      4  motor direito para tras");
  Serial.println(" 5  os dois para a frente            6  os dois para tras");
  Serial.println(" 7  giro no lugar para a direita     8  giro no lugar para a esquerda");
  Serial.println(" 9  rampa no motor esquerdo          a  rampa no motor direito");
  Serial.println("    (a rampa sobe a potencia aos poucos e diz em qual a roda comecou a girar)");
  Serial.println(" c  modo continuo: fica acionando e mostrando os pulsos ao vivo (ate 60 s)");
  Serial.println(" +  aumenta a potencia               -  diminui a potencia");
  Serial.println(" 0  voltar ao menu principal");
  Serial.println(" ATENCAO: o primeiro teste com as rodas fora do chao.");
}

/// Varre os endereços de 7 bits e diz quem respondeu, comparando com o que a pinagem espera.
void varrerBarramento() {
  Serial.printf("Varredura I2C (SDA=GPIO%d, SCL=GPIO%d):\n", pinos::I2C_SDA, pinos::I2C_SCL);
  uint8_t encontrados = 0;
  for (uint8_t endereco = 0x08; endereco < 0x78; endereco++) {
    Wire.beginTransmission(endereco);
    if (Wire.endTransmission() != 0) {
      continue;
    }
    encontrados++;
    const char* quem = "desconhecido";
    if (endereco == pinos::ENDERECO_TOF_FRONTAL) {
      quem = "VL53L0X frontal";
    } else if (endereco == pinos::ENDERECO_TOF_ESQ) {
      quem = "VL53L0X esquerdo";
    } else if (endereco == pinos::ENDERECO_TOF_DIR) {
      quem = "VL53L0X direito";
    } else if (endereco == pinos::ENDERECO_MPU6050) {
      quem = "MPU6050";
    } else if (endereco == pinos::ENDERECO_FABRICA_TOF) {
      quem = "VL53L0X no endereco de fabrica (nao foi reenderecado)";
    }
    Serial.printf("  0x%02X  %s\n", endereco, quem);
  }
  Serial.printf("%u dispositivo(s). Esperado no robo: 0x30, 0x31, 0x32 e 0x68.\n", encontrados);
}

void reiniciarSensores() {
  Serial.println("Reiniciando os sensores...");
  const bool tofOk = tof.iniciar();
  Serial.printf("  ToF: frente %s, esquerda %s, direita %s\n",
                tof.sensorIniciado(Lado::Frente) ? "ok" : "FALHOU",
                tof.sensorIniciado(Lado::Esquerda) ? "ok" : "FALHOU",
                tof.sensorIniciado(Lado::Direita) ? "ok" : "FALHOU");
  const bool imuOk = imu.iniciar();
  if (imuOk) {
    Serial.printf("  MPU6050: ok (WHO_AM_I 0x%02X)\n", imu.identificacao());
  } else {
    Serial.println("  MPU6050: FALHOU (sem resposta no 0x68; confira SDA, SCL, energia e AD0)");
  }
  Serial.println(tofOk && imuOk ? "Todos os sensores responderam." : "Algum sensor nao respondeu.");
}

/// Lê os ToF por `duracaoMs` (eles precisam ser consultados de forma contínua para ter amostra).
void lerTofPor(uint32_t duracaoMs) {
  DistanciasLaterais descartada;
  const uint32_t inicioMs = millis();
  do {
    tof.lerDistanciasLaterais(descartada);
    delay(PERIODO_LEITURA_TOF_MS);
  } while (millis() - inicioMs < duracaoMs);
}

void imprimirTof() {
  Serial.print("ToF ");
  for (uint8_t i = 0; i < 3; i++) {
    if (!tof.sensorIniciado(LADOS[i])) {
      Serial.printf(" | %s: nao iniciado", NOMES[i]);
      continue;
    }
    const AmostraTof& amostra = tof.amostra(LADOS[i]);
    if (amostra.estado == EstadoAmostraTof::Valida) {
      Serial.printf(" | %s: %u mm", NOMES[i], amostra.distanciaMm);
    } else {
      Serial.printf(" | %s: %s", NOMES[i], nomeEstado(amostra.estado));
    }
  }
  Serial.println();
}

void lerTofUmaVez() {
  lerTofPor(ESPERA_LEITURA_TOF_MS);
  imprimirTof();
}

void lerTofContinuo() {
  Serial.println("ToF continuo. Passe a mao na frente de cada sensor. Qualquer tecla para.");
  uint32_t ultimaImpressaoMs = 0;
  while (!teclaPressionada()) {
    lerTofPor(PERIODO_LEITURA_TOF_MS);
    if (millis() - ultimaImpressaoMs >= PERIODO_FLUXO_MS) {
      ultimaImpressaoMs = millis();
      imprimirTof();
    }
  }
  Serial.println("Fim da leitura continua.");
}

void imprimirImu() {
  int16_t giroDecimosDps = 0;
  int16_t axMg = 0;
  int16_t ayMg = 0;
  const bool giroOk = imu.lerTaxaGiroZ(giroDecimosDps);
  const bool acelOk = imu.lerAceleracaoPlano(axMg, ayMg);
  Serial.print("MPU6050");
  if (giroOk) {
    Serial.printf(" | giro Z: %.1f graus/s", giroDecimosDps / 10.0f);
  } else {
    Serial.print(" | giro Z: falha na leitura");
  }
  if (acelOk) {
    Serial.printf(" | aceleracao X: %d mg, Y: %d mg", axMg, ayMg);
  } else {
    Serial.print(" | aceleracao: falha na leitura");
  }
  Serial.println();
}

void lerImuContinuo() {
  Serial.println("MPU6050 continuo. Gire e incline o robo. Parado, o giro fica perto de 0. Qualquer tecla para.");
  while (!teclaPressionada()) {
    imprimirImu();
    delay(PERIODO_FLUXO_MS / 2);
  }
  Serial.println("Fim da leitura continua.");
}

void imprimirEncoders() {
  const int32_t esq = encoderEsq.lerContagem();
  const int32_t dir = encoderDir.lerContagem();
  Serial.printf("Encoder | esquerdo: %ld pulsos (%ld mm, %lu invalidas) | direito: %ld pulsos (%ld mm, %lu invalidas)\n",
                static_cast<long>(esq), static_cast<long>(conversor.pulsosParaMm(esq)),
                static_cast<unsigned long>(encoderEsq.transicoesInvalidas()), static_cast<long>(dir),
                static_cast<long>(conversor.pulsosParaMm(dir)),
                static_cast<unsigned long>(encoderDir.transicoesInvalidas()));
}

void lerEncodersContinuo() {
  Serial.println("Encoders (contagem desde o ultimo zero). Gire cada roda a mao para a frente: a contagem deve subir.");
  Serial.println("Os mm usam a constante provisoria de micrometros por pulso. Qualquer tecla para.");
  while (!teclaPressionada()) {
    imprimirEncoders();
    delay(PERIODO_FLUXO_MS);
  }
  Serial.println("Fim da leitura continua.");
}

void zerarEncoders() {
  encoderEsq.zerar();
  encoderDir.zerar();
  Serial.println("Encoders zerados.");
}

/// Soma dos itens do autoteste.
struct ResumoAutoteste {
  uint8_t ok = 0;
  uint8_t falhas = 0;
};

/// Imprime um item do autoteste; se falhou, imprime também a dica de diagnóstico.
void registrar(ResumoAutoteste& resumo, bool ok, const char* dica, const char* formato, ...) {
  char texto[120];
  va_list argumentos;
  va_start(argumentos, formato);
  vsnprintf(texto, sizeof(texto), formato, argumentos);
  va_end(argumentos);
  if (ok) {
    resumo.ok++;
    Serial.printf("  [OK]    %s\n", texto);
  } else {
    resumo.falhas++;
    Serial.printf("  [FALHA] %s\n          dica: %s\n", texto, dica);
  }
}

/// `true` se há um dispositivo respondendo no endereço I²C.
bool respondeNoBarramento(uint8_t endereco) {
  Wire.beginTransmission(endereco);
  return Wire.endTransmission() == 0;
}

/// Confere os sensores e os encoders com o robô parado. Não aciona os motores.
void autoteste() {
  ResumoAutoteste resumo;
  Serial.println("AUTOTESTE (robo parado; os motores nao sao acionados)");

  Serial.println("1) Barramento I2C");
  struct Esperado {
    uint8_t endereco;
    const char* nome;
  };
  const Esperado esperados[] = {{pinos::ENDERECO_TOF_ESQ, "VL53L0X esquerdo"},
                                {pinos::ENDERECO_TOF_FRONTAL, "VL53L0X frontal"},
                                {pinos::ENDERECO_TOF_DIR, "VL53L0X direito"},
                                {pinos::ENDERECO_MPU6050, "MPU6050"}};
  for (const Esperado& esperado : esperados) {
    registrar(resumo, respondeNoBarramento(esperado.endereco),
              "confira SDA, SCL, a alimentacao do modulo e, nos ToF, o XSHUT; a tecla 2 reinicia os sensores",
              "0x%02X %s responde", esperado.endereco, esperado.nome);
  }
  registrar(resumo, !respondeNoBarramento(pinos::ENDERECO_FABRICA_TOF),
            "um VL53L0X ficou no endereco de fabrica 0x29: reinicie com a tecla 2 e confira o XSHUT dele",
            "nenhum VL53L0X ficou no endereco de fabrica (0x29)");

  Serial.println("2) Sensores de distancia (ToF)");
  lerTofPor(AUTOTESTE_ESPERA_TOF_MS);
  for (uint8_t i = 0; i < 3; i++) {
    const AmostraTof& amostra = tof.amostra(LADOS[i]);
    const bool mediu = tof.sensorIniciado(LADOS[i]) && (amostra.estado == EstadoAmostraTof::Valida ||
                                                        amostra.estado == EstadoAmostraTof::AbaixoDoMinimo ||
                                                        amostra.estado == EstadoAmostraTof::ForaDeAlcance);
    char leitura[32];
    if (amostra.estado == EstadoAmostraTof::Valida) {
      snprintf(leitura, sizeof(leitura), "%u mm", amostra.distanciaMm);
    } else {
      snprintf(leitura, sizeof(leitura), "%s", tof.sensorIniciado(LADOS[i]) ? nomeEstado(amostra.estado) : "nao iniciado");
    }
    registrar(resumo, mediu, "sem medida: confira fiacao, XSHUT e alimentacao de 3,3 V; tente a tecla 2 e a varredura (tecla 1)",
              "ToF %s: %s", NOMES[i], leitura);
  }

  Serial.println("3) MPU6050");
  int32_t somaDecimos = 0;
  uint8_t leiturasOk = 0;
  for (uint8_t i = 0; i < AUTOTESTE_LEITURAS_GIRO; i++) {
    int16_t decimos = 0;
    if (imu.lerTaxaGiroZ(decimos)) {
      somaDecimos += decimos;
      leiturasOk++;
    }
    delay(10);
  }
  registrar(resumo, imu.iniciado() && leiturasOk == AUTOTESTE_LEITURAS_GIRO,
            "o sensor nao respondeu: confira SDA, SCL, 3,3 V e o pino AD0 (endereco 0x68); tente a tecla 2",
            "MPU6050 responde (WHO_AM_I 0x%02X, %u de %u leituras)", imu.identificacao(), leiturasOk,
            AUTOTESTE_LEITURAS_GIRO);
  if (leiturasOk > 0) {
    const int32_t mediaDecimos = somaDecimos / leiturasOk;
    registrar(resumo, labs(mediaDecimos) <= AUTOTESTE_GIRO_PARADO_MAX_DECIMOS,
              "o robo precisa estar parado; se estiver, o vies do giroscopio e alto (a calibracao do boot corrige)",
              "giro Z parado: %.1f graus/s (limite %.1f)", mediaDecimos / 10.0f,
              AUTOTESTE_GIRO_PARADO_MAX_DECIMOS / 10.0f);
  }
  int16_t axMg = 0;
  int16_t ayMg = 0;
  registrar(resumo, imu.lerAceleracaoPlano(axMg, ayMg), "falha ao ler o acelerometro: confira o I2C e reinicie com a tecla 2",
            "aceleracao X=%d mg, Y=%d mg", axMg, ayMg);

  Serial.println("4) Encoders (parados)");
  const uint32_t invalidasEsq = encoderEsq.transicoesInvalidas();
  const uint32_t invalidasDir = encoderDir.transicoesInvalidas();
  encoderEsq.zerar();
  encoderDir.zerar();
  delay(AUTOTESTE_ESPERA_ENCODER_MS);
  const int32_t pulsosEsq = encoderEsq.lerContagem();
  const int32_t pulsosDir = encoderDir.lerContagem();
  const uint32_t novasEsq = encoderEsq.transicoesInvalidas() - invalidasEsq;
  const uint32_t novasDir = encoderDir.transicoesInvalidas() - invalidasDir;
  registrar(resumo, pulsosEsq == 0 && novasEsq == 0,
            "pulsos sem a roda girar: ruido ou cabo solto; confira GND, VCC (3,3 V) e A/B do encoder",
            "encoder esquerdo estavel (%ld pulsos, %lu transicoes invalidas)", static_cast<long>(pulsosEsq),
            static_cast<unsigned long>(novasEsq));
  registrar(resumo, pulsosDir == 0 && novasDir == 0,
            "pulsos sem a roda girar: ruido ou cabo solto; confira GND, VCC (3,3 V) e A/B do encoder",
            "encoder direito estavel (%ld pulsos, %lu transicoes invalidas)", static_cast<long>(pulsosDir),
            static_cast<unsigned long>(novasDir));

  Serial.printf("RESULTADO: %u OK, %u FALHA(S).\n", resumo.ok, resumo.falhas);
  Serial.println("Nao verificado aqui: motores (tecla 7) e o sentido dos encoders (gire a roda a mao com a tecla 9).");
}

void imprimirPinagem() {
  Serial.println("Pinagem (Pinos.h, Rev 2.0):");
  Serial.printf("  I2C: SDA=GPIO%d, SCL=GPIO%d\n", pinos::I2C_SDA, pinos::I2C_SCL);
  Serial.printf("  XSHUT: frente=GPIO%d, esquerda=GPIO%d, direita=GPIO%d\n", pinos::XSHUT_FRONTAL,
                pinos::XSHUT_ESQ, pinos::XSHUT_DIR);
  Serial.printf("  Motor esquerdo: IN1=GPIO%d, IN2=GPIO%d\n", pinos::MOTOR_ESQ_IN1, pinos::MOTOR_ESQ_IN2);
  Serial.printf("  Motor direito: IN3=GPIO%d, IN4=GPIO%d\n", pinos::MOTOR_DIR_IN3, pinos::MOTOR_DIR_IN4);
  Serial.printf("  Encoders: esq A=GPIO%d B=GPIO%d, dir A=GPIO%d B=GPIO%d (entradas com pull-up, no maximo 3,3 V)\n",
                pinos::ENCODER_ESQ_A, pinos::ENCODER_ESQ_B, pinos::ENCODER_DIR_A, pinos::ENCODER_DIR_B);
}

/// Diz o que a contagem de uma roda mostra, comparando com o sinal da potência comandada.
void avaliarEncoder(const char* roda, int32_t pulsos, int16_t comando) {
  Serial.printf("  encoder %s: %ld pulsos (%ld mm)", roda, static_cast<long>(pulsos),
                static_cast<long>(conversor.pulsosParaMm(pulsos)));
  if (comando == 0) {
    Serial.println();
    return;
  }
  const int32_t modulo = pulsos < 0 ? -pulsos : pulsos;
  if (modulo < PULSOS_MINIMOS_NO_TESTE) {
    Serial.println(" -> SEM PULSOS: confira VCC, GND, A e B do encoder e se a roda girou.");
  } else if ((pulsos > 0) == (comando > 0)) {
    Serial.println(" -> sinal coerente com o comando.");
  } else {
    Serial.println(" -> SINAL OPOSTO ao comando. Se a roda andou ao contrario, o motor esta invertido;");
    Serial.println("    se andou para o lado certo, o encoder esta com A e B trocados (ou inverta o sentido no codigo).");
  }
}

/// Aciona os motores por `DURACAO_TESTE_MOTOR_MS` ou até alguma tecla, e freia no fim.
void acionar(const char* descricao, int16_t esquerda, int16_t direita) {
  Serial.printf("> %s: esquerda %d, direita %d (duty efetivo %d%%). Qualquer tecla freia.\n", descricao,
                esquerda, direita, dutyPercentual(esquerda != 0 ? esquerda : direita));
  encoderEsq.zerar();
  encoderDir.zerar();
  motores.definirPotencia({esquerda, direita});
  const uint32_t inicioMs = millis();
  bool interrompido = false;
  while (millis() - inicioMs < DURACAO_TESTE_MOTOR_MS) {
    if (teclaPressionada()) {
      interrompido = true;
      break;
    }
    delay(5);
  }
  motores.parar();
  Serial.println(interrompido ? "> freado por tecla." : "> tempo esgotado, motores freados.");
  avaliarEncoder("esquerdo", encoderEsq.lerContagem(), esquerda);
  avaliarEncoder("direito", encoderDir.lerContagem(), direita);
}

/// Sobe a potência de um motor em degraus e diz em qual a roda começou a girar, pelo encoder.
/// Serve para achar a zona morta do N20 e para separar "motor sem torque" de "motor sem energia".
void rampa(bool esquerdo) {
  IEncoder& encoder = esquerdo ? static_cast<IEncoder&>(encoderEsq) : static_cast<IEncoder&>(encoderDir);
  Serial.printf("> rampa no motor %s, de %d a %d. Qualquer tecla freia.\n", esquerdo ? "esquerdo" : "direito",
                RAMPA_INICIO, POTENCIA_MAXIMA);
  int16_t girouEm = 0;
  bool interrompido = false;
  for (int16_t p = RAMPA_INICIO; p <= POTENCIA_MAXIMA && !interrompido; p += RAMPA_PASSO) {
    encoder.zerar();
    motores.definirPotencia({esquerdo ? p : int16_t{0}, esquerdo ? int16_t{0} : p});
    const uint32_t inicioMs = millis();
    while (millis() - inicioMs < RAMPA_DEGRAU_MS) {
      if (teclaPressionada()) {
        interrompido = true;
        break;
      }
      delay(5);
    }
    const int32_t pulsos = encoder.lerContagem();
    const int32_t modulo = pulsos < 0 ? -pulsos : pulsos;
    Serial.printf("  potencia %4d (duty %2d%%): %ld pulsos\n", p, dutyPercentual(p), static_cast<long>(pulsos));
    if (girouEm == 0 && modulo >= RAMPA_PULSOS_PARA_GIRAR) {
      girouEm = p;
    }
  }
  motores.parar();
  if (girouEm > 0) {
    Serial.printf("> a roda comecou a girar na potencia %d (duty %d%%).\n", girouEm, dutyPercentual(girouEm));
  } else if (interrompido) {
    Serial.println("> freado por tecla.");
  } else {
    Serial.println("> a roda NAO girou em nenhuma potencia. Se o motor nem zumbe: confira VM, GND, nSLEEP e os fios OUT.");
    Serial.println("  Se zumbe e nao gira: falta corrente na fonte ou o motor esta travado.");
    Serial.println("  Se gira e o encoder nao conta: o problema esta no encoder (VCC, GND, A, B).");
  }
}

/// Modo contínuo: aciona o motor sem o limite de 2 s, mostra os pulsos e a velocidade dos encoders
/// a cada `PERIODO_FLUXO_MS` e deixa trocar motor, sentido e potência sem parar. Para ao receber
/// qualquer outra tecla, ao fechar o monitor ou depois de `TEMPO_MAXIMO_CONTINUO_MS`.
void modoContinuo() {
  Alvo alvo = Alvo::Esquerdo;
  bool re = false;
  Serial.println("> MODO CONTINUO. Teclas: e = motor esquerdo, d = motor direito, b = os dois,");
  Serial.println(">   r = inverte o sentido, + e - = potencia. Qualquer outra tecla (espaco) freia.");
  encoderEsq.zerar();
  encoderDir.zerar();
  const uint32_t inicioMs = millis();
  uint32_t ultimaImpressaoMs = inicioMs;
  int32_t anteriorEsq = 0;
  int32_t anteriorDir = 0;
  const char* motivo = "tempo maximo atingido";
  while (millis() - inicioMs < TEMPO_MAXIMO_CONTINUO_MS) {
    bool parar = false;
    while (Serial.available() > 0 && !parar) {
      switch (static_cast<char>(Serial.read())) {
        case '\r':
        case '\n':
          break;
        case 'e':
        case 'E':
          alvo = Alvo::Esquerdo;
          break;
        case 'd':
        case 'D':
          alvo = Alvo::Direito;
          break;
        case 'b':
        case 'B':
          alvo = Alvo::Ambos;
          break;
        case 'r':
        case 'R':
          re = !re;
          break;
        case '+':
        case '=':
          potencia = limitarPotencia(potencia + POTENCIA_PASSO);
          break;
        case '-':
          potencia = potencia - POTENCIA_PASSO < POTENCIA_MINIMA ? POTENCIA_MINIMA : potencia - POTENCIA_PASSO;
          break;
        default:
          parar = true;
          break;
      }
    }
    if (parar) {
      motivo = "freado por tecla";
      break;
    }
    if (!Serial) {
      motivo = "monitor serial fechado";
      break;
    }
    const int16_t comando = re ? -potencia : potencia;
    motores.definirPotencia({alvo == Alvo::Direito ? int16_t{0} : comando,
                             alvo == Alvo::Esquerdo ? int16_t{0} : comando});

    const uint32_t agoraMs = millis();
    if (agoraMs - ultimaImpressaoMs >= PERIODO_FLUXO_MS) {
      const uint32_t dtMs = agoraMs - ultimaImpressaoMs;
      ultimaImpressaoMs = agoraMs;
      const int32_t esq = encoderEsq.lerContagem();
      const int32_t dir = encoderDir.lerContagem();
      const int32_t pulsosPorSEsq = static_cast<int32_t>((esq - anteriorEsq) * 1000L / static_cast<int32_t>(dtMs));
      const int32_t pulsosPorSDir = static_cast<int32_t>((dir - anteriorDir) * 1000L / static_cast<int32_t>(dtMs));
      anteriorEsq = esq;
      anteriorDir = dir;
      Serial.printf("[%s, %s, potencia %d, duty %d%%] esq: %ld pulsos, %ld p/s (%ld mm/s) | dir: %ld pulsos, %ld p/s (%ld mm/s)\n",
                    alvo == Alvo::Esquerdo ? "esquerdo" : alvo == Alvo::Direito ? "direito" : "os dois",
                    re ? "re" : "frente", comando, dutyPercentual(comando), static_cast<long>(esq),
                    static_cast<long>(pulsosPorSEsq), static_cast<long>(conversor.pulsosParaMm(pulsosPorSEsq)),
                    static_cast<long>(dir), static_cast<long>(pulsosPorSDir),
                    static_cast<long>(conversor.pulsosParaMm(pulsosPorSDir)));
    }
    delay(5);
  }
  motores.parar();
  Serial.printf("> motores freados (%s). Potencia ficou em %d.\n", motivo, potencia);
}

void executarMotores(char tecla) {
  switch (tecla) {
    case '1':
      acionar("motor esquerdo para a frente", potencia, 0);
      break;
    case '2':
      acionar("motor esquerdo para tras", -potencia, 0);
      break;
    case '3':
      acionar("motor direito para a frente", 0, potencia);
      break;
    case '4':
      acionar("motor direito para tras", 0, -potencia);
      break;
    case '5':
      acionar("os dois para a frente", potencia, potencia);
      break;
    case '6':
      acionar("os dois para tras", -potencia, -potencia);
      break;
    case '7':
      acionar("giro para a direita", potencia, -potencia);
      break;
    case '8':
      acionar("giro para a esquerda", -potencia, potencia);
      break;
    case 'c':
    case 'C':
      modoContinuo();
      break;
    case '9':
      rampa(true);
      break;
    case 'a':
    case 'A':
      rampa(false);
      break;
    case '+':
    case '=':
      potencia = limitarPotencia(potencia + POTENCIA_PASSO);
      Serial.printf("Potencia: %d de 1000\n", potencia);
      break;
    case '-':
      potencia = potencia - POTENCIA_PASSO < POTENCIA_MINIMA ? POTENCIA_MINIMA : potencia - POTENCIA_PASSO;
      Serial.printf("Potencia: %d de 1000\n", potencia);
      break;
    case '0':
      tela = Tela::Principal;
      imprimirMenuPrincipal();
      break;
    default:
      imprimirMenuMotores();
      break;
  }
}

void executarPrincipal(char tecla) {
  switch (tecla) {
    case '1':
      varrerBarramento();
      break;
    case '2':
      reiniciarSensores();
      break;
    case '3':
      lerTofUmaVez();
      break;
    case '4':
      lerTofContinuo();
      break;
    case '5':
      imprimirImu();
      break;
    case '6':
      lerImuContinuo();
      break;
    case '7':
      tela = Tela::Motores;
      imprimirMenuMotores();
      break;
    case '8':
      imprimirPinagem();
      break;
    case 't':
    case 'T':
      autoteste();
      break;
    case '9':
      lerEncodersContinuo();
      break;
    case '0':
      zerarEncoders();
      break;
    case 'r':
    case 'R':
      Serial.println("Reiniciando a placa... (se o monitor fechar, abra de novo)");
      Serial.flush();
      motores.parar();
      delay(200);
      ESP.restart();
      break;
    default:
      imprimirMenuPrincipal();
      break;
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);
  motores.iniciar();  // freados durante toda a bancada, até o submenu de motores
  barramento.iniciar(pinos::I2C_SDA, pinos::I2C_SCL);  // inicia o `Wire`
  encoderEsq.iniciar();
  encoderDir.iniciar();
  delay(500);  // tempo para o monitor serial abrir a porta USB
  reiniciarSensores();
  imprimirMenuPrincipal();
}

void loop() {
  static char anterior = 0;
  static bool monitorAberto = false;
  // Mostra o menu sozinho quando o monitor serial abre, sem precisar digitar nada.
  if (static_cast<bool>(Serial) != monitorAberto) {
    monitorAberto = static_cast<bool>(Serial);
    if (monitorAberto) {
      delay(300);
      tela == Tela::Motores ? imprimirMenuMotores() : imprimirMenuPrincipal();
    }
  }
  while (Serial.available() > 0) {
    const char tecla = static_cast<char>(Serial.read());
    const bool fimDeLinha = tecla == '\r' || tecla == '\n';
    // Enter pode chegar como CR, LF ou CRLF: mostra o menu uma vez só.
    if (fimDeLinha && !(tecla == '\n' && anterior == '\r')) {
      tela == Tela::Motores ? imprimirMenuMotores() : imprimirMenuPrincipal();
    } else if (!fimDeLinha) {
      tela == Tela::Motores ? executarMotores(tecla) : executarPrincipal(tecla);
    }
    anterior = tecla;
  }
  delay(1);
}
