#pragma once

/// @file AtualizadorMapa.h
/// @brief Atualização do mapa a partir das leituras e pose do robô.
///
/// Integra a camada de percepção à de mapeamento: converte as paredes observadas
/// relativamente ao robô (frente, esquerda, direita) em direções absolutas no mapa
/// (Norte, Leste, Sul, Oeste) e registra cada parede no Mapa.
///
/// Não depende do Arduino e não aloca memória dinâmica.

#include <cstddef>

#include "ClassificadorParede.h"
#include "Direcao.h"
#include "Mapa.h"
#include "Tipos.h"

namespace micromouse {

/// Resultado da atualização do mapa a partir de uma leitura sensorial.
///
/// Conflitos ocorrem quando uma leitura contradiz uma parede já conhecida.
/// O conflito não sobrescreve o mapa (o valor anterior é preservado) e a
/// política de tratamento/decisão fica a critério de quem chama o atualizador.
struct ResultadoAtualizacao {
  ResultadoRegistro frente = ResultadoRegistro::Invalida;    ///< Registro da parede à frente.
  ResultadoRegistro esquerda = ResultadoRegistro::Invalida;  ///< Registro da parede à esquerda.
  ResultadoRegistro direita = ResultadoRegistro::Invalida;   ///< Registro da parede à direita.
  bool temConflito = false;                                  ///< Verdadeiro se qualquer uma das leituras gerou conflito.
};

/// Atualiza o Mapa integrando a classificação das paredes com a pose do robô.
class AtualizadorMapa {
 public:
  /// Atualiza o mapa a partir das paredes já classificadas e da pose atual do robô.
  /// @param mapa Mapa a ser atualizado.
  /// @param pose Pose atual do robô.
  /// @param classificacao Resultado da classificação das paredes.
  /// @return Resultado da atualização contendo o status de cada parede e flag de conflito.
  static ResultadoAtualizacao atualizar(
      Mapa &mapa,
      const Pose &pose,
      const ClassificadorParede::ResultadoClassificacao &classificacao);

  /// Sobrecarga de conveniência: recebe leituras dos sensores de distância, classifica com o classificador e atualiza o mapa.
  /// @param mapa Mapa a ser atualizado.
  /// @param pose Pose atual do robô.
  /// @param classificador Classificador de paredes.
  /// @param amostras Leituras dos sensores de distância.
  /// @param quantidadeAmostras Quantidade de amostras.
  /// @return Resultado da atualização contendo o status de cada parede e flag de conflito.
  static ResultadoAtualizacao atualizar(
      Mapa &mapa,
      const Pose &pose,
      const ClassificadorParede &classificador,
      const DistanciasLaterais *amostras,
      size_t quantidadeAmostras);
};

}  // namespace micromouse
