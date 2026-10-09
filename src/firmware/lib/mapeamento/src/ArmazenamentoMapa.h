#pragma once

/// @file ArmazenamentoMapa.h
/// @brief Dono do mapa durante a corrida: mantém o `Mapa` vivo enquanto a navegação e a
/// telemetria são reiniciadas (HU-04).
///
/// O mapa não pertence à navegação nem à telemetria: as duas recebem uma referência a ele.
/// Assim, reiniciar o estado de navegação (por exemplo, após uma colisão) ou perder a conexão
/// não apaga o que o robô já explorou.
///
/// Não depende do Arduino e não aloca memória dinâmica. Quem cria o objeto escolhe onde ele fica
/// na memória: em `main.cpp`, uma variável estática, fora das tarefas do FreeRTOS.

#include "Mapa.h"
#include "Tipos.h"

namespace micromouse {

/// Guarda o único `Mapa` da corrida.
///
/// O mapa só é zerado por `iniciarCorrida()`. Nenhuma outra operação apaga paredes.
///
/// Uso esperado (a montagem real fica em `main.cpp`):
/// @code
/// static ArmazenamentoMapa armazenamento;            // fora das tarefas: sobrevive a elas
/// armazenamento.iniciarCorrida(DimensaoMapa::Labirinto12x4);
/// Mapa& paraNavegacao = armazenamento.mapa();        // lê e registra paredes
/// const Mapa& paraTelemetria = armazenamento.mapa(); // só lê
/// @endcode
class ArmazenamentoMapa {
 public:
  /// Cria o armazenamento sem corrida iniciada. Até `iniciarCorrida()`, `mapa()` devolve um
  /// mapa 4×4 vazio (o padrão de `Mapa`).
  ArmazenamentoMapa() = default;

  /// Mapa único.
  ArmazenamentoMapa(const ArmazenamentoMapa&) = delete;
  ArmazenamentoMapa& operator=(const ArmazenamentoMapa&) = delete;

  /// Começa uma corrida: troca o mapa por um novo, do tamanho informado, com o perímetro como
  /// `Parede` e o interior `Desconhecido`. É a única operação que apaga o mapa.
  /// @param dimensao Tamanho do labirinto da corrida.
  void iniciarCorrida(DimensaoMapa dimensao);

  /// `true` depois da primeira chamada de `iniciarCorrida()`.
  bool corridaIniciada() const { return corridaIniciada_; }

  /// Mapa da corrida, para quem lê e registra paredes (navegação).
  Mapa& mapa() { return mapa_; }

  /// Mapa da corrida, só para leitura (telemetria).
  const Mapa& mapa() const { return mapa_; }

 private:
  Mapa mapa_;
  bool corridaIniciada_ = false;
};

}  // namespace micromouse
