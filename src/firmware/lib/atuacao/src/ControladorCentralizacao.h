#pragma once

/// @file ControladorCentralizacao.h
/// @brief Correção da trajetória para manter o robô no centro do corredor (#191, HU-12).

#include <stdint.h>

#include "ConfiguracaoControle.h"
#include "Tipos.h"

namespace micromouse {

/// Qual referência o controlador usou na última leitura.
enum class ModoCentralizacao : uint8_t {
  DuasParedes,   ///< Parede dos dois lados: centraliza pela diferença entre elas.
  SoEsquerda,    ///< Só a parede da esquerda: mantém a distância alvo até ela.
  SoDireita,     ///< Só a parede da direita: mantém a distância alvo até ela.
  SemReferencia  ///< Nenhuma parede ao lado: não corrige (o robô segue reto).
};

/// Controlador PD da posição lateral do robô no corredor.
///
/// O **erro** é o deslocamento do robô para a direita do centro, em mm (negativo: à esquerda).
/// Com parede dos dois lados, é `(esquerda - direita) / 2`; com uma só, é a diferença entre a
/// leitura e a distância alvo. A **correção** devolvida é a potência a tirar da roda esquerda e
/// somar à direita: positiva faz o robô virar para a esquerda, de volta ao centro.
///
/// A parte derivativa usa a variação do erro entre duas leituras, com um filtro passa-baixa
/// contra o ruído do LiDAR. Ela recomeça do zero quando o modo muda (por exemplo, quando uma
/// parede lateral acaba), para não reagir ao salto da leitura.
///
/// Sem Arduino e sem alocação dinâmica: testado no ambiente `native`.
class ControladorCentralizacao {
 public:
  explicit ControladorCentralizacao(const ConfiguracaoControle& config = ConfiguracaoControle{});

  /// Calcula a correção para uma nova leitura do LiDAR.
  ///
  /// Chame uma vez por leitura nova (o LiDAR atualiza a cerca de 10 Hz).
  /// @param leitura Distâncias à frente, à esquerda e à direita, em mm.
  /// @param dtMs Tempo desde a leitura anterior, em ms. Com 0, a derivada não é atualizada.
  /// @return Correção em potência, limitada a `±correcaoMaxima`. 0 quando não há referência.
  int16_t calcular(const DistanciasLaterais& leitura, uint32_t dtMs);

  /// Esquece o histórico (erro anterior e derivada). Use depois de um giro ou de uma parada.
  void reiniciar();

  /// Referência usada na última leitura.
  ModoCentralizacao modo() const { return modo_; }

  /// Erro lateral da última leitura, em mm (0 sem referência).
  int32_t erroMm() const { return erroMm_; }

  /// Derivada filtrada do erro, em mm/s.
  int32_t derivadaMmPorS() const { return derivadaMmPorS_; }

 private:
  ConfiguracaoControle config_;
  ModoCentralizacao modo_ = ModoCentralizacao::SemReferencia;
  bool temErroAnterior_ = false;
  int32_t erroMm_ = 0;
  int32_t derivadaMmPorS_ = 0;
};

}  // namespace micromouse
