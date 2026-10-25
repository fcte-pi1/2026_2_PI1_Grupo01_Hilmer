#pragma once

/// @file Movimento.h
/// @brief Os comandos de movimento entre células (#184) e a conversão da direção decidida pelo
/// algoritmo de resolução em giro.

#include <stdint.h>

#include "ControleGiro.h"
#include "Direcao.h"
#include "Tipos.h"

namespace micromouse {

/// Comando que o `ExecutorMovimentos` executa, um de cada vez.
enum class Movimento : uint8_t {
  AvancarCelula,    ///< Anda até o centro da próxima célula (18 cm).
  Girar90Esquerda,  ///< Gira 90° no lugar, no sentido anti-horário.
  Girar90Direita,   ///< Gira 90° no lugar, no sentido horário.
  Girar180,         ///< Meia volta no lugar (pela esquerda).
  Realinhar         ///< Desloca o robô para o lado até o centro do corredor, sem mudar o rumo (#188).
};

/// `true` para os três giros.
constexpr bool ehGiro(Movimento movimento) {
  return movimento == Movimento::Girar90Esquerda || movimento == Movimento::Girar90Direita ||
         movimento == Movimento::Girar180;
}

/// `true` para os giros e o realinhamento: movimentos em que as leituras laterais e da frente não
/// servem de referência de posição.
constexpr bool ehManobra(Movimento movimento) { return movimento != Movimento::AvancarCelula; }

/// Quanto o giro muda o rumo, em mili-graus, na convenção do giroscópio (positivo = esquerda).
/// Zero para o avanço.
constexpr int32_t variacaoRumoMiligraus(Movimento movimento) {
  return movimento == Movimento::Girar90Esquerda  ? ANGULO_GIRO_90_MILIGRAUS
         : movimento == Movimento::Girar90Direita ? -ANGULO_GIRO_90_MILIGRAUS
         : movimento == Movimento::Girar180       ? ANGULO_GIRO_180_MILIGRAUS
                                                  : 0;
}

/// Direção do robô depois do movimento (o avanço não muda a direção).
constexpr Direcao direcaoDepois(Direcao atual, Movimento movimento) {
  return movimento == Movimento::Girar90Esquerda  ? girarEsquerda(atual)
         : movimento == Movimento::Girar90Direita ? girarDireita(atual)
         : movimento == Movimento::Girar180       ? oposta(atual)
                                                  : atual;
}

/// Giro que leva o robô de `atual` para `desejada`.
/// @param[out] giro Recebe o giro quando a função retorna `true`; não muda se retorna `false`.
/// @return `false` se o robô já está virado para `desejada` (não precisa girar).
inline bool giroAte(Direcao atual, Direcao desejada, Movimento& giro) {
  if (desejada == atual) {
    return false;
  }
  giro = desejada == girarDireita(atual)    ? Movimento::Girar90Direita
         : desejada == girarEsquerda(atual) ? Movimento::Girar90Esquerda
                                            : Movimento::Girar180;
  return true;
}

/// Nome do movimento para o log: `avancar_celula`, `girar_90_esquerda`, `girar_90_direita`,
/// `girar_180` ou `realinhar`.
constexpr const char* nomeDoMovimento(Movimento movimento) {
  return movimento == Movimento::AvancarCelula     ? "avancar_celula"
         : movimento == Movimento::Girar90Esquerda ? "girar_90_esquerda"
         : movimento == Movimento::Girar90Direita  ? "girar_90_direita"
         : movimento == Movimento::Girar180        ? "girar_180"
                                                   : "realinhar";
}

}  // namespace micromouse
