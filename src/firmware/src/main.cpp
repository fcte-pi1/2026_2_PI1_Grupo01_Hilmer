/// @file main.cpp
/// @brief Ponto de entrada do firmware: inicializa o sistema e cria as tarefas FreeRTOS.
///
/// Navegação e telemetria rodam em tarefas separadas e concorrentes, como no
/// diagrama de atividades. Os encoders físicos de quadratura são decodificados por
/// interrupção de hardware no ESP32-C3, alimentam a odometria (pose na malha) e são reportados na
/// telemetria. O giroscópio do MPU6050 é lido por uma tarefa própria, em intervalo fixo, pelo
/// barramento I²C que ele divide com os sensores de distância.

#include <Arduino.h>
#include <Wire.h>

// O log (`Serial`) tem que sair pela USB nativa: GPIO20/21, a UART0, são entradas do driver dos
// motores. Se esta opção faltar no `platformio.ini`, o log iria para os pinos do motor direito.
#if !defined(ARDUINO_USB_CDC_ON_BOOT) || !ARDUINO_USB_CDC_ON_BOOT
#error "Serial pela USB nativa: defina ARDUINO_USB_MODE=1 e ARDUINO_USB_CDC_ON_BOOT=1 no platformio.ini (Pinos.h)"
#endif

#include "AcompanhadorGiro.h"
#include "ArmazenamentoMapa.h"
#include "BarramentoI2c.h"
#include "Configuracao.h"
#include "ConfiguracaoImu.h"
#include "ConversorDeslocamento.h"
#include "DetectorObjetivo.h"
#include "EncoderEsp32.h"
#include "ImuMpu6050.h"
#include "MedidorDeslocamento.h"
#include "MotoresDrv8833.h"
#include "Odometria.h"
#include "Pinos.h"
#include "TelemetriaJson.h"

/// Dono do mapa da corrida. Fica fora das tarefas para sobreviver ao reinício delas (HU-04).
/// A navegação recebe `armazenamentoMapa.mapa()` para registrar paredes; a telemetria, a versão
/// `const`, só para leitura. `iniciarCorrida()` é chamado quando o tamanho do labirinto for escolhido.
static micromouse::ArmazenamentoMapa armazenamentoMapa;

/// Encoders das rodas (roda direita invertida devido à montagem mecânica espelhada). Na Rev 2.0 os
/// pinos estão reservados, mas os encoders ainda não estão instalados.
static micromouse::EncoderEsp32 encoderEsq(micromouse::pinos::ENCODER_ESQ_A,
                                           micromouse::pinos::ENCODER_ESQ_B,
                                           false);
static micromouse::EncoderEsp32 encoderDir(micromouse::pinos::ENCODER_DIR_A,
                                           micromouse::pinos::ENCODER_DIR_B,
                                           true);

static micromouse::ConversorDeslocamento conversorDeslocamento;
static micromouse::MedidorDeslocamento medidorDeslocamento(encoderEsq, encoderDir, conversorDeslocamento);

/// Pose (célula e direção) estimada pela odometria diferencial. A tarefa de navegação a atualiza a
/// cada 10 ms e a telemetria a lê, então todo acesso passa pela seção crítica `travaOdometria`.
static micromouse::Odometria odometria;

/// Detector de chegada à célula objetivo.
/// NOTA PROVISÓRIA (#189): Inicializado provisoriamente em DimensaoMapa::Labirinto4x4 para validação
/// em bancada. Em labirintos 8x4 ou 12x4, a chegada não seria detectada com essa dimensão fixa.
/// A dimensão deverá ser configurada dinamicamente via comando/configuração de corrida quando a seleção
/// de tamanho for recebida pelo robô.
/// Além disso, esta instância estática coexiste temporariamente com Navegacao::detector_ e será unificada
/// assim que o ciclo de navegação assumir o controle do laço de percepção/decisão.
static micromouse::DetectorObjetivo detectorObjetivo(micromouse::DimensaoMapa::Labirinto4x4);
static portMUX_TYPE travaOdometria = portMUX_INITIALIZER_UNLOCKED;

/// Motores de tração pelo DRV8833 (IN1/IN2 no motor esquerdo, IN3/IN4 no direito). O direito fica
/// invertido, como o encoder, pela montagem espelhada; confirme na bancada que
/// `definirPotencia({300, 300})` faz as duas rodas andarem para a frente. O `ControleMovimento`
/// (#191) entra na tarefa de navegação junto com os sensores de distância.
static micromouse::MotoresDrv8833 motores(micromouse::pinos::MOTOR_ESQ_IN1,
                                          micromouse::pinos::MOTOR_ESQ_IN2,
                                          micromouse::pinos::MOTOR_DIR_IN3,
                                          micromouse::pinos::MOTOR_DIR_IN4,
                                          false,
                                          true);

