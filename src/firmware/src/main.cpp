/// @file main.cpp
/// @brief Ponto de entrada do firmware: inicializa o sistema e cria as tarefas FreeRTOS.
///
/// Navegação e telemetria rodam em tarefas separadas e concorrentes, como no
/// diagrama de atividades. Os encoders físicos de quadratura são decodificados por
/// interrupção de hardware no ESP32-C3, alimentam a odometria (pose na malha) e são reportados na
/// telemetria. Os 3 sensores de distância VL53L0X são lidos pelo I²C na tarefa de navegação, e o
/// giroscópio do MPU6050 por uma tarefa própria, em intervalo fixo, no mesmo barramento (com mutex).
/// O deslocamento vem dos encoders (`ENCODERS_INSTALADOS` em `ConfiguracaoDeslocamento.h`, #475). O
/// modo sem encoder, estimado pela potência dos motores e corrigido pelos ToF (#380), é o plano B.
/// Os motores só são liberados depois do autoteste do boot (#50) encontrar os 4 dispositivos do I²C.
/// A navegação percebe as paredes no centro de cada célula, grava no mapa, decide pelo Flood Fill e
/// executa um movimento por vez (avançar uma célula, girar 90° ou 180°, #184), depois da largada
/// pelo gesto da mão na frente do ToF frontal (ou do comando serial `L`). As paredes corrigem o erro
/// acumulado da odometria, e o robô se realinha ao centro do corredor antes de girar no lugar (#188).
/// Uma terceira tarefa, a de rede, envia a telemetria ao Supabase sem nunca fazer a navegação
/// esperar (#238).
/// Durante a corrida, uma falha no I²C para os motores e reinicia o dispositivo que falhou (#241).

#include <Arduino.h>
#include <Wire.h>

// O log (`Serial`) tem que sair pela USB nativa: GPIO20/21, a UART0, são entradas do driver dos
// motores. Se esta opção faltar no `platformio.ini`, o log iria para os pinos do motor direito.
#if !defined(ARDUINO_USB_CDC_ON_BOOT) || !ARDUINO_USB_CDC_ON_BOOT
#error "Serial pela USB nativa: defina ARDUINO_USB_MODE=1 e ARDUINO_USB_CDC_ON_BOOT=1 no platformio.ini (Pinos.h)"
#endif

#include "AcompanhadorGiro.h"
#include "ArmazenamentoMapa.h"
#include "AutotesteBoot.h"
#include "BarramentoI2c.h"
#include "BarramentoI2cEsp32.h"
#include "CicloNavegacao.h"
#include "Configuracao.h"
#include "ConfiguracaoDeslocamento.h"
#include "ConfiguracaoImu.h"
#include "ConfiguracaoRede.h"
#include "ContratoTelemetria.h"
#include "ControladorEnvio.h"
#include "ConversorDeslocamento.h"
#include "CorrecaoPelasParedes.h"
#include "CorretorDeslocamentoTof.h"
#include "CredenciaisSupabase.h"
#include "DeslocamentoEstimado.h"
#include "DetectorColisao.h"
#include "DetectorObjetivo.h"
#include "EncoderEsp32.h"
#include "EnvioHttpSupabase.h"
#include "FaseCorrida.h"
#include "GatilhoLargada.h"
#include "ImuMpu6050.h"
#include "MedidorDeslocamento.h"
#include "MotoresComProtecao.h"
#include "MotoresComTrava.h"
#include "MotoresDrv8833.h"
#include "Odometria.h"
#include "Pinos.h"
#include "PoliticaReenvio.h"
#include "ReconexaoWifi.h"
#include "RedeWifiEsp32.h"
#include "RegistradorPotencia.h"
#include "RelatorioBoot.h"
#include "RotinaCalibracao.h"
#include "SupervisorI2c.h"
#include "TelemetriaJson.h"
#include "TofVl53l0x.h"
#include "TravaFalhaI2c.h"
#include "TravaSecaoCritica.h"
#include "VigiaTof.h"

// Credenciais do Wi-Fi e do Supabase: ficam em `include/credenciais.h`, que o Git ignora (o modelo é
// `include/credenciais.exemplo.h`). Sem o arquivo, o firmware compila e funciona sem enviar nada.
#if __has_include("credenciais.h")
#include "credenciais.h"
#define MICROMOUSE_TEM_CREDENCIAIS 1
#else
#define MICROMOUSE_TEM_CREDENCIAIS 0
#endif

/// Dono do mapa da corrida. Fica fora das tarefas para sobreviver ao reinício delas (HU-04).
/// A navegação recebe `armazenamentoMapa.mapa()` para registrar paredes; a telemetria, a versão
/// `const`, só para leitura. `iniciarCorrida()` é chamado quando o tamanho do labirinto for escolhido.
static micromouse::ArmazenamentoMapa armazenamentoMapa;

/// Encoders das rodas (roda direita invertida devido à montagem mecânica espelhada). Estão nos motores
/// N20 e são iniciados no `setup()` quando `ENCODERS_INSTALADOS` é verdadeiro (o padrão).
static micromouse::EncoderEsp32 encoderEsq(micromouse::pinos::ENCODER_ESQ_A,
                                           micromouse::pinos::ENCODER_ESQ_B,
                                           false);
static micromouse::EncoderEsp32 encoderDir(micromouse::pinos::ENCODER_DIR_A,
                                           micromouse::pinos::ENCODER_DIR_B,
                                           true);

/// Motores de tração pelo DRV8833 (IN1/IN2 no motor esquerdo, IN3/IN4 no direito). O direito fica
/// invertido, como o encoder, pela montagem espelhada; confirme na bancada que
/// `definirPotencia({300, 300})` faz as duas rodas andarem para a frente.
static micromouse::MotoresDrv8833 motores(micromouse::pinos::MOTOR_ESQ_IN1,
                                          micromouse::pinos::MOTOR_ESQ_IN2,
                                          micromouse::pinos::MOTOR_DIR_IN3,
                                          micromouse::pinos::MOTOR_DIR_IN4,
                                          false,
                                          true);

/// Deslocamento sem encoder (#380): estima o avanço pela potência comandada e o corrige pelos ToF.
/// Quem acionar as rodas (o `CicloNavegacao`, #184) deve usar `motoresTracao`, para o
/// estimador saber a potência; com os encoders instalados, a estimativa sai e a trava envolve o
/// próprio driver.
///
/// A estimativa fica colada no driver, para ver só o que chega de fato às rodas. Os outros
/// decoradores a envolvem, nesta ordem: `RegistradorPotencia` (#192) → `MotoresComTrava` (#50) →
/// `MotoresComProtecao` (#341) → `MotoresComEstimativa` → `MotoresDrv8833`. Assim um comando recusado
/// pela trava ou pela proteção chega ao estimador como freio.
static micromouse::DeslocamentoEstimado deslocamento(motores);
static micromouse::IMotores& motoresEstimativa =
    micromouse::ENCODERS_INSTALADOS ? static_cast<micromouse::IMotores&>(motores) : deslocamento.motores();

/// Proteção contra motor travado (#341): vê todo comando de potência e, com potência alta por 500 ms
/// sem o robô se mexer, freia e segura o freio até `liberar()`. Quem fecha o ciclo é a tarefa de
/// navegação, que chama `atualizarProtecao()` com o ToF frontal e o giroscópio.
static micromouse::MotoresComProtecao motoresProtegidos(motoresEstimativa);

/// Trava dos motores (#50): começa travada e só é liberada com o autoteste do boot aprovado.
static micromouse::MotoresComTrava motoresComTrava(motoresProtegidos);

