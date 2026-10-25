#pragma once

/// @file ConfiguracaoDeslocamento.h
/// @brief TODOS os valores ajustáveis do deslocamento estimado sem encoder (#380), num só lugar.
///
/// =============================================================================================
///  ATENÇÃO: todos os valores abaixo são PROVISÓRIOS. Nenhum foi medido com o robô montado.
///  Para ajustar o modo sem encoder, mexa SÓ neste arquivo. Cada valor diz o que faz, de onde
///  veio e como medir o valor real na bancada.
/// =============================================================================================
///
/// Como o modo funciona, em uma frase: o robô não tem encoder, então a distância percorrida é
/// estimada pela velocidade que cada potência deveria dar (tabela abaixo) vezes o tempo, e essa
/// estimativa é corrigida pelos sensores ToF sempre que uma parede dá uma referência de posição.

#include <stdint.h>

#include "ConfiguracaoEncoder.h"

namespace micromouse {

// ---------------------------------------------------------------------------------------------
// 1. Os encoders estão instalados?
// ---------------------------------------------------------------------------------------------

/// `false`: a odometria usa o deslocamento ESTIMADO (este arquivo). `true`: usa os encoders reais
/// (`EncoderEsp32`, GPIO4 a 7), e o resto deste arquivo deixa de ser usado.
/// Troque para `true` quando os encoders forem montados; a `Odometria` e o `ControleMovimento` não
/// mudam com a troca.
constexpr bool ENCODERS_INSTALADOS = false;

// ---------------------------------------------------------------------------------------------
// 2. Tabela de velocidade: quanto o robô anda para cada potência comandada
// ---------------------------------------------------------------------------------------------

/// Um ponto da tabela: com `potencia` comandada (0 a 1000, a escala do `IMotores`), a roda anda a
/// `velocidadeMmPorS` milímetros por segundo.
struct PontoVelocidade {
  int16_t potencia;
  uint16_t velocidadeMmPorS;
};

/// Velocidade de cada roda em função da potência comandada, em ordem crescente de potência.
///
/// - Abaixo do primeiro ponto, a roda fica parada (zona morta do motor N20: a potência não vence o
///   atrito). Por isso o primeiro ponto é a menor potência que ainda move o robô.
/// - Entre dois pontos, a velocidade é interpolada em linha reta.
/// - Acima do último ponto, vale a velocidade do último.
/// - Potência negativa (ré) usa a mesma tabela, com o sinal trocado.
///
/// De onde veio: o mesmo modelo da simulação do `ControleMovimento` (#191), 0,3 mm/s por unidade
/// de potência, com zona morta abaixo de 150 (`potenciaMinima` de `ConfiguracaoControle.h`). Nada
/// foi medido: a tensão do VM e o modelo do N20 ainda não foram informados (#341).
///
/// Como medir: com o robô no chão, comande a mesma potência nas duas rodas por 2 s, meça com uma
/// régua quanto ele andou e divida por 2. Repita para cada ponto (comece pela zona morta: a menor
/// potência em que ele se move). Se o robô andar mais (ou menos) do que a odometria diz, é aqui
/// que se corrige.
///
/// Meça só com o limite de duty da #341 valendo (`LIMITE_DUTY_MOTORES_MILESIMOS`, provisório em
/// 70%, no `MotoresDrv8833`): com ele, a potência 1000 vira duty 716 de 1023, e as menores ficam
/// proporcionais. Medida antes dele, a tabela dá mm/s acima dos reais. Se o limite mudar, meça de
/// novo.
constexpr PontoVelocidade TABELA_VELOCIDADE[] = {
    {150, 45},
    {400, 120},
    {700, 210},
    {1000, 300},
};

// ---------------------------------------------------------------------------------------------
// 3. Onde ficam os sensores ToF no chassi (usados nas correções)
// ---------------------------------------------------------------------------------------------

/// Distância, ao longo do eixo de avanço, da face do sensor FRONTAL até o centro do robô, em mm.
/// A distância à parede da frente, medida do centro, é a leitura do sensor mais este valor.
/// De onde veio: metade do comprimento do chassi (100 mm), supondo o sensor na ponta da frente.
/// Como medir: régua do centro entre as rodas até a face do sensor frontal.
constexpr uint16_t DISTANCIA_SENSOR_FRONTAL_AO_CENTRO_MM = 50;

/// Quanto os sensores LATERAIS ficam à frente do centro do robô, ao longo do eixo de avanço, em mm
/// (negativo se ficarem atrás). Define onde o robô está quando um sensor lateral passa pela quina
/// de uma parede. De onde veio: suposição de sensores alinhados ao centro. Como medir: régua do
/// centro entre as rodas até o eixo do sensor lateral, medida para a frente.
constexpr int16_t AVANCO_SENSORES_LATERAIS_MM = 0;

/// Distância, para o lado, do centro do robô até a face de cada sensor LATERAL, em mm. A distância à
/// parede lateral, medida do centro, é a leitura do sensor mais este valor. De onde veio: o recuo
/// da percepção (`ConfiguracaoPercepcao.h`: 84 − 30 = 54 mm lidos com parede), usado no controle do
/// avanço (#184) e na correção lateral pelas paredes (#188). Como medir: régua do centro entre as
/// rodas até a face de cada sensor lateral.
constexpr uint16_t DISTANCIA_SENSOR_LATERAL_AO_CENTRO_MM = 30;

// ---------------------------------------------------------------------------------------------
// 4. Correção pela parede da frente
// ---------------------------------------------------------------------------------------------

/// Só corrige pela parede da frente quando o sensor frontal lê até esta distância, em mm. Mais
/// longe, o erro do sensor cresce e a parede pode estar várias células à frente.
/// De onde veio: uma célula e meia (o sensor enxerga a parede da célula seguinte).
constexpr uint16_t DISTANCIA_FRONTAL_MAXIMA_PARA_CORRIGIR_MM = 300;

// ---------------------------------------------------------------------------------------------
// 5. Correção pelas aberturas laterais (quina da parede na fronteira de célula)
// ---------------------------------------------------------------------------------------------

/// Leitura lateral, em mm a partir da face do sensor, até a qual o lado conta como PAREDE.
/// De onde veio: com o robô no centro do corredor, a parede fica a cerca de 24 mm de um sensor
/// na borda do chassi, e uma abertura lê pelo menos uma célula. 120 mm fica entre os dois.
/// Quando a percepção para ToF (#379) definir os limiares dela, use o mesmo valor aqui.
constexpr uint16_t LIMIAR_PAREDE_LATERAL_MM = 120;

/// Folga acima do limiar para o lado passar a contar como LIVRE, em mm (histerese). Entre
/// `LIMIAR_PAREDE_LATERAL_MM` e esse limiar mais a folga, o lado mantém o estado anterior, para uma
/// leitura com ruído não inventar uma quina.
constexpr uint16_t HISTERESE_PAREDE_LATERAL_MM = 40;

// ---------------------------------------------------------------------------------------------
// 6. Segurança das correções e confiança da pose
// ---------------------------------------------------------------------------------------------

/// Maior correção aceita de uma só vez, em mm. Uma correção maior indica leitura errada (sensor
/// vendo outra coisa) e é descartada. Precisa ser maior que a deriva esperada entre duas paredes
/// de referência, e menor que meia célula (90 mm), para não pular de célula.
constexpr uint16_t CORRECAO_MAXIMA_MM = 70;

/// Distância que o robô pode andar sem NENHUMA correção pelo ToF antes de a pose deixar de ser
/// confiável, em mm. Passando disso, a telemetria informa `pose_valida: false` até a próxima
/// correção.
/// De onde veio: duas células (360 mm), supondo erro da tabela de até ~20%, o que dá ~70 mm, ainda
/// abaixo de meia célula. Aumente se a tabela for calibrada e o robô errar pouco; diminua se ele
/// errar de célula antes disso.
constexpr uint16_t DISTANCIA_MAXIMA_SEM_CORRECAO_MM = 360;

// ---------------------------------------------------------------------------------------------
// Parâmetros em uso (os testes alteram os campos para cobrir outros cenários)
// ---------------------------------------------------------------------------------------------

/// Quantidade de pontos de `TABELA_VELOCIDADE`.
constexpr uint8_t PONTOS_TABELA_VELOCIDADE =
    sizeof(TABELA_VELOCIDADE) / sizeof(TABELA_VELOCIDADE[0]);

/// Parâmetros usados pelo `EstimadorDeslocamento` e pelo `CorretorDeslocamentoTof`. Os valores
/// padrão vêm das constantes acima.
struct ConfiguracaoDeslocamento {
  const PontoVelocidade* tabela = TABELA_VELOCIDADE;
  uint8_t pontosTabela = PONTOS_TABELA_VELOCIDADE;
  /// Unidade dos pulsos estimados. Precisa ser a mesma da `ConfiguracaoOdometria`, que converte
  /// os pulsos de volta em distância; com encoder estimado, o valor em si não importa.
  uint32_t micrometrosPorPulso = MICROMETROS_POR_PULSO_PROVISORIO;
  uint16_t distanciaSensorFrontalAoCentroMm = DISTANCIA_SENSOR_FRONTAL_AO_CENTRO_MM;
  int16_t avancoSensoresLateraisMm = AVANCO_SENSORES_LATERAIS_MM;
  uint16_t distanciaFrontalMaximaParaCorrigirMm = DISTANCIA_FRONTAL_MAXIMA_PARA_CORRIGIR_MM;
  uint16_t limiarParedeLateralMm = LIMIAR_PAREDE_LATERAL_MM;
  uint16_t histereseParedeLateralMm = HISTERESE_PAREDE_LATERAL_MM;
  uint16_t correcaoMaximaMm = CORRECAO_MAXIMA_MM;
  uint16_t distanciaMaximaSemCorrecaoMm = DISTANCIA_MAXIMA_SEM_CORRECAO_MM;
};

}  // namespace micromouse
