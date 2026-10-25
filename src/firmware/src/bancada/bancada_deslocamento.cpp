/// @file bancada_deslocamento.cpp
/// @brief Firmware de bancada do deslocamento sem encoder (#380) e da execução dos movimentos
/// (#184): aciona os motores por um tempo ou executa os comandos "avançar uma célula", "girar 90°" e
/// "girar 180°" com o mesmo `DeslocamentoEstimado` e o mesmo `ExecutorMovimentos` do robô, e manda
/// pela serial o log de cada ciclo, para a equipe comparar com a régua e o transferidor. Também mede
/// o desvio lateral pelas paredes, realinha o robô ao centro do corredor e executa um percurso de
/// vários movimentos com a correção pelas paredes, para conferir a célula estimada no fim (#188).
///
/// Não é o firmware do robô: só entra no ambiente `bancada_deslocamento` do `platformio.ini`. O menu
/// `scripts/bancada_deslocamento.py` grava este programa e conduz o roteiro; também dá para usar
/// direto pelo `pio device monitor -e bancada_deslocamento` (comando `AJUDA`).
///
/// SEGURANÇA: os motores só giram durante `ANDAR`, `GIRAR`, `CELULA` e `GIRO*` (estes com os tempos
/// limite do executor), e qualquer byte recebido pela serial durante o movimento freia os motores
/// na hora.
///
/// Protocolo: um comando por linha; cada resposta é uma linha JSON com o campo `tipo`. Linhas que
/// começam com `#` são texto para quem lê o monitor.

#include <Arduino.h>
#include <Wire.h>

#if !defined(ARDUINO_USB_CDC_ON_BOOT) || !ARDUINO_USB_CDC_ON_BOOT
#error "Serial pela USB nativa: defina ARDUINO_USB_MODE=1 e ARDUINO_USB_CDC_ON_BOOT=1 no platformio.ini (Pinos.h)"
#endif

#include "AcompanhadorGiro.h"
#include "BarramentoI2c.h"
#include "ConfiguracaoDeslocamento.h"
#include "ConfiguracaoLocalizacao.h"
#include "CorrecaoPelasParedes.h"
#include "CorretorDeslocamentoTof.h"
#include "DeslocamentoEstimado.h"
#include "ExecutorMovimentos.h"
#include "ImuMpu6050.h"
#include "MotoresDrv8833.h"
#include "Movimento.h"
#include "Odometria.h"
#include "Pinos.h"
#include "TofVl53l0x.h"

using namespace micromouse;

