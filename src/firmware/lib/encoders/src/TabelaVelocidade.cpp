#include "TabelaVelocidade.h"

namespace micromouse {

int32_t TabelaVelocidade::velocidadeMmPorS(int16_t potencia) const {
  if (quantidade == 0) {
    return 0;
  }
  const bool recuando = potencia < 0;
  const int32_t modulo = recuando ? -static_cast<int32_t>(potencia) : potencia;
  const uint8_t ultimo = quantidade > MAX_PONTOS_TABELA_VELOCIDADE ? MAX_PONTOS_TABELA_VELOCIDADE - 1 : quantidade - 1;

  int32_t velocidade;
  if (modulo >= pontos[ultimo].potencia) {
    velocidade = pontos[ultimo].velocidadeMmPorS;
  } else {
    uint8_t i = 0;
    while (i + 1 < ultimo && modulo >= pontos[i + 1].potencia) {
      ++i;
    }
    const int32_t p0 = pontos[i].potencia;
    const int32_t p1 = pontos[i + 1].potencia;
    const int32_t v0 = pontos[i].velocidadeMmPorS;
    const int32_t v1 = pontos[i + 1].velocidadeMmPorS;
    const int32_t largura = p1 - p0;
    // Interpolação linear com arredondamento para o inteiro mais próximo.
    const int32_t numerador = (v1 - v0) * (modulo - p0);
    const int32_t acrescimo = largura > 0 ? (numerador >= 0 ? (numerador + largura / 2) / largura
                                                            : (numerador - largura / 2) / largura)
                                          : 0;
    velocidade = v0 + acrescimo;
  }
  return recuando ? -velocidade : velocidade;
}

}  // namespace micromouse
