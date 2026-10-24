#pragma once

/// @file IntegradorGiro.h
/// @brief Ângulo de giro (yaw) a partir da taxa do giroscópio, com o viés removido (#378).

#include <stdint.h>

#include "ConfiguracaoImu.h"

namespace micromouse {

/// Situação da calibração do viés.
enum class EstadoCalibracao : uint8_t {
  NaoCalibrado,  ///< Nenhuma calibração feita: o viés usado é zero.
  Calibrando,    ///< Recebendo amostras; ainda faltam algumas.
  Calibrado,     ///< Viés medido e em uso.
  Recusada       ///< As amostras variaram demais (o robô se mexeu): o viés anterior foi mantido.
};

/// Soma a taxa de giro ao longo do tempo para obter o ângulo de giro (yaw).
///
/// Todo giroscópio parado mede um pequeno valor diferente de zero, o **viés**. Integrado, o viés
/// vira deriva: com 1,2 °/s, o ângulo andaria 12° em 10 s com o robô parado. Por isso:
///
/// 1. **Calibração:** com o robô parado (por exemplo, na largada), `iniciarCalibracao()` e
///    `adicionarAmostraCalibracao()` a cada leitura. Ao fim de `amostrasCalibracao` amostras, o viés
///    é a média delas. Se a maior e a menor amostra diferirem mais que
///    `variacaoMaximaCalibracaoDecimosDps`, a calibração é recusada e o viés anterior continua.
/// 2. **Integração:** `atualizar(taxa, dtMs)` tira o viés, zera o que cair na zona morta e soma
///    `taxa × dt` ao ângulo.
///
/// O ângulo é acumulado, sem dar a volta (um giro de 360° vale 360 000 mili-graus), o que facilita
/// controlar giros de 90° e 180°. `anguloNormalizadoMiligraus()` devolve o mesmo ângulo entre
/// −180° (exclusive) e 180°. Positivo é giro para a esquerda, como em `IImu`.
///
/// Só usa inteiros, porque o ESP32-C3 não tem unidade de ponto flutuante.
///
/// Uso típico:
/// @code
/// IntegradorGiro giro;
/// giro.iniciarCalibracao();
/// // na largada, robô parado, a cada 10 ms:
/// giro.adicionarAmostraCalibracao(taxa);
/// // depois, a cada leitura:
/// int32_t angulo = giro.atualizar(taxa, 10);
/// @endcode
class IntegradorGiro {
 public:
  explicit IntegradorGiro(const ConfiguracaoIntegradorGiro& config = ConfiguracaoIntegradorGiro{});

  /// Começa uma calibração nova, descartando as amostras anteriores. O viés em uso só muda quando
  /// a calibração terminar bem.
  void iniciarCalibracao();

  /// Acrescenta uma amostra da calibração (robô parado).
  /// @return O estado da calibração depois da amostra. Fora de `Calibrando`, a amostra é ignorada.
  EstadoCalibracao adicionarAmostraCalibracao(int16_t taxaDecimosDps);

  /// Estado da última calibração.
  EstadoCalibracao estadoCalibracao() const { return estado_; }

  /// Viés em uso, em décimos de °/s.
  int16_t viesDecimosDps() const { return vies_; }

  /// Soma ao ângulo a taxa medida durante `dtMs`, já sem o viés e com a zona morta aplicada.
  /// @return O ângulo acumulado, em mili-graus.
  int32_t atualizar(int16_t taxaDecimosDps, uint32_t dtMs);

  /// Ângulo acumulado desde o último `zerarAngulo()`, em mili-graus (90° = 90 000).
  int32_t anguloMiligraus() const;

  /// O mesmo ângulo, levado para o intervalo (−180°, 180°], em mili-graus.
  int32_t anguloNormalizadoMiligraus() const;

  /// Volta o ângulo para zero (por exemplo, ao começar um giro). Não muda o viés.
  void zerarAngulo() { acumulado_ = 0; }

 private:
  ConfiguracaoIntegradorGiro config_;
  EstadoCalibracao estado_ = EstadoCalibracao::NaoCalibrado;
  int16_t vies_ = 0;

  // Calibração em andamento.
  uint16_t amostras_ = 0;
  int32_t somaAmostras_ = 0;
  int16_t menorAmostra_ = 0;
  int16_t maiorAmostra_ = 0;

  // Ângulo acumulado em "décimos de °/s × ms": 10 unidades = 1 mili-grau. Guardar a soma exata
  // evita perder frações a cada ciclo.
  int64_t acumulado_ = 0;
};

}  // namespace micromouse