/// Guarda a última potência comandada, para a detecção de colisão (#192) saber se o robô foi mandado
/// andar. É a camada de fora de todas e só a tarefa de navegação a usa.
static micromouse::RegistradorPotencia registradorPotencia(motoresComTrava);

/// Motores que a navegação aciona: sempre a camada de fora, nunca o driver direto, para a trava
/// valer. Cada camada tem o próprio nome e esta referência aponta para a de fora. Com a trava por
/// fora, a proteção (#341) recebe freio enquanto o robô não foi liberado e não acusa um motor
/// travado falso.
static micromouse::IMotores& motoresTracao = registradorPotencia;

/// Encoders usados pela odometria e pela telemetria: os reais, se instalados; senão, os estimados.
/// A troca é só aqui: a `Odometria` e o `MedidorDeslocamento` não mudam.
static micromouse::IEncoder& rodaEsq =
    micromouse::ENCODERS_INSTALADOS ? static_cast<micromouse::IEncoder&>(encoderEsq) : deslocamento.encoderEsquerdo();
static micromouse::IEncoder& rodaDir =
    micromouse::ENCODERS_INSTALADOS ? static_cast<micromouse::IEncoder&>(encoderDir) : deslocamento.encoderDireito();

static micromouse::ConversorDeslocamento conversorDeslocamento;
static micromouse::MedidorDeslocamento medidorDeslocamento(rodaEsq, rodaDir, conversorDeslocamento);

/// Pose (célula e direção) estimada pela odometria diferencial. A tarefa de navegação a atualiza a
/// cada 10 ms e a telemetria a lê, então todo acesso passa pela seção crítica `travaOdometria`. A
/// mesma trava protege o estimador do deslocamento, que as duas tarefas também usam.
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

/// Resultado do autoteste do boot, para a telemetria repetir em todo pacote. Só o `setup()` escreve,
/// antes de criar as tarefas; depois é só lido, então não precisa de seção crítica.
struct EstadoBoot {
  bool ok = false;
  char falhas[micromouse::TAMANHO_MAX_FALHAS_BOOT] = "";
  uint32_t duracaoMs = 0;
  micromouse::ResultadoBoot resultado;
};
static EstadoBoot estadoBoot;
static const micromouse::AutotesteBoot autoteste;

/// Escreve no log a linha JSON do relatório do boot (`{"tipo":"boot",...}`).
static void imprimirRelatorioBoot() {
  char relatorio[micromouse::TAMANHO_MAX_JSON_RELATORIO_BOOT];
  if (micromouse::RelatorioBoot::serializar(estadoBoot.resultado, autoteste, estadoBoot.duracaoMs, relatorio,
                                            sizeof(relatorio)) > 0) {
    Serial.println(relatorio);
  }
}

/// Sensores de distância VL53L0X (#344). Só a tarefa de navegação fala com eles pelo I²C; a
/// telemetria lê a cópia `ultimaLeituraTof`, protegida pela seção crítica `travaTof`.
static micromouse::TofVl53l0x tof(Wire,
                                  {micromouse::pinos::XSHUT_FRONTAL, micromouse::pinos::ENDERECO_TOF_FRONTAL},
                                  {micromouse::pinos::XSHUT_ESQ, micromouse::pinos::ENDERECO_TOF_ESQ},
                                  {micromouse::pinos::XSHUT_DIR, micromouse::pinos::ENDERECO_TOF_DIR});

/// Última leitura dos sensores de distância, copiada pela navegação para a telemetria.
struct LeituraTofTelemetria {
  micromouse::DistanciasLaterais distancias = {0, 0, 0};
  bool valida = false;
  uint32_t idadeMs = 0;
};
static LeituraTofTelemetria ultimaLeituraTof;
static portMUX_TYPE travaTof = portMUX_INITIALIZER_UNLOCKED;

/// Barramento I²C dos 3 VL53L0X e do MPU6050. Toda transação passa pelo mutex dele (`BarramentoI2c::Uso`),
/// porque a tarefa da IMU e a navegação (sensores de distância) usam o barramento ao mesmo tempo.
static micromouse::BarramentoI2c barramentoI2c(Wire);

/// Giroscópio do MPU6050 (#378) e o acompanhamento do ângulo de giro. Só a tarefa da IMU os usa; as
/// outras tarefas leem a cópia `leituraGiro` por `lerGiro()`.
static micromouse::ImuMpu6050 imu(barramentoI2c);
static micromouse::AcompanhadorGiro acompanhadorGiro(imu);
static micromouse::LeituraGiro leituraGiro;
static portMUX_TYPE travaGiro = portMUX_INITIALIZER_UNLOCKED;

/// Última aceleração do acelerômetro do mesmo MPU6050, no plano do chão, para a detecção de colisão
/// (#192). A tarefa da IMU a escreve, e a navegação a copia, sempre sob `travaGiro`.
struct AceleracaoPlano {
  int16_t axMg = 0;
  int16_t ayMg = 0;
  bool valida = false;
};
static AceleracaoPlano ultimaAceleracao;

/// Cópia da última leitura do giroscópio: taxa e ângulo de giro (para os giros de 90° e 180°, #190,
/// e para a proteção dos motores, #341), com a validade e os contadores.
static micromouse::LeituraGiro lerGiro() {
  portENTER_CRITICAL(&travaGiro);
  const micromouse::LeituraGiro copia = leituraGiro;
  portEXIT_CRITICAL(&travaGiro);
  return copia;
}

/// Cópia da última aceleração no plano (#192).
static AceleracaoPlano lerAceleracao() {
  portENTER_CRITICAL(&travaGiro);
  const AceleracaoPlano copia = ultimaAceleracao;
  portEXIT_CRITICAL(&travaGiro);
  return copia;
}

/// Falhas do I²C durante a corrida (#241): o supervisor conta as falhas de cada um dos 4
/// dispositivos, decide quando reiniciar e quando parar os motores. A navegação registra os ToF, a
/// tarefa da IMU registra o MPU6050 e a telemetria lê o resumo, então todo acesso passa pela seção
/// crítica `travaFalhasI2c`. Os reinícios e a trava dos motores ficam fora dela, porque esperam.
static micromouse::SupervisorI2c supervisorI2c;
static portMUX_TYPE travaFalhasI2c = portMUX_INITIALIZER_UNLOCKED;

/// Só a navegação usa: transforma as amostras dos ToF em leituras para o supervisor.
static micromouse::VigiaTof vigiaTof;

/// Só a navegação usa: trava os motores enquanto o supervisor pedir, sem soltar a trava do boot.
static micromouse::TravaFalhaI2c travaFalhaI2c;

/// Registra a leitura de um dispositivo no supervisor e escreve no log a mudança de situação
/// (`{"status":"i2c","dispositivo":"tof_frontal","situacao":"em_falha"}`).
static micromouse::DecisaoI2c registrarLeituraI2c(micromouse::Dispositivo dispositivo,
                                                  micromouse::LeituraI2c leitura) {
  const uint32_t agoraMs = millis();
  portENTER_CRITICAL(&travaFalhasI2c);
  const micromouse::DecisaoI2c decisao = supervisorI2c.registrarLeitura(dispositivo, leitura, agoraMs);
  portEXIT_CRITICAL(&travaFalhasI2c);
  if (decisao.mudou()) {
    Serial.printf("{\"status\":\"i2c\",\"dispositivo\":\"%s\",\"situacao\":\"%s\"}\n",
                  micromouse::nomeDispositivo(dispositivo), micromouse::nomeSituacao(decisao.depois));
  }
  return decisao;
}

