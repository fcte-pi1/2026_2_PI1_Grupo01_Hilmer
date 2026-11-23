#pragma once

/// @file LocalizadorLargada.h
/// @brief Descobre onde o robô foi colocado e o tamanho do labirinto, pelas paredes que ele vê
/// explorando (largada em qualquer célula).

#include <stdint.h>

#include "Configuracao.h"
#include "Mapa.h"
#include "Tipos.h"

namespace micromouse {

/// Uma explicação possível do que o robô viu: a célula real da largada e o tamanho do labirinto.
/// O labirinto tem sempre 4 linhas; as colunas são 4, 8 ou 12.
struct HipoteseLargada {
  uint8_t linha;    ///< Linha real da célula da largada (0 a 3).
  uint8_t coluna;   ///< Coluna real da célula da largada (0 a `colunas` − 1).
  uint8_t colunas;  ///< Colunas do labirinto: 4, 8 ou 12.
};

/// Dimensão do labirinto com `colunas` colunas (4, 8 ou 12).
DimensaoMapa dimensaoComColunas(uint8_t colunas);

/// Mantém as hipóteses de largada que ainda explicam o que o robô viu.
///
/// **Referencial.** Sem saber onde está, o robô começa na célula `LARGADA_NA_EXPLORACAO` (3, 11),
/// virado para o Norte, de um mapa de exploração de 7 × 23 células (`DimensaoMapa::Exploracao`). Uma
/// hipótese põe o labirinto real dentro desse mapa: a célula real (l, c) é a célula de exploração
/// (l + `deslocamentoLinhas`, c + `deslocamentoColunas`). O robô precisa ter sido colocado virado para
/// o Norte do labirinto: com a direção livre, um labirinto girado mostra as mesmas paredes, e nenhum
/// robô distinguiria um canto do outro.
///
/// **Hipóteses.** No começo, todas: 4 linhas × (4 + 8 + 12) colunas = 96. Cada `atualizar()` descarta
/// as que contradizem o mapa de exploração e as células visitadas:
/// 1. **Visitadas dentro.** Toda célula em que o robô esteve fica dentro do labirinto da hipótese.
/// 2. **Perímetro fechado.** Nenhuma parede vista como `Livre` fica no perímetro da hipótese (o
///    perímetro é sempre parede).
/// 3. **Tudo alcançável.** Toda célula do labirinto da hipótese é alcançável a partir da largada,
///    passando só por paredes não vistas como `Parede`. Um labirinto de micromouse não tem célula
///    isolada; se o robô fechou uma região, o labirinto não pode ser maior que ela.
///
/// As duas primeiras são certezas; a terceira supõe o labirinto todo conectado. Se ela descartar todas
/// as hipóteses (um labirinto montado com uma célula isolada), o localizador volta a usar só as duas
/// primeiras (`conectividadeDescartada()`).
///
/// **Objetivo.** O objetivo de uma hipótese é o canto Nordeste do labirinto, (3, colunas − 1).
/// `objetivos()` lista os objetivos das hipóteses vivas no referencial da exploração: é para onde o
/// robô vai enquanto não se localiza (veja `CicloNavegacao`). Visitar a célula de um objetivo descarta
/// as hipóteses cujo labirinto não a contém, inclusive as de todo objetivo ao Sul ou a Oeste dela. Por
/// isso, dois objetivos distintos nunca sobrevivem os dois a uma visita a cada um.
///
/// Sem Arduino e sem alocação: testado no ambiente `native`.
class LocalizadorLargada {
 public:
  /// Célula da largada no mapa de exploração: o meio, para caber o labirinto real em qualquer posição.
  static constexpr PosicaoCelula LARGADA_NA_EXPLORACAO = {MAX_LINHAS_LABIRINTO - 1, MAX_COLUNAS_LABIRINTO - 1};

  /// Linhas de todo labirinto da competição.
  static constexpr uint8_t LINHAS_LABIRINTO = 4;

  /// Maior número de hipóteses: 4 linhas × (4 + 8 + 12) colunas.
  static constexpr uint8_t MAX_HIPOTESES = 96;

  LocalizadorLargada() { reiniciar(); }

  /// Volta a todas as hipóteses e esquece as células visitadas.
  void reiniciar();

  /// Registra que o robô esteve na célula (no referencial da exploração).
  void visitar(PosicaoCelula celula);

  /// Descarta as hipóteses que contradizem o mapa de exploração e as células visitadas.
  void atualizar(const Mapa& exploracao);

  /// Hipóteses ainda vivas.
  uint8_t quantidade() const { return quantidade_; }

  /// `true` quando só sobra uma hipótese: o robô sabe onde está e o tamanho do labirinto.
  bool resolvido() const { return quantidade_ == 1; }

  /// Hipótese viva `i` (0 a `quantidade()` − 1).
  HipoteseLargada hipotese(uint8_t i) const { return hipoteses_[i]; }

  /// `true` se a regra 3 (tudo alcançável) descartou todas as hipóteses e foi desligada.
  bool conectividadeDescartada() const { return semConectividade_; }

  /// Objetivos distintos das hipóteses vivas, no referencial da exploração.
  /// @return Quantos foram escritos em `saida` (no máximo `capacidade`).
  uint16_t objetivos(PosicaoCelula* saida, uint16_t capacidade) const;

  /// `true` se o robô já esteve na célula (referencial da exploração).
  bool visitada(PosicaoCelula celula) const {
    return celula.linha < MAX_LINHAS_EXPLORACAO && celula.coluna < MAX_COLUNAS_EXPLORACAO &&
           visitada(celula.linha, celula.coluna);
  }

  /// `true` se `celula` é o objetivo de todas as hipóteses vivas.
  bool objetivoDeTodas(PosicaoCelula celula) const;

  /// Linhas a somar a uma célula real para chegar à de exploração, na hipótese `h`.
  static uint8_t deslocamentoLinhas(const HipoteseLargada& h) {
    return static_cast<uint8_t>(LARGADA_NA_EXPLORACAO.linha - h.linha);
  }

  /// Colunas a somar a uma célula real para chegar à de exploração, na hipótese `h`.
  static uint8_t deslocamentoColunas(const HipoteseLargada& h) {
    return static_cast<uint8_t>(LARGADA_NA_EXPLORACAO.coluna - h.coluna);
  }

  /// Objetivo da hipótese `h`, no referencial da exploração.
  static PosicaoCelula objetivoNaExploracao(const HipoteseLargada& h);

  /// Monta o mapa real da hipótese `h` a partir do mapa de exploração: o perímetro é parede e as
  /// paredes internas vistas são copiadas.
  static Mapa mapaReal(const Mapa& exploracao, const HipoteseLargada& h);

 private:
  bool coerente(const HipoteseLargada& h, const Mapa& exploracao, bool comConectividade) const;
  bool visitada(uint8_t linha, uint8_t coluna) const;

  HipoteseLargada hipoteses_[MAX_HIPOTESES] = {};
  uint8_t quantidade_ = 0;
  bool semConectividade_ = false;
  /// Células visitadas no mapa de exploração, um bit por célula.
  uint32_t visitadas_[MAX_LINHAS_EXPLORACAO] = {};
};

static_assert(MAX_COLUNAS_EXPLORACAO <= 32, "as visitadas de uma linha cabem num uint32_t");

}  // namespace micromouse
