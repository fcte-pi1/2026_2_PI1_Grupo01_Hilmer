#pragma once

/// @file LabirintoSimulado.h
/// @brief Labirinto conhecido (verdade de referência) para simular o LiDAR sem hardware.

#include <stdint.h>

#include "Configuracao.h"
#include "Tipos.h"

namespace micromouse {

/// Resultado de `LabirintoSimulado::carregar`.
enum class ResultadoCarga : uint8_t {
  Carregado,          ///< Texto válido; o labirinto passa a ser este.
  TamanhoInvalido,    ///< Não corresponde a 4×4, 8×4 ou 12×4, ou as linhas têm larguras diferentes.
  CaractereInvalido,  ///< Caractere fora do formato (veja o formato em `LabirintoSimulado`).
  ParedeIncompleta,   ///< Trecho de parede pela metade, como `- -` ou `-  `.
  PerimetroAberto     ///< Falta parede na borda externa do labirinto.
};

/// Labirinto da competição com todas as paredes conhecidas, usado como verdade de referência
/// nos testes: o `LidarSimulado` deriva dele o que o sensor veria em cada posição (HU-02).
///
/// **Formato do texto (ASCII).** Cada célula ocupa 4 colunas de texto e 2 linhas; `+` marca os
/// cantos, `---` uma parede horizontal e `|` uma parede vertical. Espaço é passagem livre.
/// A primeira linha do texto é a borda Norte, e a célula de partida (0, 0) é a do canto
/// inferior esquerdo, com as mesmas convenções de `Tipos.h`, `Direcao.h` e `Mapa`:
/// Norte aumenta a `linha` e Leste aumenta a `coluna`.
///
/// @code
/// +---+---+---+---+   <- borda Norte (linha 3 em cima)
/// |       |       |
/// +   +---+   +   +
/// |   |       |   |
/// +   +   +---+   +
/// |       |       |
/// +   +---+   +---+
/// |   |           |   <- linha 0; a célula (0, 0) é a primeira desta linha
/// +---+---+---+---+
/// @endcode
///
/// O texto tem `2 × linhas + 1` linhas de `4 × colunas + 1` caracteres, separadas por `\n`
/// (o `\n` final é opcional). Como cada parede aparece uma única vez no texto, ela é a mesma
/// vista das duas células vizinhas: o lado leste de uma célula e o lado oeste da vizinha sempre
/// concordam.
///
/// **Distâncias.** O robô fica no centro da célula: a parede da própria célula está a
/// `DISTANCIA_PAREDE_IMEDIATA_MM` (90 mm), e cada célula livre no caminho soma
/// `TAMANHO_CELULA_MM` (180 mm).
///
/// Não depende do Arduino e não aloca memória dinâmica.
///
/// Exemplo:
/// @code
/// LabirintoSimulado labirinto;
/// labirinto.carregar(texto);                        // devolve ResultadoCarga::Carregado
/// labirinto.temParede({0, 0}, Direcao::Leste);      // true no labirinto acima
/// DistanciasLaterais d;
/// labirinto.distancias({{0, 0}, Direcao::Norte}, d);  // d = {630, 90, 90}
/// @endcode
class LabirintoSimulado {
 public:
  /// Distância do centro da célula até a parede da própria célula, em mm.
  static constexpr uint16_t DISTANCIA_PAREDE_IMEDIATA_MM = TAMANHO_CELULA_MM / 2;

  /// Lê o labirinto a partir do texto no formato ASCII descrito acima.
  /// Se o texto for inválido, o labirinto anterior é mantido.
  /// @param texto Texto terminado em `\0`.
  /// @return `Carregado` ou o motivo da recusa.
  ResultadoCarga carregar(const char* texto);

  /// `true` depois de um `carregar` bem-sucedido.
  bool carregado() const;

  /// Tamanho do labirinto carregado.
  DimensaoMapa obterDimensao() const;

  /// Número de linhas (eixo Norte-Sul).
  uint8_t obterLinhas() const;

  /// Número de colunas (eixo Leste-Oeste).
  uint8_t obterColunas() const;

  /// `true` se a célula está dentro do labirinto carregado.
  bool posValida(PosicaoCelula celula) const;

  /// Consulta se há parede num lado de uma célula.
  /// @return `true` se há parede; também `true` para células fora do labirinto.
  bool temParede(PosicaoCelula celula, Direcao direcao) const;

  /// Distâncias que o LiDAR mediria com o robô no centro da célula, virado para `pose.direcao`.
  /// @param pose Célula e direção do robô.
  /// @param[out] saida Distâncias à frente, à esquerda e à direita, em mm; não muda se a função
  ///             retorna `false`.
  /// @return `false` se nenhum labirinto foi carregado ou a célula está fora dele.
  bool distancias(const Pose& pose, DistanciasLaterais& saida) const;

 private:
  /// Paredes horizontais (Norte/Sul): `[borda][coluna]`, com a borda 0 no Sul.
  bool paredesHorizontais_[MAX_LINHAS_LABIRINTO + 1][MAX_COLUNAS_LABIRINTO] = {};
  /// Paredes verticais (Leste/Oeste): `[linha][borda]`, com a borda 0 no Oeste.
  bool paredesVerticais_[MAX_LINHAS_LABIRINTO][MAX_COLUNAS_LABIRINTO + 1] = {};
  DimensaoMapa dimensao_ = DimensaoMapa::Labirinto4x4;
  uint8_t linhas_ = 0;
  uint8_t colunas_ = 0;
};

}  // namespace micromouse
