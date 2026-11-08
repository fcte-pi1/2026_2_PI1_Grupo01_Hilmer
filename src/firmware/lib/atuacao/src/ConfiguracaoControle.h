#pragma once

/// @file ConfiguracaoControle.h
/// @brief Geometria do labirinto e do robô e parâmetros do controle de movimento (#191).
///
/// ATENÇÃO: os ganhos, as potências e as distâncias de desaceleração são PROVISÓRIOS. Foram
/// ajustados numa simulação simples (`test_controle_movimento`), com velocidade de 0,3 mm/s por
/// unidade de potência e 100 mm entre as rodas, que ainda não foram medidas. Ajuste na bancada
/// (issue #191): ande num corredor reto saindo 2 cm fora do centro e observe se o robô volta ao
/// centro sem oscilar e sem encostar.
///
/// A geometria foi estimada com um sensor no centro do robô (o LiDAR da Rev 1). Com os 3 VL53L0X da Rev 2.0, ajuste na bancada (#479).

#include <stdint.h>

#include "Configuracao.h"

namespace micromouse {

/// Espessura das paredes do labirinto: 1,2 cm (tema do PI1). A célula de 18 cm já a inclui.
constexpr uint16_t ESPESSURA_PAREDE_MM = 12;

/// Metade da largura do chassi (120 mm, `docs/04-projeto-conceitual/estruturas.md`).
constexpr uint16_t MEIA_LARGURA_CHASSI_MM = 60;

/// Metade do comprimento do chassi (100 mm, `docs/04-projeto-conceitual/estruturas.md`).
constexpr uint16_t MEIO_COMPRIMENTO_CHASSI_MM = 50;

/// Distância do centro do robô a cada parede lateral com ele centralizado no corredor:
/// metade do corredor livre, (180 - 12) / 2 = 84 mm.
constexpr uint16_t DISTANCIA_CENTRO_A_PAREDE_MM = (TAMANHO_CELULA_MM - ESPESSURA_PAREDE_MM) / 2;

static_assert(DISTANCIA_CENTRO_A_PAREDE_MM > MEIA_LARGURA_CHASSI_MM,
              "o chassi precisa caber no corredor");

/// Parâmetros usados pelo `ControleMovimento`, pelo `ControladorCentralizacao` e pelo
/// `PerfilParadaFrontal`.
struct ConfiguracaoControle {
  /// Distância lateral desejada até cada parede: o robô centralizado no corredor.
  uint16_t distanciaAlvoLateralMm = DISTANCIA_CENTRO_A_PAREDE_MM;

  /// Leituras laterais até este valor indicam parede ao lado; acima dele, o lado está livre.
  /// É o mesmo limiar do `ClassificadorParede` (uma célula).
  uint16_t limiarParedeLateralMm = TAMANHO_CELULA_MM;

  /// Ganho proporcional, em décimos: potência de correção por milímetro fora do centro, × 10
  /// (20 = 2,0 de potência por mm).
  int16_t ganhoProporcionalDecimos = 20;

  /// Ganho derivativo, em décimos: potência de correção por mm/s de aproximação da parede, × 10.
  /// Amortece a oscilação em torno do centro.
  int16_t ganhoDerivativoDecimos = 15;

  /// Filtro da derivada, em %: quanto da nova estimativa entra a cada leitura (100 = sem filtro).
  /// Reduz o efeito do ruído dos sensores na correção.
  uint8_t filtroDerivadaPercentual = 40;

  /// Maior correção, em potência, somada a uma roda e subtraída da outra.
  int16_t correcaoMaxima = 250;

  /// Maior correção em relação à potência base, em %. Evita que uma roda gire para trás quando
  /// o robô está lento (perto de parar).
  uint8_t correcaoMaximaPercentualDaBase = 60;

  /// Potência das duas rodas com o caminho livre à frente. Conservadora: errar uma tentativa
  /// custa mais caro do que ser lento.
  int16_t potenciaCruzeiro = 400;

  /// Menor potência que ainda move o robô (zona morta dos motores N20). A rampa termina nela.
  int16_t potenciaMinima = 150;

  /// Leitura frontal em que o robô começa a desacelerar: o centro da célula anterior à parede.
  uint16_t distanciaInicioDesaceleracaoMm = DISTANCIA_CENTRO_A_PAREDE_MM + TAMANHO_CELULA_MM;

  /// Leitura frontal em que o robô para: o centro da célula, a 84 mm da parede da frente
  /// (sobram 34 mm entre o chassi e a parede).
  uint16_t distanciaParadaFrontalMm = DISTANCIA_CENTRO_A_PAREDE_MM;

  /// Sem leitura válida dos sensores por este tempo, o robô freia (fail-safe). Antes disso, segue
  /// em linha reta na potência mínima, sem correção.
  uint32_t tempoMaximoSemLeituraMs = 500;

  /// Ganho proporcional do controle de rumo pelo giroscópio (#190), em décimos.
  /// Potência por grau de desvio angular: (80 = 8,0 de potência por grau de desvio).
  int16_t ganhoRumoProporcionalDecimos = 80;

  /// Ganho derivativo do controle de rumo pelo giroscópio (#190), em décimos.
  /// Amortece a velocidade angular durante o avanço em linha reta.
  int16_t ganhoRumoDerivativoDecimos = 20;

  /// Limite máximo da correção de rumo aplicada aos motores, em unidades de potência.
  int16_t correcaoRumoMaxima = 200;

  /// Ativa ou desativa a correção de rumo pelo giroscópio durante o avanço (#190).
  bool habilitarCorrecaoRumo = true;
};

}  // namespace micromouse
