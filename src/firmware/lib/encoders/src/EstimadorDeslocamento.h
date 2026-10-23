#pragma once

/// @file EstimadorDeslocamento.h
/// @brief Par de `EncoderEstimado` (roda esquerda e direita) alimentado pela potência dos motores (#380).

#include <stdint.h>

#include "ConfiguracaoEstimador.h"
#include "EncoderEstimado.h"
#include "IMotores.h"

namespace micromouse {

/// Os dois encoders estimados juntos. A tarefa de navegação chama `atualizar` uma vez por ciclo, com
/// a potência que mandou aos motores, e passa `esquerdo()` e `direito()` para a `Odometria`:
/// @code
/// EstimadorDeslocamento estimador;
/// // a cada ciclo de 10 ms:
/// estimador.atualizar(potenciaComandada, 10);
/// odometria.atualizar(estimador.esquerdo(), estimador.direito());
/// @endcode
class EstimadorDeslocamento {
 public:
  explicit EstimadorDeslocamento(const ConfiguracaoEstimador& config = ConfiguracaoEstimador{});

  /// Avança as duas rodas em um ciclo.
  void atualizar(const PotenciaRodas& potencia, uint32_t dtMs);

  /// Encoder da roda esquerda e da direita, pela interface comum.
  IEncoder& esquerdo() { return esquerdo_; }
  IEncoder& direito() { return direito_; }
  const IEncoder& esquerdo() const { return esquerdo_; }
  const IEncoder& direito() const { return direito_; }

  /// Zera as duas contagens.
  void zerar();

  /// Corrige o avanço das duas rodas por igual (por exemplo, a leitura de um ToF mostrou que o robô
  /// andou 4 mm a mais ou a menos). Não muda a diferença entre as rodas, então não gira o robô.
  void corrigirAvancoMicrometros(int32_t micrometros);

  /// Velocidade média das duas rodas estimada agora, em mm/s, com sinal.
  int32_t velocidadeMediaMmPorS() const { return (esquerdo_.velocidadeMmPorS() + direito_.velocidadeMmPorS()) / 2; }

 private:
  EncoderEstimado esquerdo_;
  EncoderEstimado direito_;
};

}  // namespace micromouse
