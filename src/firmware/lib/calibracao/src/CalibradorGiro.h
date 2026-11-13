#pragma once

/// @file CalibradorGiro.h
/// @brief Estimação do viés do giroscópio (eixo Z) e aplicação nas leituras (#240, HU-13 / RF-13).

#include <stdint.h>

#include "ConfiguracaoCalibracao.h"

namespace micromouse {

/// Situação da calibração do giroscópio.
enum class EstadoCalibracaoGiro : uint8_t {
  NaoCalibrado,  ///< Nenhuma calibração feita: o viés usado é zero.
  Calibrando,    ///< Recebendo amostras; ainda faltam algumas.
  Calibrado,     ///< Viés medido e em uso.
  Recusada       ///< Amostras variaram demais (robô se mexeu): viés anterior mantido.
};

/// Estima e aplica o viés do giroscópio com o robô parado.
///
/// 1. **Calibração:** `iniciar()` e `adicionarAmostra()` a cada leitura da taxa de giro (eixo Z).
///    Ao fim de `amostras` leituras, o viés é a média aritmética arredondada. Se a maior e a menor
///    amostra diferirem mais que `variacaoMaximaDecimosDps`, assume-se que o robô se moveu, a
///    calibração é recusada e o viés anterior é mantido.
/// 2. **Aplicação:** `corrigir(taxaBruta)` devolve `taxaBruta − viés`.
///
/// Só usa inteiros: as contas dão o mesmo resultado no computador (testes `native`) e na placa.
class CalibradorGiro {
 public:
  explicit CalibradorGiro(const ConfiguracaoCalibracaoGiro& config = ConfiguracaoCalibracaoGiro{});

  /// Começa uma calibração nova, descartando as amostras anteriores. O viés em uso só muda quando
  /// a calibração terminar com sucesso.
  void iniciar();

  /// Acrescenta uma amostra da taxa de giro (robô parado).
  /// @return O estado depois da amostra. Fora de `Calibrando`, a amostra é ignorada.
  EstadoCalibracaoGiro adicionarAmostra(int16_t taxaDecimosDps);

  /// Estado da última calibração.
  EstadoCalibracaoGiro estado() const { return estado_; }

  /// Viés em uso, em décimos de °/s.
  int16_t viesDecimosDps() const { return vies_; }

  /// Aplica o viés à taxa bruta: `corrigida = taxaBruta − viés`.
  int16_t corrigir(int16_t taxaBrutaDecimosDps) const;

  /// Quantidade de amostras recebidas na calibração atual.
  uint16_t amostrasRecebidas() const { return amostrasRecebidas_; }

  /// Média aritmética arredondada para o inteiro mais próximo (trata positivos e negativos).
  static int16_t mediaArredondada(int32_t soma, uint16_t quantidade);

 private:
  ConfiguracaoCalibracaoGiro config_;
  EstadoCalibracaoGiro estado_ = EstadoCalibracaoGiro::NaoCalibrado;
  int16_t vies_ = 0;
  uint16_t amostrasRecebidas_ = 0;
  int32_t somaAmostras_ = 0;
  int16_t menorAmostra_ = 0;
  int16_t maiorAmostra_ = 0;
};

}  // namespace micromouse

