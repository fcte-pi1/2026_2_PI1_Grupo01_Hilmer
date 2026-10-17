#pragma once

/// @file DecodificadorQuadratura.h
/// @brief Decodificação em quadratura (x4) dos canais A e B de um encoder.
///
/// Não depende do Arduino. A rotina de interrupção do driver apenas lê os dois pinos e chama
/// `processar`, então a lógica de contagem é testada no computador.

#include <stdint.h>

namespace micromouse {

/// Conta os pulsos de um encoder de quadratura a partir dos níveis dos canais A e B.
///
/// Cada mudança de nível de qualquer canal conta um pulso (x4): um ciclo completo dos dois
/// canais soma 4. Sequência que conta **+1** por passo (A adiantado em relação a B):
/// `00 → 10 → 11 → 01 → 00` (estado = `AB`). A sequência contrária conta **-1** por passo.
///
/// Se os dois canais mudarem entre duas leituras (um pulso perdido), a transição é inválida: a
/// contagem não muda e `transicoesInvalidas()` aumenta, o que ajuda a detectar rotina de
/// interrupção atrasada ou ruído no sinal.
class DecodificadorQuadratura {
 public:
  /// @param inverterSentido `true` para trocar o sinal da contagem. Usado na roda cujo motor fica
  ///        espelhado, para que as duas rodas contem positivo ao avançar.
  explicit DecodificadorQuadratura(bool inverterSentido = false);

  /// Define o estado inicial dos canais sem contar pulso. Chame na inicialização, com o nível real
  /// dos pinos, antes de habilitar as interrupções.
  void iniciar(bool canalA, bool canalB);

  /// Processa o novo nível dos canais.
  /// @return `+1` ou `-1` se contou um pulso (já com a inversão aplicada); `0` se nada mudou ou
  ///         se a transição foi inválida.
  int8_t processar(bool canalA, bool canalB);

  /// Pulsos acumulados.
  int32_t contagem() const { return contagem_; }

  /// Quantidade de transições inválidas (saltos de dois estados) desde a criação.
  uint32_t transicoesInvalidas() const { return invalidas_; }

  /// Zera a contagem. Não zera `transicoesInvalidas()` nem muda o estado dos canais.
  void zerar() { contagem_ = 0; }

 private:
  bool inverter_;
  uint8_t estado_ = 0;  ///< Último estado dos canais: bit 1 = A, bit 0 = B.
  int32_t contagem_ = 0;
  uint32_t invalidas_ = 0;
};

}  // namespace micromouse
