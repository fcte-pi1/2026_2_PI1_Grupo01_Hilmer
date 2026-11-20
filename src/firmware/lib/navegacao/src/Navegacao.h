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
#include "DetectorObjetivo.h"
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
  Bloqueado,    ///< Robô ainda na origem e o mapa já mostra parede no caminho: passo cancelado.
};

/// Gerenciador de navegação e resolução do labirinto.
///
/// A `Navegacao` guarda uma referência ao mapa e calcula o objetivo padrão na construção, a
/// partir do tamanho do mapa naquele momento. Por isso, crie a `Navegacao` **depois** de
/// `ArmazenamentoMapa::iniciarCorrida(...)`. Se o mapa for trocado por outro tamanho depois
/// (nova corrida em outro labirinto), chame `definirObjetivo()` e `definirPose()` de novo:
/// o objetivo e a pose antigos podem ficar fora do mapa novo.
///
/// Toda pose recebida é validada contra o mapa (`Mapa::posValida`). Uma pose fora do mapa nunca
/// é usada para ler o vetor de distâncias.
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
  /// @param poseInicial Pose inicial do robô (padrão: célula (0,0) voltada para o Norte). Se a
  ///        célula estiver fora do mapa, ela é ignorada e o robô começa na partida (0, 0), mantendo
  ///        a direção informada.
  Navegacao(const Mapa& mapa, PosicaoCelula objetivo, Pose poseInicial = {{0, 0}, Direcao::Norte});

  /// Posição discreta atual do robô.
  PosicaoCelula obterPosicao() const { return posicao_atual_; }

  /// Direção atual da frente do robô.
  Direcao obterDirecao() const { return direcao_atual_; }

  /// Pose completa atual (célula e direção).
  Pose obterPose() const { return {posicao_atual_, direcao_atual_}; }

  /// Célula objetivo configurada.
  PosicaoCelula obterObjetivo() const { return detector_.celulaObjetivo(); }

  /// Define a pose atual do robô sem compará-la com um passo (ex.: posição inicial).
  /// Cancela o passo pendente, se houver.
  /// @return `false` se a célula está fora do mapa: nesse caso nada muda (pose e passo pendente
  ///         continuam os anteriores); `true` se a pose foi adotada.
  bool definirPose(const Pose& pose);

  /// Posiciona o robô em uma célula e orientação específicas. Cancela o passo pendente.
  /// @return `false` se a célula está fora do mapa: nesse caso nada muda; `true` caso contrário.
  bool moverPara(uint8_t linha, uint8_t coluna, Direcao direcao);

  /// Vários objetivos de uma vez (a largada em qualquer célula, enquanto o robô ainda não sabe onde
  /// está): o Flood Fill mede a distância até o objetivo mais próximo, e `atingiuObjetivo()` vale em
  /// qualquer um deles. `obterObjetivo()` devolve o primeiro. Lista vazia: nenhum objetivo, sem caminho.
  void definirObjetivos(const PosicaoCelula* objetivos, uint16_t quantidade);

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
  /// @return true se foi encontrada uma direção válida até o objetivo; false se inalcançável, já no
  ///         objetivo ou se a pose atual está fora do mapa (por exemplo, depois de trocar o mapa).
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
  /// Se o robô continua na origem e o mapa já mostra uma `Parede` entre a origem e a célula
  /// esperada (o sensor viu a parede depois que o passo foi planejado), o passo é cancelado e o
  /// resultado é `Bloqueado`. Sem isso, o passo ficaria pendente para sempre e o robô parado.
  ///
  /// @param poseMedida Célula e direção informadas pelos sensores.
  /// @return Resultado da comparação com a célula esperada (veja `ResultadoConfirmacao`).
  ResultadoConfirmacao confirmarPose(const Pose& poseMedida);

  /// Cancela o passo pendente, se houver. O próximo `darPasso()` replaneja a partir da pose atual.
  void cancelarPasso() { passo_pendente_ = false; }

  /// Retorna se há um passo iniciado aguardando confirmação dos sensores.
  bool passoPendente() const { return passo_pendente_; }

  /// Célula esperada ao fim do passo pendente (só tem significado se `passoPendente()`).
  PosicaoCelula obterCelulaEsperada() const { return celula_esperada_; }

 private:
  const Mapa& mapa_;
  PosicaoCelula posicao_atual_ = {0, 0};
  Direcao direcao_atual_ = Direcao::Norte;

  /// Detector de objetivo interno usado pelo Flood Fill para saber a célula de destino.
  /// @note Duplicidade temporária: coexiste com o `detectorObjetivo` de `main.cpp`
  /// até que a `Navegacao` seja integrada no laço da tarefa FreeRTOS e os detectores
  /// sejam unificados e simplificados (#189 / revisão PR #351).
  DetectorObjetivo detector_;

  bool passo_pendente_ = false;
  PosicaoCelula celula_esperada_ = {0, 0};
  Direcao direcao_passo_ = Direcao::Norte;  ///< Direção do passo pendente, a partir da origem.

  static constexpr uint16_t CAPACIDADE = static_cast<uint16_t>(MAX_LINHAS_EXPLORACAO) * MAX_COLUNAS_EXPLORACAO;

  /// Objetivos do Flood Fill: normalmente um só (o do `detector_`); vários na exploração.
  PosicaoCelula objetivos_[CAPACIDADE] = {};
  uint16_t quantidadeObjetivos_ = 0;

  uint8_t distancias_[MAX_LINHAS_EXPLORACAO][MAX_COLUNAS_EXPLORACAO] = {};
};

}  // namespace micromouse
