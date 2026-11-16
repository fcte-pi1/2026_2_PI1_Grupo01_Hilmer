/// @file bancada_geral.cpp
/// @brief Firmware de bancada geral: menu de uma tecla pela serial para conferir, no hardware
/// montado, os 3 VL53L0X, o MPU6050, os dois motores e os encoders, com os mesmos drivers do robô.
///
/// Não é o firmware do robô: só entra no ambiente `bancada_geral_devkit` do `platformio.ini`. Grave e
/// abra o monitor com `pio run -e bancada_geral_devkit -t upload && pio device monitor -e bancada_geral_devkit`.
/// Digite só a tecla do menu (não precisa de Enter; Enter mostra o menu de novo).
///
/// SEGURANÇA: os motores ficam freados no boot e só giram dentro do submenu de motores, por
/// `DURACAO_TESTE_MOTOR_MS` no máximo. Qualquer tecla (menos Enter) freia os motores na hora.
/// Para o primeiro teste, deixe as rodas fora do chão.
///
/// Placa: ESP32 DevKit V1, com a serial pelo conversor USB da placa (UART0). Além dos sensores e dos
/// motores, confere o botão de largada, a tensão da bateria e a telemetria. A pinagem está em `Pinos.h`.

#include <Arduino.h>
#include <Wire.h>
#include <stdarg.h>
#include <stdlib.h>

#include "BarramentoI2c.h"
#include "ConsoleBancada.h"
#include "ConfiguracaoMotores.h"
#include "ConversorDeslocamento.h"
#include "EncoderEsp32.h"
#include "ImuMpu6050.h"
#include "MotoresDrv8833.h"
#include "Pinos.h"
#include "TofVl53l0x.h"

// Telemetria na mesma bancada (Wi-Fi, HTTPS e gravacao no Supabase): so quando o ambiente define
// `BANCADA_COM_TELEMETRIA` (o `bancada_geral_devkit`). Precisa de `include/credenciais.h`; sem ele, o
// firmware compila e o menu de telemetria explica o que fazer.
#if defined(BANCADA_COM_TELEMETRIA)
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFi.h>
#include <string.h>
#include <strings.h>

#if __has_include("credenciais.h")
#include "credenciais.h"
#define BANCADA_TEM_CREDENCIAIS 1
#else
#define BANCADA_TEM_CREDENCIAIS 0
#endif

#include "AmostraTelemetria.h"
#include "ConfiguracaoRede.h"
#include "ControladorEnvio.h"
#include "CredenciaisSupabase.h"
#include "EnvioHttpSupabase.h"
#include "ReconexaoWifi.h"
#include "RedeWifiEsp32.h"
#include "TravaSecaoCritica.h"
#endif

using namespace micromouse;

namespace {

/// Saída e entrada da bancada: a serial USB e, com Wi-Fi, um telnet espelhado (`ConsoleBancada.h`).
ConsoleBancada console;

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

/// Divisor da bateria (R1 de 100 kohm no positivo, R2 de 33 kohm no GND) e faixa da LiPo 2S. A
/// tensão no pino é a da bateria vezes R2 / (R1 + R2), e só vale com a chave geral ligada.
constexpr uint32_t BATERIA_R1_OHMS = 100000;
constexpr uint32_t BATERIA_R2_OHMS = 33000;
constexpr uint32_t BATERIA_LEITURAS = 16;
constexpr uint32_t BATERIA_CORTE_MV = 6600;   ///< Abaixo disso a LiPo 2S está descarregada.
constexpr uint32_t BATERIA_PLENA_MV = 8400;   ///< Carga plena da LiPo 2S.
constexpr uint32_t BATERIA_MAXIMA_VALIDA_MV = 8700;

/// Tensão nominal do VM (buck U4) e passo do limite de duty. O multímetro, no motor, lê a tensão
/// média do PWM: duty × VM. Com o limite do robô (70%) e VM de 6 V, o máximo é 4,2 V.
constexpr uint32_t VM_NOMINAL_MV = 6000;
constexpr uint16_t LIMITE_DUTY_PASSO_MILESIMOS = 100;
constexpr uint16_t LIMITE_DUTY_MINIMO_MILESIMOS = 100;
constexpr uint16_t LIMITE_DUTY_MAXIMO_MILESIMOS = 1000;

/// Teste integrado: duração com os dois motores ligados, e quanto a bateria pode cair (em mV) antes de
/// a bancada avisar que a fonte não aguenta os motores.
constexpr uint32_t TESTE_INTEGRADO_DURACAO_MS = 5000;
constexpr uint32_t QUEDA_MAXIMA_BATERIA_MV = 1000;

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

/// Na UART do DevKit, `if (!Serial)` não percebe o monitor fechar (ele só confere se a UART está
/// instalada). Por isso o modo contínuo exige uma tecla (`m` mantém) a cada `PRAZO_SEM_TECLA_MS`.
constexpr uint32_t PRAZO_SEM_TECLA_MS = 10000;

/// Duty efetivo, em %, de uma potência (o driver limita o duty a `LIMITE_DUTY_MOTORES_MILESIMOS`).
/// Limite de duty da bancada, em milésimos. Começa no do robô; as teclas `l` e `k` o mudam.
uint16_t limiteDutyMilesimos = LIMITE_DUTY_MOTORES_MILESIMOS;

int dutyPercentual(int16_t potencia) {
  return (potencia < 0 ? -potencia : potencia) * static_cast<int>(limiteDutyMilesimos) / 10000;
}

/// Tensão média estimada nos terminais do motor, em décimos de volt: duty × VM nominal.
int tensaoMediaDecivolts(int16_t potencia) { return dutyPercentual(potencia) * static_cast<int>(VM_NOMINAL_MV) / 1000; }

constexpr Lado LADOS[3] = {Lado::Frente, Lado::Esquerda, Lado::Direita};
constexpr const char* NOMES[3] = {"frente", "esquerda", "direita"};

enum class Tela : uint8_t { Principal, Motores, Telemetria };

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
  while (console.available() > 0) {
    const char c = static_cast<char>(console.read());
    if (c != '\r' && c != '\n') {
      pressionada = true;
    }
  }
  return pressionada;
}

void imprimirMenuPrincipal() {
  console.println();
  console.println("=== BANCADA GERAL: sensores, motores e encoders ===");
  console.println(" Build: " __DATE__ " " __TIME__);
  console.println(" t  AUTOTESTE: confere sensores e encoders parados (nao gira motor)");
  console.println(" 1  varredura I2C (quem responde no barramento)");
  console.println(" 2  reiniciar os ToF e o MPU6050");
  console.println(" 3  ToF: uma leitura dos 3 sensores");
  console.println(" 4  ToF: leitura continua (qualquer tecla para)");
  console.println(" 5  MPU6050: uma leitura (giro Z e aceleracao X/Y)");
  console.println(" 6  MPU6050: leitura continua (qualquer tecla para)");
  console.println(" 7  motores (abre o menu de motores)");
  console.println(" i  TESTE INTEGRADO: liga os dois motores e le ToF, MPU6050 e encoders ao mesmo tempo");
  console.println(" 8  pinagem (GPIO de cada sinal)");
  console.println(" 9  encoders: contagem continua, gire a roda a mao (qualquer tecla para)");
  console.println(" 0  zerar a contagem dos encoders");
  console.println(" a  bateria: tensao pelo divisor no GPIO34");
  console.println(" b  botao de largada: mostra quando aperta e solta (qualquer tecla para)");
#if defined(BANCADA_COM_TELEMETRIA)
  console.println(" w  telemetria: Wi-Fi, HTTPS e banco (submenu; grava corrida de verdade no banco do credenciais.h)");
#endif
  console.println(" r  reiniciar a placa");
  console.println(" Enter  mostra este menu");
}

void imprimirMenuMotores() {
  console.println();
  console.println("=== MOTORES (cada teste dura no maximo 2 s; qualquer tecla freia) ===");
  console.printf(" Potencia atual: %d de 1000 (duty efetivo %d%%, tensao media estimada %d.%d V com VM de 6 V)\n", potencia,
                dutyPercentual(potencia), tensaoMediaDecivolts(potencia) / 10, tensaoMediaDecivolts(potencia) % 10);
  console.printf(" Limite de duty: %u%% (o do robo e %u%%): a potencia 1000 vira %u%% do VM\n",
                static_cast<unsigned>(limiteDutyMilesimos / 10), static_cast<unsigned>(LIMITE_DUTY_MOTORES_MILESIMOS / 10),
                static_cast<unsigned>(limiteDutyMilesimos / 10));
  console.println(" 1  motor esquerdo para a frente     2  motor esquerdo para tras");
  console.println(" 3  motor direito para a frente      4  motor direito para tras");
  console.println(" 5  os dois para a frente            6  os dois para tras");
  console.println(" 7  giro no lugar para a direita     8  giro no lugar para a esquerda");
  console.println(" 9  rampa no motor esquerdo          a  rampa no motor direito");
  console.println("    (a rampa sobe a potencia aos poucos e diz em qual a roda comecou a girar)");
  console.println(" c  modo continuo: fica acionando e mostrando os pulsos ao vivo (ate 60 s)");
  console.println(" +  aumenta a potencia               -  diminui a potencia");
  console.println(" l  aumenta o limite de duty (+10%)  k  diminui o limite de duty (-10%)");
  console.println("    (mais tensao nos motores: o multimetro no motor le duty x VM; acima de 70% so para teste)");
  console.println(" 0  voltar ao menu principal");
  console.println(" ATENCAO: o primeiro teste com as rodas fora do chao.");
}

/// Varre os endereços de 7 bits e diz quem respondeu, comparando com o que a pinagem espera.
void varrerBarramento() {
  console.printf("Varredura I2C (SDA=GPIO%d, SCL=GPIO%d):\n", pinos::I2C_SDA, pinos::I2C_SCL);
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
    console.printf("  0x%02X  %s\n", endereco, quem);
  }
  console.printf("%u dispositivo(s). Esperado no robo: 0x30, 0x31, 0x32 e 0x68.\n", encontrados);
}

void reiniciarSensores() {
  console.println("Reiniciando os sensores...");
  const bool tofOk = tof.iniciar();
  console.printf("  ToF: frente %s, esquerda %s, direita %s\n",
                tof.sensorIniciado(Lado::Frente) ? "ok" : "FALHOU",
                tof.sensorIniciado(Lado::Esquerda) ? "ok" : "FALHOU",
                tof.sensorIniciado(Lado::Direita) ? "ok" : "FALHOU");
  const bool imuOk = imu.iniciar();
  if (imuOk) {
    console.printf("  MPU6050: ok (WHO_AM_I 0x%02X)\n", imu.identificacao());
  } else {
    console.println("  MPU6050: FALHOU (sem resposta no 0x68; confira SDA, SCL, energia e AD0)");
  }
  console.println(tofOk && imuOk ? "Todos os sensores responderam." : "Algum sensor nao respondeu.");
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
  console.print("ToF ");
  for (uint8_t i = 0; i < 3; i++) {
    if (!tof.sensorIniciado(LADOS[i])) {
      console.printf(" | %s: nao iniciado", NOMES[i]);
      continue;
    }
    const AmostraTof& amostra = tof.amostra(LADOS[i]);
    if (amostra.estado == EstadoAmostraTof::Valida) {
      console.printf(" | %s: %u mm", NOMES[i], amostra.distanciaMm);
    } else {
      console.printf(" | %s: %s", NOMES[i], nomeEstado(amostra.estado));
    }
  }
  console.println();
}

void lerTofUmaVez() {
  lerTofPor(ESPERA_LEITURA_TOF_MS);
  imprimirTof();
}

void lerTofContinuo() {
  console.println("ToF continuo. Passe a mao na frente de cada sensor. Qualquer tecla para.");
  uint32_t ultimaImpressaoMs = 0;
  while (!teclaPressionada()) {
    lerTofPor(PERIODO_LEITURA_TOF_MS);
    if (millis() - ultimaImpressaoMs >= PERIODO_FLUXO_MS) {
      ultimaImpressaoMs = millis();
      imprimirTof();
    }
  }
  console.println("Fim da leitura continua.");
}

void imprimirImu() {
  int16_t giroDecimosDps = 0;
  int16_t axMg = 0;
  int16_t ayMg = 0;
  const bool giroOk = imu.lerTaxaGiroZ(giroDecimosDps);
  const bool acelOk = imu.lerAceleracaoPlano(axMg, ayMg);
  console.print("MPU6050");
  if (giroOk) {
    console.printf(" | giro Z: %.1f graus/s", giroDecimosDps / 10.0f);
  } else {
    console.print(" | giro Z: falha na leitura");
  }
  if (acelOk) {
    console.printf(" | aceleracao X: %d mg, Y: %d mg", axMg, ayMg);
  } else {
    console.print(" | aceleracao: falha na leitura");
  }
  console.println();
}

