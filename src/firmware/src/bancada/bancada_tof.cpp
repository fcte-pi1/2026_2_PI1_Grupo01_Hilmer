/// @file bancada_tof.cpp
/// @brief Firmware de bancada dos 3 VL53L0X (#344, #175): responde comandos pela serial para
/// conferir a fiação, a inicialização e as leituras do `TofVl53l0x` no hardware montado.
///
/// Não é o firmware do robô: só entra no ambiente `bancada_tof` do `platformio.ini`. Os motores
/// ficam freados o tempo todo. O menu `scripts/bancada_tof.py` grava este programa e conduz o
/// roteiro; também dá para usar direto pelo `pio device monitor -e bancada_tof` (comando `AJUDA`).
///
/// Protocolo: um comando por linha; cada resposta é uma linha JSON com o campo `tipo`. Linhas que
/// começam com `#` são texto para quem lê o monitor e podem ser ignoradas por programas.

#include <Arduino.h>
#include <Wire.h>

#include "MotoresDrv8833.h"
#include "Pinos.h"
#include "TofVl53l0x.h"

using namespace micromouse;

namespace {

/// Intervalo entre leituras nos comandos `FLUXO` e `ESTAT`: o mesmo da tarefa de navegação.
constexpr uint32_t PERIODO_LEITURA_MS = 10;

/// Intervalo entre linhas impressas no comando `FLUXO`.
constexpr uint32_t PERIODO_IMPRESSAO_MS = 100;

/// Maior duração aceita em `FLUXO` e `ESTAT`, em segundos.
constexpr long MAX_SEGUNDOS = 120;

constexpr Lado LADOS[3] = {Lado::Frente, Lado::Esquerda, Lado::Direita};
constexpr const char* NOMES[3] = {"frente", "esquerda", "direita"};
constexpr uint8_t XSHUTS[3] = {pinos::XSHUT_FRONTAL, pinos::XSHUT_ESQ, pinos::XSHUT_DIR};
constexpr uint8_t ENDERECOS[3] = {pinos::ENDERECO_TOF_FRONTAL, pinos::ENDERECO_TOF_ESQ,
                                  pinos::ENDERECO_TOF_DIR};

const ConfiguracaoTof configuracao;

TofVl53l0x tof(Wire, {XSHUTS[0], ENDERECOS[0]}, {XSHUTS[1], ENDERECOS[1]},
               {XSHUTS[2], ENDERECOS[2]}, configuracao);

/// Só para manter o DRV8833 freado: `iniciar()` deixa os motores parados.
MotoresDrv8833 motores(pinos::MOTOR_ESQ_IN1, pinos::MOTOR_ESQ_IN2, pinos::MOTOR_DIR_IN3,
                       pinos::MOTOR_DIR_IN4, false, true);

const char* nomeEstado(EstadoAmostraTof estado) {
  switch (estado) {
    case EstadoAmostraTof::SemAmostra:
      return "sem_amostra";
    case EstadoAmostraTof::Valida:
      return "valida";
    case EstadoAmostraTof::AbaixoDoMinimo:
      return "abaixo_do_minimo";
    case EstadoAmostraTof::ForaDeAlcance:
      return "fora_de_alcance";
    case EstadoAmostraTof::Falha:
      return "falha";
  }
  return "?";
}

/// `true` se chegou algum byte pela serial: usado para interromper `FLUXO` e `ESTAT`.
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
  Serial.println("# Bancada dos VL53L0X (#344). Comandos (um por linha):");
  Serial.println("#   INFO           configuracao, pinos e enderecos");
  Serial.println("#   SCAN           varredura do barramento I2C");
  Serial.println("#   ISOLAR F|E|D|N liga so o sensor frente/esquerda/direita (N: nenhum) e varre");
  Serial.println("#   INIT           reinicia os 3 sensores e atribui 0x30, 0x31 e 0x32");
  Serial.println("#   LER            uma leitura, com a amostra bruta de cada sensor");
  Serial.println("#   FLUXO s        leituras a cada 100 ms por s segundos (qualquer tecla para)");
  Serial.println("#   ESTAT s        estatisticas de s segundos: taxa, media, desvio, tempo da leitura");
  Serial.println("#   AJUDA          esta lista");
}