/// Barramento I²C dos 3 VL53L0X e do MPU6050. Toda transação passa pelo mutex dele (`BarramentoI2c::Uso`),
/// porque a tarefa da IMU e a navegação (sensores de distância) usam o barramento ao mesmo tempo.
static micromouse::BarramentoI2c barramentoI2c(Wire);

/// Giroscópio do MPU6050 (#378) e o acompanhamento do ângulo de giro. Só a tarefa da IMU os usa; as
/// outras tarefas leem a cópia `leituraGiro` por `lerGiro()`.
static micromouse::ImuMpu6050 imu(barramentoI2c);
static micromouse::AcompanhadorGiro acompanhadorGiro(imu);
static micromouse::LeituraGiro leituraGiro;
static portMUX_TYPE travaGiro = portMUX_INITIALIZER_UNLOCKED;

/// Cópia da última leitura do giroscópio: taxa e ângulo de giro (para os giros de 90° e 180°, #190,
/// e para a proteção dos motores, #341), com a validade e os contadores.
[[maybe_unused]] static micromouse::LeituraGiro lerGiro() {
  portENTER_CRITICAL(&travaGiro);
  const micromouse::LeituraGiro copia = leituraGiro;
  portEXIT_CRITICAL(&travaGiro);
  return copia;
}

/// Tarefa da IMU: lê o giroscópio a cada `PERIODO_LEITURA_IMU_MS` (10 ms), em intervalo fixo.
///
/// Prioridade 3, a maior: a leitura é curta (menos de 1 ms) e o ângulo só fica certo se o intervalo
/// entre leituras não variar. Nos primeiros 2 s calibra o viés, com o robô parado na largada; se o
/// robô se mexer, a calibração recomeça sozinha.
static void tarefaImu(void*) {
  acompanhadorGiro.iniciarCalibracao();
  bool calibradoAnunciado = false;
  TickType_t ultimoInstante = xTaskGetTickCount();
  for (;;) {
    vTaskDelayUntil(&ultimoInstante, pdMS_TO_TICKS(micromouse::PERIODO_LEITURA_IMU_MS));
    // O `vTaskDelayUntil` acorda em múltiplos exatos do período, então o intervalo é o período.
    const micromouse::LeituraGiro leitura = acompanhadorGiro.ciclo(micromouse::PERIODO_LEITURA_IMU_MS);

    portENTER_CRITICAL(&travaGiro);
    leituraGiro = leitura;
    portEXIT_CRITICAL(&travaGiro);

    if (!calibradoAnunciado && leitura.calibracao == micromouse::EstadoCalibracao::Calibrado) {
      calibradoAnunciado = true;
      Serial.printf("{\"status\":\"giroscopio_calibrado\",\"recusadas\":%lu}\n",
                    static_cast<unsigned long>(leitura.calibracoesRecusadas));
    }
  }
}

