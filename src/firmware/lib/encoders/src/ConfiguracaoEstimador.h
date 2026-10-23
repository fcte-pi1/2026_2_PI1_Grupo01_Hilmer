#pragma once

/// @file ConfiguracaoEstimador.h
/// @brief Parâmetros do estimador de deslocamento sem encoder (#380).
///
/// ATENÇÃO: os valores abaixo são PROVISÓRIOS (veja `TabelaVelocidade.h`). Sem encoder, a distância
/// vem da velocidade comandada vezes o tempo, então o erro cresce com a patinagem e com a variação
/// da tensão da bateria. Ajuste na bancada e troque pelo encoder real quando ele for integrado.
/// Para mudar um valor, altere só este arquivo.

#include <stdint.h>

#include "ConfiguracaoEncoder.h"
#include "TabelaVelocidade.h"

namespace micromouse {

/// Fator de escala da velocidade da tabela, em %. 100 = a tabela como está. Se o robô andar 5% menos
/// que o estimado numa reta medida, use 95.
constexpr uint16_t FATOR_ESCALA_VELOCIDADE_PERCENTUAL = 100;

/// Constante de tempo da resposta do motor a um degrau de potência, em ms: a velocidade real leva
/// cerca de 3 vezes isto para chegar à da tabela. 0 desliga o efeito (resposta imediata).
constexpr uint32_t CONSTANTE_TEMPO_MOTOR_MS = 80;

/// Parâmetros do `EncoderEstimado` e do `EstimadorDeslocamento`.
struct ConfiguracaoEstimador {
  TabelaVelocidade tabela;
  /// Distância por pulso usada para converter a distância estimada em contagem. Igual à do
  /// encoder, para a `Odometria` não precisar mudar.
  uint32_t micrometrosPorPulso = MICROMETROS_POR_PULSO_PROVISORIO;
  uint16_t fatorEscalaPercentual = FATOR_ESCALA_VELOCIDADE_PERCENTUAL;
  uint32_t constanteTempoMs = CONSTANTE_TEMPO_MOTOR_MS;
};

}  // namespace micromouse
