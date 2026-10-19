#pragma once

/// @file EncoderSimulado.h
/// @brief Implementação falsa de `IEncoder` para testes sem hardware.

#include <stdint.h>

#include "IEncoder.h"

namespace micromouse {

/// Encoder falso: a contagem é a que o teste definir, sem hardware.
///
/// Exemplo:
/// @code
/// EncoderSimulado esquerdo, direito;
/// esquerdo.adicionarPulsos(529);   // a roda esquerda avançou
/// direito.adicionarPulsos(-100);   // a direita recuou
/// IEncoder& roda = esquerdo;       // a lógica recebe só a interface
/// @endcode
class EncoderSimulado : public IEncoder {
 public:
  /// Soma pulsos à contagem (negativos fazem a roda "recuar").
  void adicionarPulsos(int32_t pulsos) { contagem_ += pulsos; }

  /// Define a contagem diretamente.
  void definirContagem(int32_t contagem) { contagem_ = contagem; }

  /// @copydoc IEncoder::lerContagem
  int32_t lerContagem() const override { return contagem_; }

  /// @copydoc IEncoder::zerar
  void zerar() override { contagem_ = 0; }

 private:
  int32_t contagem_ = 0;
};

}  // namespace micromouse
