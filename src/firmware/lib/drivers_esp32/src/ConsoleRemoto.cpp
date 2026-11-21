#include "ConsoleRemoto.h"

#include <stdio.h>
#include <string.h>

namespace micromouse {

namespace {

constexpr const char* BANNER =
    "\r\n[robo] telnet conectado. L larga, X para na hora, Z zera e volta a esperar a largada, B repete o boot, "
    "E mostra o estado, M mostra o mapa que o robo montou.\r\n";

}  // namespace

void ConsoleRemoto::escrever(const char* texto, size_t tamanho) {
  if (texto == nullptr || tamanho == 0) {
    return;
  }
  Serial.write(reinterpret_cast<const uint8_t*>(texto), tamanho);
  if (!conectado_) {
    return;
  }
  portENTER_CRITICAL(&trava_);
  saida_.empilhar(reinterpret_cast<const uint8_t*>(texto), tamanho);
  portEXIT_CRITICAL(&trava_);
}

void ConsoleRemoto::guardarParaOPainel(const char* linha) {
  if (!ehLinhaDeEventoParaOPainel(linha)) {
    return;
  }
  portENTER_CRITICAL(&trava_);
  painel_.empilhar(linha);
  portEXIT_CRITICAL(&trava_);
}

size_t ConsoleRemoto::tirarLinhaParaOPainel(char* saida, size_t capacidade) {
  portENTER_CRITICAL(&trava_);
  const size_t n = painel_.desempilhar(saida, capacidade);
  portEXIT_CRITICAL(&trava_);
  return n;
}

uint32_t ConsoleRemoto::linhasDoPainelDescartadas() {
  portENTER_CRITICAL(&trava_);
  const uint32_t n = painel_.descartadas();
  portEXIT_CRITICAL(&trava_);
  return n;
}

void ConsoleRemoto::println(const char* texto) {
  if (texto != nullptr) {
    escrever(texto, strlen(texto));
    guardarParaOPainel(texto);
  }
  escrever("\r\n", 2);
}

void ConsoleRemoto::printf(const char* formato, ...) {
  char linha[TAMANHO_MAX_LINHA];
  va_list argumentos;
  va_start(argumentos, formato);
  const int n = vsnprintf(linha, sizeof(linha), formato, argumentos);
  va_end(argumentos);
  if (n <= 0) {
    return;
  }
  escrever(linha, static_cast<size_t>(n) < sizeof(linha) ? static_cast<size_t>(n) : sizeof(linha) - 1);
  guardarParaOPainel(linha);  // `linha` termina em `\0` (o `vsnprintf` garante); o `\n` do fim sai na fila
}

int ConsoleRemoto::ler() {
  if (Serial.available() > 0) {
    return Serial.read();
  }
  portENTER_CRITICAL(&trava_);
  const int c = entrada_.ler();
  portEXIT_CRITICAL(&trava_);
  return c;
}

uint32_t ConsoleRemoto::descartados() {
  portENTER_CRITICAL(&trava_);
  const uint32_t n = saida_.descartados();
  portEXIT_CRITICAL(&trava_);
  return n;
}

void ConsoleRemoto::largarCliente() {
  conectado_ = false;
  if (cliente_) {
    cliente_.stop();
  }
  portENTER_CRITICAL(&trava_);
  saida_.limpar();
  portEXIT_CRITICAL(&trava_);
}

void ConsoleRemoto::ciclo() {
  if (WiFi.status() != WL_CONNECTED) {
    if (conectado_ || cliente_) {
      largarCliente();  // sem Wi-Fi não há cliente; a conexão antiga morreu junto
    }
    return;
  }
  if (!servidorAtivo_) {
    servidor_.begin();
    servidor_.setNoDelay(true);
    servidorAtivo_ = true;
  }

  if (servidor_.hasClient()) {
    WiFiClient novo = servidor_.accept();
    largarCliente();
    cliente_ = novo;
    cliente_.setNoDelay(true);
    filtro_.reiniciar();
    cliente_.print(BANNER);
    conectado_ = true;
  }
  if (cliente_ && !cliente_.connected()) {
    largarCliente();
  }
  if (!conectado_) {
    return;
  }

  // Entrada: o que o cliente digitou, sem as negociações do telnet.
  while (cliente_.available() > 0) {
    const int c = cliente_.read();
    if (c < 0) {
      break;
    }
    const int texto = filtro_.filtrar(static_cast<uint8_t>(c));
    if (texto >= 0) {
      const uint8_t byte = static_cast<uint8_t>(texto);
      portENTER_CRITICAL(&trava_);
      entrada_.empilhar(&byte, 1);
      portEXIT_CRITICAL(&trava_);
    }
  }

  // Saída: esvazia a fila em blocos. Só esta tarefa espera a rede.
  uint8_t bloco[128];
  for (;;) {
    portENTER_CRITICAL(&trava_);
    const size_t n = saida_.desempilhar(bloco, sizeof(bloco));
    portEXIT_CRITICAL(&trava_);
    if (n == 0) {
      break;
    }
    if (cliente_.write(bloco, n) == 0) {
      largarCliente();  // cliente travado: larga, para o robô não ficar com a fila cheia à toa
      break;
    }
  }
}

}  // namespace micromouse