void lerImuContinuo() {
  console.println("MPU6050 continuo. Gire e incline o robo. Parado, o giro fica perto de 0. Qualquer tecla para.");
  while (!teclaPressionada()) {
    imprimirImu();
    delay(PERIODO_FLUXO_MS / 2);
  }
  console.println("Fim da leitura continua.");
}

void imprimirEncoders() {
  const int32_t esq = encoderEsq.lerContagem();
  const int32_t dir = encoderDir.lerContagem();
  console.printf("Encoder | esquerdo: %ld pulsos (%ld mm, %lu invalidas) | direito: %ld pulsos (%ld mm, %lu invalidas)\n",
                static_cast<long>(esq), static_cast<long>(conversor.pulsosParaMm(esq)),
                static_cast<unsigned long>(encoderEsq.transicoesInvalidas()), static_cast<long>(dir),
                static_cast<long>(conversor.pulsosParaMm(dir)),
                static_cast<unsigned long>(encoderDir.transicoesInvalidas()));
}

void lerEncodersContinuo() {
  console.println("Encoders (contagem desde o ultimo zero). Gire cada roda a mao para a frente: a contagem deve subir.");
  console.println("Os mm usam a constante provisoria de micrometros por pulso. Qualquer tecla para.");
  while (!teclaPressionada()) {
    imprimirEncoders();
    delay(PERIODO_FLUXO_MS);
  }
  console.println("Fim da leitura continua.");
}

void zerarEncoders() {
  encoderEsq.zerar();
  encoderDir.zerar();
  console.println("Encoders zerados.");
}

/// Tensão da bateria em mV, pela média de `BATERIA_LEITURAS` leituras do ADC no divisor R1/R2.
uint32_t lerBateriaMv() {
  uint32_t somaPinoMv = 0;
  for (uint32_t i = 0; i < BATERIA_LEITURAS; i++) {
    somaPinoMv += analogReadMilliVolts(pinos::BATERIA_ADC);
    delay(2);
  }
  const uint32_t pinoMv = somaPinoMv / BATERIA_LEITURAS;
  return pinoMv * (BATERIA_R1_OHMS + BATERIA_R2_OHMS) / BATERIA_R2_OHMS;
}

/// Uma leitura só (sem média), rápida o bastante para rodar dentro do laço dos testes de motor.
uint32_t lerBateriaRapidaMv() {
  return analogReadMilliVolts(pinos::BATERIA_ADC) * (BATERIA_R1_OHMS + BATERIA_R2_OHMS) / BATERIA_R2_OHMS;
}

void imprimirBateria() {
  const uint32_t bateriaMv = lerBateriaMv();
  const int32_t carga = (static_cast<int32_t>(bateriaMv) - static_cast<int32_t>(BATERIA_CORTE_MV)) * 100 /
                        static_cast<int32_t>(BATERIA_PLENA_MV - BATERIA_CORTE_MV);
  console.printf("Bateria: %.2f V (GPIO%d, divisor %lu/%lu ohms); carga estimada %ld%% (6,6 V a 8,4 V, aproximacao linear)\n",
                bateriaMv / 1000.0f, pinos::BATERIA_ADC, static_cast<unsigned long>(BATERIA_R1_OHMS),
                static_cast<unsigned long>(BATERIA_R2_OHMS), static_cast<long>(constrain(carga, 0, 100)));
  if (bateriaMv < 1000) {
    console.println("  aviso: quase 0 V. A chave geral esta desligada ou o divisor (R1, R2) nao esta ligado ao GPIO34.");
  } else if (bateriaMv < BATERIA_CORTE_MV) {
    console.println("  aviso: abaixo de 6,6 V. Recarregue a bateria antes de testar os motores.");
  }
}

/// Mostra o estado do botão de largada a cada mudança, até chegar uma tecla.
void lerBotaoContinuo() {
  pinMode(pinos::BOTAO_START, INPUT_PULLUP);
  console.printf("Botao BTN_START (GPIO%d, pull-up interno). Aperte e solte o botao; qualquer tecla para.\n",
                pinos::BOTAO_START);
  int anterior = -1;
  while (!teclaPressionada()) {
    const int estado = digitalRead(pinos::BOTAO_START);
    if (estado != anterior) {
      anterior = estado;
      console.printf("  botao %s (nivel %s)\n", estado == LOW ? "PRESSIONADO" : "solto", estado == LOW ? "LOW" : "HIGH");
    }
    delay(5);
  }
  console.println("Fim da leitura do botao.");
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
    console.printf("  [OK]    %s\n", texto);
  } else {
    resumo.falhas++;
    console.printf("  [FALHA] %s\n          dica: %s\n", texto, dica);
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
  console.println("AUTOTESTE (robo parado; os motores nao sao acionados)");

  console.println("1) Barramento I2C");
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

  console.println("2) Sensores de distancia (ToF)");
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

  console.println("3) MPU6050");
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

  console.println("4) Encoders (parados)");
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

  console.println("5) Bateria e botao");
  const uint32_t bateriaMv = lerBateriaMv();
  registrar(resumo, bateriaMv >= BATERIA_CORTE_MV && bateriaMv <= BATERIA_MAXIMA_VALIDA_MV,
            "esperado entre 6,6 V e 8,4 V: perto de 0 V = chave geral desligada ou divisor solto no GPIO34; abaixo de 6,6 V = recarregue",
            "bateria %.2f V", bateriaMv / 1000.0f);
  pinMode(pinos::BOTAO_START, INPUT_PULLUP);
  delay(5);
  registrar(resumo, digitalRead(pinos::BOTAO_START) == HIGH,
            "o pino esta em LOW sem apertar: botao pressionado, em curto ou terminal ligado ao GND errado",
            "botao BTN_START solto (nivel alto)");

  console.printf("RESULTADO: %u OK, %u FALHA(S).\n", resumo.ok, resumo.falhas);
  console.printf("REGISTRO: autoteste | ok=%u falhas=%u | veredito=%s\n", resumo.ok, resumo.falhas,
                resumo.falhas == 0 ? "OK" : "FALHA");
  console.println("Nao verificado aqui: motores (tecla 7), o sentido dos encoders (gire a roda a mao com a tecla 9) e o aperto do botao (tecla b).");
}

void imprimirPinagem() {
  console.println("Pinagem (Pinos.h, ESP32 DevKit V1):");
  console.printf("  I2C: SDA=GPIO%d, SCL=GPIO%d\n", pinos::I2C_SDA, pinos::I2C_SCL);
  console.printf("  XSHUT: frente=GPIO%d, esquerda=GPIO%d, direita=GPIO%d\n", pinos::XSHUT_FRONTAL,
                pinos::XSHUT_ESQ, pinos::XSHUT_DIR);
  console.printf("  Motor esquerdo: IN1=GPIO%d, IN2=GPIO%d\n", pinos::MOTOR_ESQ_IN1, pinos::MOTOR_ESQ_IN2);
  console.printf("  Motor direito: IN3=GPIO%d, IN4=GPIO%d\n", pinos::MOTOR_DIR_IN3, pinos::MOTOR_DIR_IN4);
  console.printf("  Encoders: esq A=GPIO%d B=GPIO%d, dir A=GPIO%d B=GPIO%d (entradas com pull-up, no maximo 3,3 V)\n",
                pinos::ENCODER_ESQ_A, pinos::ENCODER_ESQ_B, pinos::ENCODER_DIR_A, pinos::ENCODER_DIR_B);
  console.printf("  Botao BTN_START: GPIO%d (pull-up). Bateria (ADC): GPIO%d\n", pinos::BOTAO_START, pinos::BATERIA_ADC);
}

/// Muda o limite de duty dos motores em `delta` milésimos e avisa o que isso significa.
void mudarLimiteDuty(int16_t delta) {
  int32_t novo = static_cast<int32_t>(limiteDutyMilesimos) + delta;
  if (novo < LIMITE_DUTY_MINIMO_MILESIMOS) {
    novo = LIMITE_DUTY_MINIMO_MILESIMOS;
  } else if (novo > LIMITE_DUTY_MAXIMO_MILESIMOS) {
    novo = LIMITE_DUTY_MAXIMO_MILESIMOS;
  }
  limiteDutyMilesimos = static_cast<uint16_t>(novo);
  motores.definirLimiteDuty(limiteDutyMilesimos);
  console.printf("Limite de duty: %u%% (tensao media maxima estimada %u.%u V com VM de 6 V)\n",
                static_cast<unsigned>(limiteDutyMilesimos / 10),
                static_cast<unsigned>(limiteDutyMilesimos * VM_NOMINAL_MV / 1000000UL),
                static_cast<unsigned>((limiteDutyMilesimos * VM_NOMINAL_MV / 100000UL) % 10));
  if (limiteDutyMilesimos > LIMITE_DUTY_MOTORES_MILESIMOS) {
    console.printf("  aviso: acima do limite do robo (%u%%). So para teste: com o N20 de 6 V, nao passe de VM x duty = 6 V.\n",
                  static_cast<unsigned>(LIMITE_DUTY_MOTORES_MILESIMOS / 10));
  }
}

/// Diz o que a contagem de uma roda mostra, comparando com o sinal da potência comandada.
void avaliarEncoder(const char* roda, int32_t pulsos, int16_t comando) {
  console.printf("  encoder %s: %ld pulsos (%ld mm)", roda, static_cast<long>(pulsos),
                static_cast<long>(conversor.pulsosParaMm(pulsos)));
  if (comando == 0) {
    console.println();
    return;
  }
  const int32_t modulo = pulsos < 0 ? -pulsos : pulsos;
  if (modulo < PULSOS_MINIMOS_NO_TESTE) {
    console.println(" -> SEM PULSOS: confira VCC, GND, A e B do encoder e se a roda girou.");
  } else if ((pulsos > 0) == (comando > 0)) {
    console.println(" -> sinal coerente com o comando.");
  } else {
    console.println(" -> SINAL OPOSTO ao comando. Se a roda andou ao contrario, o motor esta invertido;");
    console.println("    se andou para o lado certo, o encoder esta com A e B trocados (ou inverta o sentido no codigo).");
  }
}

/// Aciona os motores por `DURACAO_TESTE_MOTOR_MS` ou até alguma tecla, e freia no fim.
void acionar(const char* descricao, int16_t esquerda, int16_t direita) {
  console.printf("> %s: esquerda %d, direita %d (duty efetivo %d%%). Qualquer tecla freia.\n", descricao,
                esquerda, direita, dutyPercentual(esquerda != 0 ? esquerda : direita));
  encoderEsq.zerar();
  encoderDir.zerar();
  const uint32_t bateriaAntesMv = lerBateriaMv();
  uint32_t bateriaMinimaMv = bateriaAntesMv;
  motores.definirPotencia({esquerda, direita});
  const uint32_t inicioMs = millis();
  bool interrompido = false;
  while (millis() - inicioMs < DURACAO_TESTE_MOTOR_MS) {
    if (teclaPressionada()) {
      interrompido = true;
      break;
    }
    const uint32_t bateriaMv = lerBateriaRapidaMv();
    if (bateriaMv > 1000 && bateriaMv < bateriaMinimaMv) {
      bateriaMinimaMv = bateriaMv;
    }
    delay(5);
  }
  motores.parar();
  console.println(interrompido ? "> freado por tecla." : "> tempo esgotado, motores freados.");
  console.printf("  bateria: %.2f V parado, %.2f V no minimo com o motor ligado (queda de %.2f V)\n",
                bateriaAntesMv / 1000.0f, bateriaMinimaMv / 1000.0f, (bateriaAntesMv - bateriaMinimaMv) / 1000.0f);
  avaliarEncoder("esquerdo", encoderEsq.lerContagem(), esquerda);
  avaliarEncoder("direito", encoderDir.lerContagem(), direita);
}

