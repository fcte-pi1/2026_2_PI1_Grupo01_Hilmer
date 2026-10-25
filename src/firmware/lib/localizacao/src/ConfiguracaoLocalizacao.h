#pragma once

/// @file ConfiguracaoLocalizacao.h
/// @brief Parâmetros da correção do erro acumulado pelas paredes (#188): posição lateral pelas
/// paredes laterais, realinhamento ao centro da célula antes dos giros e rumo pelo corredor.
///
/// ATENÇÃO: todos os valores são PROVISÓRIOS. Foram ajustados na simulação dos testes
/// (`test_localizacao` e `test_integracao_localizacao`), com a geometria provisória dos ToF
/// (`ConfiguracaoDeslocamento.h`). Confirme na bancada (`scripts/bancada_deslocamento.py`, etapa do
/// percurso). Para mudar um valor, altere só este arquivo.

#include <stdint.h>

#include "ConfiguracaoDeslocamento.h"

namespace micromouse {

// ---------------------------------------------------------------------------------------------
// 1. Posição lateral pelas paredes laterais
// ---------------------------------------------------------------------------------------------

/// Leitura lateral (a partir da face do sensor), em mm, até a qual o lado conta como parede para a
/// correção lateral. O mesmo limiar da correção pelas quinas (#380): uma parede imediata lê cerca
/// de 54 mm, e uma abertura, 234 mm ou mais.
constexpr uint16_t LIMIAR_PAREDE_CORRECAO_LATERAL_MM = LIMIAR_PAREDE_LATERAL_MM;

/// Maior diferença, em mm, entre a posição lateral estimada e a medida pelas paredes que ainda é
/// aceita. Acima disso a leitura é descartada: o robô está torto demais ou o sensor leu outra coisa.
constexpr uint16_t CORRECAO_LATERAL_MAXIMA_MM = 40;

/// Fração da diferença, em %, aplicada a cada leitura nova (cerca de 30 por segundo). Filtra o
/// ruído do VL53L0X sem atrasar a correção: com 50%, 5 leituras levam a 97% da diferença.
constexpr uint8_t GANHO_CORRECAO_LATERAL_PERCENTUAL = 50;

// ---------------------------------------------------------------------------------------------
// 2. Realinhamento ao centro da célula antes de um giro
// ---------------------------------------------------------------------------------------------

/// Desvio lateral, em mm, a partir do qual o robô se realinha ao centro antes de girar no lugar.
/// A folga do giro é de 5,9 mm (a quina do chassi de 120 × 100 mm passa a 78,1 mm do centro, e a
/// parede fica a 84 mm); o limiar deixa margem para o ruído do sensor.
constexpr uint16_t DESVIO_MINIMO_REALINHAMENTO_MM = 3;

/// Maior desvio, em mm, que um realinhamento tenta corrigir; acima disso, a leitura é suspeita.
constexpr uint16_t DESVIO_MAXIMO_REALINHAMENTO_MM = 25;

/// Quanto o robô pode andar no trecho reto do realinhamento com o lado livre (a célula de onde veio
/// ou a da frente sem parede), em mm. Com paredes na frente e atrás, o trecho cabe no que sobra entre
/// a quina do chassi girado e a parede (veja `planejarRealinhamento`).
constexpr uint16_t TRECHO_MAXIMO_REALINHAMENTO_LIVRE_MM = 60;

/// Folga mínima, em mm, entre a quina do chassi e a parede lateral durante o giro pequeno do
/// realinhamento. Define o maior ângulo que o realinhamento usa para cada desvio.
constexpr uint16_t FOLGA_MINIMA_REALINHAMENTO_MM = 4;

/// Quantas vezes o robô se realinha antes de um giro, se o primeiro realinhamento corrigir só parte
/// do desvio (um desvio grande pede um ângulo pequeno e um trecho maior que o espaço).
constexpr uint8_t TENTATIVAS_REALINHAMENTO = 2;

/// Potência dos trechos retos do realinhamento (a mínima que move o robô).
constexpr int16_t POTENCIA_REALINHAMENTO = 150;

/// Ganho do rumo nos trechos retos do realinhamento, em décimos de potência por grau.
constexpr int16_t GANHO_RUMO_REALINHAMENTO_DECIMOS = 80;

/// Tempo máximo de cada trecho reto do realinhamento, em ms.
constexpr uint32_t TEMPO_MAXIMO_TRECHO_REALINHAMENTO_MS = 3000;

// ---------------------------------------------------------------------------------------------
// 3. Rumo pelo corredor (deriva do giroscópio)
// ---------------------------------------------------------------------------------------------

/// Com paredes dos dois lados e o robô andando paralelo a elas (a distância à parede variando até
/// `VARIACAO_LATERAL_MAXIMA_RUMO_MM_POR_S`), o ângulo atual do giroscópio é o eixo do corredor, mesmo
/// com o robô fora do centro. O rumo de referência anda esta fração, em por mil, até ele a cada
/// leitura (cerca de 30 por segundo), e os giros seguintes partem dele: é como a deriva do giroscópio
/// é corrigida pelas paredes. 0 desliga.
///
/// Não exigir o robô centrado é proposital: o erro de rumo é justamente o que o tira do centro (a
/// correção de rumo e a centralização se equilibram com ele fora), e exigir o centro deixaria o
/// ajuste desligado quando ele mais importa. `ERRO_LATERAL_MAXIMO_RUMO_MM` só exclui os
/// transitórios grandes. Na varredura de 189 corridas simuladas, 150‰ deixou no máximo 2 corridas
/// com contato para uma deriva de até ±0,1 °/s depois da calibração (sem o ajuste: 79 a 0,1 °/s).
constexpr uint16_t AJUSTE_RUMO_PELO_CORREDOR_POR_MIL = 150;
constexpr uint16_t ERRO_LATERAL_MAXIMO_RUMO_MM = 30;
constexpr uint16_t VARIACAO_LATERAL_MAXIMA_RUMO_MM_POR_S = 10;

/// Quanto o rumo de referência pode se afastar do rumo nominal (múltiplos de 90° desde a largada),
/// em mili-graus. A deriva se acumula na corrida inteira (0,075 °/s em 10 minutos dá 45°); a
/// proteção contra uma leitura errada é a exigência das duas paredes e do robô paralelo a elas.
constexpr int32_t AJUSTE_RUMO_MAXIMO_MILIGRAUS = 45000;

// ---------------------------------------------------------------------------------------------
// Parâmetros em uso (os testes alteram os campos para cobrir outros cenários)
// ---------------------------------------------------------------------------------------------

/// Parâmetros da `CorrecaoPelasParedes`.
struct ConfiguracaoCorrecaoParedes {
  uint16_t limiarParedeLateralMm = LIMIAR_PAREDE_CORRECAO_LATERAL_MM;
  uint16_t distanciaSensorLateralAoCentroMm = DISTANCIA_SENSOR_LATERAL_AO_CENTRO_MM;
  uint16_t correcaoLateralMaximaMm = CORRECAO_LATERAL_MAXIMA_MM;
  uint8_t ganhoCorrecaoLateralPercentual = GANHO_CORRECAO_LATERAL_PERCENTUAL;
  /// Correção ao longo do avanço (parede da frente e quinas, a mesma da #380) para os encoders.
  ConfiguracaoDeslocamento deslocamento = ConfiguracaoDeslocamento{};
};

/// Parâmetros do realinhamento e do rumo pelo corredor (`ExecutorMovimentos` e `CicloNavegacao`).
struct ConfiguracaoRealinhamento {
  uint16_t limiarParedeLateralMm = LIMIAR_PAREDE_CORRECAO_LATERAL_MM;
  uint16_t distanciaSensorLateralAoCentroMm = DISTANCIA_SENSOR_LATERAL_AO_CENTRO_MM;
  uint16_t desvioMinimoMm = DESVIO_MINIMO_REALINHAMENTO_MM;
  uint16_t desvioMaximoMm = DESVIO_MAXIMO_REALINHAMENTO_MM;
  uint16_t trechoMaximoLivreMm = TRECHO_MAXIMO_REALINHAMENTO_LIVRE_MM;
  uint16_t folgaMinimaMm = FOLGA_MINIMA_REALINHAMENTO_MM;
  uint8_t tentativas = TENTATIVAS_REALINHAMENTO;
  int16_t potencia = POTENCIA_REALINHAMENTO;
  int16_t ganhoRumoDecimos = GANHO_RUMO_REALINHAMENTO_DECIMOS;
  uint32_t tempoMaximoTrechoMs = TEMPO_MAXIMO_TRECHO_REALINHAMENTO_MS;
  uint16_t ajusteRumoPeloCorredorPorMil = AJUSTE_RUMO_PELO_CORREDOR_POR_MIL;
  uint16_t erroLateralMaximoRumoMm = ERRO_LATERAL_MAXIMO_RUMO_MM;
  uint16_t variacaoLateralMaximaRumoMmPorS = VARIACAO_LATERAL_MAXIMA_RUMO_MM_POR_S;
  int32_t ajusteRumoMaximoMiligraus = AJUSTE_RUMO_MAXIMO_MILIGRAUS;
};

}  // namespace micromouse