namespace {

/// Ciclo da estimativa: o mesmo da tarefa de navegação.
constexpr uint32_t PERIODO_CICLO_MS = 10;

/// Intervalo entre linhas de log durante um movimento.
constexpr uint32_t PERIODO_LOG_MS = 50;

/// Maior duração aceita para `ANDAR` e `GIRAR`.
constexpr long MAX_DURACAO_MS = 10000;

MotoresDrv8833 motoresDrv(pinos::MOTOR_ESQ_IN1, pinos::MOTOR_ESQ_IN2, pinos::MOTOR_DIR_IN3,
                          pinos::MOTOR_DIR_IN4, false, true);
DeslocamentoEstimado deslocamento(motoresDrv);
Odometria odometria;
TofVl53l0x tof(Wire, {pinos::XSHUT_FRONTAL, pinos::ENDERECO_TOF_FRONTAL},
               {pinos::XSHUT_ESQ, pinos::ENDERECO_TOF_ESQ}, {pinos::XSHUT_DIR, pinos::ENDERECO_TOF_DIR});

/// Giroscópio do MPU6050 (#378), para o rumo e os giros da execução (#184). A bancada roda numa
/// tarefa só: o barramento é o mesmo dos ToF, sem disputa.
BarramentoI2c barramento(Wire);
ImuMpu6050 imu(barramento);
AcompanhadorGiro giro(imu);
bool imuOk = false;
uint32_t ultimoGiroMs = 0;

/// Execução dos movimentos (#184) e a direção do robô, confirmada no fim de cada giro.
ExecutorMovimentos executor;
Direcao direcaoAtual = Direcao::Norte;

/// Maior número de células num `CELULA n`.
constexpr long MAX_CELULAS = 8;

/// Correção do erro acumulado pelas paredes (#188), como no robô.
CorrecaoPelasParedes correcaoParedes;
const ConfiguracaoRealinhamento configRealinhamento;

/// Maior número de movimentos num `PERCURSO`.
constexpr unsigned MAX_MOVIMENTOS_PERCURSO = 32;

/// Se as leituras do ToF corrigem a posição (`CORRECAO ON|OFF`).
bool correcaoLigada = true;

/// Leitura mais recente do ToF e instante da amostra mais antiga dela (para saber se é nova).
DistanciasLaterais ultimaLeitura = {0, 0, 0};
bool ultimaLeituraValida = false;
uint32_t instanteUltimaLeituraUsada = 0;

const char* nomeResultado(ResultadoCorrecao resultado) {
  switch (resultado) {
    case ResultadoCorrecao::SemReferencia:
      return "sem_referencia";
    case ResultadoCorrecao::PelaFrente:
      return "pela_frente";
    case ResultadoCorrecao::PelaLateral:
      return "pela_lateral";
    case ResultadoCorrecao::Descartada:
      return "descartada";
    case ResultadoCorrecao::Girando:
      return "girando";
  }
  return "?";
}

/// `true` se chegou algum byte pela serial (descarta o que chegou).
bool interrompido() {
  if (Serial.available() == 0) {
    return false;
  }
  while (Serial.available() > 0) {
    Serial.read();
  }
  return true;
}

void imprimirAjuda() {
  Serial.println("# Bancada do deslocamento sem encoder (#380). Comandos (um por linha):");
  Serial.println("#   INFO            configuracao em uso (tabela, limites, encoders)");
  Serial.println("#   VEL p           velocidade que a tabela supoe para a potencia p, em mm/s");
  Serial.println("#   ZERAR           robo no centro de (0, 0) virado para o Norte, pose confiavel");
  Serial.println("#   CORRECAO ON|OFF liga ou desliga a correcao pelo ToF");
  Serial.println("#   ANDAR p ms      as duas rodas com potencia p por ms (qualquer tecla para)");
  Serial.println("#   GIRAR p ms      esquerda +p e direita -p por ms (giro para a direita)");
  Serial.println("#   CELULA n        executa n vezes \"avancar uma celula\" (#184), de 1 a 8");
  Serial.println("#   GIRO90E|GIRO90D|GIRO180  executa o giro pelo giroscopio (#184)");
  Serial.println("#   CALIBRAR        calibra o vies do giroscopio (robo parado)");
  Serial.println("#   DESVIO          desvio lateral medido pelas paredes, sem mover (#188)");
  Serial.println("#   REALINHAR [F]   vai para o centro do corredor; F comeca para a frente (#188)");
  Serial.println("#   PERCURSO seq    executa A (avancar), D/E (90 graus) e V (180), realinhando antes de girar");
  Serial.println("#   LER             uma linha de estado, sem mover");
  Serial.println("#   AJUDA           esta lista");
  Serial.println("# ATENCAO: ANDAR, GIRAR, CELULA e GIRO* acionam os motores. Robo no chao, com espaco livre.");
}

void imprimirInfo() {
  const ConfiguracaoDeslocamento config;
  Serial.printf("{\"tipo\":\"info\",\"programa\":\"bancada_deslocamento\",\"encoders_instalados\":%s,"
                "\"tabela\":[",
                ENCODERS_INSTALADOS ? "true" : "false");
  for (uint8_t i = 0; i < config.pontosTabela; i++) {
    Serial.printf(i == 0 ? "[%d,%u]" : ",[%d,%u]", config.tabela[i].potencia,
                  config.tabela[i].velocidadeMmPorS);
  }
  Serial.printf("],\"sensor_frontal_ao_centro_mm\":%u,\"avanco_sensores_laterais_mm\":%d,"
                "\"frontal_maxima_para_corrigir_mm\":%u,\"limiar_parede_lateral_mm\":%u,"
                "\"histerese_parede_lateral_mm\":%u,\"correcao_maxima_mm\":%u,"
                "\"maxima_sem_correcao_mm\":%u,\"correcao_ligada\":%s,\"tof_ok\":%s,\"imu_ok\":%s,"
                "\"giro_calibrado\":%s}\n",
                config.distanciaSensorFrontalAoCentroMm, config.avancoSensoresLateraisMm,
                config.distanciaFrontalMaximaParaCorrigirMm, config.limiarParedeLateralMm,
                config.histereseParedeLateralMm, config.correcaoMaximaMm,
                config.distanciaMaximaSemCorrecaoMm, correcaoLigada ? "true" : "false",
                tof.sensorIniciado(Lado::Frente) && tof.sensorIniciado(Lado::Esquerda) &&
                        tof.sensorIniciado(Lado::Direita)
                    ? "true"
                    : "false",
                imuOk ? "true" : "false",
                giro.ultima().calibracao == EstadoCalibracao::Calibrado ? "true" : "false");
}

/// Lê o giroscópio se já passou o período da IMU (10 ms). Chamado no `loop()` e durante os
/// movimentos, para o ângulo seguir o robô mesmo parado.
void acompanharGiro() {
  const uint32_t agoraMs = millis();
  const uint32_t dtMs = agoraMs - ultimoGiroMs;
  if (!imuOk || dtMs < PERIODO_LEITURA_IMU_MS) {
    return;
  }
  ultimoGiroMs = agoraMs;
  giro.ciclo(dtMs);
}

/// Calibra o viés do giroscópio com o robô parado (até 5 s, recomeçando se ele se mexer).
void calibrarGiro() {
  if (!imuOk) {
    Serial.println("{\"tipo\":\"calibracao\",\"estado\":\"sem_imu\"}");
    return;
  }
  giro.iniciarCalibracao();
  const uint32_t inicioMs = millis();
  while (giro.ultima().calibracao != EstadoCalibracao::Calibrado && millis() - inicioMs < 5000) {
    acompanharGiro();
    delay(1);
  }
  const bool ok = giro.ultima().calibracao == EstadoCalibracao::Calibrado;
  executor.definirRumoAlvo(giro.ultima().anguloMiligraus);
  Serial.printf("{\"tipo\":\"calibracao\",\"estado\":\"%s\",\"recusadas\":%lu}\n", ok ? "calibrado" : "falhou",
                static_cast<unsigned long>(giro.ultima().calibracoesRecusadas));
}

/// `true` se a última chamada de `lerTofNovo()` trouxe uma leitura nova (com a correção ligada ou
/// não): é a que o controle do avanço usa.
bool ultimaLeituraNova = false;

/// Lê o ToF e devolve a leitura se ela for nova e a correção estiver ligada (ainda não usada pela
/// correção).
const DistanciasLaterais* lerTofNovo() {
  ultimaLeituraNova = false;
  ultimaLeituraValida = tof.lerDistanciasLaterais(ultimaLeitura);
  if (!ultimaLeituraValida) {
    return nullptr;
  }
  const uint32_t instante = millis() - tof.idadeUltimaLeituraMs();
  if (instante == instanteUltimaLeituraUsada) {
    return nullptr;
  }
  instanteUltimaLeituraUsada = instante;
  ultimaLeituraNova = true;
  return correcaoLigada ? &ultimaLeitura : nullptr;
}

/// Uma linha de estado: potência, pulsos, posição, pose, confiança e ToF.
void imprimirEstado(const char* tipo, uint32_t tMs) {
  const PotenciaRodas p = deslocamento.estimador().potencia();
  const Pose pose = odometria.pose();
  Serial.printf("{\"tipo\":\"%s\",\"t_ms\":%lu,\"pot_esq\":%d,\"pot_dir\":%d,\"pulsos_esq\":%ld,"
                "\"pulsos_dir\":%ld,\"norte_mm\":%.1f,\"leste_mm\":%.1f,\"linha\":%u,\"coluna\":%u,"
                "\"orientacao\":\"%c\",\"pose_valida\":%s,\"sem_correcao_mm\":%lu,"
                "\"tof_frente_mm\":%u,\"tof_esq_mm\":%u,\"tof_dir_mm\":%u,\"tof_valida\":%s}\n",
                tipo, static_cast<unsigned long>(tMs), p.esquerda, p.direita,
                static_cast<long>(deslocamento.encoderEsquerdo().lerContagem()),
                static_cast<long>(deslocamento.encoderDireito().lerContagem()),
                odometria.posicaoNorteMicrometros() / 1000.0, odometria.posicaoLesteMicrometros() / 1000.0,
                pose.celula.linha, pose.celula.coluna, letraDaDirecao(pose.direcao),
                odometria.posicaoValida() && deslocamento.poseConfiavel() ? "true" : "false",
                static_cast<unsigned long>(deslocamento.distanciaSemCorrecaoMm()), ultimaLeitura.frenteMm,
                ultimaLeitura.esquerdaMm, ultimaLeitura.direitaMm, ultimaLeituraValida ? "true" : "false");
}

/// Aciona as rodas com `potencia` por `duracaoMs`, rodando o ciclo da estimativa a cada 10 ms.
void mover(const PotenciaRodas& potencia, long duracaoMs, const char* nome) {
  uint32_t correcoes = 0;
  uint32_t descartadas = 0;
  int32_t somaCorrecoesUm = 0;
  const double inicioNorteMm = odometria.posicaoNorteMicrometros() / 1000.0;
  const double inicioLesteMm = odometria.posicaoLesteMicrometros() / 1000.0;
  bool parou = false;

  deslocamento.motores().definirPotencia(potencia);
  const uint32_t inicioMs = millis();
  uint32_t ultimoCicloMs = inicioMs;
  uint32_t ultimoLogMs = 0;
  while (millis() - inicioMs < static_cast<uint32_t>(duracaoMs)) {
    if (interrompido()) {
      parou = true;
      break;
    }
    const uint32_t agoraMs = millis();
    if (agoraMs - ultimoCicloMs < PERIODO_CICLO_MS) {
      delay(1);
      continue;
    }
    const uint32_t dtMs = agoraMs - ultimoCicloMs;
    ultimoCicloMs = agoraMs;
    const ResultadoCorrecao r = deslocamento.ciclo(dtMs, odometria, lerTofNovo());
    if (r == ResultadoCorrecao::PelaFrente || r == ResultadoCorrecao::PelaLateral ||
        r == ResultadoCorrecao::Descartada) {
      const Correcao& c = deslocamento.ultimaCorrecao();
      if (r == ResultadoCorrecao::Descartada) {
        descartadas++;
      } else {
        correcoes++;
        somaCorrecoesUm += c.deltaUm;
      }
      Serial.printf("{\"tipo\":\"correcao\",\"t_ms\":%lu,\"resultado\":\"%s\",\"delta_mm\":%.1f,"
                    "\"norte_mm\":%.1f,\"leste_mm\":%.1f,\"tof_frente_mm\":%u,\"tof_esq_mm\":%u,"
                    "\"tof_dir_mm\":%u}\n",
                    static_cast<unsigned long>(agoraMs - inicioMs), nomeResultado(r), c.deltaUm / 1000.0,
                    odometria.posicaoNorteMicrometros() / 1000.0,
                    odometria.posicaoLesteMicrometros() / 1000.0, ultimaLeitura.frenteMm,
                    ultimaLeitura.esquerdaMm, ultimaLeitura.direitaMm);
    }
    if (agoraMs - ultimoLogMs >= PERIODO_LOG_MS) {
      ultimoLogMs = agoraMs;
      imprimirEstado("log", agoraMs - inicioMs);
    }
  }
  deslocamento.motores().parar();
  const uint32_t duracaoReal = millis() - inicioMs;
  // Um último ciclo, já parado, para a estimativa terminar no instante da parada.
  deslocamento.ciclo(0, odometria, nullptr);

  Serial.printf("{\"tipo\":\"fim_movimento\",\"movimento\":\"%s\",\"interrompido\":%s,"
                "\"duracao_ms\":%lu,\"avanco_norte_mm\":%.1f,\"avanco_leste_mm\":%.1f,"
                "\"correcoes\":%lu,\"descartadas\":%lu,\"soma_correcoes_mm\":%.1f}\n",
                nome, parou ? "true" : "false", static_cast<unsigned long>(duracaoReal),
                odometria.posicaoNorteMicrometros() / 1000.0 - inicioNorteMm,
                odometria.posicaoLesteMicrometros() / 1000.0 - inicioLesteMm,
                static_cast<unsigned long>(correcoes), static_cast<unsigned long>(descartadas),
                somaCorrecoesUm / 1000.0);
  imprimirEstado("estado", duracaoReal);
}

/// Executa um movimento com o `ExecutorMovimentos`, no mesmo ciclo de 10 ms da navegação, e manda
/// o log e a linha `fim_execucao` (o erro que fica para a régua e o transferidor).
/// @param plano Plano do realinhamento, quando `movimento` é `Realinhar`.
/// @return `true` se o movimento foi concluído.
bool executarMovimento(Movimento movimento, const PlanoRealinhamento* plano = nullptr) {
  if (ehManobra(movimento) && giro.ultima().calibracao != EstadoCalibracao::Calibrado) {
    Serial.println("{\"tipo\":\"erro\",\"mensagem\":\"giroscopio nao calibrado: use CALIBRAR\"}");
    return false;
  }
  const double inicioNorteMm = odometria.posicaoNorteMicrometros() / 1000.0;
  const double inicioLesteMm = odometria.posicaoLesteMicrometros() / 1000.0;
  const int32_t anguloInicial = giro.ultima().anguloMiligraus;
  bool parou = false;
  if (plano != nullptr) {
    executor.iniciarRealinhamento(*plano);
  } else {
    executor.iniciar(movimento);
  }
  const bool manobra = ehManobra(movimento);
  const uint32_t inicioMs = millis();
  uint32_t ultimoCicloMs = inicioMs;
  uint32_t ultimoLogMs = 0;
  while (executor.ocupado()) {
    if (interrompido()) {
      executor.cancelar(deslocamento.motores());
      parou = true;
      break;
    }
    acompanharGiro();
    const uint32_t agoraMs = millis();
    if (agoraMs - ultimoCicloMs < PERIODO_CICLO_MS) {
      delay(1);
      continue;
    }
    const uint32_t dtMs = agoraMs - ultimoCicloMs;
    ultimoCicloMs = agoraMs;
    // Como na tarefa de navegação: fora dos giros, a direção da odometria é a do giroscópio.
    if (!ehGiro(movimento)) {
      odometria.definirDirecao(direcaoAtual);
    }
    // Nas manobras, as leituras não servem de referência de posição (#188).
    const DistanciasLaterais* referencia = lerTofNovo();
    if (manobra) {
      deslocamento.esquecerLaterais();
      referencia = nullptr;
    }
    deslocamento.ciclo(dtMs, odometria, referencia);
    if (ultimaLeituraNova && correcaoLigada) {
      correcaoParedes.aplicar(odometria, ultimaLeitura, manobra ? MovimentoRobo::Manobrando : MovimentoRobo::Avancando,
                              ENCODERS_INSTALADOS);
    }
    EntradaExecucao entrada;
    // O controle precisa de toda leitura nova, mesmo com a correção da posição desligada.
    entrada.leituraNova = ultimaLeituraNova ? &ultimaLeitura : nullptr;
    entrada.giro = giro.ultima();
    entrada.posicaoAvancoUm = posicaoAoLongoDoAvancoUm(odometria);
    entrada.dtMs = dtMs;
    executor.atualizar(entrada, deslocamento.motores());
    if (agoraMs - ultimoLogMs >= PERIODO_LOG_MS) {
      ultimoLogMs = agoraMs;
      imprimirEstado("log", agoraMs - inicioMs);
    }
  }
  deslocamento.motores().parar();
  deslocamento.ciclo(0, odometria, nullptr);
  const bool concluido = executor.estado() == EstadoExecucao::Concluido;
  if (concluido && ehGiro(movimento)) {
    direcaoAtual = direcaoDepois(direcaoAtual, movimento);
    odometria.definirDirecao(direcaoAtual);
  }
  const int32_t angulo = giro.ultima().anguloMiligraus;
  Serial.printf("{\"tipo\":\"fim_execucao\",\"movimento\":\"%s\",\"estado\":\"%s\",\"falha\":\"%s\","
                "\"interrompido\":%s,\"tempo_ms\":%lu,\"restante_mm\":%ld,\"avanco_norte_mm\":%.1f,"
                "\"avanco_leste_mm\":%.1f,\"giro_medido_graus\":%.2f,\"erro_rumo_graus\":%.2f,"
                "\"orientacao\":\"%c\"}\n",
                nomeDoMovimento(movimento), nomeDoEstadoExecucao(executor.estado()),
                nomeDaFalhaExecucao(executor.falha()), parou ? "true" : "false",
                static_cast<unsigned long>(millis() - inicioMs), static_cast<long>(executor.restanteMm()),
                odometria.posicaoNorteMicrometros() / 1000.0 - inicioNorteMm,
                odometria.posicaoLesteMicrometros() / 1000.0 - inicioLesteMm, (angulo - anguloInicial) / 1000.0,
                (executor.rumoAlvoMiligraus() - angulo) / 1000.0, letraDaDirecao(direcaoAtual));
  imprimirEstado("estado", millis() - inicioMs);
  return concluido && !parou;
}

/// Mede o desvio lateral pelas paredes com o robô parado: a mediana de 3 leituras novas do ToF.
DesvioLateral medirDesvio() {
  uint16_t esquerda[3] = {0, 0, 0};
  uint16_t direita[3] = {0, 0, 0};
  uint8_t n = 0;
  const uint32_t inicioMs = millis();
  while (n < 3 && millis() - inicioMs < 1000) {
    acompanharGiro();
    lerTofNovo();
    if (ultimaLeituraNova) {
      esquerda[n] = ultimaLeitura.esquerdaMm;
      direita[n] = ultimaLeitura.direitaMm;
      n++;
    }
    delay(1);
  }
  if (n < 3) {
    return DesvioLateral{};
  }
  const auto mediana = [](uint16_t* v) {
    const uint16_t a = v[0] < v[1] ? v[0] : v[1];
    const uint16_t b = v[0] < v[1] ? v[1] : v[0];
    return v[2] < a ? a : (v[2] > b ? b : v[2]);
  };
  return medirDesvioLateral({0, mediana(esquerda), mediana(direita)}, configRealinhamento.limiarParedeLateralMm,
                            configRealinhamento.distanciaSensorLateralAoCentroMm);
}

void imprimirDesvio(const DesvioLateral& d) {
  Serial.printf("{\"tipo\":\"desvio\",\"referencia\":\"%s\",\"desvio_mm\":%.1f}\n",
                nomeDaReferenciaLateral(d.referencia), d.desvioUm / 1000.0);
}

/// Realinha ao centro do corredor se o desvio medido passa do limiar, como o ciclo de navegação faz
/// antes de girar (até `tentativas` vezes).
/// @return Quantos realinhamentos foram feitos.
uint32_t realinharSePreciso(bool recuar, bool livre) {
  uint32_t feitos = 0;
  for (uint8_t i = 0; i < configRealinhamento.tentativas; i++) {
    const DesvioLateral d = medirDesvio();
    const int32_t modulo = d.desvioUm < 0 ? -d.desvioUm : d.desvioUm;
    if (d.referencia == ReferenciaLateral::SemParede ||
        modulo < static_cast<int32_t>(configRealinhamento.desvioMinimoMm) * 1000 ||
        modulo > static_cast<int32_t>(configRealinhamento.desvioMaximoMm) * 1000) {
      break;
    }
    const PlanoRealinhamento plano = planejarRealinhamento(d.desvioUm, recuar, livre, configRealinhamento.trechoMaximoLivreMm,
                                                           configRealinhamento.folgaMinimaMm);
    if (plano.trechoUm == 0 || !executarMovimento(Movimento::Realinhar, &plano)) {
      break;
    }
    feitos++;
  }
  return feitos;
}

/// Executa um percurso de movimentos (A, D, E, V), realinhando antes de cada giro, e informa a
/// célula estimada no fim, para comparar com a real (critério 2 da #188).
void executarPercurso(const String& sequencia) {
  const unsigned total = sequencia.length() < MAX_MOVIMENTOS_PERCURSO ? sequencia.length() : MAX_MOVIMENTOS_PERCURSO;
  unsigned feitos = 0;
  uint32_t realinhamentos = 0;
  bool ok = true;
  for (unsigned i = 0; i < total && ok; i++) {
    const char c = sequencia[i];
    if (c == 'A') {
      ok = executarMovimento(Movimento::AvancarCelula);
    } else if (c == 'D' || c == 'E' || c == 'V') {
      // Atrás está a célula de onde o robô veio; no primeiro movimento, a parede da largada.
      realinhamentos += realinharSePreciso(true, i > 0);
      ok = executarMovimento(c == 'D' ? Movimento::Girar90Direita
                                      : (c == 'E' ? Movimento::Girar90Esquerda : Movimento::Girar180));
    } else {
      continue;  // espaços e letras desconhecidas são ignorados
    }
    if (ok) {
      feitos++;
    }
  }
  const Pose pose = odometria.pose();
  Serial.printf("{\"tipo\":\"fim_percurso\",\"movimentos\":%u,\"feitos\":%u,\"realinhamentos\":%lu,"
                "\"linha\":%u,\"coluna\":%u,\"orientacao\":\"%c\",\"falha\":\"%s\"}\n",
                total, feitos, static_cast<unsigned long>(realinhamentos), pose.celula.linha, pose.celula.coluna,
                letraDaDirecao(pose.direcao), ok ? "nenhuma" : nomeDaFalhaExecucao(executor.falha()));
}

/// Separa "COMANDO a b" em até dois números inteiros.
void lerArgumentos(const String& resto, long& a, long& b) {
  const int espaco = resto.indexOf(' ');
  a = resto.toInt();
  b = espaco < 0 ? 0 : resto.substring(espaco + 1).toInt();
}

long limitar(long valor, long minimo, long maximo) {
  return valor < minimo ? minimo : (valor > maximo ? maximo : valor);
}

void executar(String linha) {
  linha.trim();
  linha.toUpperCase();
  if (linha.length() == 0) {
    return;
  }
  const int espaco = linha.indexOf(' ');
  const String comando = espaco < 0 ? linha : linha.substring(0, espaco);
  String resto = espaco < 0 ? String() : linha.substring(espaco + 1);
  resto.trim();
  long a = 0;
  long b = 0;
  lerArgumentos(resto, a, b);

  if (comando == "INFO") {
    imprimirInfo();
  } else if (comando == "VEL") {
    const int16_t potencia = static_cast<int16_t>(limitar(a, -POTENCIA_MAXIMA, POTENCIA_MAXIMA));
    Serial.printf("{\"tipo\":\"vel\",\"potencia\":%d,\"mm_por_s\":%ld}\n", potencia,
                  static_cast<long>(deslocamento.estimador().velocidadeMmPorS(potencia)));
  } else if (comando == "ZERAR") {
    deslocamento.motores().parar();
    deslocamento.reiniciar(odometria, {{0, 0}, Direcao::Norte});
    correcaoParedes.reiniciar();
    direcaoAtual = Direcao::Norte;
    executor.definirRumoAlvo(giro.ultima().anguloMiligraus);
    imprimirEstado("estado", 0);
  } else if (comando == "CORRECAO") {
    correcaoLigada = resto != "OFF";
    Serial.printf("{\"tipo\":\"correcao_ligada\",\"valor\":%s}\n", correcaoLigada ? "true" : "false");
  } else if (comando == "ANDAR" || comando == "GIRAR") {
    const int16_t potencia = static_cast<int16_t>(limitar(a, -POTENCIA_MAXIMA, POTENCIA_MAXIMA));
    const long duracao = limitar(b, 0, MAX_DURACAO_MS);
    const PotenciaRodas rodas = comando == "ANDAR" ? PotenciaRodas{potencia, potencia}
                                                   : PotenciaRodas{potencia, static_cast<int16_t>(-potencia)};
    mover(rodas, duracao, comando == "ANDAR" ? "andar" : "girar");
  } else if (comando == "CELULA") {
    const long celulas = limitar(a == 0 ? 1 : a, 1, MAX_CELULAS);
    long feitas = 0;
    while (feitas < celulas && executarMovimento(Movimento::AvancarCelula)) {
      feitas++;
    }
    Serial.printf("{\"tipo\":\"fim_celulas\",\"pedidas\":%ld,\"feitas\":%ld}\n", celulas, feitas);
  } else if (comando == "GIRO90E" || comando == "GIRO90D" || comando == "GIRO180") {
    const Movimento movimento = comando == "GIRO90E"   ? Movimento::Girar90Esquerda
                                : comando == "GIRO90D" ? Movimento::Girar90Direita
                                                       : Movimento::Girar180;
    executarMovimento(movimento);
  } else if (comando == "CALIBRAR") {
    calibrarGiro();
  } else if (comando == "DESVIO") {
    imprimirDesvio(medirDesvio());
  } else if (comando == "REALINHAR") {
    const DesvioLateral antes = medirDesvio();
    imprimirDesvio(antes);
    const PlanoRealinhamento plano = planejarRealinhamento(antes.desvioUm, resto != "F", true,
                                                           configRealinhamento.trechoMaximoLivreMm,
                                                           configRealinhamento.folgaMinimaMm);
    if (antes.referencia == ReferenciaLateral::SemParede || plano.trechoUm == 0) {
      Serial.println("{\"tipo\":\"erro\",\"mensagem\":\"sem parede lateral ou sem desvio para realinhar\"}");
    } else {
      executarMovimento(Movimento::Realinhar, &plano);
      imprimirDesvio(medirDesvio());
    }
  } else if (comando == "PERCURSO") {
    executarPercurso(resto);
  } else if (comando == "LER") {
    lerTofNovo();
    imprimirEstado("estado", 0);
  } else if (comando == "AJUDA" || comando == "?") {
    imprimirAjuda();
  } else {
    Serial.printf("{\"tipo\":\"erro\",\"mensagem\":\"comando desconhecido: %s\"}\n", comando.c_str());
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);
  motoresDrv.iniciar();  // começa freado
  deslocamento.motores().parar();
  barramento.iniciar(pinos::I2C_SDA, pinos::I2C_SCL);  // inicia o `Wire`
  const bool tofOk = tof.iniciar();
  imuOk = imu.iniciar();
  delay(500);  // tempo para o monitor serial abrir a porta USB
  imprimirAjuda();
  if (!tofOk) {
    Serial.println("# ATENCAO: algum ToF nao iniciou; as correcoes nao vao acontecer.");
  }
  if (!imuOk) {
    Serial.println("# ATENCAO: o MPU6050 nao respondeu; os comandos GIRO* nao vao funcionar.");
  }
  calibrarGiro();  // robô parado na mesa
  deslocamento.reiniciar(odometria, {{0, 0}, Direcao::Norte});
  Serial.println("{\"tipo\":\"pronto\",\"programa\":\"bancada_deslocamento\"}");
}

void loop() {
  static String linha;
  acompanharGiro();
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      executar(linha);
      linha = "";
    } else if (linha.length() < 64) {
      linha += c;
    }
  }
  delay(1);
}
