/// @file main.cpp
/// @brief Ponto de entrada do firmware: inicializa o sistema e cria as tarefas FreeRTOS.
///
/// Navegação e telemetria rodam em tarefas separadas e concorrentes, como no
/// diagrama de atividades. Os encoders físicos de quadratura são decodificados por
/// interrupção de hardware no ESP32, alimentam a odometria (pose na malha) e são reportados na
/// telemetria. Os 3 sensores de distância VL53L0X são lidos pelo I²C na tarefa de navegação, e o
/// giroscópio do MPU6050 por uma tarefa própria, em intervalo fixo, no mesmo barramento (com mutex).
/// Enquanto os encoders não são instalados (`ENCODERS_INSTALADOS` em `ConfiguracaoDeslocamento.h`),
/// o deslocamento é estimado pela potência dos motores e corrigido pelos ToF (#380).
/// Os motores só são liberados depois do autoteste do boot (#50) encontrar os 4 dispositivos do I²C.
/// A navegação percebe as paredes no centro de cada célula, grava no mapa, decide pelo Flood Fill e
/// executa um movimento por vez (avançar uma célula, girar 90° ou 180°, #184), depois da largada
/// pelo gesto da mão na frente do ToF frontal (ou do comando serial `L`). As paredes corrigem o erro
/// acumulado da odometria, e o robô se realinha ao centro do corredor antes de girar no lugar (#188).
/// Uma terceira tarefa, a de rede, envia a telemetria ao Supabase sem nunca fazer a navegação
/// esperar (#238).
/// Durante a corrida, uma falha no I²C para os motores e reinicia o dispositivo que falhou (#241).
/// O computador acompanha e ativa o robô à distância pelo `ConsoleRemoto`: a serial USB e, com o Wi-Fi,
/// um telnet (porta 23) com os mesmos comandos (`L` larga, `X` para, `Z`, `B`, `E`), enquanto a tarefa de
/// rede segue enviando a telemetria ao banco. A bateria é lida no GPIO34 e vai na telemetria. Não há
/// botão de largada (a chave geral liga a energia direto da bateria): a largada vem do painel.
/// Com a largada em qualquer célula (`LARGADA_EM_QUALQUER_CELULA`), o robô é posto numa célula qualquer,
/// virado para o Norte do labirinto, sem saber qual nem o tamanho do labirinto: a odometria fica no
/// referencial do mapa de exploração, e o `CicloNavegacao` diz quando ele se localizou e converte a pose
/// para o labirinto (`poseNoLabirinto`), que é o que a telemetria e a chegada usam. O tamanho que ele
/// descobriu vai ao banco (`corridas/{id}/labirinto`), e só então a telemetria passa a sair.

#include <Arduino.h>
#include <Wire.h>

#include "AcompanhadorGiro.h"
#include "ArmazenamentoMapa.h"
#include "AutotesteBoot.h"
#include "Bateria.h"
#include "BateriaAdc.h"
#include "BarramentoI2c.h"
#include "BarramentoI2cEsp32.h"
#include "CicloNavegacao.h"
#include "Configuracao.h"
#include "ConfiguracaoDeslocamento.h"
#include "EstimadorVelocidade.h"
#include "ConfiguracaoImu.h"
#include "ConfiguracaoRede.h"
#include "ConsoleRemoto.h"
#include "ContratoTelemetria.h"
#include "ControladorEnvio.h"
#include "ConversorDeslocamento.h"
#include "CorrecaoPelasParedes.h"
#include "CorretorDeslocamentoTof.h"
#include "CredenciaisSupabase.h"
#include "DeslocamentoEstimado.h"
#include "DetectorColisao.h"
#include "DetectorPatinagem.h"
#include "DetectorObjetivo.h"
#include "EncoderEsp32.h"
#include "ParDeEncoders.h"
#include "EnvioHttpSupabase.h"
#include "FaseCorrida.h"
#include "GatilhoLargada.h"
#include "ImuMpu6050.h"
#include "LocalizadorLargada.h"
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
#include "ServicoOta.h"
#include "RotinaCalibracao.h"
#include "SupervisorI2c.h"
#include "TelemetriaJson.h"
#include "TofVl53l0x.h"
#include "TravaFalhaI2c.h"
#include "TravaSecaoCritica.h"
#include "VigiaTof.h"

// Wi-Fi e URL do Supabase vêm de `include/credenciais_padrao.h` (versionado). A chave do Supabase e o
// `DEVICE_TOKEN` ficam só em `include/credenciais.h`, que o Git ignora (o modelo é
// `include/credenciais.exemplo.h`), e esse arquivo, se existir, vale mais que os padrões. Sem a chave
// e o token, o firmware compila, conecta no Wi-Fi, aceita OTA e não envia telemetria.
#if __has_include("credenciais.h")
#include "credenciais.h"
#endif
#include "credenciais_padrao.h"
#define MICROMOUSE_TEM_CREDENCIAIS 1

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
///
/// Os encoders reais passam por um `ParDeEncoders`: se um lado ficar mudo (por exemplo, um fio solto)
/// enquanto o outro conta, a odometria usa o lado que conta para as duas rodas, e o robô avisa no log.
static micromouse::ParDeEncoders paresDeEncoders(encoderEsq, encoderDir);
static micromouse::IEncoder& rodaEsq =
    micromouse::ENCODERS_INSTALADOS ? paresDeEncoders.esquerdo() : deslocamento.encoderEsquerdo();
static micromouse::IEncoder& rodaDir =
    micromouse::ENCODERS_INSTALADOS ? paresDeEncoders.direito() : deslocamento.encoderDireito();

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

/// Console do robô: a serial USB e o telnet. Toda a saída do log passa por ele (veja `ConsoleRemoto`).
static micromouse::ConsoleRemoto console;

/// Pose no labirinto real, para a telemetria: a da odometria convertida pelo `CicloNavegacao` (na
/// largada em qualquer célula, a odometria fica no referencial da exploração). `poseNoLabirintoValida`
/// é `false` enquanto o robô não se localizou. A navegação escreve, a telemetria lê; sob `travaOdometria`.
static micromouse::Pose poseNoLabirinto = {{0, 0}, micromouse::Direcao::Norte};
static volatile bool poseNoLabirintoValida = true;

/// Tamanho do labirinto que o robô descobriu (`DimensaoMapa`), para a tarefa de rede informá-lo ao banco,
/// e se o banco já o recebeu. A navegação escreve o tamanho (um byte); a tarefa de rede escreve o resto.
/// Sem a largada em qualquer célula, o tamanho é o 4x4 da criação da corrida e já conta como informado.
static volatile uint8_t dimensaoDescoberta = static_cast<uint8_t>(micromouse::DimensaoMapa::Labirinto4x4);
static volatile bool labirintoInformado = !micromouse::LARGADA_EM_QUALQUER_CELULA;

/// Bateria (GPIO34): a telemetria a mede a cada ciclo e a guarda aqui, para o comando `E` e a coleta.
/// Valor de 32 bits, então a leitura sem trava só pode pegar a medida anterior.
static micromouse::BateriaAdc bateria;
static micromouse::MonitorBateria monitorBateria;
static volatile uint32_t ultimaBateriaMv = 0;

