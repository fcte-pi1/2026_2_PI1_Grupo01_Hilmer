#pragma once

/// @file ConfiguracaoProtecaoMotor.h
/// @brief Parâmetros da proteção contra motor travado (#341).
///
/// ATENÇÃO: os valores abaixo são PROVISÓRIOS. A eletrônica ainda não informou a corrente de
/// travamento do N20, a tensão do VM do driver nem a redução do motor, e nada foi medido com o robô
/// montado. Referência para os limites: o DRV8833 aguenta 1 A contínuo e 2 A de pico por ponte H,
/// então um motor preso em duty alto precisa ser cortado em frações de segundo. Ajuste na bancada:
/// prenda uma roda com a mão, com o robô em potência de cruzeiro, e confira se o corte vem antes
/// de o driver esquentar. Para mudar um valor, altere só este arquivo.
///
/// Sem encoder (#380), a roda parada não aparece nas contagens. O robô é considerado travado
/// quando recebe potência alta por um tempo e **nem** o ToF frontal **nem** o giroscópio
/// acusam movimento.

#include <stdint.h>

namespace micromouse {

/// Menor `|potência|` de uma das rodas, em milésimos da total, que conta como "duty alto". Abaixo
/// disso o motor pode ficar parado sem estar travado (zona morta do N20, `potenciaMinima`).
constexpr int16_t POTENCIA_ALTA_MINIMA = 300;

/// Tempo contínuo, em ms, com potência alta e sem movimento até o robô ser considerado travado.
/// Curto o bastante para proteger o driver; longo o bastante para a partida do robô parado.
constexpr uint32_t TEMPO_LIMITE_TRAVADO_MS = 500;

/// Variação mínima da leitura do ToF frontal, em mm, dentro do tempo limite, para o robô contar
/// como em movimento. Acima do ruído do VL53L0X (cerca de 3 mm).
constexpr uint16_t VARIACAO_MINIMA_FRENTE_MM = 5;

/// Taxa de giro mínima, em décimos de grau por segundo (50 = 5 °/s), para o robô contar como
/// girando. Acima do ruído do giroscópio parado.
constexpr uint16_t TAXA_GIRO_MINIMA_DECIMOS_DPS = 50;

/// Parâmetros em uso pela `ProtecaoMotorTravado`. Os valores padrão vêm das constantes acima;
/// os testes alteram os campos para cobrir outros cenários.
struct ConfiguracaoProtecaoMotor {
  int16_t potenciaAltaMinima = POTENCIA_ALTA_MINIMA;
  uint32_t tempoLimiteMs = TEMPO_LIMITE_TRAVADO_MS;
  uint16_t variacaoMinimaFrenteMm = VARIACAO_MINIMA_FRENTE_MM;
  uint16_t taxaGiroMinimaDecimosDps = TAXA_GIRO_MINIMA_DECIMOS_DPS;
};

}  // namespace micromouse
