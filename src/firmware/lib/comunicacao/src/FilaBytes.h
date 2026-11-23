#pragma once

/// @file FilaBytes.h
/// @brief Fila circular de bytes de tamanho fixo, para o console remoto (telnet), sem Arduino e sem
/// alocação. Não é segura entre tarefas: quem a usa protege cada chamada com uma seção crítica.

#include <stddef.h>
#include <stdint.h>

namespace micromouse {

/// Fila de até `CAPACIDADE` bytes. Quem escreve (as tarefas do robô, que não podem esperar a rede)
/// nunca bloqueia: sem espaço para o texto inteiro, ele é descartado e contado.
template <size_t CAPACIDADE>
class FilaBytes {
 public:
  /// Guarda os `tamanho` bytes, ou nenhum: um texto cortado no meio embaralharia o log.
  /// @return `false` se não coube (o texto foi descartado e entrou em `descartados()`).
  bool empilhar(const uint8_t* dados, size_t tamanho) {
    if (tamanho > CAPACIDADE - tamanho_) {
      ++descartados_;
      return false;
    }
    for (size_t i = 0; i < tamanho; ++i) {
      bytes_[(inicio_ + tamanho_ + i) % CAPACIDADE] = dados[i];
    }
    tamanho_ += tamanho;
    return true;
  }

  /// Tira até `maximo` bytes do começo da fila.
  /// @return Quantos bytes foram escritos em `saida`.
  size_t desempilhar(uint8_t* saida, size_t maximo) {
    size_t n = 0;
    while (n < maximo && tamanho_ > 0) {
      saida[n++] = bytes_[inicio_];
      inicio_ = (inicio_ + 1) % CAPACIDADE;
      --tamanho_;
    }
    return n;
  }

  /// Tira um byte do começo da fila, ou -1 se estiver vazia.
  int ler() {
    uint8_t c = 0;
    return desempilhar(&c, 1) == 1 ? c : -1;
  }

  size_t tamanho() const { return tamanho_; }
  bool vazia() const { return tamanho_ == 0; }

  /// Textos descartados por falta de espaço desde o começo.
  uint32_t descartados() const { return descartados_; }

  /// Esvazia a fila. O contador de descartados continua.
  void limpar() {
    inicio_ = 0;
    tamanho_ = 0;
  }

 private:
  uint8_t bytes_[CAPACIDADE] = {};
  size_t inicio_ = 0;
  size_t tamanho_ = 0;
  uint32_t descartados_ = 0;
};

/// Tira do fluxo do telnet as negociações (IAC + comando + opção) que o cliente manda ao conectar e
/// deixa passar o texto digitado.
class FiltroTelnet {
 public:
  /// @return O byte, se ele é texto; -1 se é parte de uma negociação e deve ser ignorado.
  int filtrar(uint8_t c) {
    if (estado_ == Estado::Texto) {
      if (c == IAC) {
        estado_ = Estado::Comando;
        return -1;
      }
      return c;
    }
    if (estado_ == Estado::Comando) {
      // WILL, WONT, DO e DONT trazem mais um byte (a opção); os outros comandos acabam aqui. IAC IAC é
      // o byte 255 como texto.
      if (c == IAC) {
        estado_ = Estado::Texto;
        return c;
      }
      estado_ = (c >= WILL && c <= DONT) ? Estado::Opcao : Estado::Texto;
      return -1;
    }
    estado_ = Estado::Texto;
    return -1;
  }

  void reiniciar() { estado_ = Estado::Texto; }

 private:
  static constexpr uint8_t IAC = 255;
  static constexpr uint8_t WILL = 251;
  static constexpr uint8_t DONT = 254;
  enum class Estado : uint8_t { Texto, Comando, Opcao };
  Estado estado_ = Estado::Texto;
};

}  // namespace micromouse