/// Registra o resultado de um reinício pedido pelo supervisor e o escreve no log
/// (`{"status":"reinicio_i2c","dispositivo":"tof_frontal","sucesso":true}`).
static void registrarReinicioI2c(micromouse::Dispositivo dispositivo, bool sucesso) {
  portENTER_CRITICAL(&travaFalhasI2c);
  supervisorI2c.registrarReinicio(dispositivo, sucesso);
  portEXIT_CRITICAL(&travaFalhasI2c);
  Serial.printf("{\"status\":\"reinicio_i2c\",\"dispositivo\":\"%s\",\"sucesso\":%s}\n",
                micromouse::nomeDispositivo(dispositivo), sucesso ? "true" : "false");
}

/// Para os motores enquanto algum dispositivo estiver em falha e os solta quando todos voltarem,
/// sem soltar a trava do boot reprovado. Só a navegação chama. Devolve `true` durante a falha.
static bool aplicarTravaFalhaI2c() {
  portENTER_CRITICAL(&travaFalhasI2c);
  const bool deveParar = supervisorI2c.motoresDevemParar();
  portEXIT_CRITICAL(&travaFalhasI2c);
  // Travar freia pelo estimador do deslocamento, que a telemetria também lê: por isso a
  // `travaOdometria`, como no `detectorObjetivo.processarChegada`.
  portENTER_CRITICAL(&travaOdometria);
  const micromouse::MudancaTrava mudanca = travaFalhaI2c.aplicar(deveParar, motoresComTrava);
  portEXIT_CRITICAL(&travaOdometria);
  if (mudanca == micromouse::MudancaTrava::Travou) {
    Serial.println("{\"status\":\"motores_travados_i2c\"}");
  } else if (mudanca == micromouse::MudancaTrava::Soltou) {
    Serial.println("{\"status\":\"motores_liberados_i2c\"}");
  }
  return deveParar;
}

/// Na navegação, logo depois da leitura dos ToF: registra cada sensor no supervisor, aplica a trava
/// dos motores (que também vale para a falha da IMU, registrada pela tarefa dela) e reinicia pelo
/// XSHUT o ToF que o supervisor pedir. A trava vem antes do reinício, que segura a navegação e o
/// barramento por algumas dezenas de ms; nesse tempo, a IMU perde leituras sem contar como falha.
/// Devolve `true` enquanto algum dispositivo estiver em falha: a navegação tira a permissão de mover.
static bool supervisionarI2c() {
  static constexpr micromouse::Lado LADOS[3] = {micromouse::Lado::Frente, micromouse::Lado::Esquerda,
                                                micromouse::Lado::Direita};
  bool reiniciar[3] = {false, false, false};
  for (uint8_t i = 0; i < 3; ++i) {
    const micromouse::LeituraI2c leitura = vigiaTof.avaliar(LADOS[i], tof.amostra(LADOS[i]), millis());
    reiniciar[i] = registrarLeituraI2c(micromouse::dispositivoDoTof(LADOS[i]), leitura).acao ==
                   micromouse::AcaoI2c::Reiniciar;
  }

  const bool emFalha = aplicarTravaFalhaI2c();

  for (uint8_t i = 0; i < 3; ++i) {
    if (!reiniciar[i]) {
      continue;
    }
    bool voltou = false;
    {
      micromouse::BarramentoI2c::Uso uso(barramentoI2c);
      voltou = uso.conseguiu() && tof.reiniciarSensor(LADOS[i]);
    }
    vigiaTof.sensorReiniciado(LADOS[i], millis());
    registrarReinicioI2c(micromouse::dispositivoDoTof(LADOS[i]), voltou);
  }
  return emFalha;
}

/// Na tarefa da IMU, depois de cada leitura: registra o MPU6050 no supervisor e o reinicia
/// (`imu.iniciar()`) se ele pedir. Devolve `true` se reiniciou: o reinício espera 100 ms.
static bool supervisionarImu(bool falhou, bool barramentoOcupado) {
  const micromouse::LeituraI2c leitura = micromouse::leituraDaImu(falhou, barramentoOcupado);
  if (registrarLeituraI2c(micromouse::Dispositivo::Mpu6050, leitura).acao != micromouse::AcaoI2c::Reiniciar) {
    return false;
  }
  registrarReinicioI2c(micromouse::Dispositivo::Mpu6050, imu.iniciar());
  return true;
}

/// Ciclo percepção → mapa → decisão → atuação (#184): grava as paredes no mapa da corrida, decide
/// pelo Flood Fill e executa os movimentos (o `ControleMovimento` e o `ControleGiro` da #190/#191
/// ficam dentro dele). Só a tarefa de navegação o usa; só ela escreve no mapa.
static micromouse::CicloNavegacao cicloNavegacao(armazenamentoMapa.mapa());

/// Correção do erro acumulado da odometria pelas paredes (#188): a posição lateral pelas paredes
/// laterais e, com os encoders instalados, também ao longo do avanço (no modo estimado, quem corrige
/// o avanço é o `DeslocamentoEstimado`). Protegida por `travaOdometria`, como a odometria.
static micromouse::CorrecaoPelasParedes correcaoParedes;

/// Gatilho da largada (#184): a mão na frente do ToF frontal, ou o comando serial `L`. Só a tarefa
/// de navegação o usa.
static micromouse::GatilhoLargada gatilhoLargada;

/// Detecção de colisão (#192): o pico de aceleração da IMU e a frente parada pelo ToF. Só a tarefa de
/// navegação a usa. `colisoesPendentes` são as colisões que a telemetria ainda não enviou como evento
/// `colisao`, protegidas por `travaOdometria`.
static micromouse::DetectorColisao detectorColisao;
static uint32_t colisoesPendentes = 0;

/// Pedidos da telemetria (comandos seriais) para a tarefa de navegação, protegidos por
/// `travaOdometria`: `Z` recoloca o robô na largada e `L` libera a largada sem o gesto.
static bool pedidoReinicioNavegacao = false;
static bool pedidoLargada = false;

/// Calibração do boot (#240, RNF-08): desvio de cada ToF e viés do giroscópio, em até 1 s, com o robô
/// parado na célula de partida. O `corrigirTof` é aplicado a toda leitura dos ToF (navegação) e o viés
/// vai para o `acompanhadorGiro`. Falhou, o robô segue sem correção de ToF e a tarefa da IMU calibra o
/// viés sozinha.
static micromouse::RotinaCalibracao rotinaCalibracao;

/// Roda a `RotinaCalibracao` com os sensores reais, a cada 10 ms, até ela terminar (sucesso, falha ou
/// 1 s). Só pode ser chamada no `setup()`, antes de criar as tarefas, porque usa o ToF e a IMU sem
/// outra tarefa disputando o I²C. Conta como amostra de ToF só a leitura com medida nova.
static micromouse::EstadoRotinaCalibracao calibrarSensores() {
  rotinaCalibracao.iniciar(micromouse::REFERENCIAS_TOF_NA_LARGADA);
  uint32_t anteriorMs = millis();
  uint32_t instanteUltimaLeituraTof = 0;
  bool leuTof = false;
  while (rotinaCalibracao.estado() == micromouse::EstadoRotinaCalibracao::EmAndamento) {
    delay(micromouse::PERIODO_AMOSTRAGEM_GIRO_MS);
    const uint32_t agoraMs = millis();

    int16_t taxaGiro = 0;
    const bool giroValido = imu.lerTaxaGiroZ(taxaGiro);

    micromouse::DistanciasLaterais distancias = {0, 0, 0};
    const bool tofLido = tof.lerDistanciasLaterais(distancias);
    const uint32_t instanteLeitura = agoraMs - tof.idadeUltimaLeituraMs();
    const bool tofNovo = tofLido && (!leuTof || instanteLeitura != instanteUltimaLeituraTof);
    if (tofNovo) {
      leuTof = true;
      instanteUltimaLeituraTof = instanteLeitura;
    }

    rotinaCalibracao.atualizar(distancias, tofNovo, taxaGiro, giroValido, agoraMs - anteriorMs);
    anteriorMs = agoraMs;
  }
  return rotinaCalibracao.estado();
}

