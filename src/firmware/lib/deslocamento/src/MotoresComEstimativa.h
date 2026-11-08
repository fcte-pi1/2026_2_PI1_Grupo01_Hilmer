#pragma once

/// @file MotoresComEstimativa.h
/// @brief `IMotores` que repassa os comandos ao motor real e os informa ao estimador (#380).

#include "EstimadorDeslocamento.h"
#include "IMotores.h"

namespace micromouse {

/// Envolve os motores reais: cada comando vai para o driver e para o `EstimadorDeslocamento`, que
/// precisa saber a potência para estimar o deslocamento. Quem aciona os motores (por exemplo, o
/// `ControleMovimento`) recebe este objeto no lugar do driver e não muda.
///
/// @code
/// MotoresDrv8833 drv(...);
/// EstimadorDeslocamento estimador;
/// MotoresComEstimativa motores(drv, estimador);
/// controle.atualizar(sensorSimulado, motores, dtMs);   // o ControleMovimento não sabe da estimativa
/// @endcode
class MotoresComEstimativa : public IMotores {
 public:
  /// @param reais Driver que aciona os motores de verdade.
  /// @param estimador Recebe a potência de cada comando.
  MotoresComEstimativa(IMotores& reais, EstimadorDeslocamento& estimador)
      : reais_(reais), estimador_(estimador) {}

  /// @copydoc IMotores::definirPotencia
  void definirPotencia(const PotenciaRodas& potencia) override {
    reais_.definirPotencia(potencia);
    estimador_.comandar(potencia);
  }

  /// @copydoc IMotores::parar
  void parar() override {
    reais_.parar();
    estimador_.parar();
  }

 private:
  IMotores& reais_;
  EstimadorDeslocamento& estimador_;
};

}  // namespace micromouse
