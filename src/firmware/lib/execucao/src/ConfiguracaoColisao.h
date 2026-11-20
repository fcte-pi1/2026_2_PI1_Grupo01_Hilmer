#pragma once

/// @file ConfiguracaoColisao.h
/// @brief Parâmetros da detecção de colisão e da recuperação (#192).
///
/// ATENÇÃO: todos os valores são PROVISÓRIOS. Foram ajustados na simulação dos testes
/// (`test_colisao`, `test_recuperacao_colisao` e `test_integracao_colisao`). O pico de aceleração de
/// uma batida real e o ruído do acelerômetro com os motores ligados só se medem na bancada: bata o
/// robô devagar contra a parede, veja o desvio lido (`desvio_mg` no log) e ajuste
/// `LIMIAR_IMPACTO_MG` entre o maior desvio andando sem bater e o menor desvio batendo. Para mudar
/// um valor, altere só este arquivo.

#include <stdint.h>

namespace micromouse {

/// O que a detecção percebeu.
enum class TipoColisao : uint8_t {
  Nenhuma,
  Impacto,       ///< Pico de aceleração no plano (IMU).
  FrenteParada   ///< Potência alta e o ToF frontal parado, sem giro: o robô está preso contra algo.
};

/// Nome do tipo para o log: `nenhuma`, `impacto` ou `frente_parada`.
constexpr const char* nomeDoTipoColisao(TipoColisao tipo) {
  return tipo == TipoColisao::Impacto ? "impacto" : tipo == TipoColisao::FrenteParada ? "frente_parada" : "nenhuma";
}

// ---------------------------------------------------------------------------------------------
// 1. Detecção pelo acelerômetro
// ---------------------------------------------------------------------------------------------

/// Variação mínima da aceleração no plano, em mg, em relação ao repouso para valer como impacto.
/// Medido no robô: ao cruzar o vinco entre duas células (onde as paredes são fixadas) o acelerômetro
/// marca ~1,0 g (1024 mg na bancada), e o limiar antigo de 1000 mg tratava isso como batida, fazendo o
/// robô recuar no meio do labirinto. Com o EVA nos vincos a leitura passou de 1,6 g (1610 mg), então o limiar é 2200 mg: o vinco passa e uma batida na parede (acima de 2,2 g,
/// a ~0,7 m/s) continua sendo detectada. Provisório: se uma batida real não for detectada, baixe de
/// 100 em 100 mg; se o vinco ainda disparar, suba.
constexpr uint16_t LIMIAR_IMPACTO_MG = 2200;

/// Leituras seguidas acima do limiar para confirmar o impacto. Uma só (10 ms): o pico de uma batida
/// dura poucas leituras e o filtro do sensor (42 Hz) já tira o ruído mais rápido.
constexpr uint8_t AMOSTRAS_CONFIRMACAO_IMPACTO = 1;

/// Suavização da referência de repouso: a cada leitura calma, ela anda `1 / 2^n` do caminho até a
/// leitura. A referência segue a inclinação do sensor e o desvio do zero sem acompanhar uma batida.
constexpr uint8_t DESLIZAMENTO_REFERENCIA = 4;

// ---------------------------------------------------------------------------------------------
// 2. Recuperação
// ---------------------------------------------------------------------------------------------

/// Distância do recuo, em mm. Do contato com a parede ao centro da célula são cerca de 34 mm (84 mm
/// do centro à parede, menos os 50 mm do sensor frontal até o centro); sobra uma folga para o robô
/// não parar colado. Com a parede da frente à vista, o recuo termina antes, no centro.
constexpr uint16_t RECUO_COLISAO_MM = 40;

/// Menor recuo, em mm, mesmo com o robô já no centro (uma batida no giro, por exemplo): afasta o chassi
/// do que ele tocou.
constexpr uint16_t RECUO_MINIMO_COLISAO_MM = 10;

/// Quanto antes do centro (do centro à parede da frente) o recuo pode terminar, em mm, e quanto
/// depois dele ainda vale como "parede da própria célula". Acima disso a parede vista é a de outra
/// célula e não serve de referência do centro.
constexpr uint16_t TOLERANCIA_CENTRO_RECUO_MM = 3;
constexpr uint16_t FOLGA_PAREDE_DA_CELULA_RECUO_MM = 26;

/// Leitura do ToF frontal, em mm, acima da qual (e abaixo do alcance máximo do sensor, 1200 mm) a
/// frente conta como livre para decidir se uma colisão repetida marca a parede da frente: o impacto
/// veio de outro lado, e não de uma parede. Um ToF que não vê nada não contradiz o impacto.
constexpr uint16_t LIMIAR_FRENTE_LIVRE_COLISAO_MM = 120;

/// Faixa, em mm, de quanto o centro do robô está além do centro da célula (pela odometria, ao longo
/// do avanço) para uma batida valer como a parede da frente da própria célula. Com o robô de
/// 100 mm de comprimento e a parede a 84 mm do centro, o encontrão acontece a cerca de 34 mm do
/// centro. Fora da faixa, o robô está noutro lugar (já na célula seguinte, ou longe da parede) e a
/// parede da frente da célula da navegação não é a que ele bateu.
constexpr uint16_t EXCESSO_MINIMO_PAREDE_MM = 10;
constexpr uint16_t EXCESSO_MAXIMO_PAREDE_MM = 70;

/// Colisões seguidas, sem concluir uma célula entre elas, a partir das quais o ciclo desiste e pára
/// em `Falha`: sem esse limite, um problema que o robô não resolve (uma saliência no piso, por
/// exemplo) o faria bater e recuar para sempre.
constexpr uint8_t COLISOES_SEGUIDAS_MAXIMAS = 4;

/// Parâmetros em uso pelo `DetectorColisao` e pelo `CicloNavegacao`. Os testes alteram os campos
/// para cobrir outros cenários.
struct ConfiguracaoColisao {
  uint16_t limiarImpactoMg = LIMIAR_IMPACTO_MG;
  uint8_t amostrasConfirmacao = AMOSTRAS_CONFIRMACAO_IMPACTO;
  uint8_t deslizamentoReferencia = DESLIZAMENTO_REFERENCIA;
  uint16_t recuoMm = RECUO_COLISAO_MM;
  uint16_t recuoMinimoMm = RECUO_MINIMO_COLISAO_MM;
  uint16_t excessoMinimoParedeMm = EXCESSO_MINIMO_PAREDE_MM;
  uint16_t excessoMaximoParedeMm = EXCESSO_MAXIMO_PAREDE_MM;
  uint16_t limiarFrenteLivreMm = LIMIAR_FRENTE_LIVRE_COLISAO_MM;
  uint8_t colisoesSeguidasMaximas = COLISOES_SEGUIDAS_MAXIMAS;
};

}  // namespace micromouse
