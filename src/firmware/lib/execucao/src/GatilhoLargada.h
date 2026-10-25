#pragma once

/// @file GatilhoLargada.h
/// @brief Gatilho da largada pelo gesto da mão na frente do ToF frontal, ou pelo comando serial
/// (#184).
///
/// A Super Mini não tem botão livre (o BOOT fica no GPIO9, o SDA do I²C) e o dashboard ainda não
/// manda comandos ao robô. O gesto é o truque clássico de micromouse: cobrir o sensor da frente e
/// tirar a mão.

#include <stdint.h>

#include "ConfiguracaoExecucao.h"
#include "Tipos.h"

namespace micromouse {

/// Etapa do gesto.
enum class EstadoGatilho : uint8_t {
  Desarmado,  ///< Ainda não viu a frente livre (por exemplo, há uma parede logo à frente).
  Armado,     ///< Frente livre: esperando a mão cobrir o sensor.
  Coberto,    ///< A mão cobre o sensor: contando o tempo mínimo.
  Contando,   ///< A mão saiu depois do tempo mínimo: esperando `esperaAposGestoMs` para largar.
  Disparado   ///< Largada liberada, até `reiniciar()`.
};

/// Reconhece o gesto: frente livre → coberta por pelo menos `tempoMinimoCobertoMs` → livre de
/// novo, e então a espera de `esperaAposGestoMs`.
///
/// Exigir a frente livre antes e depois do gesto evita uma largada falsa por uma parede parada na
/// frente: com uma parede ali, o gatilho nunca arma, e a largada fica só pelo comando serial
/// (`dispararManual()`). A mão de volta na frente durante a espera recomeça o gesto. Um vulto mais
/// curto que o tempo mínimo é ignorado.
///
/// Quem chama só alimenta o gatilho com o robô pronto (boot aprovado e giroscópio calibrado); antes
/// disso, o gesto não é observado. Sem Arduino e sem alocação: testado no ambiente `native`.
///
/// @code
/// GatilhoLargada gatilho;
/// // a cada 10 ms, com o robô pronto na partida:
/// if (gatilho.atualizar(leituraNova, 10)) { /* pode largar */ }
/// @endcode
class GatilhoLargada {
 public:
  explicit GatilhoLargada(const ConfiguracaoGatilhoLargada& config = ConfiguracaoGatilhoLargada{});

  /// Um ciclo do gesto.
  /// @param leituraNova Leitura nova do ToF (a partir da face dos sensores), ou `nullptr` se não
  ///        houver uma neste ciclo: vale a última frente vista.
  /// @param dtMs Tempo desde o ciclo anterior, em ms.
  /// @return `true` se a largada está liberada.
  bool atualizar(const DistanciasLaterais* leituraNova, uint32_t dtMs);

  /// Libera a largada sem o gesto (comando serial, na bancada).
  void dispararManual() { estado_ = EstadoGatilho::Disparado; }

  /// Volta ao começo do gesto (robô recolocado na partida à mão, comando `Z`).
  void reiniciar();

  bool disparado() const { return estado_ == EstadoGatilho::Disparado; }
  EstadoGatilho estado() const { return estado_; }

 private:
  ConfiguracaoGatilhoLargada config_;
  EstadoGatilho estado_ = EstadoGatilho::Desarmado;
  uint32_t tempoMs_ = 0;  ///< Tempo coberto (`Coberto`) ou de espera (`Contando`).
  uint16_t frenteMm_ = 0;
  bool temFrente_ = false;
};

/// Nome do estado para o log: `desarmado`, `armado`, `coberto`, `contando` ou `disparado`.
const char* nomeDoEstadoGatilho(EstadoGatilho estado);

}  // namespace micromouse