void imprimirInfo() {
  Serial.printf(
      "{\"tipo\":\"info\",\"programa\":\"bancada_tof\",\"sda\":%d,\"scl\":%d,"
      "\"xshut\":{\"frente\":%u,\"esquerda\":%u,\"direita\":%u},"
      "\"enderecos\":{\"frente\":%u,\"esquerda\":%u,\"direita\":%u},"
      "\"tempo_medicao_us\":%lu,\"distancia_minima_mm\":%u,\"distancia_maxima_mm\":%u,"
      "\"idade_maxima_ms\":%lu,\"periodo_leitura_ms\":%lu}\n",
      pinos::I2C_SDA, pinos::I2C_SCL, XSHUTS[0], XSHUTS[1], XSHUTS[2], ENDERECOS[0], ENDERECOS[1],
      ENDERECOS[2], static_cast<unsigned long>(configuracao.tempoMedicaoUs),
      configuracao.distanciaMinimaMm, configuracao.distanciaMaximaMm,
      static_cast<unsigned long>(configuracao.idadeMaximaLeituraMs),
      static_cast<unsigned long>(PERIODO_LEITURA_MS));
}

/// Varre os endereços de 7 bits e imprime os que responderam.
/// @param contexto Texto livre devolvido no campo `contexto` (por exemplo, o sensor isolado).
void varrer(const char* contexto) {
  Serial.printf("{\"tipo\":\"varredura\",\"contexto\":\"%s\",\"enderecos\":[", contexto);
  bool primeiro = true;
  for (uint8_t endereco = 0x08; endereco < 0x78; endereco++) {
    Wire.beginTransmission(endereco);
    if (Wire.endTransmission() == 0) {
      Serial.printf(primeiro ? "%u" : ",%u", endereco);
      primeiro = false;
    }
  }
  Serial.println("]}");
}

/// Põe todos os sensores em reset e solta só o do `indice` (3 = nenhum), sem passar pelo driver.
/// Serve para conferir a fiação de cada XSHUT: o sensor solto aparece sozinho no 0x29.
/// Depois disso, os sensores precisam de `INIT` de novo.
void isolar(uint8_t indice) {
  for (uint8_t i = 0; i < 3; i++) {
    pinMode(XSHUTS[i], OUTPUT);
    digitalWrite(XSHUTS[i], LOW);
  }
  delay(configuracao.esperaPartidaMs);
  if (indice < 3) {
    digitalWrite(XSHUTS[indice], HIGH);
    delay(configuracao.esperaPartidaMs);
  }
  varrer(indice < 3 ? NOMES[indice] : "nenhum");
}

void iniciarSensores() {
  const uint32_t inicioMs = millis();
  const bool ok = tof.iniciar();
  const uint32_t duracaoMs = millis() - inicioMs;
  Serial.printf("{\"tipo\":\"inicio\",\"ok\":%s,\"frente\":%s,\"esquerda\":%s,\"direita\":%s,"
                "\"duracao_ms\":%lu}\n",
                ok ? "true" : "false", tof.sensorIniciado(Lado::Frente) ? "true" : "false",
                tof.sensorIniciado(Lado::Esquerda) ? "true" : "false",
                tof.sensorIniciado(Lado::Direita) ? "true" : "false",
                static_cast<unsigned long>(duracaoMs));
}

/// Faz uma leitura e imprime o resultado e a amostra bruta de cada sensor.
void imprimirLeitura() {
  DistanciasLaterais distancias = {0, 0, 0};
  const uint32_t inicioUs = micros();
  const bool valida = tof.lerDistanciasLaterais(distancias);
  const uint32_t duracaoUs = micros() - inicioUs;
  const uint32_t idade = tof.idadeUltimaLeituraMs();
  const uint32_t agoraMs = millis();

  Serial.printf("{\"tipo\":\"leitura\",\"t_ms\":%lu,\"valida\":%s,\"idade_ms\":%ld,"
                "\"tempo_leitura_us\":%lu",
                static_cast<unsigned long>(agoraMs), valida ? "true" : "false",
                idade == CombinadorTof::IDADE_DESCONHECIDA ? -1L : static_cast<long>(idade),
                static_cast<unsigned long>(duracaoUs));
  // O que a interface entrega à percepção, já limitado à faixa (só vale com "valida": true).
  Serial.printf(",\"limitada\":{\"frente\":%u,\"esquerda\":%u,\"direita\":%u}", distancias.frenteMm,
                distancias.esquerdaMm, distancias.direitaMm);
  for (uint8_t i = 0; i < 3; i++) {
    const AmostraTof& amostra = tof.amostra(LADOS[i]);
    Serial.printf(",\"%s\":{\"mm\":%u,\"estado\":\"%s\",\"idade_ms\":%lu}", NOMES[i],
                  amostra.distanciaMm, nomeEstado(amostra.estado),
                  static_cast<unsigned long>(agoraMs - amostra.instanteMs));
  }
  Serial.println("}");
}

