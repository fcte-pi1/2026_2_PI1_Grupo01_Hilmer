#pragma once

/// @file ConfiguracaoExecucao.h
/// @brief Parâmetros da execução dos movimentos entre células (#184): avanço de uma célula, giros,
/// percepção no centro da célula e gatilho da largada.
///
/// ATENÇÃO: todos os valores são PROVISÓRIOS. Foram ajustados na simulação dos testes
/// (`test_execucao_movimentos` e `test_integracao_execucao`), com a tabela de velocidade e a
/// geometria provisórias dos outros módulos. O erro de deslocamento real é medido na bancada
/// (`scripts/bancada_deslocamento.py`, comandos de célula e de giro). Para mudar um valor, altere
/// só este arquivo.

#include <stdint.h>

#include "Configuracao.h"
#include "ConfiguracaoDeslocamento.h"

namespace micromouse {

// ---------------------------------------------------------------------------------------------
// 1. Geometria dos sensores para o controle do avanço
// ---------------------------------------------------------------------------------------------

// O `ControleMovimento` trabalha com distâncias a partir do centro do robô (84 mm até a parede com
// ele centralizado), e o ToF mede a partir da face do sensor: a leitura recebe o recuo de cada
// sensor, `DISTANCIA_SENSOR_FRONTAL_AO_CENTRO_MM` e `DISTANCIA_SENSOR_LATERAL_AO_CENTRO_MM`, ambos
// em `ConfiguracaoDeslocamento.h` (os mesmos das correções pelas paredes).

/// Leitura lateral (a partir da face do sensor), em mm, acima da qual o lado conta como sem parede
/// para a centralização do avanço (#188). Uma parede imediata lê 54 mm com o robô centrado (até
/// cerca de 80 mm com ele 25 mm fora). Perto do fim de uma parede, o raio lateral pega a parede de
/// uma célula vizinha a 100–180 mm, e o `ControleMovimento` (limiar de 180 mm do centro) a tomaria
/// como parede imediata e viraria o robô para o lado errado.
constexpr uint16_t LIMIAR_PAREDE_LATERAL_CONTROLE_MM = 100;

/// Distância lateral (do centro) passada ao `ControleMovimento` para um lado sem parede.
constexpr uint16_t LATERAL_SEM_PAREDE_MM = 1000;

// ---------------------------------------------------------------------------------------------
// 2. Avanço de uma célula
// ---------------------------------------------------------------------------------------------

/// Distância até o centro da célula de destino em que o avanço termina, em mm. O robô para
/// quando faltar isto ou menos (ou quando passar do centro).
constexpr uint16_t TOLERANCIA_CHEGADA_MM = 3;

/// Distância antes do centro de destino em que a rampa de desaceleração começa, em mm. Na rampa, a
/// potência base cai de `potenciaCruzeiro` até `potenciaMinima` (`ConfiguracaoControle`).
constexpr uint16_t DISTANCIA_DESACELERACAO_MM = 60;

/// Se a parede da frente fizer o robô parar a até esta distância do centro de destino, em mm, o
/// avanço conta como concluído: a parede é a melhor referência do centro. Mais longe, o avanço
/// termina como `ParedeNaFrente` (uma parede que o mapa ainda não tinha).
constexpr uint16_t FOLGA_PARADA_NA_PAREDE_MM = 40;

/// Tempo máximo de um avanço de uma célula, em ms. A 45 mm/s (potência mínima), 180 mm levam 4 s.
constexpr uint32_t TEMPO_MAXIMO_AVANCO_MS = 6000;

// ---------------------------------------------------------------------------------------------
// 3. Percepção no centro da célula
// ---------------------------------------------------------------------------------------------

/// Leituras novas do ToF, com o robô parado no centro, que entram na classificação das paredes
/// (mediana do `ClassificadorParede`). A 33 ms por leitura, 3 leituras levam cerca de 0,1 s.
constexpr uint8_t AMOSTRAS_PERCEPCAO = 3;

// ---------------------------------------------------------------------------------------------
// 4. Gatilho da largada: a mão na frente do ToF frontal
// ---------------------------------------------------------------------------------------------

/// Leitura frontal, em mm, a partir da qual a frente conta como livre (a mão saiu). Fica acima da
/// parede imediata (cerca de 34 mm com o sensor a 50 mm do centro): uma parede parada na frente
/// nunca arma o gatilho, e a largada fica só pelo comando serial.
constexpr uint16_t LIMIAR_FRENTE_LIVRE_GATILHO_MM = 100;

/// Leitura frontal, em mm, até a qual a frente conta como coberta pela mão.
constexpr uint16_t LIMIAR_FRENTE_COBERTA_GATILHO_MM = 50;

/// Tempo mínimo com a frente coberta, em ms, para valer como gesto (um vulto rápido não larga).
constexpr uint32_t TEMPO_MINIMO_COBERTO_GATILHO_MS = 300;

/// Espera depois de a mão sair, em ms, antes de largar: dá tempo de tirar a mão do caminho.
constexpr uint32_t ESPERA_APOS_GESTO_GATILHO_MS = 1000;

// ---------------------------------------------------------------------------------------------
// 5. Largada em qualquer célula
// ---------------------------------------------------------------------------------------------

/// Quantas vezes, numa corrida, a exploração da largada em qualquer célula pode recomeçar da célula
/// atual, com o mapa limpo, quando o que foi gravado deixa de fazer sentido (nenhuma hipótese de
/// largada sobra, ou nenhum objetivo possível é alcançável). Acontece quando a estimativa do
/// deslocamento erra uma célula e uma parede vai para o lugar errado.
constexpr uint8_t RELARGADAS_MAXIMAS = 3;

// ---------------------------------------------------------------------------------------------
// Parâmetros em uso (os testes alteram os campos para cobrir outros cenários)
// ---------------------------------------------------------------------------------------------

/// Parâmetros do `ExecutorMovimentos` e do `CicloNavegacao`.
struct ConfiguracaoExecucao {
  uint16_t distanciaSensorFrontalAoCentroMm = DISTANCIA_SENSOR_FRONTAL_AO_CENTRO_MM;
  uint16_t distanciaSensorLateralAoCentroMm = DISTANCIA_SENSOR_LATERAL_AO_CENTRO_MM;
  uint16_t limiarParedeLateralControleMm = LIMIAR_PAREDE_LATERAL_CONTROLE_MM;
  uint16_t toleranciaChegadaMm = TOLERANCIA_CHEGADA_MM;
  uint16_t distanciaDesaceleracaoMm = DISTANCIA_DESACELERACAO_MM;
  uint16_t folgaParadaNaParedeMm = FOLGA_PARADA_NA_PAREDE_MM;
  uint32_t tempoMaximoAvancoMs = TEMPO_MAXIMO_AVANCO_MS;
  uint8_t amostrasPercepcao = AMOSTRAS_PERCEPCAO;
  /// Com `true`, o robô pode largar de qualquer célula (virado para o Norte do labirinto), sem saber
  /// qual nem o tamanho do labirinto: o ciclo explora até se localizar (veja `CicloNavegacao`).
  bool largadaEmQualquerCelula = LARGADA_EM_QUALQUER_CELULA;
  uint8_t relargadasMaximas = RELARGADAS_MAXIMAS;
  /// `true` se os ToF laterais ficam na diagonal (cerca de 45° da frente), como no robô real, e não a 90°.
  /// De frente para uma parede próxima, eles enxergam a parede da frente em ângulo e leem "parede" mesmo
  /// com a lateral aberta. Então, com a frente classificada como parede, as laterais daquela célula não
  /// são gravadas no mapa (ficam desconhecidas): o robô tenta a passagem e, se bater, o mapa é corrigido.
  /// Desligado por padrão: o modelo simulado tem os sensores a 90°.
  bool lateraisEmDiagonal = false;
  /// Se o ToF frontal ler menos que `frenteParaRecuarMm` na hora de girar, o robô recua `recuoAntesDoGiroMm`
  /// em linha reta antes do giro, para o chassi ter folga para rodar sem bater nas paredes. 0 desliga.
  /// Desligado por padrão: o modelo simulado gira no lugar sem problema.
  uint16_t recuoAntesDoGiroMm = 0;
  uint16_t frenteParaRecuarMm = 0;
  /// Limiar de parede das laterais, em mm (leitura abaixo disso conta como parede). 0 mantém o da percepção
  /// (140 mm, calculado para sensores a 90°). Com os sensores na diagonal e o robô fora do centro, a leitura de
  /// uma parede vai de ~105 a ~175 mm, então o robô real usa mais folga.
  uint16_t limiarParedeLateralPercepcaoMm = 0;
  /// Quantas vezes o robô, na largada fixa, apaga o mapa e tenta de novo quando o planejador conclui "sem
  /// caminho", em vez de parar. Um mapa com uma parede falsa (leitura errada perto de uma quina) fecha o
  /// caminho; recomeçar o mapa a partir da célula atual dá outra chance. 0 desliga.
  uint8_t reiniciosDoMapaNoSemCaminho = 0;
  /// Desvio de quina: durante o avanço, se uma parede lateral estiver mais perto que `desvioParedePertoMm`
  /// e o outro lado estiver mais longe pelo menos `desvioFolgaOutroLadoMm`, o rumo alvo vira
  /// `desvioRumoMiligraus` para o lado oposto, para o robô se afastar da quina em vez de encostar nela. 0 desliga.
  uint16_t desvioParedePertoMm = 0;
  uint16_t desvioFolgaOutroLadoMm = 40;
  int32_t desvioRumoMiligraus = 6000;
  /// Ajuste de chegada pela frente: ao fim do avanço pela odometria, se o ToF frontal mostra uma parede a mais
  /// de `FOLGA` do ponto onde o avanço para pela parede (`distanciaParadaFrontalMm`) (a odometria superestimou o caminho), o robô anda a diferença, até
  /// este máximo em mm. 0 desliga.
  uint16_t ajusteChegadaPelaFrenteMaxMm = 0;
  /// Mesmo ajuste, com teto maior, quando se sabe que há parede no fim da célula de destino (a borda do
  /// labirinto): aí uma leitura longa só pode ser a odometria curta, nunca a parede da célula seguinte.
  uint16_t ajusteChegadaParedeGarantidaMaxMm = 0;
};

/// Parâmetros do `GatilhoLargada`.
struct ConfiguracaoGatilhoLargada {
  uint16_t limiarFrenteLivreMm = LIMIAR_FRENTE_LIVRE_GATILHO_MM;
  uint16_t limiarFrenteCobertaMm = LIMIAR_FRENTE_COBERTA_GATILHO_MM;
  uint32_t tempoMinimoCobertoMs = TEMPO_MINIMO_COBERTO_GATILHO_MS;
  uint32_t esperaAposGestoMs = ESPERA_APOS_GESTO_GATILHO_MS;
};

}  // namespace micromouse