/// Tarefa de navegação: ciclo percepção → decisão → atuação, a cada 10 ms.
///
/// Prioridade 2 (maior que a da telemetria), para que o envio de dados nunca
/// atrase o controle do robô.
static void tarefaNavegacao(void*) {
  for (;;) {
    bool alcancou = false;
    // Odometria e detecção de chegada executam na seção crítica travaOdometria para sincronizar com tarefaTelemetria.
    portENTER_CRITICAL(&travaOdometria);
    odometria.atualizar(encoderEsq, encoderDir);
    const micromouse::Pose pose = odometria.pose();
    const bool poseValida = odometria.posicaoValida();
    if (poseValida) {
      // Chegada detectada pela posição na malha: para os motores imediatamente e congela o mapa final (HU-07 / #189)
      alcancou = detectorObjetivo.processarChegada(pose, motores, &armazenamentoMapa);
    } else {
      alcancou = detectorObjetivo.objetivoAlcancado();
    }
    portEXIT_CRITICAL(&travaOdometria);

    if (alcancou) {
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }

    // Ciclo percepção, decisão e atuação: percepção, mapeamento, navegação e atuação entram aqui.
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

/// Tarefa de telemetria: coleta e envia os dados a cada `PERIODO_TELEMETRIA_MS`.
///
/// Prioridade 1. Transmite as leituras dos encoders, pose da odometria e eventos pontuais em formato JSON
/// estruturado e aceita o comando 'Z' para zerar os encoders e voltar a odometria à pose atual.
static void tarefaTelemetria(void*) {
  static uint32_t sequencia = 0;

  for (;;) {
    // Leitura dos encoders
    const micromouse::DeslocamentoRodas d = medidorDeslocamento.medir();
    const int32_t medio = medidorDeslocamento.deslocamentoMedioMm();
    const int32_t pulsosEsq = encoderEsq.lerContagem();
    const int32_t pulsosDir = encoderDir.lerContagem();

    // Pose da odometria e evento pontual consumidos de forma atômica na seção crítica
    portENTER_CRITICAL(&travaOdometria);
    const micromouse::Pose pose = odometria.pose();
    const bool poseValida = odometria.posicaoValida();
    const bool dispararEvento = detectorObjetivo.consumirEventoChegada();
    portEXIT_CRITICAL(&travaOdometria);

    micromouse::PacoteTelemetria pacote;
    pacote.numeroSequencia = ++sequencia;
    pacote.pulsosEsq = pulsosEsq;
    pacote.pulsosDir = pulsosDir;
    pacote.esqMm = d.esquerdaMm;
    pacote.dirMm = d.direitaMm;
    pacote.medioMm = medio;
    pacote.pose = pose;
    pacote.poseValida = poseValida;
    pacote.velocidade = 0.0f;     // Provisório até inclusão de cálculo de velocidade instantânea
    pacote.tensaoBateria = 0.0f;  // Provisório até integração de leitura analógica da bateria
    pacote.evento = dispararEvento ? "objetivo_alcancado" : nullptr;

    char buffer[256];
    if (micromouse::TelemetriaJson::serializar(pacote, buffer, sizeof(buffer)) > 0) {
      Serial.println(buffer);
    }

    // Comando serial para zerar medição (usado na bancada)
    while (Serial.available()) {
      const char c = static_cast<char>(Serial.read());
      if (c == 'Z' || c == 'z') {
        // Os encoders voltam a zero: a odometria mantém a pose e passa a contar a partir de zero.
        portENTER_CRITICAL(&travaOdometria);
        medidorDeslocamento.zerar();
        odometria.reiniciar(odometria.pose(), 0, 0);
        detectorObjetivo.resetar();
        portEXIT_CRITICAL(&travaOdometria);
        Serial.println("{\"status\":\"encoders_zerados\"}");
      }
    }

    vTaskDelay(pdMS_TO_TICKS(micromouse::PERIODO_TELEMETRIA_MS));
  }
}

/// Inicializa o log serial pela USB nativa, os encoders, o barramento I²C e a IMU, e cria as tarefas.
void setup() {
  Serial.begin(115200);
  // Sem um computador conectado, a escrita na USB CDC esperaria o tempo limite a cada chamada e
  // atrasaria a telemetria. Com tempo zero, o dado é descartado se ninguém estiver lendo.
  Serial.setTxTimeoutMs(0);
  Serial.println("micromouse: iniciando");

  // Configuração provisória para bancada (#189): fixa Labirinto4x4.
  // Em 8x4 e 12x4 a corrida deve receber a dimensão por comando antes de iniciar.
  armazenamentoMapa.iniciarCorrida(micromouse::DimensaoMapa::Labirinto4x4);
  motores.iniciar();  // começa freado
  encoderEsq.iniciar();
  encoderDir.iniciar();

  // Barramento I²C (ToF e IMU) e o MPU6050. Sem a IMU o firmware segue, sem ângulo de giro válido.
  if (!barramentoI2c.iniciar(micromouse::pinos::I2C_SDA, micromouse::pinos::I2C_SCL)) {
    Serial.println("{\"status\":\"falha_barramento_i2c\"}");
  }
  if (imu.iniciar()) {
    Serial.printf("{\"status\":\"imu_iniciada\",\"who_am_i\":\"0x%02X\"}\n", imu.identificacao());
  } else {
    Serial.println("{\"status\":\"falha_imu\"}");
  }

  xTaskCreate(tarefaImu, "imu", 3072, nullptr, 3, nullptr);
  xTaskCreate(tarefaNavegacao, "navegacao", 4096, nullptr, 2, nullptr);
  xTaskCreate(tarefaTelemetria, "telemetria", 4096, nullptr, 1, nullptr);
}

/// Laço principal do Arduino. Fica ocioso: o trabalho acontece nas tarefas.
void loop() {
  delay(1000);
}