void fluxo(long segundos) {
  const uint32_t inicioMs = millis();
  uint32_t ultimaImpressaoMs = 0;
  while (millis() - inicioMs < static_cast<uint32_t>(segundos) * 1000UL && !interrompido()) {
    if (millis() - ultimaImpressaoMs >= PERIODO_IMPRESSAO_MS) {
      ultimaImpressaoMs = millis();
      imprimirLeitura();
    } else {
      DistanciasLaterais descartada;
      tof.lerDistanciasLaterais(descartada);
    }
    delay(PERIODO_LEITURA_MS);
  }
  Serial.println("{\"tipo\":\"fim_fluxo\"}");
}

/// Acumulado das amostras novas de um sensor durante o `ESTAT`.
struct EstatisticaSensor {
  uint32_t amostras = 0;
  uint32_t contagemPorEstado[5] = {0, 0, 0, 0, 0};
  uint32_t ultimoInstanteMs = 0;
  bool temUltimo = false;
  double soma = 0;
  double somaQuadrados = 0;
  uint16_t minimo = UINT16_MAX;
  uint16_t maximo = 0;
};

void estatisticas(long segundos) {
  EstatisticaSensor sensores[3];
  uint32_t leituras = 0;
  uint32_t leiturasValidas = 0;
  uint64_t somaTempoUs = 0;
  uint32_t maiorTempoUs = 0;

  const uint32_t inicioMs = millis();
  const uint32_t duracaoMs = static_cast<uint32_t>(segundos) * 1000UL;
  bool parou = false;
  while (millis() - inicioMs < duracaoMs) {
    if (interrompido()) {
      parou = true;
      break;
    }
    DistanciasLaterais distancias;
    const uint32_t antesUs = micros();
    const bool valida = tof.lerDistanciasLaterais(distancias);
    const uint32_t tempoUs = micros() - antesUs;
    leituras++;
    leiturasValidas += valida ? 1 : 0;
    somaTempoUs += tempoUs;
    maiorTempoUs = tempoUs > maiorTempoUs ? tempoUs : maiorTempoUs;

    for (uint8_t i = 0; i < 3; i++) {
      const AmostraTof& amostra = tof.amostra(LADOS[i]);
      EstatisticaSensor& e = sensores[i];
      // Amostra nova = instante diferente do último visto (o sensor mede a cada ~33 ms).
      if (amostra.estado == EstadoAmostraTof::SemAmostra ||
          (e.temUltimo && amostra.instanteMs == e.ultimoInstanteMs)) {
        continue;
      }
      e.temUltimo = true;
      e.ultimoInstanteMs = amostra.instanteMs;
      e.amostras++;
      e.contagemPorEstado[static_cast<uint8_t>(amostra.estado)]++;
      if (amostra.estado == EstadoAmostraTof::Valida) {
        e.soma += amostra.distanciaMm;
        e.somaQuadrados += static_cast<double>(amostra.distanciaMm) * amostra.distanciaMm;
        e.minimo = amostra.distanciaMm < e.minimo ? amostra.distanciaMm : e.minimo;
        e.maximo = amostra.distanciaMm > e.maximo ? amostra.distanciaMm : e.maximo;
      }
    }
    delay(PERIODO_LEITURA_MS);
  }

  const double segundosMedidos = (millis() - inicioMs) / 1000.0;
  Serial.printf("{\"tipo\":\"estatisticas\",\"interrompido\":%s,\"segundos\":%.2f,\"leituras\":%lu,"
                "\"leituras_validas\":%lu,\"tempo_leitura_us_medio\":%.0f,"
                "\"tempo_leitura_us_max\":%lu",
                parou ? "true" : "false", segundosMedidos, static_cast<unsigned long>(leituras),
                static_cast<unsigned long>(leiturasValidas),
                leituras > 0 ? static_cast<double>(somaTempoUs) / leituras : 0.0,
                static_cast<unsigned long>(maiorTempoUs));
  for (uint8_t i = 0; i < 3; i++) {
    const EstatisticaSensor& e = sensores[i];
    const uint32_t validas = e.contagemPorEstado[static_cast<uint8_t>(EstadoAmostraTof::Valida)];
    const double media = validas > 0 ? e.soma / validas : 0.0;
    const double variancia = validas > 1 ? (e.somaQuadrados - validas * media * media) / (validas - 1) : 0.0;
    Serial.printf(",\"%s\":{\"amostras\":%lu,\"hz\":%.1f,\"validas\":%lu,\"abaixo_do_minimo\":%lu,"
                  "\"fora_de_alcance\":%lu,\"falhas\":%lu,\"media_mm\":%.1f,\"desvio_mm\":%.1f,"
                  "\"min_mm\":%u,\"max_mm\":%u}",
                  NOMES[i], static_cast<unsigned long>(e.amostras),
                  segundosMedidos > 0 ? e.amostras / segundosMedidos : 0.0,
                  static_cast<unsigned long>(validas),
                  static_cast<unsigned long>(
                      e.contagemPorEstado[static_cast<uint8_t>(EstadoAmostraTof::AbaixoDoMinimo)]),
                  static_cast<unsigned long>(
                      e.contagemPorEstado[static_cast<uint8_t>(EstadoAmostraTof::ForaDeAlcance)]),
                  static_cast<unsigned long>(
                      e.contagemPorEstado[static_cast<uint8_t>(EstadoAmostraTof::Falha)]),
                  media, variancia > 0 ? sqrt(variancia) : 0.0, validas > 0 ? e.minimo : 0,
                  e.maximo);
  }
  Serial.println("}");
}