/// Liga os dois motores e, ao mesmo tempo, lê ToF, MPU6050 e encoders (e a bateria, no DevKit): é o que
/// o robô faz andando. Aponta se o PWM dos motores derruba a leitura dos sensores, se os dois motores
/// giram juntos e se a bateria aguenta. O resultado sai em itens `[OK]`/`[FALHA]` e numa linha `REGISTRO`.
void testeIntegrado() {
  console.printf("TESTE INTEGRADO: os dois motores para a frente (potencia %d, duty %d%%) por %lu s,\n"
                "lendo ToF, MPU6050 e encoders. Rodas fora do chao. Qualquer tecla freia.\n",
                potencia, dutyPercentual(potencia), static_cast<unsigned long>(TESTE_INTEGRADO_DURACAO_MS / 1000));
  lerTofPor(ESPERA_LEITURA_TOF_MS);  // os ToF precisam de leituras contínuas para ter amostra

  uint32_t leiturasTof = 0;
  uint32_t falhasTof = 0;
  uint32_t leiturasImu = 0;
  uint32_t falhasImu = 0;
  uint16_t menorMm[3] = {UINT16_MAX, UINT16_MAX, UINT16_MAX};
  uint16_t maiorMm[3] = {0, 0, 0};
  const uint32_t invalidasEsqAntes = encoderEsq.transicoesInvalidas();
  const uint32_t invalidasDirAntes = encoderDir.transicoesInvalidas();
  const uint32_t bateriaAntesMv = lerBateriaMv();
  uint32_t bateriaMinimaMv = bateriaAntesMv;

  encoderEsq.zerar();
  encoderDir.zerar();
  motores.definirPotencia({potencia, potencia});
  const uint32_t inicioMs = millis();
  bool interrompido = false;
  while (millis() - inicioMs < TESTE_INTEGRADO_DURACAO_MS) {
    if (teclaPressionada()) {
      interrompido = true;
      break;
    }
    DistanciasLaterais descartada;
    tof.lerDistanciasLaterais(descartada);
    for (uint8_t i = 0; i < 3; i++) {
      const AmostraTof& amostra = tof.amostra(LADOS[i]);
      leiturasTof++;
      if (amostra.estado == EstadoAmostraTof::Falha) {
        falhasTof++;
      } else if (amostra.estado == EstadoAmostraTof::Valida) {
        menorMm[i] = amostra.distanciaMm < menorMm[i] ? amostra.distanciaMm : menorMm[i];
        maiorMm[i] = amostra.distanciaMm > maiorMm[i] ? amostra.distanciaMm : maiorMm[i];
      }
    }
    int16_t giro = 0;
    leiturasImu++;
    if (!imu.lerTaxaGiroZ(giro)) {
      falhasImu++;
    }
    const uint32_t bateriaMv = lerBateriaRapidaMv();
    if (bateriaMv > 1000 && bateriaMv < bateriaMinimaMv) {
      bateriaMinimaMv = bateriaMv;
    }
    delay(PERIODO_LEITURA_TOF_MS);
  }
  motores.parar();
  const int32_t pulsosEsq = encoderEsq.lerContagem();
  const int32_t pulsosDir = encoderDir.lerContagem();
  const uint32_t invalidas = (encoderEsq.transicoesInvalidas() - invalidasEsqAntes) +
                             (encoderDir.transicoesInvalidas() - invalidasDirAntes);
  console.println(interrompido ? "> freado por tecla." : "> tempo esgotado, motores freados.");

  ResumoAutoteste resumo;
  registrar(resumo, labs(pulsosEsq) >= PULSOS_MINIMOS_NO_TESTE,
            "a roda esquerda nao girou: veja a rampa (menu de motores) e a tensao do VM",
            "roda esquerda girou: %ld pulsos", static_cast<long>(pulsosEsq));
  registrar(resumo, labs(pulsosDir) >= PULSOS_MINIMOS_NO_TESTE,
            "a roda direita nao girou: confira IN3 (GPIO27), IN4 (GPIO14), o VM e os fios OUT3/OUT4",
            "roda direita girou: %ld pulsos", static_cast<long>(pulsosDir));
  registrar(resumo, invalidas == 0,
            "transicoes invalidas com o motor ligado: ruido do PWM no encoder; confira GND, VCC de 3,3 V e a blindagem dos fios",
            "encoders sem transicoes invalidas (%lu)", static_cast<unsigned long>(invalidas));
  registrar(resumo, falhasTof == 0,
            "ToF falhando com o motor ligado: ruido ou queda de tensao no 3,3 V; confira GND comum e o capacitor do trilho",
            "ToF sem falhas (%lu leituras)", static_cast<unsigned long>(leiturasTof));
  registrar(resumo, falhasImu == 0,
            "o MPU6050 falhou com o motor ligado: ruido no I2C; afaste os fios do motor e confira os pull-ups",
            "MPU6050 sem falhas (%lu leituras)", static_cast<unsigned long>(leiturasImu));
  registrar(resumo, bateriaAntesMv - bateriaMinimaMv <= QUEDA_MAXIMA_BATERIA_MV,
            "a bateria cai demais com os motores ligados: a fonte nao aguenta a corrente; veja o buck do VM e o capacitor de bulk",
            "bateria %.2f V parado, %.2f V no minimo (queda de %.2f V, limite %.2f V)", bateriaAntesMv / 1000.0f,
            bateriaMinimaMv / 1000.0f, (bateriaAntesMv - bateriaMinimaMv) / 1000.0f, QUEDA_MAXIMA_BATERIA_MV / 1000.0f);
  for (uint8_t i = 0; i < 3; i++) {
    if (maiorMm[i] > 0) {
      console.printf("  ToF %s variou de %u a %u mm durante o teste\n", NOMES[i], menorMm[i], maiorMm[i]);
    }
  }
  console.printf("RESULTADO: %u OK, %u FALHA(S).\n", resumo.ok, resumo.falhas);
  console.printf("REGISTRO: integrado | potencia=%d duty=%d%% | pulsos_esq=%ld pulsos_dir=%ld | invalidas=%lu | tof_falhas=%lu imu_falhas=%lu | veredito=%s\n",
                potencia, dutyPercentual(potencia), static_cast<long>(pulsosEsq), static_cast<long>(pulsosDir),
                static_cast<unsigned long>(invalidas), static_cast<unsigned long>(falhasTof),
                static_cast<unsigned long>(falhasImu), resumo.falhas == 0 ? "OK" : "FALHA");
}

/// Sobe a potência de um motor em degraus e diz em qual a roda começou a girar, pelo encoder.
/// Serve para achar a zona morta do N20 e para separar "motor sem torque" de "motor sem energia".
void rampa(bool esquerdo) {
  IEncoder& encoder = esquerdo ? static_cast<IEncoder&>(encoderEsq) : static_cast<IEncoder&>(encoderDir);
  console.printf("> rampa no motor %s, de %d a %d. Qualquer tecla freia.\n", esquerdo ? "esquerdo" : "direito",
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
    console.printf("  potencia %4d (duty %2d%%): %ld pulsos\n", p, dutyPercentual(p), static_cast<long>(pulsos));
    if (girouEm == 0 && modulo >= RAMPA_PULSOS_PARA_GIRAR) {
      girouEm = p;
    }
  }
  motores.parar();
  if (girouEm > 0) {
    console.printf("> a roda comecou a girar na potencia %d (duty %d%%).\n", girouEm, dutyPercentual(girouEm));
  } else if (interrompido) {
    console.println("> freado por tecla.");
  } else {
    console.println("> a roda NAO girou em nenhuma potencia. Se o motor nem zumbe: confira VM, GND, nSLEEP e os fios OUT.");
    console.println("  Se zumbe e nao gira: falta corrente na fonte ou o motor esta travado.");
    console.println("  Se gira e o encoder nao conta: o problema esta no encoder (VCC, GND, A, B).");
  }
}