/// Tarefa da IMU: lê o giroscópio a cada `PERIODO_LEITURA_IMU_MS` (10 ms), em intervalo fixo.
///
/// Prioridade 3, a maior: a leitura é curta (menos de 1 ms) e o ângulo só fica certo se o intervalo
/// entre leituras não variar. Se a calibração do boot (`calibrarSensores`) não deu o viés, calibra
/// por conta própria nos primeiros 2 s, com o robô parado na largada; se o robô se mexer, a
/// calibração recomeça sozinha. Cada leitura passa pelo supervisor do I²C (#241).
static void tarefaImu(void*) {
  if (acompanhadorGiro.estadoCalibracao() != micromouse::EstadoCalibracao::Calibrado) {
    acompanhadorGiro.iniciarCalibracao();
  }
  bool calibradoAnunciado = false;
  TickType_t ultimoInstante = xTaskGetTickCount();
  for (;;) {
    vTaskDelayUntil(&ultimoInstante, pdMS_TO_TICKS(micromouse::PERIODO_LEITURA_IMU_MS));
    // O `vTaskDelayUntil` acorda em múltiplos exatos do período, então o intervalo é o período.
    const uint32_t falhasAntes = acompanhadorGiro.ultima().falhasLeitura;
    const uint32_t esperasAntes = barramentoI2c.esperasEsgotadas();
    const micromouse::LeituraGiro leitura = acompanhadorGiro.ciclo(micromouse::PERIODO_LEITURA_IMU_MS);

    AceleracaoPlano aceleracao;
    aceleracao.valida = imu.lerAceleracaoPlano(aceleracao.axMg, aceleracao.ayMg);

    portENTER_CRITICAL(&travaGiro);
    leituraGiro = leitura;
    ultimaAceleracao = aceleracao;
    portEXIT_CRITICAL(&travaGiro);

    // O giroscópio e o acelerômetro são o mesmo MPU6050: qualquer uma das duas leituras que falhar
    // conta. Falha com espera esgotada é o barramento com a navegação (reiniciando um ToF), não a IMU.
    const bool falhou = leitura.falhasLeitura != falhasAntes || !aceleracao.valida;
    if (supervisionarImu(falhou, barramentoI2c.esperasEsgotadas() != esperasAntes)) {
      // Recomeça a contagem do período: os 100 ms do reinício não viram uma rajada de leituras.
      ultimoInstante = xTaskGetTickCount();
    }

    if (!calibradoAnunciado && leitura.calibracao == micromouse::EstadoCalibracao::Calibrado) {
      calibradoAnunciado = true;
      Serial.printf("{\"status\":\"giroscopio_calibrado\",\"recusadas\":%lu}\n",
                    static_cast<unsigned long>(leitura.calibracoesRecusadas));
    }
  }
}

// ---------------------------------------------------------------- envio da telemetria (#238, #236)

/// Credenciais do envio, lidas de `include/credenciais.h` (vazias sem o arquivo).
static micromouse::CredenciaisSupabase lerCredenciais() {
  micromouse::CredenciaisSupabase c;
#if MICROMOUSE_TEM_CREDENCIAIS
  c.ssidWifi = CREDENCIAL_WIFI_SSID;
  c.senhaWifi = CREDENCIAL_WIFI_SENHA;
  c.urlProjeto = CREDENCIAL_SUPABASE_URL;
  c.chaveAnon = CREDENCIAL_SUPABASE_ANON;
  c.tokenDispositivo = CREDENCIAL_DEVICE_TOKEN;
  c.certificadoRaiz = CREDENCIAL_CERTIFICADO_RAIZ;
  c.semValidarCertificado = CREDENCIAL_SEM_VALIDAR_CERTIFICADO != 0;
#endif
  return c;
}
static const micromouse::CredenciaisSupabase credenciais = lerCredenciais();

/// Wi-Fi, cliente HTTPS e o buffer com reenvio. Só a tarefa de rede faz chamadas de rede; a coleta
/// (tarefa de telemetria) só chama `controladorEnvio.registrar()`, que nunca usa a rede.
static micromouse::RedeWifiEsp32 redeWifi(credenciais.ssidWifi, credenciais.senhaWifi);
static micromouse::ReconexaoWifi reconexaoWifi(redeWifi);
static micromouse::EnvioHttpSupabase clienteSupabase(credenciais);
static micromouse::TravaSecaoCritica travaBufferEnvio;
static micromouse::ControladorEnvio controladorEnvio(clienteSupabase, travaBufferEnvio);

/// O que a coleta e a tarefa de rede combinam sobre a corrida, protegido por `travaCorrida`.
struct EstadoCorridaEnvio {
  bool ativa = false;               ///< A corrida existe no servidor: a coleta pode registrar.
  uint32_t inicioMs = 0;            ///< `millis()` da criação da corrida (o cronômetro dos 10 min é o da `faseCorrida`, da largada).
  bool encerramentoPedido = false;  ///< A coleta registrou a última amostra (objetivo ou tempo).
  bool concluida = false;           ///< Chegou ao objetivo.
};
static EstadoCorridaEnvio estadoCorrida;
static portMUX_TYPE travaCorrida = portMUX_INITIALIZER_UNLOCKED;

/// Fase do robô na telemetria (#242): aguardando a calibração, aguardando a largada, em corrida ou
/// encerrada. Também protegida por `travaCorrida`: a tarefa de rede decide criar a corrida por ela e a de
/// telemetria a atualiza a cada ciclo.
static micromouse::FaseCorrida faseCorrida;

static EstadoCorridaEnvio lerEstadoCorrida() {
  portENTER_CRITICAL(&travaCorrida);
  const EstadoCorridaEnvio copia = estadoCorrida;
  portEXIT_CRITICAL(&travaCorrida);
  return copia;
}

/// `true` se `agoraMs` já alcançou `alvoMs`, correto mesmo quando o contador de ms dá a volta.
static bool chegouAoInstante(uint32_t agoraMs, uint32_t alvoMs) {
  return static_cast<int32_t>(agoraMs - alvoMs) >= 0;
}

/// Coleta: chamada pela tarefa de telemetria a cada `PERIODO_TELEMETRIA_MS`. Guarda a amostra no
/// buffer, com o número de sequência dado agora (e que nunca muda num reenvio). A chegada ao objetivo
/// ou o fim do tempo (10 min) registram a última amostra e pedem o encerramento da corrida.
static void coletarAmostra(const micromouse::Pose& pose, bool chegouAoObjetivo, int32_t deslocamentoMm) {
  const EstadoCorridaEnvio corrida = lerEstadoCorrida();
  if (!corrida.ativa || corrida.encerramentoPedido) {
    return;
  }
  portENTER_CRITICAL(&travaCorrida);
  // Aguardando a largada: a amostra mostra o robô parado na partida. Largou: o cronômetro dos 10 min
  // começa. Chegou ao objetivo ou o tempo acabou: última amostra.
  const micromouse::DecisaoColeta decisao = faseCorrida.decidir(deslocamentoMm, millis(), chegouAoObjetivo);
  if (decisao.ultima) {
    estadoCorrida.encerramentoPedido = true;
    estadoCorrida.concluida = chegouAoObjetivo;
  }
  portEXIT_CRITICAL(&travaCorrida);

  if (decisao.largouAgora) {
    Serial.println("{\"status\":\"largada\"}");
  }
  if (!decisao.registrar) {
    return;
  }
  const micromouse::AmostraTelemetria amostra = micromouse::montarAmostra(
      pose, decisao.tempoDecorridoMs, 0,  // velocidade: 0 até a estimativa sem encoder (#380) entrar na telemetria
      decisao.estado,
      chegouAoObjetivo ? micromouse::EventoTelemetria::ObjetivoAlcancado : micromouse::EventoTelemetria::Nenhum);
  controladorEnvio.registrar(amostra);
}

