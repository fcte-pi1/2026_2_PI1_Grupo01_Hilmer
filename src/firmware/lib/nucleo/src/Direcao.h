#pragma once

/// @file Direcao.h
/// @brief Operações sobre direções: girar o robô, converter lados relativos em direções absolutas
/// e achar a célula vizinha.
///
/// As funções de giro e de conversão são `constexpr`: podem ser avaliadas em tempo de compilação
/// e não têm efeitos colaterais. Dependem da ordem horária de `Direcao`
/// (Norte, Leste, Sul, Oeste), usando soma módulo 4 (`& 3`).

#include "Tipos.h"

namespace micromouse {

/// Direção resultante de um giro de 90° para a direita (sentido horário).
/// @param d Direção atual.
/// @return Próxima direção no sentido horário (ex.: Norte → Leste).
constexpr Direcao girarDireita(Direcao d) {
  return static_cast<Direcao>((static_cast<uint8_t>(d) + 1) & 3);
}

/// Direção resultante de um giro de 90° para a esquerda (sentido anti-horário).
/// @param d Direção atual.
/// @return Próxima direção no sentido anti-horário (ex.: Norte → Oeste).
constexpr Direcao girarEsquerda(Direcao d) {
  return static_cast<Direcao>((static_cast<uint8_t>(d) + 3) & 3);
}

/// Direção oposta, equivalente a um giro de 180°.
/// @param d Direção atual.
/// @return Direção oposta (ex.: Norte → Sul).
constexpr Direcao oposta(Direcao d) {
  return static_cast<Direcao>((static_cast<uint8_t>(d) + 2) & 3);
}

/// Converte um lado relativo ao robô (frente, esquerda, direita) na direção absoluta do labirinto.
///
/// Usada para gravar no mapa as paredes lidas pelo LiDAR: com o robô virado para
/// Leste, a parede à esquerda está ao Norte da célula.
/// @param direcaoDoRobo Direção para a qual o robô está virado.
/// @param lado Lado relativo ao robô.
/// @return Direção absoluta correspondente.
constexpr Direcao direcaoAbsoluta(Direcao direcaoDoRobo, Lado lado) {
  return lado == Lado::Frente     ? direcaoDoRobo
         : lado == Lado::Esquerda ? girarEsquerda(direcaoDoRobo)
                                  : girarDireita(direcaoDoRobo);
}

/// Letra da direção na telemetria e no banco (coluna `orientacao` da tabela `telemetria`):
/// `'N'`, `'L'`, `'S'` ou `'O'`.
/// @param d Direção.
/// @return A letra correspondente (Leste é `'L'` e Oeste é `'O'`).
constexpr char letraDaDirecao(Direcao d) {
  return d == Direcao::Norte ? 'N' : d == Direcao::Leste ? 'L' : d == Direcao::Sul ? 'S' : 'O';
}

/// Calcula a célula vizinha de `origem` na direção `d`.
///
/// Convenção de eixos: Norte aumenta a `linha` e Leste aumenta a `coluna`, com a partida
/// (0, 0) no canto inferior esquerdo. A função não conhece os limites do labirinto: quem
/// chama confere se o destino está dentro do mapa (por exemplo, com `Mapa::posValida`).
/// @param origem Célula de partida.
/// @param d Direção do deslocamento.
/// @param[out] destino Recebe a vizinha quando a função retorna `true`; não muda se retorna `false`.
/// @return `false` se o destino sairia do intervalo de `uint8_t` (por exemplo, ao sul da linha 0).
inline bool vizinha(PosicaoCelula origem, Direcao d, PosicaoCelula& destino) {
  int linha = origem.linha;
  int coluna = origem.coluna;
  switch (d) {
    case Direcao::Norte:
      linha++;
      break;
    case Direcao::Leste:
      coluna++;
      break;
    case Direcao::Sul:
      linha--;
      break;
    case Direcao::Oeste:
      coluna--;
      break;
  }
  if (linha < 0 || linha > UINT8_MAX || coluna < 0 || coluna > UINT8_MAX) {
    return false;
  }
  destino = PosicaoCelula{static_cast<uint8_t>(linha), static_cast<uint8_t>(coluna)};
  return true;
}

}  // namespace micromouse
