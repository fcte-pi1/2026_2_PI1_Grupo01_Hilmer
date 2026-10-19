#pragma once

/// @file Navegacao.h
/// @brief Resolução e navegação do labirinto com algoritmo Flood Fill.
///
/// Decide a próxima direção a seguir calculando a menor distância até a célula
/// objetivo a partir das paredes conhecidas no Mapa.
///
/// Não depende do Arduino e não aloca memória dinâmica.

#include <cstdint>

#include "Configuracao.h"
#include "Direcao.h"
#include "Mapa.h"
#include "Tipos.h"

namespace micromouse {

/// Resultado de `Navegacao::confirmarPose()`.
enum class ResultadoConfirmacao : uint8_t {
  Confirmada,   ///< Robô na célula esperada (ou sem passo pendente): pose adotada.
  EmAndamento,  ///< Robô ainda na célula de origem do passo: pose adotada, passo segue pendente.
  Divergente,   ///< Robô em outra célula: pose medida adotada e passo cancelado para replanejar.
  Invalida,     ///< Pose fora do mapa: ignorada, a pose e o passo pendente não mudam.
};

/// Gerenciador de navegação e resolução do labirinto.
class Navegacao {
 public:
  /// Valor sentinela que representa distância infinita / célula inacessível.
  static constexpr uint8_t DISTANCIA_INFINITA = 255;

  /// Cria o módulo de navegação associado a um mapa.
  /// O objetivo padrão é configurado para o canto oposto à partida (linhas - 1, colunas - 1).
  /// @param mapa Referência do mapa a ser consultado.
  explicit Navegacao(const Mapa& mapa);

  /// Cria o módulo de navegação com objetivo e pose inicial personalizados.
  /// @param mapa Referência do mapa a ser consultado.
  /// @param objetivo Célula de destino desejada.
  /// @param poseInicial Pose inicial do robô (padrão: célula (0,0) voltada para o Norte).
  Navegacao(const Mapa& mapa, PosicaoCelula objetivo, Pose poseInicial = {{0, 0}, Direcao::Norte});

  /// Posição discreta atual do robô.
  PosicaoCelula obterPosicao() const { return posicao_atual_; }

  /// Direção atual da frente do robô.
  Direcao obterDirecao() const { return direcao_atual_; }

  /// Pose completa atual (célula e direção).
  Pose obterPose() const { return {posicao_atual_, direcao_atual_}; }

  /// Célula objetivo configurada.
  PosicaoCelula obterObjetivo() const { return objetivo_; }

  /// Define a pose atual do robô sem compará-la com um passo (ex.: posição inicial).
  /// Cancela o passo pendente, se houver.
  void definirPose(const Pose& pose);

  /// Posiciona o robô em uma célula e orientação específicas. Cancela o passo pendente.
  void moverPara(uint8_t linha, uint8_t coluna, Direcao direcao);

  /// Altera a célula objetivo.
  void definirObjetivo(PosicaoCelula objetivo);

  /// Retorna se o robô alcançou a célula objetivo.
  bool atingiuObjetivo() const;

  /// Recalcula as distâncias até o objetivo usando o algoritmo Flood Fill (BFS).
  /// Paredes 'Livre' e 'Desconhecido' são consideradas transitáveis (heurística otimista).
  /// Apenas paredes confirmadas como 'Parede' bloqueiam o trajeto.
  void recalcularDistancias();

  /// Retorna a distância calculada pelo Flood Fill para uma célula específica.
  /// @param celula Célula a consultar.
  /// @return Distância em células até o objetivo, ou DISTANCIA_INFINITA se inalcançável.
  uint8_t obterDistancia(PosicaoCelula celula) const;

  /// Decide a próxima direção a seguir a partir da posição atual do robô.
  ///
  /// Recalcula o Flood Fill e escolhe a direção vizinha livre com menor distância.
  /// Em caso de empate, prioriza manter a direção atual (Frente) para evitar manobras,
  /// seguida por Direita, Esquerda e Trás.
  ///
  /// @param[out] proximaDirecao Recebe a direção decidida quando há caminho.
  /// @return true se foi encontrada uma direção válida até o objetivo; false se inalcançável ou já no objetivo.
  bool decidirProximaDirecao(Direcao& proximaDirecao);

  /// Inicia um passo: decide a próxima direção e registra a célula vizinha esperada.
  ///
  /// A pose não muda aqui. O controle de movimento deve levar o robô até a célula esperada,
  /// e a pose só é atualizada quando os sensores a confirmarem por `confirmarPose()`.
  /// Não inicia outro passo enquanto houver um pendente.
  ///
  /// @param[out] direcaoMovimento Recebe a direção em que o robô deve se mover.
  /// @return true se o passo foi iniciado; false se há passo pendente, não há caminho ou já
  ///         atingiu o objetivo.
  bool darPasso(Direcao& direcaoMovimento);

  /// Atualiza a pose com a medida dos sensores (por exemplo, a odometria) e a compara com o
  /// passo pendente.
  ///
  /// A pose medida é a referência: exceto quando inválida, ela substitui a pose atual, e
  /// o próximo `darPasso()` replaneja a partir dela.
  ///
  /// @param poseMedida Célula e direção informadas pelos sensores.
  /// @return Resultado da comparação com a célula esperada (veja `ResultadoConfirmacao`).
  ResultadoConfirmacao confirmarPose(const Pose& poseMedida);

  /// Retorna se há um passo iniciado aguardando confirmação dos sensores.
  bool passoPendente() const { return passo_pendente_; }

  /// Célula esperada ao fim do passo pendente (só tem significado se `passoPendente()`).
  PosicaoCelula obterCelulaEsperada() const { return celula_esperada_; }

 private:
  const Mapa& mapa_;
  PosicaoCelula posicao_atual_ = {0, 0};
  Direcao direcao_atual_ = Direcao::Norte;
  PosicaoCelula objetivo_ = {0, 0};

  bool passo_pendente_ = false;
  PosicaoCelula celula_esperada_ = {0, 0};

  uint8_t distancias_[MAX_LINHAS_LABIRINTO][MAX_COLUNAS_LABIRINTO] = {};
};

}  // namespace micromouse