/// Se a calibração do boot falhou, mas o autoteste foi aprovado e o giroscópio calibrou sozinho (tarefa da
/// IMU), o robô também está pronto na largada: a partida sem a correção dos ToF funciona. Chamada a cada
/// ciclo da telemetria; depois de calibrado, não faz nada (e nem toca na seção crítica do giroscópio).
static void atualizarFaseDaCalibracao() {
  if (!estadoBoot.ok) {
    return;
  }
  portENTER_CRITICAL(&travaCorrida);
  const bool jaCalibrado = faseCorrida.calibrado();
  portEXIT_CRITICAL(&travaCorrida);
  if (jaCalibrado || lerGiro().calibracao != micromouse::EstadoCalibracao::Calibrado) {
    return;
  }
  portENTER_CRITICAL(&travaCorrida);
  faseCorrida.calibracaoConcluida();
  portEXIT_CRITICAL(&travaCorrida);
  Serial.println("{\"status\":\"aguardando_largada\",\"calibracao\":\"giroscopio\"}");
}

/// Uma linha de log com os contadores do envio, para a bancada e o dashboard.
static void registrarContadoresEnvio() {
  const micromouse::ContadoresEnvio& c = controladorEnvio.contadores();
  Serial.printf(
      "{\"status\":\"envio\",\"ocupacao\":%u,\"perdidas\":%lu,\"confirmadas\":%lu,\"existentes\":%lu,"
      "\"descartadas\":%lu,\"tentativas\":%lu,\"falhas\":%lu,\"reenvios\":%lu,\"wifi_quedas\":%lu,"
      "\"pausado\":%s}\n",
      static_cast<unsigned>(controladorEnvio.ocupacao()), static_cast<unsigned long>(controladorEnvio.amostrasPerdidas()),
      static_cast<unsigned long>(c.amostrasConfirmadas), static_cast<unsigned long>(c.amostrasJaExistentes),
      static_cast<unsigned long>(c.amostrasDescartadas), static_cast<unsigned long>(c.tentativas),
      static_cast<unsigned long>(c.falhas), static_cast<unsigned long>(c.reenvios),
      static_cast<unsigned long>(reconexaoWifi.quedas()), controladorEnvio.pausado() ? "true" : "false");
}

/// Tarefa de rede: Wi-Fi, criação da corrida, envio dos lotes e encerramento.
///
/// Prioridade 1, a menor, com pilha maior por causa do TLS. As chamadas HTTPS bloqueiam esta tarefa
/// por até `TEMPO_LIMITE_REDE_MS`, nunca a navegação: a coleta só deixa a amostra no buffer.
static void tarefaRede(void*) {
  if (!micromouse::credenciaisValidas(credenciais)) {
    Serial.println("{\"status\":\"envio_desligado\",\"motivo\":\"sem_credenciais\"}");
    vTaskDelete(nullptr);
    return;
  }
  if (credenciais.semValidarCertificado) {
    Serial.println("{\"status\":\"aviso\",\"motivo\":\"https_sem_validar_certificado\"}");
  }
  redeWifi.iniciar();
  clienteSupabase.iniciar();

  micromouse::PoliticaReenvio esperaCriacao;  // 1 s, 2 s, 4 s... até 30 s entre tentativas de criar a corrida
  uint32_t proximaCriacaoMs = millis();
  uint32_t proximoLogMs = millis() + micromouse::PERIODO_LOG_ENVIO_MS;
  bool corridaCriada = false;
  bool avisouPausa = false;
  bool avisouEncerrada = false;

  for (;;) {
    const uint32_t agora = millis();
    if (reconexaoWifi.ciclo(agora)) {
      // A corrida só é criada no servidor depois da calibração (o estado Aguardando Largada, #242).
      portENTER_CRITICAL(&travaCorrida);
      const bool calibrado = faseCorrida.calibrado();
      portEXIT_CRITICAL(&travaCorrida);
      if (!corridaCriada) {
        if (calibrado && chegouAoInstante(agora, proximaCriacaoMs)) {
          uint32_t corridaId = 0;
          const micromouse::ResultadoEnvio r =
              clienteSupabase.criarCorrida(micromouse::LABIRINTO_CORRIDA_PADRAO, corridaId);
          if (r.codigoHttp >= 200 && r.codigoHttp < 300) {
            controladorEnvio.iniciarCorrida(corridaId);
            corridaCriada = true;
            portENTER_CRITICAL(&travaCorrida);
            estadoCorrida.ativa = true;
            estadoCorrida.inicioMs = agora;
            portEXIT_CRITICAL(&travaCorrida);
            Serial.printf("{\"status\":\"corrida_criada\",\"corrida_id\":%lu}\n", static_cast<unsigned long>(corridaId));
          } else {
            proximaCriacaoMs = agora + esperaCriacao.registrarFalha();
            Serial.printf("{\"status\":\"falha_criar_corrida\",\"codigo\":%d}\n", r.codigoHttp);
          }
        }
      } else {
        const EstadoCorridaEnvio corrida = lerEstadoCorrida();
        if (corrida.encerramentoPedido && !controladorEnvio.encerramentoSolicitado()) {
          clienteSupabase.definirResultadoCorrida(corrida.concluida);
          controladorEnvio.solicitarEncerramento(agora);  // sai só depois de o buffer esvaziar
        }
        const micromouse::ResultadoCiclo r = controladorEnvio.ciclo(agora);
        if (r == micromouse::ResultadoCiclo::Pausado && !avisouPausa) {
          avisouPausa = true;  // 401/403: a chave ou o token estão errados; não adianta insistir
          Serial.println("{\"status\":\"envio_pausado\",\"motivo\":\"credenciais_recusadas\"}");
        }
        if (controladorEnvio.encerrada() && !avisouEncerrada) {
          avisouEncerrada = true;
          Serial.println("{\"status\":\"corrida_encerrada\"}");
        }
      }
    }
    if (chegouAoInstante(agora, proximoLogMs)) {
      proximoLogMs = agora + micromouse::PERIODO_LOG_ENVIO_MS;
      registrarContadoresEnvio();
    }
    vTaskDelay(pdMS_TO_TICKS(micromouse::PERIODO_CICLO_REDE_MS));
  }
}

/// Medidas que dizem se o robô se mexeu, para a proteção dos motores: o ToF frontal e a taxa do
/// giroscópio (sem o viés). O giroscópio é obrigatório: sem ele, um giro no lugar (ToF frontal
/// parado) seria lido como motor travado. Por isso, sem ToF ou sem giroscópio válidos a leitura vai
/// como inválida e a proteção não conclui nada: ela **não freia por engano**, mas também não protege.
/// Uma frente além do alcance do ToF (leitura no máximo, 1200 mm) também vai como inválida: ela não muda
/// andando por um corredor longo e seria lida como robô parado (#192).
static micromouse::MedidasProtecao medidasDaProtecao(const LeituraTofTelemetria& leituraTof) {
  const micromouse::LeituraGiro giro = lerGiro();
  const bool sensoresValidos = leituraTof.valida && giro.valida &&
                               leituraTof.distancias.frenteMm < micromouse::DISTANCIA_MAXIMA_VALIDA_TOF_MM;
  return micromouse::MedidasProtecao{sensoresValidos, leituraTof.distancias.frenteMm, giro.taxaDecimosDps};
}