/// Lê o argumento numérico de `FLUXO` e `ESTAT`, limitado a 1..`MAX_SEGUNDOS` (padrão: 5).
long segundosDoArgumento(const String& argumento) {
  const long valor = argumento.length() > 0 ? argumento.toInt() : 5;
  if (valor < 1) {
    return 1;
  }
  return valor > MAX_SEGUNDOS ? MAX_SEGUNDOS : valor;
}

void executar(String linha) {
  linha.trim();
  linha.toUpperCase();
  if (linha.length() == 0) {
    return;
  }
  const int espaco = linha.indexOf(' ');
  const String comando = espaco < 0 ? linha : linha.substring(0, espaco);
  String argumento = espaco < 0 ? String() : linha.substring(espaco + 1);
  argumento.trim();

  if (comando == "INFO") {
    imprimirInfo();
  } else if (comando == "SCAN") {
    varrer("atual");
  } else if (comando == "ISOLAR") {
    const char lado = argumento.length() > 0 ? argumento[0] : 'N';
    isolar(lado == 'F' ? 0 : lado == 'E' ? 1 : lado == 'D' ? 2 : 3);
  } else if (comando == "INIT") {
    iniciarSensores();
  } else if (comando == "LER") {
    imprimirLeitura();
  } else if (comando == "FLUXO") {
    fluxo(segundosDoArgumento(argumento));
  } else if (comando == "ESTAT") {
    estatisticas(segundosDoArgumento(argumento));
  } else if (comando == "AJUDA" || comando == "?") {
    imprimirAjuda();
  } else {
    Serial.printf("{\"tipo\":\"erro\",\"mensagem\":\"comando desconhecido: %s\"}\n", comando.c_str());
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  motores.iniciar();  // freados durante toda a bancada
  Wire.begin(pinos::I2C_SDA, pinos::I2C_SCL);
  delay(500);  // tempo para o monitor serial, que reinicia a placa ao abrir, começar a ler
  imprimirAjuda();
  iniciarSensores();
  Serial.println("{\"tipo\":\"pronto\",\"programa\":\"bancada_tof\"}");
}

void loop() {
  static String linha;
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
