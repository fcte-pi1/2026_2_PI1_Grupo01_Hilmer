#pragma once

/// @file Mapa.h
/// @brief Mapa do labirinto: matriz de células com o estado das quatro paredes de cada uma.
///
/// Convenções, as mesmas de `Tipos.h` e `Direcao.h`: a célula de partida é (0, 0), no canto
/// inferior esquerdo; Norte aumenta a `linha` e Leste aumenta a `coluna`.
///
/// Não depende do Arduino e não aloca memória dinâmica: a matriz tem tamanho fixo
/// (`MAX_LINHAS_LABIRINTO` × `MAX_COLUNAS_LABIRINTO`).

#include <cstdint>

#include "Configuracao.h"
#include "Tipos.h"

namespace micromouse {

/// Resultado de `Mapa::registrarParede`.
enum class ResultadoRegistro : uint8_t {
  Registrada,   ///< A parede era desconhecida e foi gravada.
  JaConhecida,  ///< A parede já tinha exatamente esse estado; nada mudou.
  Conflito,     ///< A parede já tem outro estado conhecido; nada foi alterado.
  Invalida      ///< Célula fora do mapa ou estado `Desconhecido`.
};

/// Mapa de um labirinto de até `MAX_LINHAS_LABIRINTO` × `MAX_COLUNAS_LABIRINTO` células.
///
/// Cada célula guarda o estado (`EstadoParede`) das suas quatro paredes. Paredes ainda não
/// observadas ficam `Desconhecido`.
/// O perímetro já é conhecido na criação: toda parede externa começa como `Parede`.
/// Uma parede entre duas células é a mesma vista dos dois lados: registrar o lado leste de
/// uma célula grava também o lado oeste da vizinha (e o mesmo para norte e sul).
class Mapa {
 public:
  /// Estado das quatro paredes de uma célula.
  struct Celula {
    EstadoParede norte = EstadoParede::Desconhecido;
    EstadoParede sul = EstadoParede::Desconhecido;
    EstadoParede leste = EstadoParede::Desconhecido;
    EstadoParede oeste = EstadoParede::Desconhecido;
  };

  /// Cria um mapa vazio com o tamanho informado.
  /// @param dimensao Tamanho do labirinto (padrão: 4×4).
  explicit Mapa(DimensaoMapa dimensao = DimensaoMapa::Labirinto4x4);

  /// Tamanho informado na criação.
  DimensaoMapa obterDimensao() const { return dimensao; }

  /// Número de linhas (eixo Norte-Sul).
  uint8_t obterLinhas() const { return linhas; }

  /// Número de colunas (eixo Leste-Oeste).
  uint8_t obterColunas() const { return colunas; }

  /// `true` se a célula está dentro do mapa.
  bool posValida(PosicaoCelula posicao) const;

  /// Estado das quatro paredes de uma célula; todas `Desconhecido` se a célula está fora do mapa.
  Celula obterCelula(PosicaoCelula posicao) const;

  /// Estado de uma parede; `Desconhecido` se ela não foi observada ou a célula está fora do mapa.
  EstadoParede obterParede(PosicaoCelula posicao, Direcao direcao) const;

  /// Registra o estado de uma parede observada.
  ///
  /// A parede também é gravada na célula vizinha (lado oposto), quando ela existe.
  ///
  /// Não sobrescreve um estado já conhecido: se a parede já tem outro estado, o mapa não muda e
  /// o retorno é `Conflito`. A política para esse caso fica com quem chama.
  /// @param posicao Célula da parede.
  /// @param direcao Lado da célula (norte, leste, sul ou oeste).
  /// @param estado `Parede` ou `Livre`; `Desconhecido` não pode ser registrado.
  /// @return O que aconteceu com o registro.
  ResultadoRegistro registrarParede(PosicaoCelula posicao, Direcao direcao, EstadoParede estado);

  /// Grava o estado de uma parede mesmo que ela já tenha outro estado conhecido (nos dois lados).
  ///
  /// Para quando quem chama tem uma evidência mais forte que a observação anterior: por exemplo, o
  /// robô parou numa parede que a percepção tinha classificado como livre (#184). Sem isso, o
  /// Flood Fill planejaria pela mesma parede para sempre.
  /// @return `Registrada` se o estado mudou, `JaConhecida` se já era esse, `Invalida` como em
  ///         `registrarParede` ou para `Livre` no perímetro (que é sempre parede).
  ResultadoRegistro corrigirParede(PosicaoCelula posicao, Direcao direcao, EstadoParede estado);

  /// Dois mapas são iguais quando têm o mesmo tamanho e o mesmo estado em todas as paredes.
  bool operator==(const Mapa& outro) const;
  bool operator!=(const Mapa& outro) const;

 private:
  DimensaoMapa dimensao;
  uint8_t linhas;
  uint8_t colunas;
  Celula celulas[MAX_LINHAS_LABIRINTO][MAX_COLUNAS_LABIRINTO];

  void preencherPerimetro();
};

}  // namespace micromouse
