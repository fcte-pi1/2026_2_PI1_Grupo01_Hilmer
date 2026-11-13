/// @file ConsoleBancada.h
/// @brief Console da bancada geral: a serial USB e, quando há Wi-Fi, um servidor telnet (porta 23)
/// que espelha a saída e aceita as mesmas teclas, para usar a bancada sem o cabo.
///
/// Só a bancada usa. A saída vai para a serial e para o cliente telnet conectado; a entrada vem dos
/// dois. Só um cliente telnet por vez (um novo derruba o anterior).
#pragma once

#include <Arduino.h>

#if defined(BANCADA_COM_TELEMETRIA)
#include <WiFi.h>
#endif

namespace micromouse {

class ConsoleBancada : public Stream {
 public:
  static constexpr uint16_t PORTA_TELNET = 23;

  void iniciar(unsigned long baud) { Serial.begin(baud); }

  /// A UART do DevKit não informa se o monitor fechou; o console conta como sempre aberto.
  explicit operator bool() const { return true; }

#if defined(BANCADA_COM_TELEMETRIA)
  /// Abre o servidor telnet. Chamar depois que o Wi-Fi conectou; repetir é inofensivo.
  void iniciarTelnet() {
    if (!telnetAtivo_) {
      servidor_.begin();
      servidor_.setNoDelay(true);
      telnetAtivo_ = true;
    }
  }

  bool telnetAtivo() const { return telnetAtivo_; }
  bool clienteConectado() { return cliente_ && cliente_.connected(); }
#endif

  int available() override {
    atualizar();
    return Serial.available() + static_cast<int>(entradaTamanho_);
  }

  int read() override {
    atualizar();
    if (Serial.available() > 0) {
      return Serial.read();
    }
    if (entradaTamanho_ > 0) {
      const uint8_t c = entrada_[entradaInicio_];
      entradaInicio_ = (entradaInicio_ + 1) % TAMANHO_ENTRADA;
      --entradaTamanho_;
      return c;
    }
    return -1;
  }

  int peek() override {
    atualizar();
    if (Serial.available() > 0) {
      return Serial.peek();
    }
    return entradaTamanho_ > 0 ? entrada_[entradaInicio_] : -1;
  }

  size_t write(uint8_t c) override { return write(&c, 1); }

  size_t write(const uint8_t* dados, size_t tamanho) override {
    Serial.write(dados, tamanho);
#if defined(BANCADA_COM_TELEMETRIA)
    for (size_t i = 0; i < tamanho; ++i) {
      if (saidaTamanho_ == TAMANHO_SAIDA) {
        descarregar();
      }
      saida_[saidaTamanho_++] = dados[i];
    }
    // Uma linha por vez evita um pacote para cada pedaço de `printf`.
    if (saidaTamanho_ > 0 && saida_[saidaTamanho_ - 1] == '\n') {
      descarregar();
    }
#endif
    return tamanho;
  }

  void flush() override {
    Serial.flush();
#if defined(BANCADA_COM_TELEMETRIA)
    descarregar();
#endif
  }

  /// Aceita cliente novo, lê as teclas do telnet e envia a saída pendente. Barato; é chamado a cada
  /// `available()`, então qualquer laço de espera por tecla já mantém o telnet vivo.
  void atualizar() {
#if defined(BANCADA_COM_TELEMETRIA)
    if (!telnetAtivo_) {
      return;
    }
    if (servidor_.hasClient()) {
      WiFiClient novo = servidor_.accept();
      if (cliente_) {
        cliente_.stop();
      }
      cliente_ = novo;
      cliente_.setNoDelay(true);
      saidaTamanho_ = 0;
      entradaTamanho_ = 0;
      estadoIac_ = 0;
      cliente_.print("\r\n[bancada] conectado por telnet. Enter mostra o menu.\r\n");
    }
    if (cliente_ && !cliente_.connected()) {
      cliente_.stop();
    }
    while (cliente_ && cliente_.available() > 0 && entradaTamanho_ < TAMANHO_ENTRADA) {
      const int c = cliente_.read();
      if (c >= 0) {
        guardarDoTelnet(static_cast<uint8_t>(c));
      }
    }
    descarregar();
#endif
  }

 private:
  static constexpr size_t TAMANHO_ENTRADA = 64;
  uint8_t entrada_[TAMANHO_ENTRADA] = {};
  size_t entradaInicio_ = 0;
  size_t entradaTamanho_ = 0;

#if defined(BANCADA_COM_TELEMETRIA)
  static constexpr size_t TAMANHO_SAIDA = 512;

  /// Descarta as negociações do telnet (IAC + comando + opção) e guarda o resto.
  void guardarDoTelnet(uint8_t c) {
    if (estadoIac_ == 0 && c == 255) {
      estadoIac_ = 1;
      return;
    }
    if (estadoIac_ == 1) {
      estadoIac_ = (c >= 251 && c <= 254) ? 2 : 0;  // WILL/WONT/DO/DONT trazem mais um byte
      return;
    }
    if (estadoIac_ == 2) {
      estadoIac_ = 0;
      return;
    }
    entrada_[(entradaInicio_ + entradaTamanho_) % TAMANHO_ENTRADA] = c;
    ++entradaTamanho_;
  }

  void descarregar() {
    if (saidaTamanho_ == 0) {
      return;
    }
    if (cliente_ && cliente_.connected()) {
      if (cliente_.write(saida_, saidaTamanho_) == 0) {
        cliente_.stop();  // cliente travado: larga, para não prender a bancada
      }
    }
    saidaTamanho_ = 0;
  }

  WiFiServer servidor_{PORTA_TELNET};
  WiFiClient cliente_;
  bool telnetAtivo_ = false;
  uint8_t saida_[TAMANHO_SAIDA] = {};
  size_t saidaTamanho_ = 0;
  uint8_t estadoIac_ = 0;
#endif
};

}  // namespace micromouse