/// Tarefa de navegação: ciclo percepção → decisão → atuação, a cada 10 ms.
///
/// Prioridade 2 (maior que a da telemetria), para que o envio de dados nunca
/// atrase o controle do robô.
static void tarefaNavegacao(void*) {
  constexpr uint32_t PERIODO_NAVEGACAO_MS = 10;
  // Instante da amostra mais antiga da última leitura usada: se não mudou, a leitura é a mesma.
  uint32_t instanteUltimaLeituraUsada = 0;
  // Comando `L` recebido antes de o robô estar pronto: vale quando ele ficar pronto.
  bool largadaPendente = false;
  for (;;) {
    // Sensores de distância: a leitura não espera medida nova, só junta as últimas amostras. Fica
    // antes da detecção do objetivo para a telemetria continuar recebendo o ToF depois da chegada.
    // A tarefa da IMU usa o mesmo I²C: a leitura só acontece com o barramento segurado. Se ele não
    // vier a tempo, o ciclo segue sem leitura nova do ToF.
    LeituraTofTelemetria leituraTof;
    {
      micromouse::BarramentoI2c::Uso uso(barramentoI2c);
      leituraTof.valida = uso.conseguiu() && tof.lerDistanciasLaterais(leituraTof.distancias);
    }
    if (leituraTof.valida) {
      // Desvio de cada sensor medido na calibração do boot (zero se ela não aconteceu).
      leituraTof.distancias = rotinaCalibracao.corrigirTof(leituraTof.distancias);
    }
    leituraTof.idadeMs = tof.idadeUltimaLeituraMs();
    // Falha no I²C (#241): cada ToF passa pelo supervisor, e os motores param enquanto algum
    // dispositivo (inclusive a IMU, vista pela tarefa dela) estiver em falha.
    const bool falhaI2c = supervisionarI2c();
    portENTER_CRITICAL(&travaTof);
    ultimaLeituraTof = leituraTof;
    portEXIT_CRITICAL(&travaTof);

    // Proteção dos motores (#341): com potência alta por 500 ms sem o robô se mexer, freia e segura o
    // freio até a navegação liberar (`motoresProtegidos.liberar()`).
    const uint32_t cortesAntes = motoresProtegidos.cortes();
    motoresProtegidos.atualizarProtecao(medidasDaProtecao(leituraTof), PERIODO_NAVEGACAO_MS);
    if (motoresProtegidos.cortes() != cortesAntes) {
      Serial.println("{\"status\":\"motor_travado\"}");
    }

    const uint32_t instanteLeitura = millis() - leituraTof.idadeMs;
    const bool leituraNova = leituraTof.valida && instanteLeitura != instanteUltimaLeituraUsada;
    if (leituraNova) {
      instanteUltimaLeituraUsada = instanteLeitura;
    }

    // Nas manobras (giros e realinhamento), as leituras não servem de referência de posição (#188).
    const micromouse::MovimentoRobo movimento = cicloNavegacao.movimentoRobo();
    const bool referencia = leituraNova && movimento != micromouse::MovimentoRobo::Manobrando;

    bool alcancou = false;
    // Odometria e detecção de chegada executam na seção crítica travaOdometria para sincronizar com
    // tarefaTelemetria. O intervalo curto (10 ms) mantém cada atualização com giro ou avanço, não os dois.
    portENTER_CRITICAL(&travaOdometria);
    const bool reiniciarNavegacao = pedidoReinicioNavegacao;
    const bool largarAgora = pedidoLargada;
    pedidoReinicioNavegacao = false;
    pedidoLargada = false;
    // Fora dos giros, a direção da odometria é a confirmada pelo giroscópio no fim de cada giro, e não
    // a da diferença entre as rodas (#184). A cada ciclo, para a diferença também não se acumular nos
    // giros pequenos do realinhamento (#188).
    if (cicloNavegacao.direcaoDefinida()) {
      odometria.definirDirecao(cicloNavegacao.direcao());
    }
    if (micromouse::ENCODERS_INSTALADOS) {
      odometria.atualizar(rodaEsq, rodaDir);
    } else {
      // Estima o avanço dos 10 ms e corrige a posição pela leitura nova do ToF, se houver.
      if (movimento == micromouse::MovimentoRobo::Manobrando) {
        deslocamento.esquecerLaterais();
      }
      deslocamento.ciclo(PERIODO_NAVEGACAO_MS, odometria, referencia ? &leituraTof.distancias : nullptr);
    }
    // Pelas paredes: a posição lateral sempre e, com os encoders, também ao longo do avanço (#188).
    if (leituraNova) {
      correcaoParedes.aplicar(odometria, leituraTof.distancias, movimento, micromouse::ENCODERS_INSTALADOS);
    }
    const micromouse::Pose pose = odometria.pose();
    // No modo estimado, uma pose sem correção recente não serve para declarar a chegada (#380).
    const bool poseValida = odometria.posicaoValida() &&
                            (micromouse::ENCODERS_INSTALADOS || deslocamento.poseConfiavel());
    if (poseValida) {
      // Chegada detectada pela posição na malha: para os motores imediatamente e congela o mapa
      // final (HU-07 / #189). Freia por `motoresTracao`, para o estimador saber que o robô parou.
      alcancou = detectorObjetivo.processarChegada(pose, motoresTracao, &armazenamentoMapa);
    } else {
      alcancou = detectorObjetivo.objetivoAlcancado();
    }
    const int64_t posicaoAvancoUm = micromouse::posicaoAoLongoDoAvancoUm(odometria);
    const bool posicaoNaMalha = odometria.posicaoValida();
    portEXIT_CRITICAL(&travaOdometria);

    if (reiniciarNavegacao) {
      largadaPendente = false;
      detectorColisao.reiniciar();
      portENTER_CRITICAL(&travaOdometria);
      correcaoParedes.reiniciar();
      portEXIT_CRITICAL(&travaOdometria);
      gatilhoLargada.reiniciar();
      cicloNavegacao.reiniciar(pose, motoresTracao);
    }

    if (alcancou) {
      vTaskDelay(pdMS_TO_TICKS(PERIODO_NAVEGACAO_MS));
      continue;
    }

    // Ciclo percepção → mapa → decisão → atuação (#184).
    const micromouse::LeituraGiro giro = lerGiro();
    const micromouse::DistanciasLaterais* tofNovo = leituraNova ? &leituraTof.distancias : nullptr;
    // Permissão para mover: boot aprovado, giroscópio calibrado, gatilho da largada disparado, a corrida
    // não encerrada pela `FaseCorrida` (#242: o objetivo ou os 10 minutos) e nenhuma falha do I²C
    // (#241). Antes de o robô estar pronto, o gesto não é observado.
    const bool pronto = estadoBoot.ok && giro.calibracao == micromouse::EstadoCalibracao::Calibrado;
    largadaPendente = largadaPendente || largarAgora;
    if (pronto) {
      const bool jaLiberado = gatilhoLargada.disparado();
      if (largadaPendente) {
        largadaPendente = false;
        gatilhoLargada.dispararManual();
      }
      if (gatilhoLargada.atualizar(tofNovo, PERIODO_NAVEGACAO_MS) && !jaLiberado) {
        Serial.println("{\"status\":\"largada_liberada\"}");
      }
    }

    micromouse::EntradaCiclo entrada;
    portENTER_CRITICAL(&travaCorrida);
    const bool corridaEncerrada = faseCorrida.encerrada();
    portEXIT_CRITICAL(&travaCorrida);
    // Na falha do I²C (#241), a permissão sai: o ciclo freia, cancela o movimento e espera; quando a
    // falha passa, percebe de novo a partir de onde parou. A trava dos motores segura o freio.
    entrada.liberado = pronto && gatilhoLargada.disparado() && !corridaEncerrada && !falhaI2c;
    entrada.pose = pose;
    entrada.poseValida = posicaoNaMalha;
    entrada.posicaoAvancoUm = posicaoAvancoUm;
    entrada.leituraNova = tofNovo;
    entrada.giro = giro;
    entrada.dtMs = PERIODO_NAVEGACAO_MS;

    // Colisão (#192): só com o robô sendo conduzido. Parado, em recuperação ou antes da largada, a
    // detecção fica desligada.
    const AceleracaoPlano aceleracao = lerAceleracao();
    micromouse::EntradaDeteccao deteccao;
    deteccao.ativo = entrada.liberado && cicloNavegacao.emMovimento();
    deteccao.aceleracaoValida = aceleracao.valida;
    deteccao.axMg = aceleracao.axMg;
    deteccao.ayMg = aceleracao.ayMg;
    deteccao.potencia = registradorPotencia.ultimaPotencia();
    deteccao.medidas = micromouse::MedidasProtecao{leituraTof.valida, leituraTof.distancias.frenteMm, giro.taxaDecimosDps};
    deteccao.dtMs = PERIODO_NAVEGACAO_MS;
    entrada.colisao = detectorColisao.atualizar(deteccao);

    const uint32_t colisoesAntes = cicloNavegacao.colisoes();
    const micromouse::EstadoCiclo antes = cicloNavegacao.estado();
    const micromouse::EstadoCiclo depois = cicloNavegacao.atualizar(entrada, motoresTracao);
    if (cicloNavegacao.colisoes() != colisoesAntes) {
      Serial.printf("{\"status\":\"colisao\",\"tipo\":\"%s\",\"desvio_mg\":%lu,\"celulas\":%lu}\n",
                    micromouse::nomeDoTipoColisao(cicloNavegacao.ultimaColisao()),
                    static_cast<unsigned long>(detectorColisao.desvioMg()),
                    static_cast<unsigned long>(cicloNavegacao.celulasAvancadas()));
      portENTER_CRITICAL(&travaOdometria);
      ++colisoesPendentes;
      portEXIT_CRITICAL(&travaOdometria);
      // Se a proteção do motor travado (#341) freou o robô, ela segura o freio até aqui: o recuo precisa
      // de ré. O ciclo já parou os motores e só manda potência nos ciclos seguintes.
      motoresProtegidos.liberar();
    }
    if (depois != antes) {
      Serial.printf("{\"status\":\"navegacao\",\"estado\":\"%s\",\"falha\":\"%s\",\"celulas\":%lu}\n",
                    micromouse::nomeDoEstadoCiclo(depois),
                    micromouse::nomeDaFalhaExecucao(cicloNavegacao.falhaExecucao()),
                    static_cast<unsigned long>(cicloNavegacao.celulasAvancadas()));
    }
    vTaskDelay(pdMS_TO_TICKS(PERIODO_NAVEGACAO_MS));
  }
}

