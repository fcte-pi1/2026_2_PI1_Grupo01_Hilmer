#pragma once

/// @file TabelaVelocidade.h
/// @brief Tabela de calibração: velocidade da roda para cada potência comandada (#380).
///
/// ATENÇÃO: os pontos padrão são PROVISÓRIOS. Foram escolhidos para bater com a simulação do controle
/// de movimento (`ConfiguracaoControle.h`: 0,3 mm/s por unidade de potência, de 400 a 1000) e com
/// uma zona morta de 100 de potência. A tensão do VM, a redução e a roda do N20 ainda não foram
/// informadas e nada foi medido com o robô montado. Calibre na bancada: com o robô numa reta de
/// 1 m, comande cada potência da tabela, meça a velocidade (distância dividida pelo tempo) e troque
/// os pontos. Para ajustar só a escala, use `ConfiguracaoEstimador::fatorEscalaPercentual`.

#include <stdint.h>

namespace micromouse {

/// Quantos pontos a tabela comporta.
constexpr uint8_t MAX_PONTOS_TABELA_VELOCIDADE = 8;

/// Um ponto da tabela: com esta potência (módulo, de 0 a `POTENCIA_MAXIMA`), a roda anda nesta velocidade.
struct PontoVelocidade {
  uint16_t potencia;
  uint16_t velocidadeMmPorS;
};

/// Velocidade da roda em função da potência, por interpolação linear entre os pontos.
///
/// Os pontos precisam estar em ordem crescente de potência e o primeiro deve ser a potência 0. Acima
/// do último ponto, vale a velocidade do último. A potência negativa dá a mesma velocidade com sinal
/// trocado (a roda recua). Abaixo da zona morta (pontos com velocidade 0), o motor não anda.
struct TabelaVelocidade {
  PontoVelocidade pontos[MAX_PONTOS_TABELA_VELOCIDADE] = {
      {0, 0}, {100, 0}, {200, 45}, {400, 120}, {700, 210}, {1000, 300}, {0, 0}, {0, 0}};
  uint8_t quantidade = 6;

  /// Velocidade da roda, em mm/s, com sinal (positiva avançando), para a potência comandada.
  int32_t velocidadeMmPorS(int16_t potencia) const;
};

}  // namespace micromouse
