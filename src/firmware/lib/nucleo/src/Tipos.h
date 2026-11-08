#pragma once

/// @file Tipos.h
/// @brief Tipos básicos compartilhados por todas as camadas do firmware.
///
/// Não depende do Arduino, para poder ser compilado e testado no computador
/// (ambiente `native`).

#include <stdint.h>

namespace micromouse {

/// Direção absoluta no labirinto.
///
/// A ordem dos valores segue o sentido horário (Norte → Leste → Sul → Oeste).
/// As funções de `Direcao.h` dependem dessa ordem para girar o robô com
/// aritmética módulo 4; não reordene os valores.
///
/// Convenção de eixos: Norte aumenta a `linha` e Leste aumenta a `coluna` de `PosicaoCelula`,
/// com a partida (0, 0) no canto inferior esquerdo (veja `vizinha()` em `Direcao.h`).
enum class Direcao : uint8_t { Norte, Leste, Sul, Oeste };

/// Lado relativo ao robô, usado nas leituras dos sensores de distância.
///
/// Para converter um lado em direção do labirinto, use `direcaoAbsoluta()`.
enum class Lado : uint8_t { Frente, Esquerda, Direita };

/// Estado conhecido de uma parede do labirinto.
///
/// `Desconhecido` indica uma parede que o robô ainda não observou (HU-02, HU-03).
enum class EstadoParede : uint8_t { Desconhecido, Livre, Parede };

/// Tamanho do labirinto, entre os três da competição.
///
/// Os nomes seguem a notação "colunas × linhas": `Labirinto12x4` tem 12 colunas e 4 linhas
/// (216 × 72 cm). `Labirinto4x4` mede 72 × 72 cm e `Labirinto8x4`, 144 × 72 cm.
enum class DimensaoMapa : uint8_t { Labirinto4x4, Labirinto8x4, Labirinto12x4 };

/// Posição discreta de uma célula do labirinto.
///
/// Os limites são dados por `MAX_LINHAS_LABIRINTO` e `MAX_COLUNAS_LABIRINTO`
/// (`Configuracao.h`). A célula de partida é a origem (0, 0) (HU-05).
struct PosicaoCelula {
  uint8_t linha;   ///< Linha da célula, a partir de 0.
  uint8_t coluna;  ///< Coluna da célula, a partir de 0.
};

/// Localização do robô: em qual célula está e para qual direção está virado.
struct Pose {
  PosicaoCelula celula;  ///< Célula ocupada pelo robô.
  Direcao direcao;       ///< Direção para a qual a frente do robô aponta.
};

/// Distâncias medidas pelos sensores nas três direções relativas ao robô.
///
/// Os valores estão em milímetros. É a entrada da classificação `parede`/`livre`
/// da camada de percepção (HU-02).
struct DistanciasLaterais {
  uint16_t frenteMm;    ///< Distância até o obstáculo à frente, em mm.
  uint16_t esquerdaMm;  ///< Distância até o obstáculo à esquerda, em mm.
  uint16_t direitaMm;   ///< Distância até o obstáculo à direita, em mm.
};

}  // namespace micromouse