/// Tarefa de telemetria: coleta e envia os dados a cada `PERIODO_TELEMETRIA_MS`.
///
/// Prioridade 1. Transmite as leituras dos encoders, pose da odometria e eventos pontuais em formato JSON
/// estruturado e aceita os comandos 'Z', para zerar os encoders, voltar a odometria à pose atual e
/// aguardar a largada de novo, 'B', para repetir o relatório do boot (a linha do boot se perde se o
/// monitor abrir depois), e 'L', para liberar a largada sem o gesto (#184).
///
/// No modo sem encoder, `pose_valida` também fica `false` depois de o robô andar mais que
/// `DISTANCIA_MAXIMA_SEM_CORRECAO_MM` sem correção pelo ToF, e `deslocamento` diz qual fonte está
/// em uso (`"estimado"` ou `"encoder"`).
static void tarefaTelemetria(void*) {
  static uint32_t sequencia = 0;

  for (;;) {
    // Encoders (reais ou estimados) e pose: uma cópia feita na seção crítica, para tudo ser da
    // mesma atualização.
    portENTER_CRITICAL(&travaOdometria);
    const micromouse::DeslocamentoRodas d = medidorDeslocamento.medir();
    const int32_t medio = medidorDeslocamento.deslocamentoMedioMm();
    const int32_t pulsosEsq = rodaEsq.lerContagem();
    const int32_t pulsosDir = rodaDir.lerContagem();
    const micromouse::Pose pose = odometria.pose();
    const bool poseConfiavel = micromouse::ENCODERS_INSTALADOS || deslocamento.poseConfiavel();
    const bool poseValida = odometria.posicaoValida() && poseConfiavel;
    const uint32_t semCorrecaoMm = deslocamento.distanciaSemCorrecaoMm();
    const bool dispararEvento = detectorObjetivo.consumirEventoChegada();
    // Um evento por amostra: a chegada vai primeiro, e cada colisão pendente espera a sua vez.
    const bool dispararColisao = !dispararEvento && colisoesPendentes > 0;
    if (dispararColisao) {
      --colisoesPendentes;
    }
    portEXIT_CRITICAL(&travaOdometria);

    portENTER_CRITICAL(&travaTof);
    const LeituraTofTelemetria leituraTof = ultimaLeituraTof;
    portEXIT_CRITICAL(&travaTof);

    char falhasI2c[micromouse::TAMANHO_MAX_FALHAS_I2C];
    portENTER_CRITICAL(&travaFalhasI2c);
    supervisorI2c.descreverFalhas(falhasI2c, sizeof(falhasI2c));
    const uint32_t reiniciosI2c = supervisorI2c.reiniciosTotais();
    portEXIT_CRITICAL(&travaFalhasI2c);

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
    pacote.evento = dispararEvento ? "objetivo_alcancado" : (dispararColisao ? "colisao" : nullptr);
    pacote.tofFrenteMm = leituraTof.distancias.frenteMm;
    pacote.tofEsqMm = leituraTof.distancias.esquerdaMm;
    pacote.tofDirMm = leituraTof.distancias.direitaMm;
    pacote.tofValida = leituraTof.valida;
    pacote.tofIdadeMs = leituraTof.idadeMs;
    pacote.modoDeslocamento = micromouse::ENCODERS_INSTALADOS ? "encoder" : "estimado";
    pacote.semCorrecaoMm = micromouse::ENCODERS_INSTALADOS ? 0 : semCorrecaoMm;
    pacote.bootOk = estadoBoot.ok;
    pacote.falhasBoot = estadoBoot.falhas;
    pacote.bootMs = estadoBoot.duracaoMs;
    pacote.falhasI2c = falhasI2c;
    pacote.reiniciosI2c = reiniciosI2c;
    portENTER_CRITICAL(&travaCorrida);
    pacote.fase = micromouse::nomeDaFase(faseCorrida.fase());
    portEXIT_CRITICAL(&travaCorrida);

    // Envio ao Supabase: a amostra vai para o buffer; quem envia é a tarefa de rede.
    atualizarFaseDaCalibracao();
    coletarAmostra(pose, dispararEvento, medio);

    // Comporta o pior caso do JSON; com menos, a linha seria descartada em silêncio.
    char buffer[micromouse::TAMANHO_MAX_JSON_TELEMETRIA];
    if (micromouse::TelemetriaJson::serializar(pacote, buffer, sizeof(buffer)) > 0) {
      Serial.println(buffer);
    }

    // Comando serial para zerar medição (usado na bancada)
    while (Serial.available()) {
      const char c = static_cast<char>(Serial.read());
      if (c == 'Z' || c == 'z') {
        // Os encoders voltam a zero: a odometria mantém a pose e passa a contar a partir de zero.
        // No modo estimado, o robô também volta ao centro da célula e a pose volta a ser confiável
        // (na bancada, 'Z' é usado com o robô posicionado à mão).
        portENTER_CRITICAL(&travaOdometria);
        if (micromouse::ENCODERS_INSTALADOS) {
          medidorDeslocamento.zerar();
          odometria.reiniciar(odometria.pose(), 0, 0);
        } else {
          deslocamento.reiniciar(odometria, odometria.pose());
        }
        detectorObjetivo.resetar();
        pedidoReinicioNavegacao = true;
        portEXIT_CRITICAL(&travaOdometria);
        portENTER_CRITICAL(&travaCorrida);
        faseCorrida.reiniciar();  // recolocado na partida à mão: volta a aguardar a largada
        const bool corridaEncerrada = faseCorrida.encerrada();
        portEXIT_CRITICAL(&travaCorrida);
        Serial.println("{\"status\":\"encoders_zerados\"}");
        if (corridaEncerrada) {
          // A corrida já foi encerrada no servidor: o `Z` zera a odometria, mas não começa outra tentativa.
          Serial.println("{\"status\":\"aviso\",\"motivo\":\"corrida_encerrada_reinicie_a_placa_para_nova_tentativa\"}");
        }
      } else if (c == 'B' || c == 'b') {
        imprimirRelatorioBoot();
      } else if (c == 'L' || c == 'l') {
        portENTER_CRITICAL(&travaOdometria);
        pedidoLargada = true;
        portEXIT_CRITICAL(&travaOdometria);
        Serial.println("{\"status\":\"largada_pedida\"}");
      }
    }

    vTaskDelay(pdMS_TO_TICKS(micromouse::PERIODO_TELEMETRIA_MS));
  }
}

