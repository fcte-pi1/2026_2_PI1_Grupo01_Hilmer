/// @file main.cpp
/// @brief Ponto de entrada do firmware: inicializa o sistema e cria as tarefas FreeRTOS.
///
/// Navegação e telemetria rodam em tarefas separadas e concorrentes, como no
/// diagrama de atividades. Os encoders físicos de quadratura são decodificados por
/// interrupção de hardware no ESP32-C3, alimentam a odometria (pose na malha) e são reportados na
/// telemetria.

#include <Arduino.h>

#include "ArmazenamentoMapa.h"
#include "Configuracao.h"
#include "ConversorDeslocamento.h"
#include "EncoderEsp32.h"
#include "MedidorDeslocamento.h"
#include "Odometria.h"
#include "Pinos.h"
#include "MotoresDrv8833.h"

/// Dono do mapa da corrida. Fica fora das tarefas para sobreviver ao reinício delas (HU-04).
/// A navegação recebe `armazenamentoMapa.mapa()` para registrar paredes; a telemetria, a versão
/// `const`, só para leitura. `iniciarCorrida()` é chamado quando o tamanho do labirinto for escolhido.
static micromouse::ArmazenamentoMapa armazenamentoMapa;

/// Encoders ópticos das rodas (roda direita invertida devido à montagem mecânica espelhada).
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
static portMUX_TYPE travaOdometria = portMUX_INITIALIZER_UNLOCKED;

/// Tarefa de navegação: ciclo percepção → decisão → atuação, a cada 10 ms.
///
/// Prioridade 2 (maior que a da telemetria), para que o envio de dados nunca
/// atrase o controle do robô.
static void tarefaNavegacao(void*) {
  for (;;) {
    // Odometria: o intervalo curto (10 ms) mantém cada atualização com giro ou avanço, não os dois.
    portENTER_CRITICAL(&travaOdometria);
    odometria.atualizar(encoderEsq, encoderDir);
    portEXIT_CRITICAL(&travaOdometria);

    // Ciclo percepção, decisão e atuação: percepção, mapeamento, navegação e atuação entram aqui.
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

/// Tarefa de telemetria: coleta e envia os dados a cada `PERIODO_TELEMETRIA_MS`.
///
/// Prioridade 1. Transmite as leituras dos encoders e a pose da odometria em formato JSON
/// estruturado e aceita o comando 'Z' para zerar os encoders e voltar a odometria à pose atual.
static void tarefaTelemetria(void*) {
  for (;;) {
    // Leitura dos encoders
    const micromouse::DeslocamentoRodas d = medidorDeslocamento.medir();
    const int32_t medio = medidorDeslocamento.deslocamentoMedioMm();
    const int32_t pulsosEsq = encoderEsq.lerContagem();
    const int32_t pulsosDir = encoderDir.lerContagem();

    // Pose da odometria: uma cópia feita na seção crítica, para a linha e a coluna serem da mesma
    // atualização.
    portENTER_CRITICAL(&travaOdometria);
    const micromouse::Pose pose = odometria.pose();
    const bool poseValida = odometria.posicaoValida();
    portEXIT_CRITICAL(&travaOdometria);

    Serial.printf("{\"esq_pulsos\":%ld,\"dir_pulsos\":%ld,\"esq_mm\":%ld,\"dir_mm\":%ld,\"medio_mm\":%ld,"
                  "\"linha\":%u,\"coluna\":%u,\"orientacao\":\"%c\",\"pose_valida\":%s}\n",
                  static_cast<long>(pulsosEsq),
                  static_cast<long>(pulsosDir),
                  static_cast<long>(d.esquerdaMm),
                  static_cast<long>(d.direitaMm),
                  static_cast<long>(medio),
                  static_cast<unsigned>(pose.celula.linha),
                  static_cast<unsigned>(pose.celula.coluna),
                  micromouse::letraDaDirecao(pose.direcao),
                  poseValida ? "true" : "false");

    // Comando serial para zerar medição (usado na bancada)
    while (Serial.available()) {
      const char c = static_cast<char>(Serial.read());
      if (c == 'Z' || c == 'z') {
        // Os encoders voltam a zero: a odometria mantém a pose e passa a contar a partir de zero.
        portENTER_CRITICAL(&travaOdometria);
        medidorDeslocamento.zerar();
        odometria.reiniciar(odometria.pose(), 0, 0);
        portEXIT_CRITICAL(&travaOdometria);
        Serial.println("{\"status\":\"encoders_zerados\"}");
      }
    }

    vTaskDelay(pdMS_TO_TICKS(micromouse::PERIODO_TELEMETRIA_MS));
  }
}

/// Motores de tração pelo DRV8833 (M1 à esquerda, M2 à direita). O direito fica invertido, como o
/// encoder, pela montagem espelhada; confirme na bancada que `definirPotencia({300, 300})` faz as
/// duas rodas andarem para a frente. O `ControleMovimento` (#191) entra na tarefa de navegação
/// junto com o LiDAR.
static micromouse::MotoresDrv8833 motores(micromouse::pinos::MOTOR_A_IN1,
                                          micromouse::pinos::MOTOR_A_IN2,
                                          micromouse::pinos::MOTOR_B_IN1,
                                          micromouse::pinos::MOTOR_B_IN2,
                                          false,
                                          true);

/// Inicializa o log serial (115200 baud), os encoders e cria as duas tarefas.
void setup() {
  Serial.begin(115200);
  Serial.println("micromouse: iniciando");

  motores.iniciar();  // começa freado
  encoderEsq.iniciar();
  encoderDir.iniciar();

  xTaskCreate(tarefaNavegacao, "navegacao", 4096, nullptr, 2, nullptr);
  xTaskCreate(tarefaTelemetria, "telemetria", 4096, nullptr, 1, nullptr);
}

/// Laço principal do Arduino. Fica ocioso: o trabalho acontece nas tarefas.
void loop() {
  delay(1000);
}
