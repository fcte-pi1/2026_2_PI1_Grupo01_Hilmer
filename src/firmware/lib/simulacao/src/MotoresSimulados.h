#pragma once

/// @file MotoresSimulados.h
/// @brief Implementação falsa de `IMotores` para testes sem hardware.

#include <stdint.h>

#include "IMotores.h"

namespace micromouse {

/// Motores falsos: guardam o último comando recebido, para o teste conferir.
///
/// Exemplo:
/// @code
/// MotoresSimulados motores;
/// IMotores& tracao = motores;            // a lógica recebe só a interface
/// tracao.definirPotencia({400, 420});
/// motores.ultimaPotencia().direita;      // 420
/// @endcode
class MotoresSimulados : public IMotores {
 public:
  /// @copydoc IMotores::definirPotencia
  void definirPotencia(const PotenciaRodas& potencia) override {
    ultima_ = {limitarPotencia(potencia.esquerda), limitarPotencia(potencia.direita)};
    freados_ = false;
    ++comandos_;
  }

  /// @copydoc IMotores::parar
  void parar() override {
    ultima_ = {0, 0};
    freados_ = true;
    ++comandos_;
  }

  /// Último comando aplicado, já limitado à faixa permitida.
  PotenciaRodas ultimaPotencia() const { return ultima_; }

  /// `true` se o último comando foi `parar()`.
  bool freados() const { return freados_; }

  /// Quantos comandos (`definirPotencia` ou `parar`) foram recebidos.
  uint32_t comandos() const { return comandos_; }

 private:
  PotenciaRodas ultima_ = {0, 0};
  bool freados_ = false;
  uint32_t comandos_ = 0;
};

}  // namespace micromouse