/// Intervalo da tarefa do console, em ms: o atraso máximo de um comando (`X`, `L`) e do log no telnet.
static constexpr uint32_t PERIODO_CICLO_CONSOLE_MS = 20;

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
    console.println(relatorio);
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
    console.printf("{\"status\":\"i2c\",\"dispositivo\":\"%s\",\"situacao\":\"%s\"}\n",
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
  console.printf("{\"status\":\"reinicio_i2c\",\"dispositivo\":\"%s\",\"sucesso\":%s}\n",
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
    console.println("{\"status\":\"motores_travados_i2c\"}");
  } else if (mudanca == micromouse::MudancaTrava::Soltou) {
    console.println("{\"status\":\"motores_liberados_i2c\"}");
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
/// Parâmetros do controle só para o robô real. O ToF frontal não lê menos de ~48 mm (nariz encostado na
/// parede), então parar no centro exato da célula (84 mm da parede, leitura de 34 mm) era impossível: o
/// robô empurrava a parede até estourar o tempo do avanço. Para a 105 mm do centro à parede (leitura de
/// ~55 mm, 7 mm antes de encostar). O modelo simulado e os testes seguem com o valor de projeto.
static micromouse::ConfiguracaoControle configuracaoControleDoRobo() {
  micromouse::ConfiguracaoControle c;
  c.distanciaParadaFrontalMm = 105;
  // Sem centralização pelas paredes laterais: os ToF laterais ficam na diagonal, e quando uma parede termina
  // a leitura daquele lado "some", o que puxava o robô para a abertura, até encostar na quina da parede (o
  // vídeo da pista mostrou o robô preso ali). O rumo reto segue pelo giroscópio, que não depende dos ToF.
  // Isso só é seguro com o realinhamento desligado (`configuracaoRealinhamentoDoRobo`): o mesmo PD o comandava.
  c.ganhoProporcionalDecimos = 0;
  c.ganhoDerivativoDecimos = 0;
  return c;
}

/// Realinhamento do robô real: desligado. Ele recentraliza o robô pelas leituras laterais, que assumem
/// sensores a 90°; os ToF laterais do robô ficam na diagonal e, de frente para uma parede, enxergam a
/// parede da frente, então a manobra não converge (estourava o tempo do giro antes de largar). Também sem
/// ajuste de rumo pelo corredor, pelo mesmo motivo: o rumo reto segue pelo giroscópio.
/// Espaço do texto de cada lado do `perfil_lateral` (24 amostras de até 3 dígitos e vírgula).
static constexpr size_t PERFIL_TEXTO_MAX = 112;

static micromouse::ConfiguracaoRealinhamento configuracaoRealinhamentoDoRobo() {
  micromouse::ConfiguracaoRealinhamento c;
  c.tentativas = 0;
  c.ajusteRumoPeloCorredorPorMil = 0;
  return c;
}

/// Os ToF laterais do robô ficam na diagonal (cerca de 45° da frente): de frente para uma parede próxima,
/// eles leem a parede da frente, e a percepção não grava as laterais dessa célula (ficam desconhecidas).
static micromouse::ConfiguracaoExecucao configuracaoExecucaoDoRobo() {
  micromouse::ConfiguracaoExecucao c;
  c.lateraisEmDiagonal = true;
  // O chassi não tem folga para girar no lugar colado na parede da frente (os cantos batem e as rodas
  // patinam, o que cortava o motor por "travado"): com a frente a menos de 90 mm, recua 45 mm antes do giro.
  c.recuoAntesDoGiroMm = 25;  // ré curta: uma ré longa levava a traseira para outra quina e o robô prendia de novo
  c.frenteParaRecuarMm = 90;
  // Laterais na diagonal e robô fora do centro: uma parede lê de ~105 a ~175 mm. Com 140 mm, uma parede a
  // 173 mm virava "livre" (e o planejamento errava); com 190 mm ela conta como parede, e uma abertura
  // (acima de 250 mm) continua livre.
  c.limiarParedeLateralPercepcaoMm = 190;
  // Sem caminho (mapa com parede falsa perto de uma quina): apaga o mapa e tenta de novo, até 3 vezes.
  c.reiniciosDoMapaNoSemCaminho = 3;
  // Desvio de quina: com uma parede lateral a menos de 90 mm e o outro lado mais livre, vira 10° para longe dela.
  // A odometria parou ~90 mm antes de onde o robô costuma parar na parede. O teto (135) deixa de fora a parede da
  // célula seguinte (lida a ~214 mm do sensor), para não andar uma célula a mais.
  c.seguirSeReNaoAndar = true;
  c.ajusteChegadaPelaFrenteMaxMm = 135;
  c.ajusteChegadaParedeGarantidaMaxMm = 200;  // última célula antes da borda do labirinto: a parede é certa
  // Abertura lateral pelo perfil do avanço. Medido no labirinto de teste: paredes leem de 105 a 146 mm durante o avanço
  // e aberturas leem 200 a 237 mm por 8 a 13 amostras seguidas; 190 mm por 4 amostras separa os dois com folga.
  c.aberturaLateralMm = 190;
  c.amostrasAberturaLateral = 4;
  c.desvioParedePertoMm = 115;  // perto de uma parede com o outro lado livre, afasta-se: dá folga ao giro
  c.desvioFolgaOutroLadoMm = 40;
  c.desvioRumoMiligraus = 10000;
  return c;
}

static micromouse::CicloNavegacao cicloNavegacao(armazenamentoMapa.mapa(), configuracaoExecucaoDoRobo(),
                                                 configuracaoControleDoRobo(), micromouse::ConfiguracaoGiro{},
                                                 configuracaoRealinhamentoDoRobo());

/// Correção do erro acumulado da odometria pelas paredes (#188): a posição lateral pelas paredes
/// laterais e, com os encoders instalados, também ao longo do avanço (no modo estimado, quem corrige
/// o avanço é o `DeslocamentoEstimado`). Protegida por `travaOdometria`, como a odometria.
static micromouse::CorrecaoPelasParedes correcaoParedes;

/// Gatilho da largada (#184): a mão na frente do ToF frontal, ou o comando serial `L`. Só a tarefa
/// de navegação o usa.
static micromouse::GatilhoLargada gatilhoLargada;

/// A largada vem só do painel (INICIAR) e do comando `L`. O gesto de cobrir o sensor frontal fica
/// desligado: com a célula de partida colada na parede da frente, a mão ao posicionar o robô já o
/// disparava, e ele largava ainda na mão. Com `true`, o gesto volta a largar o robô.
static constexpr bool LARGADA_POR_GESTO = false;

/// `true` se a largada foi liberada: pelo gesto da mão ou pelo comando `L` (painel, telnet ou serial).
static bool largadaLiberada() { return gatilhoLargada.disparado(); }

/// Detecção de colisão (#192): o pico de aceleração da IMU e a frente parada pelo ToF. Só a tarefa de
/// navegação a usa. `colisoesPendentes` são as colisões que a telemetria ainda não enviou como evento
/// `colisao`, protegidas por `travaOdometria`.
static micromouse::DetectorColisao detectorColisao;

/// Rodas patinando (roda no vinco, chassi preso na quina): a odometria anda e o ToF frontal não muda. Trata
/// como colisão `frente_parada`: o ciclo para, recua em linha reta e tenta de novo.
static micromouse::DetectorPatinagem detectorPatinagem;

/// Correção da posição da odometria pelas paredes. Desligada no robô real: ela assume ToF laterais a 90° e a
/// parede da frente a 50 mm do centro, mas os laterais ficam na diagonal. Cada leitura "corrigia" a posição
/// para o lado errado (a odometria chegou a achar o robô 219 mm ao sul depois de andar uma célula para o
/// leste), deslocava as paredes do mapa e invalidava a pose. Sem ela, a posição vem só dos encoders, que
/// medem 2466 pulsos por célula, e a direção, do giroscópio.
static constexpr bool CORRECAO_PELAS_PAREDES = false;
static uint32_t colisoesPendentes = 0;

/// Pedidos da telemetria (comandos seriais) para a tarefa de navegação, protegidos por
/// `travaOdometria`: `Z` recoloca o robô na largada e `L` libera a largada sem o gesto.
static bool pedidoReinicioNavegacao = false;
static bool pedidoLargada = false;
static bool pedidoParada = false;  ///< `X`: para na hora e volta a esperar a largada.

/// Cópias para o comando `E` (estado), escritas pela navegação e lidas pelo console. Valores simples,
/// então a leitura sem trava só pode pegar o ciclo anterior.
static volatile uint8_t estadoCicloAtual = 0;

/// Corrida em andamento no banco (0 antes de ela existir), para os logs do painel. A tarefa de rede
/// escreve; ela mesma lê, então não precisa de trava.
static uint32_t corridaIdAtual = 0;

/// Logs de evento que o envio ao painel perdeu (a chamada falhou), só para o comando `E`.
static volatile uint32_t logsPerdidosNoEnvio = 0;

/// Tempo que um comando de largada (`L` ou o `iniciar` do painel) espera o robô ficar pronto, em ms.
/// Passado isso, ele expira: um `iniciar` dado cedo demais não larga o robô minutos depois.
static constexpr uint32_t LARGADA_PENDENTE_MAXIMA_MS = 30000;
static volatile bool largadaLiberadaAtual = false;

/// Calibração do boot (#240, RNF-08): desvio de cada ToF e viés do giroscópio, em até 1 s, com o robô
/// parado na célula de partida. O `corrigirTof` é aplicado a toda leitura dos ToF (navegação) e o viés
/// vai para o `acompanhadorGiro`. Falhou, o robô segue sem correção de ToF e a tarefa da IMU calibra o
/// viés sozinha.
static micromouse::RotinaCalibracao rotinaCalibracao;

/// Roda a `RotinaCalibracao` com os sensores reais, a cada 10 ms, até ela terminar (sucesso, falha ou
/// 1 s). Só pode ser chamada no `setup()`, antes de criar as tarefas, porque usa o ToF e a IMU sem
/// outra tarefa disputando o I²C. Conta como amostra de ToF só a leitura com medida nova.
static micromouse::EstadoRotinaCalibracao calibrarSensores() {
  // Largado numa célula qualquer, o robô não tem parede a uma distância conhecida: calibra só o
  // giroscópio, e os ToF ficam sem correção de desvio.
  rotinaCalibracao.iniciar(micromouse::LARGADA_EM_QUALQUER_CELULA ? micromouse::REFERENCIAS_TOF_SEM_PAREDE
                                                                  : micromouse::REFERENCIAS_TOF_NA_LARGADA);
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
      console.printf("{\"status\":\"giroscopio_calibrado\",\"recusadas\":%lu}\n",
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
#if defined(CREDENCIAL_WIFI_SSID_RESERVA) && defined(CREDENCIAL_WIFI_SENHA_RESERVA)
  c.ssidWifiReserva = CREDENCIAL_WIFI_SSID_RESERVA;  // rede reserva (#491), opcional
  c.senhaWifiReserva = CREDENCIAL_WIFI_SENHA_RESERVA;
#endif
  c.urlProjeto = CREDENCIAL_SUPABASE_URL;
  c.chaveAnon = CREDENCIAL_SUPABASE_ANON;
  c.tokenDispositivo = CREDENCIAL_DEVICE_TOKEN;
#endif
  return c;
}
static const micromouse::CredenciaisSupabase credenciais = lerCredenciais();

/// Wi-Fi, cliente HTTPS e o buffer com reenvio. Só a tarefa de rede faz chamadas de rede; a coleta
/// (tarefa de telemetria) só chama `controladorEnvio.registrar()`, que nunca usa a rede.
static micromouse::RedeWifiEsp32 redeWifi(credenciais.ssidWifi, credenciais.senhaWifi, credenciais.ssidWifiReserva,
                                           credenciais.senhaWifiReserva);
static micromouse::ReconexaoWifi reconexaoWifi(redeWifi);
static micromouse::EnvioHttpSupabase clienteSupabase(credenciais);
/// Gravação pelo Wi-Fi (OTA): liga quando a rede conecta, mesmo sem credenciais do Supabase.
static micromouse::ServicoOta servicoOta;
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
static void coletarAmostra(const micromouse::Pose& pose, bool chegouAoObjetivo, int32_t deslocamentoMm,
                           int32_t velocidadeMmPorS) {
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
    console.println("{\"status\":\"largada\"}");
  }
  if (!decisao.registrar) {
    return;
  }
  // Antes de o robô se localizar e o banco saber o tamanho do labirinto, a posição não existe ou cairia
  // fora da malha da corrida (e o banco recusaria o lote inteiro): a amostra não sai. A fase e o
  // cronômetro seguem (a `decidir` acima), e a numeração começa na primeira amostra que sai.
  if (!poseNoLabirintoValida || !labirintoInformado) {
    return;
  }
  micromouse::AmostraTelemetria amostra = micromouse::montarAmostra(
      pose, decisao.tempoDecorridoMs, velocidadeMmPorS,
      decisao.estado,
      chegouAoObjetivo ? micromouse::EventoTelemetria::ObjetivoAlcancado : micromouse::EventoTelemetria::Nenhum);
  // A bateria vai na telemetria; sem medida (chave geral desligada), 0, que o banco ignora.
  const uint32_t bateriaMv = ultimaBateriaMv;
  amostra.tensaoBateriaMv = bateriaMv < micromouse::BATERIA_MINIMA_MEDIDA_MV
                                ? 0
                                : static_cast<uint16_t>(bateriaMv > UINT16_MAX ? UINT16_MAX : bateriaMv);
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
  console.println("{\"status\":\"aguardando_largada\",\"calibracao\":\"giroscopio\"}");
}

/// Uma linha de log com os contadores do envio, para a bancada e o dashboard.
static void registrarContadoresEnvio() {
  const micromouse::ContadoresEnvio& c = controladorEnvio.contadores();
  console.printf(
      "{\"status\":\"envio\",\"ocupacao\":%u,\"perdidas\":%lu,\"confirmadas\":%lu,\"existentes\":%lu,"
      "\"descartadas\":%lu,\"tentativas\":%lu,\"falhas\":%lu,\"reenvios\":%lu,\"wifi_quedas\":%lu,"
      "\"wifi_rede\":\"%s\",\"wifi_trocas\":%lu,\"rssi\":%d,\"pausado\":%s}\n",
      static_cast<unsigned>(controladorEnvio.ocupacao()), static_cast<unsigned long>(controladorEnvio.amostrasPerdidas()),
      static_cast<unsigned long>(c.amostrasConfirmadas), static_cast<unsigned long>(c.amostrasJaExistentes),
      static_cast<unsigned long>(c.amostrasDescartadas), static_cast<unsigned long>(c.tentativas),
      static_cast<unsigned long>(c.falhas), static_cast<unsigned long>(c.reenvios),
      static_cast<unsigned long>(reconexaoWifi.quedas()), redeWifi.nomeRedeAtual(),
      static_cast<unsigned long>(reconexaoWifi.trocasDeRede()), redeWifi.conectada() ? static_cast<int>(WiFi.RSSI()) : 0,
      controladorEnvio.pausado() ? "true" : "false");
}

static void executarComando(char c);  // definida adiante, com os outros comandos
static void imprimirMapaDoRobo();  // comando `M`, definida adiante

/// Busca o comando que o painel mandou e o executa como se tivesse vindo do telnet. Só a tarefa de rede.
/// Falha de rede ou resposta ilegível não gera log (o log seria enviado ao painel e falharia de novo).
static void consultarComandoDoPainel() {
  micromouse::ComandoRobo comando = micromouse::ComandoRobo::Nenhum;
  const micromouse::ResultadoEnvio r = clienteSupabase.buscarComando(comando);
  if (r.codigoHttp < 200 || r.codigoHttp >= 300 || comando == micromouse::ComandoRobo::Nenhum) {
    return;
  }
  const char* nome = comando == micromouse::ComandoRobo::Iniciar ? "iniciar"
                     : comando == micromouse::ComandoRobo::Parar ? "parar"
                                                                  : "zerar";
  console.printf("{\"status\":\"comando_do_painel\",\"comando\":\"%s\"}\n", nome);
  executarComando(comando == micromouse::ComandoRobo::Iniciar ? 'L' : (comando == micromouse::ComandoRobo::Parar ? 'X' : 'Z'));
}

/// Envia ao painel as linhas de evento que o robô registrou desde o último envio, até
/// `LINHAS_POR_LOTE_DE_LOGS` por chamada. Só a tarefa de rede. É de melhor esforço: se a chamada falha,
/// as linhas se perdem (e contam em `logsPerdidosNoEnvio`), porque o log do robô não pode atrasar a
/// telemetria nem a corrida.
static void enviarLogsParaOPainel() {
  char armazenamento[micromouse::LINHAS_POR_LOTE_DE_LOGS][micromouse::TAMANHO_MAX_LINHA_DE_LOG];
  const char* linhas[micromouse::LINHAS_POR_LOTE_DE_LOGS];
  size_t quantidade = 0;
  while (quantidade < micromouse::LINHAS_POR_LOTE_DE_LOGS &&
         console.tirarLinhaParaOPainel(armazenamento[quantidade], sizeof(armazenamento[quantidade])) > 0) {
    linhas[quantidade] = armazenamento[quantidade];
    ++quantidade;
  }
  if (quantidade == 0) {
    return;
  }
  const micromouse::ResultadoEnvio r = clienteSupabase.enviarLogs(corridaIdAtual, linhas, quantidade);
  if (r.codigoHttp < 200 || r.codigoHttp >= 300) {
    logsPerdidosNoEnvio = logsPerdidosNoEnvio + static_cast<uint32_t>(quantidade);
  }
}

/// Tarefa de rede: Wi-Fi, criação da corrida, envio dos lotes e encerramento.
///
/// Prioridade 1, a menor, com pilha maior por causa do TLS. As chamadas HTTPS bloqueiam esta tarefa
/// por até `TEMPO_LIMITE_REDE_MS`, nunca a navegação: a coleta só deixa a amostra no buffer.
static void tarefaRede(void*) {
  // Sem as credenciais do Supabase o robô não envia telemetria, mas o Wi-Fi e a gravação pelo Wi-Fi
  // (OTA) continuam valendo. Sem nem rede configurada, não há o que fazer aqui.
  const bool envioPossivel = micromouse::credenciaisValidas(credenciais);
  if (!envioPossivel) {
    console.println("{\"status\":\"envio_desligado\",\"motivo\":\"sem_credenciais\"}");
  }
  if (credenciais.ssidWifi == nullptr || credenciais.ssidWifi[0] == '\0') {
    vTaskDelete(nullptr);
    return;
  }
  redeWifi.iniciar();
  // Sem economia de energia do rádio: com ela, o Wi-Fi dorme entre os beacons e o telnet e o envio da
  // telemetria ganham latência e perdem pacotes. A bateria aguenta; os motores gastam muito mais.
  WiFi.setSleep(false);
  clienteSupabase.iniciar();
  console.printf("{\"status\":\"wifi_redes\",\"quantidade\":%u}\n", static_cast<unsigned>(redeWifi.quantidadeRedes()));

  micromouse::PoliticaReenvio esperaCriacao;  // 1 s, 2 s, 4 s... até 30 s entre tentativas de criar a corrida
  uint32_t proximaCriacaoMs = millis();
  uint32_t proximoLogMs = millis() + micromouse::PERIODO_LOG_ENVIO_MS;
  uint32_t proximoComandoMs = millis();
  uint32_t proximoEnvioLogsMs = millis();
  uint32_t proximaInformacaoMs = millis();
  bool corridaCriada = false;
  bool avisouPausa = false;
  bool avisouEncerrada = false;
  bool estavaConectado = false;

  for (;;) {
    const uint32_t agora = millis();
    const bool conectado = reconexaoWifi.ciclo(agora);
    if (conectado != estavaConectado) {  // cada conexão e cada queda saem no log, com a rede e o sinal
      estavaConectado = conectado;
      if (conectado) {
        console.printf("{\"status\":\"wifi_conectado\",\"rede\":\"%s\",\"rssi\":%d,\"ip\":\"%s\",\"telnet\":%u}\n",
                      redeWifi.nomeRedeAtual(), static_cast<int>(WiFi.RSSI()), WiFi.localIP().toString().c_str(),
                      static_cast<unsigned>(micromouse::ConsoleRemoto::PORTA_TELNET));
      } else {
        console.printf("{\"status\":\"wifi_caiu\",\"rede\":\"%s\"}\n", redeWifi.nomeRedeAtual());
      }
    }
    if (conectado) {
      servicoOta.iniciar([]() { motoresComTrava.travar(); });  // gravando: motores freados e travados
      servicoOta.ciclo();
    }
    if (conectado && envioPossivel) {
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
            corridaIdAtual = corridaId;
            corridaCriada = true;
            portENTER_CRITICAL(&travaCorrida);
            estadoCorrida.ativa = true;
            estadoCorrida.inicioMs = agora;
            portEXIT_CRITICAL(&travaCorrida);
            console.printf("{\"status\":\"corrida_criada\",\"corrida_id\":%lu}\n", static_cast<unsigned long>(corridaId));
          } else {
            proximaCriacaoMs = agora + esperaCriacao.registrarFalha();
            console.printf("{\"status\":\"falha_criar_corrida\",\"codigo\":%d}\n", r.codigoHttp);
          }
        }
      } else {
        const EstadoCorridaEnvio corrida = lerEstadoCorrida();
        if (corrida.encerramentoPedido && !controladorEnvio.encerramentoSolicitado()) {
          clienteSupabase.definirResultadoCorrida(corrida.concluida);
          controladorEnvio.solicitarEncerramento(agora);  // sai só depois de o buffer esvaziar
        }
        // Largada em qualquer célula: o robô informa o tamanho que descobriu, e só então a telemetria sai
        // (a posição cairia fora da malha 4x4 com que a corrida nasceu). Tenta de 2 em 2 s.
        if (!labirintoInformado && poseNoLabirintoValida && chegouAoInstante(agora, proximaInformacaoMs)) {
          proximaInformacaoMs = agora + 2000;
          const char* nome = micromouse::nomeDoLabirinto(static_cast<micromouse::DimensaoMapa>(dimensaoDescoberta));
          const micromouse::ResultadoEnvio r = clienteSupabase.definirLabirintoCorrida(corridaIdAtual, nome);
          if (r.codigoHttp >= 200 && r.codigoHttp < 300) {
            labirintoInformado = true;
            console.printf("{\"status\":\"labirinto_descoberto\",\"labirinto\":\"%s\"}\n", nome);
          } else if (r.codigoHttp >= 400 && r.codigoHttp < 500) {
            // O banco recusou de vez (corrida encerrada ou com posições fora da malha): repetir não adianta.
            labirintoInformado = true;
            console.printf("{\"status\":\"falha_definir_labirinto\",\"labirinto\":\"%s\",\"codigo\":%d}\n", nome, r.codigoHttp);
          }
        }
        const micromouse::ResultadoCiclo r = controladorEnvio.ciclo(agora);
        if (r == micromouse::ResultadoCiclo::Pausado && !avisouPausa) {
          avisouPausa = true;  // 401/403: a chave ou o token estão errados; não adianta insistir
          console.println("{\"status\":\"envio_pausado\",\"motivo\":\"credenciais_recusadas\"}");
        }
        if (controladorEnvio.encerrada() && !avisouEncerrada) {
          avisouEncerrada = true;
          console.println("{\"status\":\"corrida_encerrada\"}");
        }
      }
    }
    if (conectado && envioPossivel) {
      // O painel: busca o comando (largar, parar, zerar) e manda os logs de evento. Cada um tem o seu
      // ritmo, e as chamadas ficam na mesma tarefa da telemetria, uma depois da outra.
      if (chegouAoInstante(agora, proximoComandoMs)) {
        proximoComandoMs = agora + micromouse::PERIODO_CONSULTA_COMANDO_MS;
        consultarComandoDoPainel();
      }
      if (chegouAoInstante(agora, proximoEnvioLogsMs)) {
        proximoEnvioLogsMs = agora + micromouse::PERIODO_ENVIO_LOGS_MS;
        enviarLogsParaOPainel();
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
  uint32_t largadaPendenteMs = 0;  // há quanto tempo o `L` espera o robô ficar pronto
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
      console.println("{\"status\":\"motor_travado\"}");
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
    const bool pararAgora = pedidoParada;
    pedidoReinicioNavegacao = false;
    pedidoLargada = false;
    pedidoParada = false;
    // Fora dos giros, a direção da odometria é a confirmada pelo giroscópio no fim de cada giro, e não
    // a da diferença entre as rodas (#184). A cada ciclo, para a diferença também não se acumular nos
    // giros pequenos do realinhamento (#188).
    if (cicloNavegacao.direcaoDefinida()) {
      odometria.definirDirecao(cicloNavegacao.direcao());
    }
    bool avisoEncoderEsquerdo = false;
    bool avisoEncoderDireito = false;
    if (micromouse::ENCODERS_INSTALADOS) {
      paresDeEncoders.atualizar(cicloNavegacao.estado() == micromouse::EstadoCiclo::Girando);
      avisoEncoderEsquerdo = paresDeEncoders.consumirAvisoEsquerdo();
      avisoEncoderDireito = paresDeEncoders.consumirAvisoDireito();
      odometria.atualizar(rodaEsq, rodaDir);
    } else {
      // Estima o avanço dos 10 ms e corrige a posição pela leitura nova do ToF, se houver.
      if (movimento == micromouse::MovimentoRobo::Manobrando) {
        deslocamento.esquecerLaterais();
      }
      deslocamento.ciclo(PERIODO_NAVEGACAO_MS, odometria, referencia ? &leituraTof.distancias : nullptr);
    }
    // Pelas paredes: a posição lateral sempre e, com os encoders, também ao longo do avanço (#188).
    if (CORRECAO_PELAS_PAREDES && leituraNova) {
      correcaoParedes.aplicar(odometria, leituraTof.distancias, movimento, micromouse::ENCODERS_INSTALADOS);
    }
    // A pose da odometria vai para o ciclo; a chegada e a telemetria usam a convertida para o labirinto.
    const micromouse::Pose poseOdometria = odometria.pose();
    const micromouse::Pose pose = cicloNavegacao.poseNoLabirinto(poseOdometria);
    const bool localizado = cicloNavegacao.localizado() && cicloNavegacao.poseDentroDoLabirinto(poseOdometria);
    poseNoLabirinto = pose;
    poseNoLabirintoValida = localizado;
    if (micromouse::LARGADA_EM_QUALQUER_CELULA && localizado) {
      // O tamanho do labirinto só é conhecido depois de o robô se localizar.
      const micromouse::Mapa& mapaReal = armazenamentoMapa.mapa();
      detectorObjetivo.definirObjetivo(mapaReal.obterLinhas(), mapaReal.obterColunas());
      dimensaoDescoberta = static_cast<uint8_t>(mapaReal.obterDimensao());
    }
    // No modo estimado, uma pose sem correção recente não serve para declarar a chegada (#380).
    const bool poseValida = odometria.posicaoValida() && localizado &&
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
    // Um encoder mudo vira aviso no log (e no painel): a odometria segue pelo outro lado.
    if (avisoEncoderEsquerdo) {
      console.println("{\"status\":\"aviso\",\"motivo\":\"encoder_esquerdo_sem_sinal\",\"usando\":\"encoder_direito\"}");
    }
    if (avisoEncoderDireito) {
      console.println("{\"status\":\"aviso\",\"motivo\":\"encoder_direito_sem_sinal\",\"usando\":\"encoder_esquerdo\"}");
    }

    if (reiniciarNavegacao) {
      largadaPendente = false;
      detectorColisao.reiniciar();
      detectorPatinagem.reiniciar();
      portENTER_CRITICAL(&travaOdometria);
      correcaoParedes.reiniciar();
      portEXIT_CRITICAL(&travaOdometria);
      gatilhoLargada.reiniciar();
      cicloNavegacao.reiniciar(poseOdometria, motoresTracao);
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
    if (pararAgora) {
      // Parada remota: tira a largada, cancela o comando pendente e freia. Sem a permissão, o ciclo
      // cancela o movimento e volta a esperar; larga de novo pelo painel, com `L` ou o gesto.
      largadaPendente = false;
      gatilhoLargada.reiniciar();
      motoresTracao.parar();
      console.println("{\"status\":\"parada_remota\"}");
    }
    largadaPendente = largadaPendente || largarAgora;
    if (largarAgora) {
      largadaPendenteMs = 0;
    }
    if (largadaPendente && !pronto) {
      largadaPendenteMs += PERIODO_NAVEGACAO_MS;
      if (largadaPendenteMs >= LARGADA_PENDENTE_MAXIMA_MS) {
        largadaPendente = false;  // o robô não ficou pronto a tempo: o pedido não vale mais
        console.println("{\"status\":\"largada_expirada\",\"motivo\":\"robo_nao_ficou_pronto\"}");
      }
    }
    if (pronto) {
      const bool jaLiberado = largadaLiberada();
      if (largadaPendente) {
        largadaPendente = false;
        gatilhoLargada.dispararManual();
      }
      if (gatilhoLargada.atualizar(LARGADA_POR_GESTO ? tofNovo : nullptr, PERIODO_NAVEGACAO_MS) && !jaLiberado) {
        console.println("{\"status\":\"largada_liberada\"}");
      }
    }

    micromouse::EntradaCiclo entrada;
    portENTER_CRITICAL(&travaCorrida);
    const bool corridaEncerrada = faseCorrida.encerrada();
    portEXIT_CRITICAL(&travaCorrida);
    // Na falha do I²C (#241), a permissão sai: o ciclo freia, cancela o movimento e espera; quando a
    // falha passa, percebe de novo a partir de onde parou. A trava dos motores segura o freio.
    const bool largadaLiberadaAgora = largadaLiberada();
    largadaLiberadaAtual = largadaLiberadaAgora;
    entrada.liberado = pronto && largadaLiberadaAgora && !corridaEncerrada && !falhaI2c;
    entrada.pose = poseOdometria;  // o ciclo converte para o mapa em uso
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
    const bool patinando = detectorPatinagem.atualizar(
        deteccao.ativo && cicloNavegacao.estado() == micromouse::EstadoCiclo::Avancando, posicaoAvancoUm,
        leituraTof.valida, leituraTof.distancias.frenteMm, PERIODO_NAVEGACAO_MS);
    if (patinando && entrada.colisao == micromouse::TipoColisao::Nenhuma) {
      entrada.colisao = micromouse::TipoColisao::FrenteParada;
      console.println("{\"status\":\"patinagem\",\"acao\":\"recuar_e_tentar_de_novo\"}");
    }

    const uint32_t colisoesAntes = cicloNavegacao.colisoes();
    const uint32_t percepcoesAntes = cicloNavegacao.percepcoes();
    const uint8_t reiniciosAntes = cicloNavegacao.reiniciosDoMapa();
    const uint8_t ajustesAntes = cicloNavegacao.executor().ajustesDeChegada();
    const uint32_t perfisAntes = cicloNavegacao.perfisConcluidos();
    const micromouse::EstadoCiclo antes = cicloNavegacao.estado();
    const micromouse::EstadoCiclo depois = cicloNavegacao.atualizar(entrada, motoresTracao);
    estadoCicloAtual = static_cast<uint8_t>(depois);
    if (cicloNavegacao.percepcoes() != percepcoesAntes) {
      // O que o robô viu e gravou no mapa, em qual célula e direção (depuração do mapa).
      const auto estado = [](micromouse::EstadoParede e) { return e == micromouse::EstadoParede::Parede ? "parede" : (e == micromouse::EstadoParede::Livre ? "livre" : "?"); };
      const micromouse::Pose& pose = cicloNavegacao.poseDaUltimaPercepcao();
      const micromouse::ClassificadorParede::ResultadoClassificacao& c = cicloNavegacao.ultimaClassificacao();
      console.printf("{\"status\":\"percepcao\",\"linha\":%u,\"coluna\":%u,\"direcao\":\"%c\",\"frente\":\"%s\",\"esquerda\":\"%s\",\"direita\":\"%s\",\"tof\":[%u,%u,%u]}\n",
                    static_cast<unsigned>(pose.celula.linha), static_cast<unsigned>(pose.celula.coluna),
                    "NLSO"[static_cast<uint8_t>(pose.direcao)], estado(c.frente), estado(c.esquerda), estado(c.direita),
                    static_cast<unsigned>(leituraTof.distancias.frenteMm), static_cast<unsigned>(leituraTof.distancias.esquerdaMm),
                    static_cast<unsigned>(leituraTof.distancias.direitaMm));
    }
    if (cicloNavegacao.perfisConcluidos() != perfisAntes) {
      // Leituras laterais do avanço que acabou (uma a cada 2 leituras do ToF, mm): para calibrar a abertura lateral.
      const micromouse::PerfilLateral& perfil = cicloNavegacao.perfilLateral();
      char esq[PERFIL_TEXTO_MAX];
      char dir[PERFIL_TEXTO_MAX];
      size_t ne = 0;
      size_t nd = 0;
      esq[0] = dir[0] = '\0';
      for (uint8_t i = 0; i < perfil.quantidade() && ne + 6 < sizeof(esq) && nd + 6 < sizeof(dir); i++) {
        ne += snprintf(esq + ne, sizeof(esq) - ne, i == 0 ? "%u" : ",%u", static_cast<unsigned>(perfil.esquerda(i)));
        nd += snprintf(dir + nd, sizeof(dir) - nd, i == 0 ? "%u" : ",%u", static_cast<unsigned>(perfil.direita(i)));
      }
      console.printf("{\"status\":\"perfil_lateral\",\"direcao\":\"%c\",\"esq\":\"%s\",\"dir\":\"%s\"}\n",
                    "NLSO"[static_cast<uint8_t>(cicloNavegacao.direcaoDoPerfil())], esq, dir);
    }
    if (cicloNavegacao.executor().ajustesDeChegada() > ajustesAntes) {
      console.printf("{\"status\":\"ajuste_chegada\",\"n\":%u,\"frente_mm\":%u,\"restante_mm\":%ld}\n",
                    static_cast<unsigned>(cicloNavegacao.executor().ajustesDeChegada()),
                    static_cast<unsigned>(cicloNavegacao.executor().ultimaFrenteMm()),
                    static_cast<long>(cicloNavegacao.executor().restanteMm()));
    }
    if (cicloNavegacao.reiniciosDoMapa() != reiniciosAntes) {
      console.printf("{\"status\":\"aviso\",\"motivo\":\"mapa_reiniciado_sem_caminho\",\"vez\":%u}\n",
                    static_cast<unsigned>(cicloNavegacao.reiniciosDoMapa()));
    }
    if (cicloNavegacao.colisoes() != colisoesAntes) {
      console.printf("{\"status\":\"colisao\",\"tipo\":\"%s\",\"desvio_mg\":%lu,\"celulas\":%lu}\n",
                    micromouse::nomeDoTipoColisao(cicloNavegacao.ultimaColisao()),
                    static_cast<unsigned long>(detectorColisao.desvioMg()),
                    static_cast<unsigned long>(cicloNavegacao.celulasAvancadas()));
      portENTER_CRITICAL(&travaOdometria);
      ++colisoesPendentes;
      portEXIT_CRITICAL(&travaOdometria);
      // Se a proteção do motor travado (#341) freou o robô, ela segura o freio até aqui: o recuo precisa
      // de ré. O ciclo já parou os motores e só manda potência nos ciclos seguintes.
      motoresProtegidos.liberar();
      detectorPatinagem.reiniciar();
    }
    if (depois != antes) {
      console.printf("{\"status\":\"navegacao\",\"estado\":\"%s\",\"falha\":\"%s\",\"celulas\":%lu}\n",
                    micromouse::nomeDoEstadoCiclo(depois),
                    micromouse::nomeDaFalhaExecucao(cicloNavegacao.falhaExecucao()),
                    static_cast<unsigned long>(cicloNavegacao.celulasAvancadas()));
      if (depois == micromouse::EstadoCiclo::Falha) {
        // Onde a odometria achava que o robô estava, em mm (norte e leste a partir do centro da célula 0,0).
        console.printf("{\"status\":\"odometria_na_falha\",\"norte_mm\":%ld,\"leste_mm\":%ld}\n",
                      static_cast<long>(odometria.posicaoNorteMicrometros() / 1000),
                      static_cast<long>(odometria.posicaoLesteMicrometros() / 1000));
      }
    }
    vTaskDelay(pdMS_TO_TICKS(PERIODO_NAVEGACAO_MS));
  }
}

/// Mede a bateria e avisa no log quando ela fica fraca ou volta (`{"status":"bateria",...}`). Chamada
/// a cada ciclo da telemetria (1 s).
static void medirBateria() {
  const uint32_t mv = bateria.lerMv();
  ultimaBateriaMv = mv;
  if (monitorBateria.atualizar(mv)) {
    console.printf("{\"status\":\"bateria\",\"situacao\":\"%s\",\"mv\":%lu}\n",
                   micromouse::nomeDaSituacaoBateria(monitorBateria.situacao()), static_cast<unsigned long>(mv));
  }
}

/// Resposta ao comando `E`: o estado do robô numa linha, para quem o acompanha pelo telnet.
static void imprimirEstado() {
  char fase[24];
  portENTER_CRITICAL(&travaCorrida);
  strncpy(fase, micromouse::nomeDaFase(faseCorrida.fase()), sizeof(fase) - 1);
  fase[sizeof(fase) - 1] = '\0';
  const bool corridaAtiva = estadoCorrida.ativa;
  portEXIT_CRITICAL(&travaCorrida);
  const bool wifi = WiFi.status() == WL_CONNECTED;
  const uint32_t bateriaMv = ultimaBateriaMv;
  console.printf(
      "{\"status\":\"estado\",\"fase\":\"%s\",\"ciclo\":\"%s\",\"largada_liberada\":%s,\"boot_ok\":%s,"
      "\"bateria_mv\":%lu,\"bateria_pct\":%u,\"wifi\":%s,\"rssi\":%d,\"ip\":\"%s\",\"telnet\":%s,\"corrida_ativa\":%s,"
      "\"encoder_esq\":\"%s\",\"encoder_dir\":\"%s\","
      "\"console_descartados\":%lu,\"logs_painel_descartados\":%lu,\"logs_painel_perdidos\":%lu}\n",
      fase, micromouse::nomeDoEstadoCiclo(static_cast<micromouse::EstadoCiclo>(estadoCicloAtual)),
      largadaLiberadaAtual ? "true" : "false", estadoBoot.ok ? "true" : "false", static_cast<unsigned long>(bateriaMv),
      static_cast<unsigned>(micromouse::bateriaPercentual(bateriaMv)), wifi ? "true" : "false", wifi ? WiFi.RSSI() : 0,
      wifi ? WiFi.localIP().toString().c_str() : "", console.clienteConectado() ? "true" : "false", corridaAtiva ? "true" : "false",
      paresDeEncoders.esquerdoSemSinal() ? "sem_sinal" : "ok", paresDeEncoders.direitoSemSinal() ? "sem_sinal" : "ok",
      static_cast<unsigned long>(console.descartados()),
      static_cast<unsigned long>(console.linhasDoPainelDescartadas()), static_cast<unsigned long>(logsPerdidosNoEnvio));
}

/// Comandos de uma letra, da serial, do telnet e do painel (que manda `iniciar` como `L`, `parar` como
/// `X` e `zerar` como `Z`, pela tarefa de rede):
/// - `L`: libera a largada sem o gesto (vale quando o robô estiver pronto, por até
///   `LARGADA_PENDENTE_MAXIMA_MS`);
/// - `X`: para na hora e volta a esperar a largada (parada de emergência);
/// - `Z`: zera os encoders (ou a estimativa), volta ao centro da célula e a aguardar a largada, para o
///   robô recolocado na partida à mão;
/// - `B`: repete o relatório do boot (a linha se perde se o monitor abrir depois);
/// - `E`: mostra o estado (fase, ciclo, bateria, Wi-Fi e telnet).
/// Chamada pela tarefa do console, a cada poucas dezenas de ms: o comando age quase na hora, mesmo com
/// a tarefa de telemetria esperando o próximo segundo.
static void executarComando(char c) {
  if (c == 'Z' || c == 'z') {
    // Os encoders voltam a zero: a odometria mantém a pose e passa a contar a partir de zero.
    // No modo estimado, o robô também volta ao centro da célula e a pose volta a ser confiável
    // (na bancada, 'Z' é usado com o robô posicionado à mão). Na largada em qualquer célula, o robô foi
    // recolocado numa célula qualquer: a odometria volta ao meio do mapa de exploração, a navegação
    // recomeça a se localizar e o tamanho do labirinto precisa ser informado de novo.
    portENTER_CRITICAL(&travaOdometria);
    const micromouse::Pose recolocado =
        micromouse::LARGADA_EM_QUALQUER_CELULA
            ? micromouse::Pose{micromouse::LocalizadorLargada::LARGADA_NA_EXPLORACAO, micromouse::Direcao::Norte}
            : odometria.pose();
    if (micromouse::ENCODERS_INSTALADOS) {
      medidorDeslocamento.zerar();
      odometria.reiniciar(recolocado, 0, 0);
    } else {
      deslocamento.reiniciar(odometria, recolocado);
    }
    labirintoInformado = !micromouse::LARGADA_EM_QUALQUER_CELULA;
    detectorObjetivo.resetar();
    pedidoReinicioNavegacao = true;
    portEXIT_CRITICAL(&travaOdometria);
    portENTER_CRITICAL(&travaCorrida);
    faseCorrida.reiniciar();  // recolocado na partida à mão: volta a aguardar a largada
    const bool corridaEncerrada = faseCorrida.encerrada();
    portEXIT_CRITICAL(&travaCorrida);
    console.println("{\"status\":\"encoders_zerados\"}");
    if (corridaEncerrada) {
      // A corrida já foi encerrada no servidor: o `Z` zera a odometria, mas não começa outra tentativa.
      console.println("{\"status\":\"aviso\",\"motivo\":\"corrida_encerrada_reinicie_a_placa_para_nova_tentativa\"}");
    }
  } else if (c == 'B' || c == 'b') {
    imprimirRelatorioBoot();
  } else if (c == 'L' || c == 'l') {
    portENTER_CRITICAL(&travaOdometria);
    pedidoLargada = true;
    portEXIT_CRITICAL(&travaOdometria);
    console.println("{\"status\":\"largada_pedida\"}");
  } else if (c == 'X' || c == 'x') {
    portENTER_CRITICAL(&travaOdometria);
    pedidoParada = true;
    portEXIT_CRITICAL(&travaOdometria);
  } else if (c == 'E' || c == 'e') {
    imprimirEstado();
  } else if (c == 'M' || c == 'm') {
    imprimirMapaDoRobo();
  }
}

/// Comando `M`: imprime o mapa que o robô montou, com o Norte em cima. `---` e `|` são paredes, espaço é
/// passagem livre e `?` é o que ele ainda não sabe. É só para depurar: lê o mapa enquanto a navegação o
/// escreve, então uma linha pode sair no meio de uma atualização.
static void imprimirMapaDoRobo() {
  const micromouse::Mapa& mapa = armazenamentoMapa.mapa();
  const uint8_t linhas = mapa.obterLinhas();
  const uint8_t colunas = mapa.obterColunas();
  console.printf("{\"status\":\"mapa\",\"linhas\":%u,\"colunas\":%u}\n", static_cast<unsigned>(linhas),
                 static_cast<unsigned>(colunas));
  char texto[4 * micromouse::MAX_COLUNAS_EXPLORACAO + 3];
  const auto horizontal = [](micromouse::EstadoParede e) { return e == micromouse::EstadoParede::Parede ? "---" : (e == micromouse::EstadoParede::Livre ? "   " : " ? "); };
  const auto vertical = [](micromouse::EstadoParede e) { return e == micromouse::EstadoParede::Parede ? '|' : (e == micromouse::EstadoParede::Livre ? ' ' : '?'); };
  for (int l = static_cast<int>(linhas) - 1; l >= -1; --l) {
    // Linha das paredes horizontais: o Norte de cada célula da linha `l` (ou o Sul da linha 0, no fim).
    size_t n = 0;
    for (uint8_t c = 0; c < colunas; ++c) {
      texto[n++] = '+';
      const micromouse::EstadoParede e = l >= 0 ? mapa.obterParede({static_cast<uint8_t>(l), c}, micromouse::Direcao::Norte)
                                                : mapa.obterParede({0, c}, micromouse::Direcao::Sul);
      const char* h = horizontal(e);
      texto[n++] = h[0];
      texto[n++] = h[1];
      texto[n++] = h[2];
    }
    texto[n++] = '+';
    texto[n] = '\0';
    console.println(texto);
    if (l < 0) {
      break;
    }
    n = 0;
    for (uint8_t c = 0; c < colunas; ++c) {
      texto[n++] = vertical(mapa.obterParede({static_cast<uint8_t>(l), c}, micromouse::Direcao::Oeste));
      texto[n++] = ' ';
      texto[n++] = ' ';
      texto[n++] = ' ';
    }
    texto[n++] = vertical(mapa.obterParede({static_cast<uint8_t>(l), static_cast<uint8_t>(colunas - 1)}, micromouse::Direcao::Leste));
    texto[n] = '\0';
    console.println(texto);
  }
}

/// Lê os comandos que chegaram pela serial e pelo telnet e os executa.
static void tratarComandos() {
  for (int lido = console.ler(); lido >= 0; lido = console.ler()) {
    executarComando(static_cast<char>(lido));
  }
}

/// Tarefa do console: o telnet (aceitar cliente, ler o que ele digita, esvaziar o log pela rede) e os
/// comandos, a cada `PERIODO_CICLO_CONSOLE_MS`.
///
/// Prioridade 1, como a de rede, e **separada dela**: a tarefa de rede fica até `TEMPO_LIMITE_REDE_MS`
/// esperando o servidor numa chamada HTTPS, e o comando `X` não pode esperar isso. Só esta tarefa
/// espera o cliente telnet; a navegação e a telemetria só escrevem na fila do `ConsoleRemoto`.
static void tarefaConsole(void*) {
  for (;;) {
    console.ciclo();
    tratarComandos();
    vTaskDelay(pdMS_TO_TICKS(PERIODO_CICLO_CONSOLE_MS));
  }
}

/// Tarefa de telemetria: coleta e envia os dados a cada `PERIODO_TELEMETRIA_MS`.
///
/// Prioridade 1. Mede a bateria e transmite as leituras dos encoders, pose da odometria e eventos
/// pontuais em formato JSON estruturado. Os comandos (`L`, `X`, `Z`, `B`, `E`) são da tarefa do console
/// (`tratarComandos`).
///
/// No modo sem encoder, `pose_valida` também fica `false` depois de o robô andar mais que
/// `DISTANCIA_MAXIMA_SEM_CORRECAO_MM` sem correção pelo ToF, e `deslocamento` diz qual fonte está
/// em uso (`"estimado"` ou `"encoder"`).
static void tarefaTelemetria(void*) {
  static uint32_t sequencia = 0;
  micromouse::EstimadorVelocidade estimadorVelocidade;  // velocidade da telemetria: mm/s pelo deslocamento das rodas

  for (;;) {
    medirBateria();

    // Encoders (reais ou estimados) e pose: uma cópia feita na seção crítica, para tudo ser da
    // mesma atualização.
    portENTER_CRITICAL(&travaOdometria);
    const micromouse::DeslocamentoRodas d = medidorDeslocamento.medir();
    const int32_t medio = medidorDeslocamento.deslocamentoMedioMm();
    const int32_t pulsosEsq = rodaEsq.lerContagem();
    const int32_t pulsosDir = rodaDir.lerContagem();
    const int32_t velocidadeMmPorS = estimadorVelocidade.atualizar(medio, millis());
    const micromouse::Pose pose = poseNoLabirinto;
    const bool poseConfiavel = micromouse::ENCODERS_INSTALADOS || deslocamento.poseConfiavel();
    // Antes de o robô se localizar (largada em qualquer célula), a pose não vale para o dashboard.
    const bool poseValida = odometria.posicaoValida() && poseConfiavel && poseNoLabirintoValida;
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
    pacote.velocidade = static_cast<float>(velocidadeMmPorS) / 1000.0f;  // m/s, pelo deslocamento das rodas
    pacote.tensaoBateria = ultimaBateriaMv / 1000.0f;
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
    coletarAmostra(pose, dispararEvento, medio, velocidadeMmPorS);

    // Comporta o pior caso do JSON; com menos, a linha seria descartada em silêncio.
    char buffer[micromouse::TAMANHO_MAX_JSON_TELEMETRIA];
    if (micromouse::TelemetriaJson::serializar(pacote, buffer, sizeof(buffer)) > 0) {
      console.println(buffer);
    }

    vTaskDelay(pdMS_TO_TICKS(micromouse::PERIODO_TELEMETRIA_MS));
  }
}

/// Sequência de boot da Rev 2.0 (#50): motores freados e travados, barramento I²C, sensores de
/// distância com os endereços novos, autoteste do I²C e, só com ele aprovado, motores liberados;
/// depois a IMU (#378) e as tarefas de navegação, telemetria e rede. As tarefas sobem mesmo com
/// falha, para a telemetria continuar informando o que falhou.
void setup() {
  // Primeira linha: freia os motores antes de qualquer outra coisa, porque o GPIO14 (IN4) emite um
  // pulso PWM curto durante o reset (ver README, "Tranco no boot").
  motores.iniciar();
  // O log sai pela UART0 (conversor USB da placa): sem computador conectado, a UART transmite do
  // mesmo jeito e não trava a telemetria.
  console.iniciar(115200);
  bateria.iniciar();
  console.println("micromouse: iniciando");

  // Configuração provisória para bancada (#189): fixa Labirinto4x4.
  // Em 8x4 e 12x4 a corrida deve receber a dimensão por comando antes de iniciar.
  armazenamentoMapa.iniciarCorrida(micromouse::DimensaoMapa::Labirinto4x4);
  // A navegação refaz o objetivo pelo tamanho do mapa e aguarda a largada na partida.
  cicloNavegacao.reiniciar(micromouse::Pose{{0, 0}, micromouse::Direcao::Norte}, motoresTracao);
  motoresTracao.parar();
  // A odometria parte de onde o ciclo pede: a célula (0, 0) ou, na largada em qualquer célula, o meio
  // do mapa de exploração.
  if (micromouse::ENCODERS_INSTALADOS) {
    odometria.reiniciar(cicloNavegacao.poseInicialOdometria(), 0, 0);
  } else {
    deslocamento.reiniciar(odometria, cicloNavegacao.poseInicialOdometria());
  }
  poseNoLabirintoValida = cicloNavegacao.localizado();
  if (micromouse::LARGADA_EM_QUALQUER_CELULA) {
    console.println("{\"status\":\"largada_em_qualquer_celula\"}");
  }
  if (micromouse::ENCODERS_INSTALADOS) {
    encoderEsq.iniciar();
    encoderDir.iniciar();
  }

  // O barramento I²C é dos 3 VL53L0X e do MPU6050: `barramentoI2c.iniciar` cria o mutex e inicia o
  // `Wire`. O `tof.iniciar()` e o autoteste usam o `Wire` sem o mutex porque rodam aqui, antes dos
  // `xTaskCreate` (inclusive o da IMU). Se falhar, o autoteste aponta os dispositivos sem resposta.
  if (!barramentoI2c.iniciar(micromouse::pinos::I2C_SDA, micromouse::pinos::I2C_SCL)) {
    console.println("{\"status\":\"falha_barramento_i2c\"}");
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
    console.printf("micromouse: falha no boot (%s): motores travados ate reiniciar\n", estadoBoot.falhas);
  }

  // Tempo desde o início do programa (RNF-09); a ROM e o bootloader rodam antes e ficam de fora.
  estadoBoot.duracaoMs = millis();
  imprimirRelatorioBoot();

  // MPU6050 (#378). Sem a IMU o firmware segue, sem ângulo de giro válido.
  const bool imuIniciada = imu.iniciar();
  if (imuIniciada) {
    console.printf("{\"status\":\"imu_iniciada\",\"who_am_i\":\"0x%02X\"}\n", imu.identificacao());
  } else {
    console.println("{\"status\":\"falha_imu\"}");
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
      console.println("{\"status\":\"aguardando_largada\"}");
    }
    console.printf("{\"status\":\"calibracao\",\"estado\":\"%s\",\"duracao_ms\":%lu,\"no_prazo\":%s}\n",
                  micromouse::nomeEstadoRotina(calibracao),
                  static_cast<unsigned long>(rotinaCalibracao.duracaoMedidaMs()),
                  rotinaCalibracao.dentroDoOrcamento() ? "true" : "false");
  }

  xTaskCreate(tarefaImu, "imu", 3072, nullptr, 3, nullptr);
  // As pilhas de navegação e telemetria subiram de 4096 para 5120 bytes: o `printf` do console formata
  // numa linha de até `ConsoleRemoto::TAMANHO_MAX_LINHA` bytes na pilha de quem o chama.
  xTaskCreate(tarefaNavegacao, "navegacao", 5120, nullptr, 2, nullptr);
  xTaskCreate(tarefaTelemetria, "telemetria", 5120, nullptr, 1, nullptr);
  xTaskCreate(tarefaRede, "rede", 10240, nullptr, 1, nullptr);
  xTaskCreate(tarefaConsole, "console", 4096, nullptr, 1, nullptr);
}

/// Laço principal do Arduino. Fica ocioso: o trabalho acontece nas tarefas.
void loop() {
  delay(1000);
}
