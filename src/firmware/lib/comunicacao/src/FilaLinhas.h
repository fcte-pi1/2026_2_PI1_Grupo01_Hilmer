#pragma once

/// @file FilaLinhas.h
/// @brief Fila circular de linhas de texto de tamanho fixo, e a regra de quais linhas do log do robô
/// vão para o painel. Sem Arduino e sem alocação. Não é segura entre tarefas: quem a usa protege cada
/// chamada com uma seção crítica.

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace micromouse {

/// `true` se a linha do log é um evento para o painel: um `{"status":"..."}`, menos os contadores do
/// envio (`"status":"envio"`, a cada 10 s) e o relatório da corrida, que o painel já recebe pela
/// telemetria. A telemetria (1 por segundo) e o relatório do boot não começam com `{"status"`.
inline bool ehLinhaDeEventoParaOPainel(const char* linha) {
  static constexpr const char* PREFIXO = "{\"status\":\"";
  static constexpr const char* ENVIO = "{\"status\":\"envio\"";
  if (linha == nullptr || strncmp(linha, PREFIXO, strlen(PREFIXO)) != 0) {
    return false;
  }
  // `envio` termina em aspas: `envio_pausado` é um evento e passa.
  return strncmp(linha, ENVIO, strlen(ENVIO)) != 0;
}

/// Fila de até `LINHAS` linhas de até `TAMANHO - 1` caracteres. Quem escreve (as tarefas do robô, que
/// não podem esperar a rede) nunca bloqueia: com a fila cheia, a linha nova é descartada e contada.
template <size_t LINHAS, size_t TAMANHO>
class FilaLinhas {
 public:
  /// Guarda a linha sem o `\r` e o `\n` do fim. Linhas vazias ou que não cabem (com o `\0`) são
  /// descartadas e contadas, como as que chegam com a fila cheia.
  /// @return `true` se guardou.
  bool empilhar(const char* linha) {
    size_t n = linha == nullptr ? 0 : strlen(linha);
    while (n > 0 && (linha[n - 1] == '\n' || linha[n - 1] == '\r')) {
      --n;
    }
    if (n == 0 || n >= TAMANHO || quantidade_ == LINHAS) {
      ++descartadas_;
      return false;
    }
    char* destino = linhas_[(inicio_ + quantidade_) % LINHAS];
    memcpy(destino, linha, n);
    destino[n] = '\0';
    ++quantidade_;
    return true;
  }

  /// Tira a linha mais antiga e a copia para `saida`.
  /// @return O tamanho da linha (sem o `\0`), ou 0 se a fila estava vazia ou `capacidade` é pequena
  ///         (a linha fica na fila).
  size_t desempilhar(char* saida, size_t capacidade) {
    if (quantidade_ == 0) {
      return 0;
    }
    const size_t n = strlen(linhas_[inicio_]);
    if (capacidade <= n) {
      return 0;
    }
    memcpy(saida, linhas_[inicio_], n + 1);
    inicio_ = (inicio_ + 1) % LINHAS;
    --quantidade_;
    return n;
  }

  size_t tamanho() const { return quantidade_; }
  bool vazia() const { return quantidade_ == 0; }

  /// Linhas descartadas (vazias, grandes demais ou com a fila cheia) desde o começo.
  uint32_t descartadas() const { return descartadas_; }

 private:
  char linhas_[LINHAS][TAMANHO] = {};
  size_t inicio_ = 0;
  size_t quantidade_ = 0;
  uint32_t descartadas_ = 0;
};

}  // namespace micromouse