/// Modo contínuo: aciona o motor sem o limite de 2 s, mostra os pulsos e a velocidade dos encoders
/// a cada `PERIODO_FLUXO_MS` e deixa trocar motor, sentido e potência sem parar. Para ao receber
/// qualquer outra tecla, depois de `PRAZO_SEM_TECLA_MS` sem tecla (na UART, o programa não percebe o
/// monitor fechar) ou depois de `TEMPO_MAXIMO_CONTINUO_MS`.
void modoContinuo() {
  Alvo alvo = Alvo::Esquerdo;
  bool re = false;
  console.println("> MODO CONTINUO. Teclas: e = motor esquerdo, d = motor direito, b = os dois,");
  console.println(">   r = inverte o sentido, + e - = potencia, l e k = limite de duty. Qualquer outra tecla (espaco) freia.");
  console.println(">   Neste modo o programa nao percebe o monitor fechar: tecle m a cada 10 s para manter, senao os motores freiam.");
  encoderEsq.zerar();
  encoderDir.zerar();
  const uint32_t inicioMs = millis();
  uint32_t ultimaImpressaoMs = inicioMs;
  int32_t anteriorEsq = 0;
  int32_t anteriorDir = 0;
  const char* motivo = "tempo maximo atingido";
  uint32_t ultimaTeclaMs = inicioMs;
  while (millis() - inicioMs < TEMPO_MAXIMO_CONTINUO_MS) {
    bool parar = false;
    while (console.available() > 0 && !parar) {
      const char tecla = static_cast<char>(console.read());
      if (tecla != '\r' && tecla != '\n') {
        ultimaTeclaMs = millis();
      }
      switch (tecla) {
        case '\r':
        case '\n':
        case 'm':
        case 'M':
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
        case 'l':
        case 'L':
          mudarLimiteDuty(LIMITE_DUTY_PASSO_MILESIMOS);
          break;
        case 'k':
        case 'K':
          mudarLimiteDuty(-static_cast<int16_t>(LIMITE_DUTY_PASSO_MILESIMOS));
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
    if (!console) {
      motivo = "monitor serial fechado";
      break;
    }
    if (millis() - ultimaTeclaMs >= PRAZO_SEM_TECLA_MS) {
      motivo = "10 s sem tecla (o monitor pode ter fechado)";
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
      console.printf("[%s, %s, potencia %d, duty %d%%] esq: %ld pulsos, %ld p/s (%ld mm/s) | dir: %ld pulsos, %ld p/s (%ld mm/s)\n",
                    alvo == Alvo::Esquerdo ? "esquerdo" : alvo == Alvo::Direito ? "direito" : "os dois",
                    re ? "re" : "frente", comando, dutyPercentual(comando), static_cast<long>(esq),
                    static_cast<long>(pulsosPorSEsq), static_cast<long>(conversor.pulsosParaMm(pulsosPorSEsq)),
                    static_cast<long>(dir), static_cast<long>(pulsosPorSDir),
                    static_cast<long>(conversor.pulsosParaMm(pulsosPorSDir)));
    }
    delay(5);
  }
  motores.parar();
  console.printf("> motores freados (%s). Potencia ficou em %d.\n", motivo, potencia);
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
      console.printf("Potencia: %d de 1000\n", potencia);
      break;
    case '-':
      potencia = potencia - POTENCIA_PASSO < POTENCIA_MINIMA ? POTENCIA_MINIMA : potencia - POTENCIA_PASSO;
      console.printf("Potencia: %d de 1000\n", potencia);
      break;
    case 'l':
    case 'L':
      mudarLimiteDuty(LIMITE_DUTY_PASSO_MILESIMOS);
      break;
    case 'k':
    case 'K':
      mudarLimiteDuty(-static_cast<int16_t>(LIMITE_DUTY_PASSO_MILESIMOS));
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

#if defined(BANCADA_COM_TELEMETRIA)
// ---------------------------------------------------------------- telemetria

/// Tempo máximo esperando o Wi-Fi conectar nos comandos que precisam dele, em ms: uma tentativa
/// (`TEMPO_LIMITE_CONEXAO_WIFI_MS`) por rede conhecida, mais esta folga.
constexpr uint32_t FOLGA_ESPERA_WIFI_MS = 10000;

/// Medição do sinal (#491): intervalo entre leituras do RSSI e as faixas da avaliação, em dBm.
constexpr uint32_t SINAL_PERIODO_MS = 500;
constexpr int SINAL_BOM_DBM = -67;
constexpr int SINAL_ACEITAVEL_DBM = -75;
constexpr int SINAL_FRACO_DBM = -80;

/// Escolha de rede pela serial (`p`): espaço em flash (NVS) da rede salva, tempo máximo para digitar
/// e quantas redes da varredura entram na lista numerada.
constexpr const char* NVS_REDE_BANCADA = "bancada_wifi";
constexpr uint32_t ESPERA_DIGITACAO_MS = 120000;
constexpr int16_t REDES_NA_LISTA = 20;

/// Corrida sem rede (#491, #478): amostras a mais que a capacidade do buffer no teste do buffer cheio,
/// e de quantas em quantas amostras geradas a bancada mostra o andamento.
constexpr uint16_t SEM_REDE_EXCESSO = 50;
constexpr uint16_t SEM_REDE_PERIODO_LOG = 100;

/// Teste de reenvio (#478): gera amostras por pelo menos `REENVIO_DURACAO_MS` e até o Wi-Fi cair e
/// voltar (mais `REENVIO_DEPOIS_DA_VOLTA_MS`), no máximo `REENVIO_DURACAO_MAXIMA_MS`; depois espera o
/// buffer esvaziar por até `REENVIO_ESPERA_FINAL_MS`. `REENVIO_PERIODO_LOG_MS` é o intervalo do andamento.
constexpr uint32_t REENVIO_DURACAO_MS = 60000;
constexpr uint32_t REENVIO_DEPOIS_DA_VOLTA_MS = 15000;
constexpr uint32_t REENVIO_DURACAO_MAXIMA_MS = 300000;
constexpr uint32_t REENVIO_ESPERA_FINAL_MS = 120000;
constexpr uint32_t REENVIO_PERIODO_LOG_MS = 5000;

/// Quantas amostras de teste o comando de lote envia.
constexpr uint16_t AMOSTRAS_POR_LOTE = 5;

/// Falha de HTTP que nem saiu do robô (valor do `EnvioHttpSupabase`, que não é público).
constexpr int SEM_CHAMADA = -1000;

CredenciaisSupabase lerCredenciais() {
  CredenciaisSupabase c;
#if BANCADA_TEM_CREDENCIAIS
  c.ssidWifi = CREDENCIAL_WIFI_SSID;
  c.senhaWifi = CREDENCIAL_WIFI_SENHA;
#if defined(CREDENCIAL_WIFI_SSID_RESERVA) && defined(CREDENCIAL_WIFI_SENHA_RESERVA)
  c.ssidWifiReserva = CREDENCIAL_WIFI_SSID_RESERVA;  // rede reserva (#491), opcional
  c.senhaWifiReserva = CREDENCIAL_WIFI_SENHA_RESERVA;
#endif
  c.urlProjeto = CREDENCIAL_SUPABASE_URL;
  c.chaveAnon = CREDENCIAL_SUPABASE_ANON;
  c.tokenDispositivo = CREDENCIAL_DEVICE_TOKEN;
  c.certificadoRaiz = CREDENCIAL_CERTIFICADO_RAIZ;
  c.semValidarCertificado = CREDENCIAL_SEM_VALIDAR_CERTIFICADO != 0;
#endif
  return c;
}

const CredenciaisSupabase credenciais = lerCredenciais();
RedeWifiEsp32 redeWifi(credenciais.ssidWifi, credenciais.senhaWifi, credenciais.ssidWifiReserva,
                       credenciais.senhaWifiReserva);
ReconexaoWifi reconexaoWifi(redeWifi);
EnvioHttpSupabase clienteSupabase(credenciais);
TravaSecaoCritica travaBufferEnvio;
/// O mesmo buffer com reenvio do robô (#238).
ControladorEnvio controladorEnvio(clienteSupabase, travaBufferEnvio);

bool wifiIniciado = false;

/// Rede escolhida pela serial (`p`), que substitui a principal do `credenciais.h` só na bancada. Fica
/// salva na flash (NVS) se quem testa pedir; o firmware do robô não lê esse espaço.
char ssidEscolhido[33] = "";
char senhaEscolhida[65] = "";
bool redeEscolhidaSalva = false;
uint32_t corridaId = 0;
bool corridaCriada = false;
bool corridaEncerrada = false;
uint32_t proximaSequencia = 1;
AmostraTelemetria ultimoLote[AMOSTRAS_POR_LOTE];
uint16_t ultimoLoteTamanho = 0;

/// Contadores da sessão, mostrados no status.
uint16_t chamadasOk = 0;
uint16_t chamadasComFalha = 0;

// ---------------------------------------------------------------- utilidades

/// `true` se as credenciais do Supabase existem e estão completas; senão, diz o que fazer. O Wi-Fi não
/// entra aqui: a rede pode vir do `credenciais.h` ou da escolha pela serial (`p`).
bool credenciaisOk() {
  if (!BANCADA_TEM_CREDENCIAIS) {
    console.println("  [FALHA] sem include/credenciais.h: o Wi-Fi funciona (tecla p), mas o banco precisa do arquivo.");
    console.println("          dica: copie include/credenciais.exemplo.h para include/credenciais.h,");
    console.println("          preencha os campos, use CREDENCIAL_SEM_VALIDAR_CERTIFICADO 1 na bancada e grave de novo.");
    return false;
  }
  CredenciaisSupabase supabase = credenciais;
  supabase.ssidWifi = "rede";  // a rede é conferida no Wi-Fi; aqui, só a URL, a chave, o token e o certificado
  supabase.senhaWifi = "";
  if (!credenciaisValidas(supabase)) {
    console.println("  [FALHA] credenciais do Supabase incompletas em include/credenciais.h.");
    console.println("          dica: confira a URL (https://...), a chave anon, o device token e o certificado");
    console.println("          (ou CREDENCIAL_SEM_VALIDAR_CERTIFICADO 1). Os valores do exemplo contam como preenchidos:");
    console.println("          troque todos pelos reais.");
    return false;
  }
  return true;
}

/// Nome do estado do Wi-Fi e a dica mais provável.
const char* explicarStatusWifi(wl_status_t status) {
  switch (status) {
    case WL_NO_SSID_AVAIL:
      return "rede nao encontrada. Confira o nome e se ela e 2,4 GHz (o ESP32 nao enxerga 5 GHz; no iPhone,"
             " ative Maximizar Compatibilidade no Acesso Pessoal).";
    case WL_CONNECT_FAILED:
      return "a rede recusou a conexao. Confira a senha.";
    case WL_CONNECTION_LOST:
      return "a conexao caiu. Aproxime a placa do roteador.";
    case WL_DISCONNECTED:
      return "sem conexao ainda. Senha errada, sinal fraco ou o roteador nao aceita a placa.";
    default:
      return "estado desconhecido.";
  }
}

void imprimirRedes();

/// Liga o Wi-Fi na primeira vez que for preciso e carrega a rede salva pela tecla `p`, se houver.
void iniciarWifiSeNecessario() {
  if (wifiIniciado) {
    return;
  }
  redeWifi.iniciar();
  clienteSupabase.iniciar();
  wifiIniciado = true;
  Preferences nvs;
  if (nvs.begin(NVS_REDE_BANCADA, true)) {
    if (nvs.getString("ssid", ssidEscolhido, sizeof(ssidEscolhido)) > 0) {
      nvs.getString("senha", senhaEscolhida, sizeof(senhaEscolhida));
      redeWifi.definirRede(0, ssidEscolhido, senhaEscolhida);
      redeEscolhidaSalva = true;
    }
    nvs.end();
  }
  imprimirRedes();
}

/// Nome na rede (mDNS) e senha da gravação sem cabo (OTA). A senha só protege a bancada no laboratório;
/// pode ser trocada com `-DBANCADA_SENHA_OTA=\"outra\"` no `platformio.ini`.
constexpr const char* NOME_NA_REDE = "micromouse-bancada";
#if !defined(BANCADA_SENHA_OTA)
#define BANCADA_SENHA_OTA "micromouse"
#endif

bool servicosDeRedeAtivos = false;

/// Com o Wi-Fi conectado, liga o telnet e a gravação sem cabo, uma vez só.
void iniciarServicosDeRede() {
  if (servicosDeRedeAtivos) {
    return;
  }
  servicosDeRedeAtivos = true;
  console.iniciarTelnet();
  ArduinoOTA.setHostname(NOME_NA_REDE);
  ArduinoOTA.setPassword(BANCADA_SENHA_OTA);
  ArduinoOTA.onStart([]() {
    motores.parar();  // gravando, nada de motor girando
    console.println("\n[OTA] gravando; a placa reinicia no fim.");
  });
  ArduinoOTA.onEnd([]() { console.println("[OTA] gravado, reiniciando."); console.flush(); });
  ArduinoOTA.onError([](ota_error_t erro) { console.printf("[OTA] erro %u\n", static_cast<unsigned>(erro)); });
  ArduinoOTA.begin();
  console.printf("[rede] telnet na porta %u e OTA prontos: %s (nc %s %u)\n", ConsoleBancada::PORTA_TELNET,
                 WiFi.localIP().toString().c_str(), WiFi.localIP().toString().c_str(), ConsoleBancada::PORTA_TELNET);
}

/// Chamado em todo ciclo do laço: Wi-Fi, telnet e OTA.
void cuidarDaRede() {
  if (!wifiIniciado) {
    return;
  }
  reconexaoWifi.ciclo(millis());
  if (WiFi.status() == WL_CONNECTED) {
    iniciarServicosDeRede();
    ArduinoOTA.handle();
  }
}

/// Uma linha com as redes conhecidas e de onde vem cada uma.
void imprimirRedes() {
  const char* principal = redeWifi.ssid(0);
  if (principal[0] == '\0') {
    console.println("  nenhuma rede configurada. Tecle p para escolher uma das redes visiveis.");
    return;
  }
  console.printf("  rede principal: \"%s\" (%s)", principal,
                redeEscolhidaSalva ? "salva na placa pela tecla p; f esquece"
                : ssidEscolhido[0] != '\0' ? "escolhida pela tecla p, so ate reiniciar"
                                           : "do credenciais.h");
  if (redeWifi.quantidadeRedes() > 1) {
    console.printf("; reserva: \"%s\" (do credenciais.h)", redeWifi.ssid(1));
  }
  console.println();
}

/// `"principal \"nome\""` ou `"reserva \"nome\""` da rede atual, para as mensagens.
const char* descreverRedeAtual() {
  static char texto[48];
  snprintf(texto, sizeof(texto), "%s \"%s\"", redeWifi.nomeRedeAtual(), redeWifi.ssidAtual());
  return texto;
}

/// Avaliação de um RSSI, pelas faixas da medição do sinal.
const char* avaliarSinal(int rssi) {
  if (rssi >= SINAL_BOM_DBM) {
    return "bom";
  }
  if (rssi >= SINAL_ACEITAVEL_DBM) {
    return "aceitavel";
  }
  if (rssi >= SINAL_FRACO_DBM) {
    return "fraco";
  }
  return "CRITICO";
}

/// Espera o Wi-Fi conectar (uma tentativa por rede, mais a folga), mostrando um ponto por segundo.
bool conectarWifi() {
  iniciarWifiSeNecessario();
  if (redeWifi.ssid(0)[0] == '\0') {
    console.println("  [FALHA] nenhuma rede Wi-Fi configurada.");
    console.println("          dica: tecle p para escolher uma rede visivel e digitar a senha (ou preencha o credenciais.h).");
    return false;
  }
  const uint32_t espera = TEMPO_LIMITE_CONEXAO_WIFI_MS * redeWifi.quantidadeRedes() + FOLGA_ESPERA_WIFI_MS;
  const uint32_t inicio = millis();
  uint32_t ultimoPonto = inicio;
  bool avisou = false;
  uint8_t redeAvisada = reconexaoWifi.redeAtual();
  while (!reconexaoWifi.ciclo(millis())) {
    if (!avisou) {
      console.printf("  conectando na %s ", descreverRedeAtual());
      avisou = true;
    }
    if (reconexaoWifi.redeAtual() != redeAvisada) {
      redeAvisada = reconexaoWifi.redeAtual();
      console.printf("\n  nao conectou; tentando a %s ", descreverRedeAtual());
    }
    if (millis() - ultimoPonto >= 1000) {
      console.print('.');
      ultimoPonto = millis();
    }
    if (millis() - inicio >= espera) {
      console.println();
      console.printf("  [FALHA] Wi-Fi nao conectou em %lu s.\n", static_cast<unsigned long>(espera / 1000));
      console.printf("          dica: %s\n", explicarStatusWifi(WiFi.status()));
      console.println("          Tecle v para ver as redes visiveis, ou p para escolher outra rede e digitar a senha.");
      return false;
    }
    delay(50);
  }
  if (avisou) {
    console.println();
  }
  return true;
}

/// Antes de falar com o banco: credenciais do Supabase completas e Wi-Fi conectado.
bool conectarParaBanco() { return credenciaisOk() && conectarWifi(); }

/// Host da URL do projeto (`https://abc.supabase.co/` -> `abc.supabase.co`).
bool extrairHost(char* saida, size_t capacidade) {
  const char* url = credenciais.urlProjeto;
  if (url == nullptr || strncmp(url, "https://", 8) != 0) {
    return false;
  }
  const char* inicio = url + 8;
  size_t tamanho = 0;
  while (inicio[tamanho] != '\0' && inicio[tamanho] != '/' && inicio[tamanho] != ':') {
    ++tamanho;
  }
  if (tamanho == 0 || tamanho >= capacidade) {
    return false;
  }
  memcpy(saida, inicio, tamanho);
  saida[tamanho] = '\0';
  return true;
}

/// Diz o que o código HTTP significa e o que fazer.
void explicarCodigoHttp(int codigo) {
  if (codigo >= 200 && codigo < 300) {
    return;
  }
  if (codigo == SEM_CHAMADA) {
    console.println("          dica: a chamada nem saiu da placa (URL ou corpo grande demais). Confira CREDENCIAL_SUPABASE_URL.");
  } else if (codigo <= 0) {
    console.printf("          dica: sem resposta do servidor (%s).\n", HTTPClient::errorToString(codigo).c_str());
    console.println("          Se for connection refused ou tempo esgotado: use a tecla d para ver o DNS; se o DNS passa,");
    console.println("          o problema e o TLS. Na bancada, use CREDENCIAL_SEM_VALIDAR_CERTIFICADO 1.");
  } else if (codigo == 401 || codigo == 403) {
    console.println("          dica: chave anon ou device token recusados. Confira CREDENCIAL_SUPABASE_ANON e CREDENCIAL_DEVICE_TOKEN");
    console.println("          (o token precisa ser igual ao segredo DEVICE_TOKEN das Edge Functions).");
  } else if (codigo == 404) {
    console.println("          dica: leia a resposta do servidor acima. \"Requested function was not found\" = a Edge Function");
    console.println("          nao foi publicada (supabase functions deploy telemetria). \"corrida_inexistente\" = a corrida");
    console.println("          nao existe nesse banco (tecle c de novo).");
  } else if (codigo == 400) {
    console.println("          dica: o servidor recusou o corpo. Confira se as migracoes foram aplicadas no banco.");
  } else if (codigo >= 500) {
    console.println("          dica: erro dentro da funcao ou do banco. Veja os logs em Edge Functions > Logs no painel do Supabase.");
  }
}

/// Registra o resultado de uma chamada: `[OK]`/`[FALHA]`, código, tempo e a dica.
bool relatarChamada(const char* nome, int codigo, uint32_t duracaoMs, const char* detalhe) {
  const bool ok = codigo >= 200 && codigo < 300;
  if (ok) {
    ++chamadasOk;
    console.printf("  [OK]    %s: HTTP %d em %lu ms%s%s\n", nome, codigo, static_cast<unsigned long>(duracaoMs),
                  detalhe[0] != '\0' ? ", " : "", detalhe);
  } else {
    ++chamadasComFalha;
    console.printf("  [FALHA] %s: HTTP %d em %lu ms\n", nome, codigo, static_cast<unsigned long>(duracaoMs));
    if (clienteSupabase.ultimaResposta().length() > 0) {
      console.printf("          resposta do servidor: %s\n", clienteSupabase.ultimaResposta().c_str());
    }
    explicarCodigoHttp(codigo);
  }
  return ok;
}

// ---------------------------------------------------------------- comandos

/// `w`: conecta e mostra o que a rede entregou.
bool comandoWifi() {
  console.println("--- Wi-Fi ---");
  if (!conectarWifi()) {
    return false;
  }
  console.printf("  [OK]    conectado na %s, IP %s, sinal %d dBm (%s)\n", descreverRedeAtual(),
                WiFi.localIP().toString().c_str(), WiFi.RSSI(), avaliarSinal(WiFi.RSSI()));
  if (WiFi.RSSI() < -80) {
    console.println("          aviso: sinal fraco (abaixo de -80 dBm). Pode falhar HTTPS; aproxime do roteador.");
  }
  return true;
}

/// `d`: resolve o nome do servidor, o primeiro passo antes do HTTPS.
bool comandoDns() {
  console.println("--- DNS ---");
  if (!conectarParaBanco()) {
    return false;
  }
  char host[96];
  if (!extrairHost(host, sizeof(host))) {
    console.println("  [FALHA] CREDENCIAL_SUPABASE_URL invalida. dica: use o formato https://SEU-PROJETO.supabase.co");
    return false;
  }
  IPAddress ip;
  const uint32_t inicio = millis();
  if (WiFi.hostByName(host, ip) != 1) {
    ++chamadasComFalha;
    console.printf("  [FALHA] nao resolveu \"%s\".\n", host);
    console.println("          dica: confira o nome do projeto na URL e se a rede tem internet (portal cativo de");
    console.println("          universidade pede login no navegador e bloqueia a placa; use um hotspot do celular).");
    return false;
  }
  ++chamadasOk;
  console.printf("  [OK]    %s -> %s em %lu ms\n", host, ip.toString().c_str(), static_cast<unsigned long>(millis() - inicio));
  return true;
}

/// `c`: `POST /corridas`, que cria a corrida no banco.
bool comandoCriarCorrida() {
  console.println("--- Criar corrida (POST /functions/v1/corridas) ---");
  if (!conectarParaBanco()) {
    return false;
  }
  uint32_t id = 0;
  const uint32_t inicio = millis();
  const ResultadoEnvio r = clienteSupabase.criarCorrida(LABIRINTO_CORRIDA_PADRAO, id);
  char detalhe[40] = "";
  if (r.codigoHttp >= 200 && r.codigoHttp < 300) {
    snprintf(detalhe, sizeof(detalhe), "corrida_id %lu", static_cast<unsigned long>(id));
  }
  if (!relatarChamada("criar corrida", r.codigoHttp, millis() - inicio, detalhe)) {
    return false;
  }
  corridaId = id;
  corridaCriada = true;
  corridaEncerrada = false;
  proximaSequencia = 1;
  ultimoLoteTamanho = 0;
  console.println("          confira no painel: Table Editor > corridas.");
  return true;
}

/// Preenche `ultimoLote` com amostras de teste, que andam pelas células do 4x4.
void montarLoteDeTeste() {
  for (uint16_t i = 0; i < AMOSTRAS_POR_LOTE; ++i) {
    AmostraTelemetria a;
    a.numeroSequencia = proximaSequencia + i;
    a.tempoDecorridoMs = a.numeroSequencia * PERIODO_TELEMETRIA_MS;
    a.velocidadeMmPorS = 100;
    a.posicaoLinha = static_cast<uint8_t>(((a.numeroSequencia - 1) / 4) % 4);
    a.posicaoColuna = static_cast<uint8_t>((a.numeroSequencia - 1) % 4);
    a.orientacao = 'N';
    a.estado = EstadoCorrida::Explorando;
    ultimoLote[i] = a;
  }
  ultimoLoteTamanho = AMOSTRAS_POR_LOTE;
  proximaSequencia += AMOSTRAS_POR_LOTE;
}

/// Envia `ultimoLote` e confere as contagens. `esperadasNovas` é quantas o servidor deveria gravar.
bool enviarUltimoLote(const char* nome, uint16_t esperadasNovas) {
  const uint32_t inicio = millis();
  const ResultadoEnvio r = clienteSupabase.enviarLote(corridaId, ultimoLote, ultimoLoteTamanho);
  char detalhe[64];
  snprintf(detalhe, sizeof(detalhe), "gravadas %u, ja existentes %u", r.gravadas, r.existentes);
  if (!relatarChamada(nome, r.codigoHttp, millis() - inicio, detalhe)) {
    return false;
  }
  if (r.gravadas != esperadasNovas || r.gravadas + r.existentes != ultimoLoteTamanho) {
    ++chamadasComFalha;
    console.printf("  [FALHA] contagem inesperada: esperava %u gravadas e %u existentes.\n", esperadasNovas,
                  static_cast<unsigned>(ultimoLoteTamanho - esperadasNovas));
    console.println("          dica: o servidor respondeu 2xx mas as contagens nao fecham. Veja a funcao telemetria no painel.");
    return false;
  }
  return true;
}

/// `l`: envia um lote novo de amostras de teste.
bool comandoEnviarLote() {
  console.printf("--- Enviar lote de %u amostras (POST /functions/v1/telemetria) ---\n", AMOSTRAS_POR_LOTE);
  if (!corridaCriada || corridaEncerrada) {
    console.println("  [FALHA] nao ha corrida aberta. dica: tecle c para criar uma.");
    return false;
  }
  if (!conectarParaBanco()) {
    return false;
  }
  montarLoteDeTeste();
  if (!enviarUltimoLote("enviar lote", AMOSTRAS_POR_LOTE)) {
    return false;
  }
  console.printf("          sequencias %lu a %lu. Confira no painel: Table Editor > telemetria.\n",
                static_cast<unsigned long>(ultimoLote[0].numeroSequencia),
                static_cast<unsigned long>(ultimoLote[ultimoLoteTamanho - 1].numeroSequencia));
  return true;
}

/// `r`: reenvia o último lote. O servidor tem de dizer que todas as amostras já existiam.
bool comandoReenviarLote() {
  console.println("--- Reenviar o ultimo lote (teste de duplicata) ---");
  if (!corridaCriada || corridaEncerrada || ultimoLoteTamanho == 0) {
    console.println("  [FALHA] nao ha lote para reenviar. dica: tecle c e depois l.");
    return false;
  }
  if (!conectarParaBanco()) {
    return false;
  }
  if (!enviarUltimoLote("reenviar lote", 0)) {
    return false;
  }
  console.println("          nenhuma amostra duplicada: o reenvio e seguro.");
  return true;
}

/// `e`: encerra a corrida como `nao_concluida`.
bool comandoEncerrarCorrida() {
  console.println("--- Encerrar corrida (POST /functions/v1/corridas/{id}/encerrar) ---");
  if (!corridaCriada) {
    console.println("  [FALHA] nao ha corrida criada. dica: tecle c.");
    return false;
  }
  if (!conectarParaBanco()) {
    return false;
  }
  clienteSupabase.definirResultadoCorrida(false);
  const uint32_t inicio = millis();
  const ResultadoEnvio r = clienteSupabase.encerrarCorrida(corridaId);
  if (!relatarChamada("encerrar corrida", r.codigoHttp, millis() - inicio, "status nao_concluida")) {
    return false;
  }
  corridaEncerrada = true;
  return true;
}

/// `a`: o fluxo inteiro, na ordem em que o robô o faz.
void comandoFluxoCompleto() {
  console.println("=== FLUXO COMPLETO: Wi-Fi, DNS, corrida, lote, reenvio e encerramento ===");
  if (!credenciaisOk()) {
    return;
  }
  corridaCriada = false;
  const uint16_t okAntes = chamadasOk;
  const uint16_t falhasAntes = chamadasComFalha;
  const bool passou = comandoWifi() && comandoDns() && comandoCriarCorrida() && comandoEnviarLote() &&
                      comandoReenviarLote() && comandoEncerrarCorrida();
  console.println("---");
  if (passou) {
    console.printf("RESULTADO: %u etapas OK, 0 falhas. A placa fala com o banco.\n", chamadasOk - okAntes);
  } else {
    console.printf("RESULTADO: parou na primeira falha (%u OK, %u FALHA). Leia a dica da etapa que falhou.\n",
                  chamadasOk - okAntes, chamadasComFalha - falhasAntes);
  }
}

/// `b`: teste de reenvio com queda de Wi-Fi (#478). Usa o mesmo `ControladorEnvio` do robô (buffer,
/// reenvio em ordem, sem perder e sem duplicar): a bancada gera 1 amostra por segundo, e quem testa
/// desliga o Wi-Fi (hotspot) por cerca de 30 s no meio e religa. No fim, confere as contagens e diz
/// a consulta para conferir no banco.
bool comandoTesteReenvio() {
  console.println("--- Teste de reenvio com queda de Wi-Fi (#478) ---");
  console.printf("  A bancada gera 1 amostra por segundo. Quando quiser, desligue os hotspots (principal e reserva)\n"
                "  por cerca de 30 s e religue: o teste termina %lu s depois de o Wi-Fi voltar (pelo menos %lu s,\n"
                "  no maximo %lu s). Qualquer tecla cancela.\n",
                static_cast<unsigned long>(REENVIO_DEPOIS_DA_VOLTA_MS / 1000), static_cast<unsigned long>(REENVIO_DURACAO_MS / 1000),
                static_cast<unsigned long>(REENVIO_DURACAO_MAXIMA_MS / 1000));
  if (!conectarParaBanco()) {
    return false;
  }
  uint32_t id = 0;
  const uint32_t inicioCriacao = millis();
  const ResultadoEnvio criada = clienteSupabase.criarCorrida(LABIRINTO_CORRIDA_PADRAO, id);
  char detalhe[40] = "";
  if (criada.codigoHttp >= 200 && criada.codigoHttp < 300) {
    snprintf(detalhe, sizeof(detalhe), "corrida_id %lu", static_cast<unsigned long>(id));
  }
  if (!relatarChamada("criar corrida", criada.codigoHttp, millis() - inicioCriacao, detalhe)) {
    return false;
  }
  controladorEnvio.iniciarCorrida(id);
  clienteSupabase.definirResultadoCorrida(false);
  // Quedas e trocas só deste teste: os contadores da `ReconexaoWifi` valem para a sessão inteira.
  const uint32_t quedasAntes = reconexaoWifi.quedas();
  const uint32_t trocasAntes = reconexaoWifi.trocasDeRede();
  uint32_t msSemConexao = 0;
  uint32_t voltouMs = 0;  // quando o Wi-Fi voltou depois da última queda deste teste
  bool estavaConectado = true;

  const uint32_t inicioMs = millis();
  uint32_t cicloAnteriorMs = inicioMs;
  uint32_t proximaAmostraMs = inicioMs;
  uint32_t proximoLogMs = inicioMs + REENVIO_PERIODO_LOG_MS;
  uint32_t geradas = 0;
  bool cancelado = false;
  bool encerramentoPedido = false;
  uint32_t fimGeracaoMs = 0;
  while (true) {
    const uint32_t agoraMs = millis();
    if (teclaPressionada()) {
      cancelado = true;
      break;
    }
    const bool conectado = reconexaoWifi.ciclo(agoraMs);
    if (!conectado) {
      msSemConexao += agoraMs - cicloAnteriorMs;
    }
    cicloAnteriorMs = agoraMs;
    if (conectado != estavaConectado) {
      estavaConectado = conectado;
      if (conectado) {
        voltouMs = agoraMs;
        console.printf("  >>> Wi-Fi voltou na %s depois de %lu s sem conexao no total\n", descreverRedeAtual(),
                      static_cast<unsigned long>(msSemConexao / 1000));
      } else {
        console.println("  >>> Wi-Fi caiu; as amostras ficam no buffer");
      }
    }
    const uint32_t decorridoMs = agoraMs - inicioMs;
    const bool caiuEVoltou = reconexaoWifi.quedas() > quedasAntes && conectado &&
                             agoraMs - voltouMs >= REENVIO_DEPOIS_DA_VOLTA_MS;
    if (!encerramentoPedido &&
        ((decorridoMs >= REENVIO_DURACAO_MS && caiuEVoltou) || decorridoMs >= REENVIO_DURACAO_MAXIMA_MS)) {
      controladorEnvio.solicitarEncerramento(agoraMs);
      encerramentoPedido = true;
      fimGeracaoMs = agoraMs;
      console.println("  fim da geracao; esperando o buffer esvaziar e a corrida encerrar");
    }
    if (!encerramentoPedido && static_cast<int32_t>(agoraMs - proximaAmostraMs) >= 0) {
      AmostraTelemetria a;
      a.tempoDecorridoMs = agoraMs - inicioMs;
      a.velocidadeMmPorS = 100;
      a.posicaoLinha = static_cast<uint8_t>((geradas / 4) % 4);
      a.posicaoColuna = static_cast<uint8_t>(geradas % 4);
      a.orientacao = 'N';
      a.estado = EstadoCorrida::Explorando;
      controladorEnvio.registrar(a);
      geradas++;
      proximaAmostraMs += PERIODO_TELEMETRIA_MS;
    }
    // Como na tarefa de rede do robô: só chama o servidor com o Wi-Fi conectado.
    if (conectado) {
      controladorEnvio.ciclo(agoraMs);
    }
    if (static_cast<int32_t>(agoraMs - proximoLogMs) >= 0) {
      proximoLogMs = agoraMs + REENVIO_PERIODO_LOG_MS;
      const ContadoresEnvio& c = controladorEnvio.contadores();
      char wifi[32];
      if (conectado) {
        snprintf(wifi, sizeof(wifi), "%s %d dBm", redeWifi.nomeRedeAtual(), static_cast<int>(WiFi.RSSI()));
      } else {
        snprintf(wifi, sizeof(wifi), "SEM CONEXAO");
      }
      console.printf("  t=%3lu s | wifi %-17s | geradas %3lu | no buffer %3u | confirmadas %3lu | falhas %lu | reenvios %lu\n",
                    static_cast<unsigned long>((agoraMs - inicioMs) / 1000), wifi,
                    static_cast<unsigned long>(geradas), controladorEnvio.ocupacao(),
                    static_cast<unsigned long>(c.amostrasConfirmadas), static_cast<unsigned long>(c.falhas),
                    static_cast<unsigned long>(c.reenvios));
    }
    if (encerramentoPedido && (controladorEnvio.encerrada() || agoraMs - fimGeracaoMs >= REENVIO_ESPERA_FINAL_MS)) {
      break;
    }
    delay(20);
  }
  if (cancelado) {
    console.println("  [FALHA] cancelado por tecla; a corrida ficou aberta no banco.");
    return false;
  }

  const ContadoresEnvio& c = controladorEnvio.contadores();
  const uint32_t quedas = reconexaoWifi.quedas() - quedasAntes;
  const uint32_t trocas = reconexaoWifi.trocasDeRede() - trocasAntes;
  const bool tudoGravado = c.amostrasConfirmadas + c.amostrasJaExistentes == geradas;
  console.printf("  geradas %lu | confirmadas %lu | ja existentes %lu | descartadas %lu | perdidas no buffer %lu | no buffer %u\n",
                static_cast<unsigned long>(geradas), static_cast<unsigned long>(c.amostrasConfirmadas),
                static_cast<unsigned long>(c.amostrasJaExistentes), static_cast<unsigned long>(c.amostrasDescartadas),
                static_cast<unsigned long>(controladorEnvio.amostrasPerdidas()), controladorEnvio.ocupacao());
  console.printf("  tentativas %lu | falhas %lu | reenvios %lu | quedas do Wi-Fi %lu | trocas de rede %lu | corrida encerrada: %s\n",
                static_cast<unsigned long>(c.tentativas), static_cast<unsigned long>(c.falhas),
                static_cast<unsigned long>(c.reenvios), static_cast<unsigned long>(quedas),
                static_cast<unsigned long>(trocas), controladorEnvio.encerrada() ? "sim" : "nao");
  ResumoAutoteste resumo;
  const uint32_t segundosSemConexao = msSemConexao / 1000;
  registrar(resumo, quedas > 0,
            "o Wi-Fi nao caiu durante o teste: desligue os hotspots por 30 s no meio e rode de novo",
            "o Wi-Fi caiu %lu vez(es) durante o teste, %lu s sem conexao", static_cast<unsigned long>(quedas),
            static_cast<unsigned long>(segundosSemConexao));
  registrar(resumo, tudoGravado && c.amostrasDescartadas == 0 && controladorEnvio.amostrasPerdidas() == 0,
            "amostras perdidas ou descartadas: veja 'descartadas' (o servidor recusou) e 'perdidas no buffer' (encheu)",
            "todas as %lu amostras foram gravadas, sem perda", static_cast<unsigned long>(geradas));
  registrar(resumo, controladorEnvio.encerrada(), "a corrida nao foi encerrada: o buffer nao esvaziou a tempo; veja as falhas acima",
            "corrida encerrada no servidor");
  console.printf("RESULTADO: %u OK, %u FALHA(S).\n", resumo.ok, resumo.falhas);
  console.printf("  Confira no banco (SQL Editor): select count(*), count(distinct numero_sequencia), min(numero_sequencia), "
                "max(numero_sequencia) from telemetria where corrida_id = %lu;  esperado: %lu, %lu, 1, %lu\n",
                static_cast<unsigned long>(id), static_cast<unsigned long>(geradas), static_cast<unsigned long>(geradas),
                static_cast<unsigned long>(geradas));
  console.printf("REGISTRO: reenvio | corrida=%lu geradas=%lu confirmadas=%lu existentes=%lu descartadas=%lu perdidas=%lu reenvios=%lu quedas=%lu trocas=%lu rede_final=%s | veredito=%s\n",
                static_cast<unsigned long>(id), static_cast<unsigned long>(geradas), static_cast<unsigned long>(c.amostrasConfirmadas),
                static_cast<unsigned long>(c.amostrasJaExistentes), static_cast<unsigned long>(c.amostrasDescartadas),
                static_cast<unsigned long>(controladorEnvio.amostrasPerdidas()), static_cast<unsigned long>(c.reenvios),
                static_cast<unsigned long>(quedas), static_cast<unsigned long>(trocas), redeWifi.nomeRedeAtual(),
                resumo.falhas == 0 ? "OK" : "FALHA");
  return resumo.falhas == 0;
}

/// `m`: sinal ao vivo, para medir o RSSI no ponto mais distante da pista (#491). A cada 0,5 s mostra a
/// rede, o RSSI e o mínimo, a média e o máximo desde o início. Segue reconectando como o robô: se a
/// principal cair, passa para a reserva, e a troca aparece na tela. Qualquer tecla para.
bool comandoMedirSinal() {
  console.println("--- Medir o sinal do Wi-Fi (#491) ---");
  if (!conectarWifi()) {
    return false;
  }
  console.println("  Leve o robo (bateria ligada) ao ponto mais distante da pista e ande com ele. Qualquer tecla para.");
  const uint32_t quedasAntes = reconexaoWifi.quedas();
  const uint32_t trocasAntes = reconexaoWifi.trocasDeRede();
  int minimo = 0;
  int maximo = -127;
  int32_t soma = 0;
  uint32_t leituras = 0;
  uint32_t abaixoDoFraco = 0;
  uint32_t semConexao = 0;
  const uint32_t inicioMs = millis();
  uint32_t proximaMs = inicioMs;
  while (!teclaPressionada()) {
    const uint32_t agoraMs = millis();
    const bool conectado = reconexaoWifi.ciclo(agoraMs);
    if (static_cast<int32_t>(agoraMs - proximaMs) >= 0) {
      proximaMs += SINAL_PERIODO_MS;
      const unsigned long segundos = static_cast<unsigned long>((agoraMs - inicioMs) / 1000);
      if (conectado) {
        const int rssi = WiFi.RSSI();
        if (leituras == 0 || rssi < minimo) {
          minimo = rssi;
        }
        if (rssi > maximo) {
          maximo = rssi;
        }
        soma += rssi;
        ++leituras;
        if (rssi < SINAL_FRACO_DBM) {
          ++abaixoDoFraco;
        }
        console.printf("  t=%3lu s | %-28s | %4d dBm %-9s | min %4d | media %4ld | max %4d\n", segundos,
                      descreverRedeAtual(), rssi, avaliarSinal(rssi), minimo, static_cast<long>(soma / static_cast<int32_t>(leituras)),
                      maximo);
      } else {
        ++semConexao;
        console.printf("  t=%3lu s | SEM CONEXAO, tentando a %s\n", segundos, descreverRedeAtual());
      }
    }
    delay(20);
  }
  if (leituras == 0) {
    console.println("  [FALHA] nenhuma leitura com o Wi-Fi conectado.");
    return false;
  }
  const long media = static_cast<long>(soma / static_cast<int32_t>(leituras));
  console.printf("  leituras %lu | min %d dBm | media %ld dBm | max %d dBm | abaixo de %d dBm: %lu | sem conexao: %lu\n",
                static_cast<unsigned long>(leituras), minimo, media, maximo, SINAL_FRACO_DBM,
                static_cast<unsigned long>(abaixoDoFraco), static_cast<unsigned long>(semConexao));
  console.printf("REGISTRO: sinal | rede_final=%s leituras=%lu min=%d media=%ld max=%d abaixo_de_%d=%lu sem_conexao=%lu quedas=%lu trocas=%lu | avaliacao_pior=%s\n",
                redeWifi.nomeRedeAtual(), static_cast<unsigned long>(leituras), minimo, media, maximo, SINAL_FRACO_DBM,
                static_cast<unsigned long>(abaixoDoFraco), static_cast<unsigned long>(semConexao),
                static_cast<unsigned long>(reconexaoWifi.quedas() - quedasAntes),
                static_cast<unsigned long>(reconexaoWifi.trocasDeRede() - trocasAntes), avaliarSinal(minimo));
  return minimo >= SINAL_FRACO_DBM;
}

/// `n` e `x`: corrida inteira sem rede (#491, #478). Cria a corrida com o Wi-Fi ligado, desliga o Wi-Fi,
/// gera `amostras` amostras no buffer do robô (o tempo de cada uma simula 1 por segundo, mas a bancada
/// as gera em poucos segundos), religa o Wi-Fi e espera o buffer esvaziar e a corrida encerrar.
/// `n` gera a capacidade do buffer (600 = 10 min): nada pode se perder. `x` gera 50 a mais: as 50 mais
/// antigas têm de ser descartadas, e o banco fica com as sequências 51 a 650.
bool comandoCorridaSemRede(uint16_t amostras) {
  const uint32_t perdasEsperadas = amostras > CAPACIDADE_BUFFER_AMOSTRAS ? amostras - CAPACIDADE_BUFFER_AMOSTRAS : 0;
  const uint32_t gravacoesEsperadas = amostras - perdasEsperadas;
  console.printf("--- Corrida sem rede: %u amostras com o Wi-Fi desligado (buffer de %u) ---\n", amostras,
                CAPACIDADE_BUFFER_AMOSTRAS);
  if (perdasEsperadas > 0) {
    console.printf("  O buffer estoura: as %lu amostras mais antigas devem ser descartadas.\n",
                  static_cast<unsigned long>(perdasEsperadas));
  }
  if (!conectarParaBanco()) {
    return false;
  }
  uint32_t id = 0;
  const uint32_t inicioCriacao = millis();
  const ResultadoEnvio criada = clienteSupabase.criarCorrida(LABIRINTO_CORRIDA_PADRAO, id);
  char detalhe[40] = "";
  if (criada.codigoHttp >= 200 && criada.codigoHttp < 300) {
    snprintf(detalhe, sizeof(detalhe), "corrida_id %lu", static_cast<unsigned long>(id));
  }
  if (!relatarChamada("criar corrida", criada.codigoHttp, millis() - inicioCriacao, detalhe)) {
    return false;
  }
  controladorEnvio.iniciarCorrida(id);
  clienteSupabase.definirResultadoCorrida(false);
  const uint32_t quedasAntes = reconexaoWifi.quedas();

  redeWifi.desconectar();
  console.println("  Wi-Fi desligado pela bancada; gerando as amostras no buffer. Qualquer tecla cancela.");
  for (uint16_t i = 0; i < amostras; ++i) {
    if (teclaPressionada()) {
      console.println("  [FALHA] cancelado por tecla; a corrida ficou aberta no banco.");
      return false;
    }
    AmostraTelemetria a;
    a.tempoDecorridoMs = static_cast<uint32_t>(i) * PERIODO_TELEMETRIA_MS;
    a.velocidadeMmPorS = 100;
    a.posicaoLinha = static_cast<uint8_t>((i / 4) % 4);
    a.posicaoColuna = static_cast<uint8_t>(i % 4);
    a.orientacao = 'N';
    a.estado = EstadoCorrida::Explorando;
    controladorEnvio.registrar(a);
    if ((i + 1) % SEM_REDE_PERIODO_LOG == 0 || i + 1 == amostras) {
      console.printf("  sem rede | geradas %3u | no buffer %3u | descartadas por buffer cheio %lu\n", i + 1,
                    controladorEnvio.ocupacao(), static_cast<unsigned long>(controladorEnvio.amostrasPerdidas()));
    }
    delay(2);
  }
  console.println("  fim da corrida; religando o Wi-Fi e esvaziando o buffer");
  const uint32_t inicioEnvioMs = millis();
  controladorEnvio.solicitarEncerramento(inicioEnvioMs);
  uint32_t proximoLogMs = inicioEnvioMs;
  while (!controladorEnvio.encerrada() && millis() - inicioEnvioMs < 2 * REENVIO_ESPERA_FINAL_MS) {
    if (teclaPressionada()) {
      console.println("  [FALHA] cancelado por tecla; a corrida ficou aberta no banco.");
      return false;
    }
    const uint32_t agoraMs = millis();
    const bool conectado = reconexaoWifi.ciclo(agoraMs);
    if (conectado) {
      controladorEnvio.ciclo(agoraMs);
    }
    if (static_cast<int32_t>(agoraMs - proximoLogMs) >= 0) {
      proximoLogMs = agoraMs + REENVIO_PERIODO_LOG_MS;
      console.printf("  t=%3lu s | wifi %-12s | no buffer %3u | confirmadas %3lu | falhas %lu\n",
                    static_cast<unsigned long>((agoraMs - inicioEnvioMs) / 1000), conectado ? redeWifi.nomeRedeAtual() : "SEM CONEXAO",
                    controladorEnvio.ocupacao(), static_cast<unsigned long>(controladorEnvio.contadores().amostrasConfirmadas),
                    static_cast<unsigned long>(controladorEnvio.contadores().falhas));
    }
    delay(20);
  }
  const uint32_t duracaoEnvioS = (millis() - inicioEnvioMs) / 1000;

  const ContadoresEnvio& c = controladorEnvio.contadores();
  console.printf("  geradas %u | confirmadas %lu | ja existentes %lu | descartadas pelo servidor %lu | perdidas no buffer %lu"
                " | no buffer %u | envio em %lu s\n",
                amostras, static_cast<unsigned long>(c.amostrasConfirmadas), static_cast<unsigned long>(c.amostrasJaExistentes),
                static_cast<unsigned long>(c.amostrasDescartadas), static_cast<unsigned long>(controladorEnvio.amostrasPerdidas()),
                controladorEnvio.ocupacao(), static_cast<unsigned long>(duracaoEnvioS));
  ResumoAutoteste resumo;
  registrar(resumo, controladorEnvio.amostrasPerdidas() == perdasEsperadas,
            "o buffer nao descartou o esperado: veja 'perdidas no buffer' acima",
            "buffer: %lu amostra(s) mais antiga(s) descartada(s), como esperado",
            static_cast<unsigned long>(controladorEnvio.amostrasPerdidas()));
  registrar(resumo, c.amostrasConfirmadas + c.amostrasJaExistentes == gravacoesEsperadas && c.amostrasDescartadas == 0,
            "o servidor nao gravou todas as amostras do buffer: veja 'descartadas pelo servidor' e as falhas",
            "as %lu amostras do buffer foram gravadas depois de religar o Wi-Fi", static_cast<unsigned long>(gravacoesEsperadas));
  registrar(resumo, controladorEnvio.encerrada(), "a corrida nao foi encerrada a tempo; veja as falhas acima",
            "corrida encerrada no servidor");
  console.printf("RESULTADO: %u OK, %u FALHA(S).\n", resumo.ok, resumo.falhas);
  console.printf("  Confira no banco (SQL Editor): select count(*), count(distinct numero_sequencia), min(numero_sequencia), "
                "max(numero_sequencia) from telemetria where corrida_id = %lu;  esperado: %lu, %lu, %lu, %u\n",
                static_cast<unsigned long>(id), static_cast<unsigned long>(gravacoesEsperadas),
                static_cast<unsigned long>(gravacoesEsperadas), static_cast<unsigned long>(perdasEsperadas + 1), amostras);
  console.printf("REGISTRO: sem_rede | corrida=%lu geradas=%u confirmadas=%lu existentes=%lu descartadas=%lu perdidas=%lu"
                " esperado_perdidas=%lu quedas=%lu rede_final=%s envio_s=%lu | veredito=%s\n",
                static_cast<unsigned long>(id), amostras, static_cast<unsigned long>(c.amostrasConfirmadas),
                static_cast<unsigned long>(c.amostrasJaExistentes), static_cast<unsigned long>(c.amostrasDescartadas),
                static_cast<unsigned long>(controladorEnvio.amostrasPerdidas()), static_cast<unsigned long>(perdasEsperadas),
                static_cast<unsigned long>(reconexaoWifi.quedas() - quedasAntes), redeWifi.nomeRedeAtual(),
                static_cast<unsigned long>(duracaoEnvioS), resumo.falhas == 0 ? "OK" : "FALHA");
  return resumo.falhas == 0;
}

/// Como uma rede da varredura bate com a do `credenciais.h`: igual, só na caixa das letras, ou não.
const char* compararSsid(const char* visto, const char* configurado) {
  if (configurado == nullptr || configurado[0] == '\0') {
    return nullptr;
  }
  if (strcmp(visto, configurado) == 0) {
    return "igual";
  }
  if (strcasecmp(visto, configurado) == 0) {
    return "CAIXA DIFERENTE";
  }
  return nullptr;
}

/// `v`: varre as redes de 2,4 GHz que a placa enxerga (#491) e marca a principal e a reserva do
/// `credenciais.h`. Mostra se uma delas não aparece ou aparece com maiúsculas diferentes (o SSID
/// diferencia maiúsculas: "iPhone" não é "Iphone").
bool comandoVarrerRedes() {
  console.println("--- Redes Wi-Fi visiveis (2,4 GHz) ---");
  iniciarWifiSeNecessario();
  redeWifi.desconectar();  // a varredura nao convive com uma tentativa em andamento; a reconexao volta depois
  delay(100);
  const int16_t encontradas = WiFi.scanNetworks();
  if (encontradas < 0) {
    console.printf("  [FALHA] a varredura falhou (%d).\n", encontradas);
    return false;
  }
  bool viuPrincipal = false;
  bool viuReserva = false;
  for (int16_t i = 0; i < encontradas; ++i) {
    const String ssid = WiFi.SSID(i);
    const char* principal = compararSsid(ssid.c_str(), redeWifi.ssid(0));
    const char* reserva = compararSsid(ssid.c_str(), redeWifi.ssid(1));
    viuPrincipal = viuPrincipal || (principal != nullptr && strcmp(principal, "igual") == 0);
    viuReserva = viuReserva || (reserva != nullptr && strcmp(reserva, "igual") == 0);
    char marca[40] = "";
    if (principal != nullptr) {
      snprintf(marca, sizeof(marca), "<- principal (%s)", principal);
    } else if (reserva != nullptr) {
      snprintf(marca, sizeof(marca), "<- reserva (%s)", reserva);
    }
    console.printf("  %2d  %-32s %4d dBm  canal %2d  %s  %s\n", i + 1, ssid.c_str(), static_cast<int>(WiFi.RSSI(i)),
                  static_cast<int>(WiFi.channel(i)), WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "aberta " : "senha  ", marca);
  }
  WiFi.scanDelete();
  console.printf("  %d rede(s). Principal \"%s\": %s.", encontradas, redeWifi.ssid(0),
                viuPrincipal ? "visivel" : "NAO VISTA");
  if (redeWifi.quantidadeRedes() > 1) {
    console.printf(" Reserva \"%s\": %s.", redeWifi.ssid(1), viuReserva ? "visivel" : "NAO VISTA");
  }
  console.println();
  if (!viuPrincipal || (redeWifi.quantidadeRedes() > 1 && !viuReserva)) {
    console.println("  dica: rede nao vista = desligada, em 5 GHz ou com outro nome. No iPhone, ative Maximizar");
    console.println("        Compatibilidade e deixe a tela do Acesso Pessoal aberta (sem cliente, o iOS esconde a rede).");
    console.println("        Para usar uma das redes acima, tecle p.");
  }
  return viuPrincipal && (redeWifi.quantidadeRedes() == 1 || viuReserva);
}

/// Lê uma linha digitada no monitor serial, com eco (`*` em `oculto`), Backspace e Enter. Esc ou
/// `ESPERA_DIGITACAO_MS` sem terminar cancelam.
/// @return `true` se a pessoa terminou com Enter (a linha pode ser vazia).
bool lerLinha(char* saida, size_t capacidade, bool oculto) {
  size_t tamanho = 0;
  saida[0] = '\0';
  delay(50);
  while (console.available() > 0) {  // descarta o resto da tecla anterior (o LF do CRLF)
    console.read();
  }
  const uint32_t inicio = millis();
  while (millis() - inicio < ESPERA_DIGITACAO_MS) {
    if (console.available() == 0) {
      delay(5);
      continue;
    }
    const char c = static_cast<char>(console.read());
    if (c == '\r' || c == '\n') {
      console.println();
      delay(20);
      while (console.available() > 0 && (console.peek() == '\n' || console.peek() == '\r')) {
        console.read();
      }
      return true;
    }
    if (c == 27) {  // Esc
      console.println(" (cancelado)");
      return false;
    }
    if (c == 8 || c == 127) {  // Backspace
      if (tamanho > 0) {
        --tamanho;
        saida[tamanho] = '\0';
        console.print("\b \b");
      }
      continue;
    }
    if (c >= 32 && c < 127 && tamanho + 1 < capacidade) {
      saida[tamanho++] = c;
      saida[tamanho] = '\0';
      console.print(oculto ? '*' : c);
    }
  }
  console.println(" (tempo esgotado)");
  return false;
}

/// `true` se `texto` é um número inteiro positivo; o valor vai para `numero`.
bool lerNumero(const char* texto, int& numero) {
  if (texto[0] == '\0') {
    return false;
  }
  numero = 0;
  for (const char* c = texto; *c != '\0'; ++c) {
    if (*c < '0' || *c > '9' || numero > 1000) {
      return false;
    }
    numero = numero * 10 + (*c - '0');
  }
  return numero > 0;
}

/// `p`: escolhe a rede pela serial, sem regravar a placa (#491). Varre as redes, mostra as 20 de sinal
/// mais forte numeradas, e quem testa digita o número (ou o nome exato) e a senha. Conectando, pergunta
/// se salva a rede na flash: salva, ela volta a ser a principal da bancada a cada boot, até a tecla `f`.
/// A reserva do `credenciais.h` continua valendo. O firmware do robô não usa esta rede.
bool comandoEscolherRede() {
  console.println("--- Escolher a rede Wi-Fi pela serial ---");
  iniciarWifiSeNecessario();
  redeWifi.desconectar();
  delay(100);
  console.println("  procurando redes de 2,4 GHz...");
  const int16_t encontradas = WiFi.scanNetworks();
  if (encontradas < 0) {
    console.printf("  [FALHA] a varredura falhou (%d).\n", encontradas);
    return false;
  }
  // Índices da varredura em ordem de sinal, sem nomes repetidos (vários pontos da mesma rede) e sem ocultas.
  int16_t lista[REDES_NA_LISTA];
  int16_t naLista = 0;
  bool usado[64] = {};
  while (naLista < REDES_NA_LISTA) {
    int16_t melhor = -1;
    for (int16_t i = 0; i < encontradas && i < 64; ++i) {
      if (!usado[i] && WiFi.SSID(i).length() > 0 && (melhor < 0 || WiFi.RSSI(i) > WiFi.RSSI(melhor))) {
        melhor = i;
      }
    }
    if (melhor < 0) {
      break;
    }
    for (int16_t i = 0; i < encontradas && i < 64; ++i) {
      if (WiFi.SSID(i) == WiFi.SSID(melhor)) {
        usado[i] = true;
      }
    }
    lista[naLista++] = melhor;
  }
  for (int16_t n = 0; n < naLista; ++n) {
    const int16_t i = lista[n];
    console.printf("  %2d  %-32s %4d dBm  %s\n", n + 1, WiFi.SSID(i).c_str(), static_cast<int>(WiFi.RSSI(i)),
                  WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "aberta" : "senha");
  }
  if (naLista == 0) {
    console.println("  nenhuma rede visivel. Ligue o hotspot em 2,4 GHz e tecle p de novo, ou digite o nome abaixo.");
  }
  console.println("  Redes de universidade (UNB Wireless, eduroam) nao funcionam: pedem login. Use um hotspot de celular.");
  console.print("  Numero da rede, ou o nome exato, e Enter (Esc cancela): ");
  char entrada[33];
  if (!lerLinha(entrada, sizeof(entrada), false) || entrada[0] == '\0') {
    WiFi.scanDelete();
    console.println("  nada mudou.");
    return false;
  }
  int numero = 0;
  bool aberta = false;
  char ssid[33];
  if (lerNumero(entrada, numero) && numero <= naLista) {
    const int16_t i = lista[numero - 1];
    snprintf(ssid, sizeof(ssid), "%s", WiFi.SSID(i).c_str());
    aberta = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
  } else {
    snprintf(ssid, sizeof(ssid), "%s", entrada);
  }
  WiFi.scanDelete();

  char senha[65] = "";
  if (aberta) {
    console.printf("  \"%s\" e aberta: sem senha.\n", ssid);
  } else {
    console.printf("  Senha de \"%s\" e Enter (vazia se for aberta; Esc cancela): ", ssid);
    if (!lerLinha(senha, sizeof(senha), true)) {
      console.println("  nada mudou.");
      return false;
    }
    const size_t tamanho = strlen(senha);
    if (tamanho > 0 && tamanho < 8) {
      console.println("  [FALHA] senha de Wi-Fi (WPA2) tem pelo menos 8 caracteres. Nada mudou.");
      return false;
    }
  }

  snprintf(ssidEscolhido, sizeof(ssidEscolhido), "%s", ssid);
  snprintf(senhaEscolhida, sizeof(senhaEscolhida), "%s", senha);
  redeEscolhidaSalva = false;
  redeWifi.definirRede(0, ssidEscolhido, senhaEscolhida);
  reconexaoWifi.reiniciar();
  if (!comandoWifi()) {
    console.println("  A rede fica escolhida ate reiniciar a placa; tecle p para tentar de novo (confira a senha).");
    return false;
  }
  console.print("  Salvar esta rede na placa para os proximos boots da bancada? (s/n): ");
  char resposta[4];
  if (lerLinha(resposta, sizeof(resposta), false) && (resposta[0] == 's' || resposta[0] == 'S')) {
    Preferences nvs;
    if (nvs.begin(NVS_REDE_BANCADA, false)) {
      nvs.putString("ssid", ssidEscolhido);
      nvs.putString("senha", senhaEscolhida);
      nvs.end();
      redeEscolhidaSalva = true;
      console.println("  [OK]    salva. Tecle f para esquecer. O firmware do robo continua usando o credenciais.h.");
    } else {
      console.println("  [FALHA] nao consegui gravar na flash; a rede vale ate reiniciar.");
    }
  } else {
    console.println("  nao salva: vale ate reiniciar a placa.");
  }
  return true;
}

/// `f`: esquece a rede salva pela tecla `p` e volta para a principal do `credenciais.h`.
void comandoEsquecerRede() {
  console.println("--- Esquecer a rede escolhida ---");
  iniciarWifiSeNecessario();
  Preferences nvs;
  if (nvs.begin(NVS_REDE_BANCADA, false)) {
    nvs.clear();
    nvs.end();
  }
  const bool havia = ssidEscolhido[0] != '\0';
  ssidEscolhido[0] = '\0';
  senhaEscolhida[0] = '\0';
  redeEscolhidaSalva = false;
  redeWifi.definirRede(0, credenciais.ssidWifi, credenciais.senhaWifi);
  reconexaoWifi.reiniciar();
  console.println(havia ? "  [OK]    rede escolhida esquecida." : "  nao havia rede escolhida pela tecla p.");
  imprimirRedes();
}

/// `s`: o que a bancada sabe agora.
void comandoStatus() {
  console.println("--- Status ---");
  console.printf("  Wi-Fi: %s", WiFi.status() == WL_CONNECTED ? "conectado" : "desconectado");
  if (WiFi.status() == WL_CONNECTED) {
    console.printf(" (%s, %s, %d dBm)", descreverRedeAtual(), WiFi.localIP().toString().c_str(), WiFi.RSSI());
  }
  console.println();
  if (WiFi.status() == WL_CONNECTED) {
    console.printf("  Remoto: telnet %s:%u (cliente %s), OTA \"%s.local\"\n", WiFi.localIP().toString().c_str(),
                   ConsoleBancada::PORTA_TELNET, console.clienteConectado() ? "conectado" : "nenhum", NOME_NA_REDE);
  }
  imprimirRedes();
  console.printf("  Redes conhecidas: %u. Quedas do Wi-Fi: %lu, tentativas de conexao: %lu, trocas de rede: %lu\n",
                static_cast<unsigned>(redeWifi.quantidadeRedes()), static_cast<unsigned long>(reconexaoWifi.quedas()),
                static_cast<unsigned long>(reconexaoWifi.tentativas()), static_cast<unsigned long>(reconexaoWifi.trocasDeRede()));
  if (corridaCriada) {
    console.printf("  Corrida: id %lu, %s, proxima sequencia %lu\n", static_cast<unsigned long>(corridaId),
                  corridaEncerrada ? "encerrada" : "aberta", static_cast<unsigned long>(proximaSequencia));
  } else {
    console.println("  Corrida: nenhuma criada nesta sessao");
  }
  console.printf("  Chamadas: %u OK, %u com falha\n", chamadasOk, chamadasComFalha);
}

void imprimirMenuTelemetria() {
  console.println();
  console.println("=== TELEMETRIA: Wi-Fi, HTTPS e banco ===");
  console.println(" ATENCAO: a, c e l gravam uma corrida e amostras de teste DE VERDADE no banco do credenciais.h.");
  console.println("          Use o projeto Supabase de testes ou apague a corrida depois, para nao encher o historico.");
  console.println(" a  FLUXO COMPLETO: faz tudo abaixo e diz se a placa fala com o banco");
  console.println(" p  ESCOLHER A REDE: lista as redes visiveis, voce digita o numero e a senha (sem regravar)");
  console.println(" f  esquecer a rede escolhida pela tecla p (volta ao credenciais.h)");
  console.println(" w  conectar no Wi-Fi");
  console.println(" d  resolver o nome do Supabase (DNS)");
  console.println(" c  criar uma corrida no banco");
  console.println(" l  enviar um lote de 5 amostras de teste");
  console.println(" r  reenviar o ultimo lote (confere que nao duplica)");
  console.println(" e  encerrar a corrida");
  console.println(" b  TESTE DE REENVIO: amostras ate o Wi-Fi cair e voltar (buffer do robo, #478)");
  console.println(" v  varrer as redes visiveis e conferir a principal e a reserva do credenciais.h (#491)");
  console.println(" m  medir o sinal ao vivo (RSSI, rede principal ou reserva) andando pela pista (#491)");
  console.println(" n  CORRIDA SEM REDE: 600 amostras (10 min) com o Wi-Fi desligado, depois envia tudo (#491)");
  console.println(" x  BUFFER CHEIO: 650 amostras sem rede; as 50 mais antigas devem ser descartadas (#478)");
  console.println(" s  status");
  console.println(" 0  voltar ao menu principal");
}

void executarTelemetria(char tecla) {
  switch (tecla) {
    case 'a':
      comandoFluxoCompleto();
      break;
    case 'w':
      comandoWifi();
      break;
    case 'd':
      comandoDns();
      break;
    case 'c':
      comandoCriarCorrida();
      break;
    case 'l':
      comandoEnviarLote();
      break;
    case 'r':
      comandoReenviarLote();
      break;
    case 'e':
      comandoEncerrarCorrida();
      break;
    case 'b':
      comandoTesteReenvio();
      break;
    case 'm':
      comandoMedirSinal();
      break;
    case 'v':
      comandoVarrerRedes();
      break;
    case 'p':
      comandoEscolherRede();
      break;
    case 'f':
      comandoEsquecerRede();
      break;
    case 'n':
      comandoCorridaSemRede(CAPACIDADE_BUFFER_AMOSTRAS);
      break;
    case 'x':
      comandoCorridaSemRede(CAPACIDADE_BUFFER_AMOSTRAS + SEM_REDE_EXCESSO);
      break;
    case 's':
      comandoStatus();
      break;
    case '0':
      tela = Tela::Principal;
      imprimirMenuPrincipal();
      break;
    default:
      imprimirMenuTelemetria();
      break;
  }
}
#endif

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
    case 'i':
    case 'I':
      testeIntegrado();
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
#if defined(BANCADA_COM_TELEMETRIA)
    case 'w':
    case 'W':
      tela = Tela::Telemetria;
      imprimirMenuTelemetria();
      break;
#endif
    case 'a':
    case 'A':
      imprimirBateria();
      break;
    case 'b':
    case 'B':
      lerBotaoContinuo();
      break;
    case 'r':
    case 'R':
      console.println("Reiniciando a placa... (se o monitor fechar, abra de novo)");
      console.flush();
      motores.parar();
      delay(200);
      ESP.restart();
      break;
    default:
      imprimirMenuPrincipal();
      break;
  }
}

/// Mostra o menu da tela em que a bancada está.
void imprimirMenuAtual() {
  switch (tela) {
    case Tela::Motores:
      imprimirMenuMotores();
      break;
#if defined(BANCADA_COM_TELEMETRIA)
    case Tela::Telemetria:
      imprimirMenuTelemetria();
      break;
#endif
    default:
      imprimirMenuPrincipal();
      break;
  }
}

/// Executa a tecla na tela em que a bancada está.
void executarTecla(char tecla) {
  switch (tela) {
    case Tela::Motores:
      executarMotores(tecla);
      break;
#if defined(BANCADA_COM_TELEMETRIA)
    case Tela::Telemetria:
      executarTelemetria(tecla);
      break;
#endif
    default:
      executarPrincipal(tecla);
      break;
  }
}

}  // namespace