/// Sequência de boot da Rev 2.0 (#50): motores freados e travados, barramento I²C, sensores de
/// distância com os endereços novos, autoteste do I²C e, só com ele aprovado, motores liberados;
/// depois a IMU (#378) e as tarefas de navegação, telemetria e rede. As tarefas sobem mesmo com
/// falha, para a telemetria continuar informando o que falhou.
void setup() {
  // Primeira linha: freia os motores antes de qualquer outra coisa, porque o GPIO20/21 (IN3/IN4)
  // recebe um tranco da ROM durante o reset (ver README, "Tranco no boot").
  motores.iniciar();
  Serial.begin(115200);
  // Sem um computador conectado, a escrita na USB CDC esperaria o tempo limite a cada chamada e
  // atrasaria a telemetria. Com tempo zero, o dado é descartado se ninguém estiver lendo.
  Serial.setTxTimeoutMs(0);
  Serial.println("micromouse: iniciando");

  // Configuração provisória para bancada (#189): fixa Labirinto4x4.
  // Em 8x4 e 12x4 a corrida deve receber a dimensão por comando antes de iniciar.
  armazenamentoMapa.iniciarCorrida(micromouse::DimensaoMapa::Labirinto4x4);
  // A navegação refaz o objetivo pelo tamanho do mapa e aguarda a largada na partida.
  cicloNavegacao.reiniciar(micromouse::Pose{{0, 0}, micromouse::Direcao::Norte}, motoresTracao);
  motoresTracao.parar();
  if (micromouse::ENCODERS_INSTALADOS) {
    encoderEsq.iniciar();
    encoderDir.iniciar();
  }

  // O barramento I²C é dos 3 VL53L0X e do MPU6050: `barramentoI2c.iniciar` cria o mutex e inicia o
  // `Wire`. O `tof.iniciar()` e o autoteste usam o `Wire` sem o mutex porque rodam aqui, antes dos
  // `xTaskCreate` (inclusive o da IMU). Se falhar, o autoteste aponta os dispositivos sem resposta.
  if (!barramentoI2c.iniciar(micromouse::pinos::I2C_SDA, micromouse::pinos::I2C_SCL)) {
    Serial.println("{\"status\":\"falha_barramento_i2c\"}");
  }
  tof.iniciar();  // o autoteste abaixo diz qual sensor falhou

  // Autoteste: os 3 ToF em 0x30, 0x31 e 0x32, o MPU6050 em 0x68 e ninguém no 0x29.
  micromouse::BarramentoI2cEsp32 barramento(Wire);
  estadoBoot.resultado = autoteste.verificar(barramento, {tof.sensorIniciado(micromouse::Lado::Esquerda),
                                                          tof.sensorIniciado(micromouse::Lado::Frente),
                                                          tof.sensorIniciado(micromouse::Lado::Direita)});
  micromouse::AutotesteBoot::descreverFalhas(estadoBoot.resultado, estadoBoot.falhas, sizeof(estadoBoot.falhas));
  estadoBoot.ok = estadoBoot.resultado.aprovado();
  if (estadoBoot.ok) {
    motoresComTrava.liberar();
  } else {
    motoresComTrava.travar();
    Serial.printf("micromouse: falha no boot (%s): motores travados ate reiniciar\n", estadoBoot.falhas);
  }

  // Tempo desde o início do programa (RNF-09); a ROM e o bootloader rodam antes e ficam de fora.
  estadoBoot.duracaoMs = millis();
  imprimirRelatorioBoot();

  // MPU6050 (#378). Sem a IMU o firmware segue, sem ângulo de giro válido.
  const bool imuIniciada = imu.iniciar();
  if (imuIniciada) {
    Serial.printf("{\"status\":\"imu_iniciada\",\"who_am_i\":\"0x%02X\"}\n", imu.identificacao());
  } else {
    Serial.println("{\"status\":\"falha_imu\"}");
  }

  // Calibração dos ToF e do giroscópio (#240, RNF-08), com o robô parado na célula de partida. Só
  // com o autoteste aprovado e a IMU de pé: sem isso, não há o que calibrar.
  if (estadoBoot.ok && imuIniciada) {
    const micromouse::EstadoRotinaCalibracao calibracao = calibrarSensores();
    if (calibracao == micromouse::EstadoRotinaCalibracao::Sucesso) {
      acompanhadorGiro.definirVies(rotinaCalibracao.viesGiroDecimosDps());
      // Calibrado e na partida: aguarda a largada (#242). A corrida é criada no servidor e o
      // dashboard passa a mostrar o robô parado na célula de partida.
      faseCorrida.calibracaoConcluida();
      Serial.println("{\"status\":\"aguardando_largada\"}");
    }
    Serial.printf("{\"status\":\"calibracao\",\"estado\":\"%s\",\"duracao_ms\":%lu,\"no_prazo\":%s}\n",
                  micromouse::nomeEstadoRotina(calibracao),
                  static_cast<unsigned long>(rotinaCalibracao.duracaoMedidaMs()),
                  rotinaCalibracao.dentroDoOrcamento() ? "true" : "false");
  }

  xTaskCreate(tarefaImu, "imu", 3072, nullptr, 3, nullptr);
  xTaskCreate(tarefaNavegacao, "navegacao", 4096, nullptr, 2, nullptr);
  xTaskCreate(tarefaTelemetria, "telemetria", 4096, nullptr, 1, nullptr);
  xTaskCreate(tarefaRede, "rede", 10240, nullptr, 1, nullptr);
}

/// Laço principal do Arduino. Fica ocioso: o trabalho acontece nas tarefas.
void loop() {
  delay(1000);
}
