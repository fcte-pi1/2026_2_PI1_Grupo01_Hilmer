#pragma once

/// @file ConsoleRemoto.h
/// @brief Console do robô: a serial USB e, com o Wi-Fi conectado, um servidor telnet (porta 23) que
/// espelha o log e aceita os mesmos comandos. É como o computador acompanha e ativa o robô à
/// distância, sem cabo, enquanto ele anda e a telemetria segue para o banco.
///
/// **Nunca faz a navegação esperar a rede.** As tarefas do robô escrevem com `println`/`printf`, que
/// mandam o texto direto para a serial e o guardam numa fila na memória (`FilaBytes`), sem tocar no
/// Wi-Fi. Quem fala com o cliente telnet é só a tarefa do console, em `ciclo()`, que esvazia a fila
/// pela rede. Se o Wi-Fi ou o cliente travarem, a fila enche e o texto novo é descartado (e contado),
/// mas o robô não espera. A fila só recebe texto com um cliente conectado.
///
/// Um cliente telnet por vez: um novo derruba o anterior. A entrada (os comandos) vem da serial e do
/// telnet pela mesma `ler()`.

#include <Arduino.h>
#include <WiFi.h>
#include <stdarg.h>

#include "ContratoTelemetria.h"
#include "FilaBytes.h"
#include "FilaLinhas.h"

namespace micromouse {

class ConsoleRemoto {
 public:
  static constexpr uint16_t PORTA_TELNET = 23;

  /// Maior linha de `printf`, em bytes, com o `\0`. Passando disso, a linha sai cortada.
  static constexpr size_t TAMANHO_MAX_LINHA = 384;

  /// Liga a serial. Chame uma vez, no começo do `setup()`.
  void iniciar(unsigned long baud) { Serial.begin(baud); }

  /// Escreve `tamanho` bytes na serial e na fila do telnet (se houver cliente). Pode ser chamada de
  /// qualquer tarefa; não espera a rede.
  void escrever(const char* texto, size_t tamanho);

  /// Como `Serial.println(texto)`: o texto e `\r\n`.
  void println(const char* texto);
  void println() { escrever("\r\n", 2); }

  /// Como `Serial.printf`. Linhas maiores que `TAMANHO_MAX_LINHA` saem cortadas.
  void printf(const char* formato, ...) __attribute__((format(printf, 2, 3)));

  /// Próximo byte de comando: primeiro da serial, depois do telnet. -1 se não há nenhum. Qualquer tarefa.
  int ler();

  /// Trabalho do telnet: aceita cliente, lê o que ele digitou e esvazia a fila de saída pela rede.
  /// Chame de uma tarefa só (a do console), a cada poucas dezenas de ms. Pode esperar a rede.
  void ciclo();

  /// `true` com um cliente telnet conectado.
  bool clienteConectado() const { return conectado_; }

  /// Textos que não couberam na fila de saída (cliente lento ou parado) desde o boot.
  uint32_t descartados();

  /// Tira a próxima linha de evento (`{"status":"..."}`) que o robô registrou para o painel. Qualquer
  /// `println` ou `printf` com uma linha de evento a põe na fila, com ou sem cliente telnet; quem as
  /// envia ao banco é a tarefa de rede, em lotes.
  /// @return O tamanho da linha (sem o `\0`), ou 0 se não há nenhuma.
  size_t tirarLinhaParaOPainel(char* saida, size_t capacidade);

  /// Linhas de evento que não couberam na fila do painel desde o boot.
  uint32_t linhasDoPainelDescartadas();

 private:
  static constexpr size_t TAMANHO_FILA_SAIDA = 2048;
  static constexpr size_t TAMANHO_FILA_ENTRADA = 64;
  static constexpr size_t TAMANHO_FILA_PAINEL = 24;  ///< Linhas de evento à espera do envio ao banco.

  /// Guarda a linha para o painel se ela for um evento.
  void guardarParaOPainel(const char* linha);

  /// Fecha o cliente e esvazia a fila de saída.
  void largarCliente();

  portMUX_TYPE trava_ = portMUX_INITIALIZER_UNLOCKED;  ///< Protege as duas filas (tarefas e console).
  FilaBytes<TAMANHO_FILA_SAIDA> saida_;
  FilaBytes<TAMANHO_FILA_ENTRADA> entrada_;
  FilaLinhas<TAMANHO_FILA_PAINEL, TAMANHO_MAX_LINHA_DE_LOG> painel_;
  FiltroTelnet filtro_;                    ///< Só a tarefa do console usa.
  WiFiServer servidor_{PORTA_TELNET};      ///< Só a tarefa do console usa.
  WiFiClient cliente_;                     ///< Só a tarefa do console usa.
  bool servidorAtivo_ = false;
  volatile bool conectado_ = false;        ///< A tarefa do console escreve; as outras leem.
};

}  // namespace micromouse