void setup() {
  console.iniciar(115200);
  motores.iniciar();  // freados durante toda a bancada, até o submenu de motores
  pinMode(pinos::BOTAO_START, INPUT_PULLUP);
  analogSetPinAttenuation(pinos::BATERIA_ADC, ADC_11db);  // faixa até cerca de 3,1 V no pino
  barramento.iniciar(pinos::I2C_SDA, pinos::I2C_SCL);  // inicia o `Wire`
  encoderEsq.iniciar();
  encoderDir.iniciar();
  delay(500);  // tempo para o monitor serial abrir a porta USB
  reiniciarSensores();
#if defined(BANCADA_COM_TELEMETRIA)
  // Liga o Wi-Fi já no boot (sem bloquear) quando há rede salva ou no credenciais.h: assim o telnet e a
  // gravação sem cabo ficam disponíveis sem precisar do menu.
  iniciarWifiSeNecessario();
#endif
  imprimirMenuPrincipal();
}

void loop() {
#if defined(BANCADA_COM_TELEMETRIA)
  // Mantém o Wi-Fi vivo entre os comandos, depois que ele foi ligado a primeira vez.
  cuidarDaRede();
#endif
  static char anterior = 0;
  // Abrir o monitor reinicia a placa (DTR/RTS do conversor USB), então o menu sai no `setup()`.
  while (console.available() > 0) {
    const char tecla = static_cast<char>(console.read());
    const bool fimDeLinha = tecla == '\r' || tecla == '\n';
    // Enter pode chegar como CR, LF ou CRLF: mostra o menu uma vez só.
    if (fimDeLinha && !(tecla == '\n' && anterior == '\r')) {
      imprimirMenuAtual();
    } else if (!fimDeLinha) {
      executarTecla(tecla);
    }
    anterior = tecla;
  }
  delay(1);
}
