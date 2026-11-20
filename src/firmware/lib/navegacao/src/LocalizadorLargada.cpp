#include "LocalizadorLargada.h"

#include "Direcao.h"

namespace micromouse {

namespace {

constexpr uint8_t COLUNAS_POSSIVEIS[3] = {4, 8, 12};
constexpr Direcao DIRECOES[4] = {Direcao::Norte, Direcao::Leste, Direcao::Sul, Direcao::Oeste};

bool mesmaCelula(PosicaoCelula a, PosicaoCelula b) { return a.linha == b.linha && a.coluna == b.coluna; }

}  // namespace

DimensaoMapa dimensaoComColunas(uint8_t colunas) {
  return colunas == 12 ? DimensaoMapa::Labirinto12x4
                       : (colunas == 8 ? DimensaoMapa::Labirinto8x4 : DimensaoMapa::Labirinto4x4);
}

void LocalizadorLargada::reiniciar() {
  quantidade_ = 0;
  semConectividade_ = false;
  for (uint8_t colunas : COLUNAS_POSSIVEIS) {
    for (uint8_t linha = 0; linha < LINHAS_LABIRINTO; ++linha) {
      for (uint8_t coluna = 0; coluna < colunas; ++coluna) {
        hipoteses_[quantidade_++] = HipoteseLargada{linha, coluna, colunas};
      }
    }
  }
  for (uint32_t& linha : visitadas_) {
    linha = 0;
  }
  visitar(LARGADA_NA_EXPLORACAO);
}

void LocalizadorLargada::visitar(PosicaoCelula celula) {
  if (celula.linha < MAX_LINHAS_EXPLORACAO && celula.coluna < MAX_COLUNAS_EXPLORACAO) {
    visitadas_[celula.linha] |= (1u << celula.coluna);
  }
}

bool LocalizadorLargada::visitada(uint8_t linha, uint8_t coluna) const {
  return (visitadas_[linha] >> coluna) & 1u;
}

PosicaoCelula LocalizadorLargada::objetivoNaExploracao(const HipoteseLargada& h) {
  return PosicaoCelula{static_cast<uint8_t>(LINHAS_LABIRINTO - 1 + deslocamentoLinhas(h)),
                       static_cast<uint8_t>(h.colunas - 1 + deslocamentoColunas(h))};
}

bool LocalizadorLargada::coerente(const HipoteseLargada& h, const Mapa& exploracao, bool comConectividade) const {
  const uint8_t l0 = deslocamentoLinhas(h);
  const uint8_t c0 = deslocamentoColunas(h);
  const uint8_t l1 = static_cast<uint8_t>(l0 + LINHAS_LABIRINTO - 1);
  const uint8_t c1 = static_cast<uint8_t>(c0 + h.colunas - 1);
  const auto dentro = [&](uint8_t l, uint8_t c) { return l >= l0 && l <= l1 && c >= c0 && c <= c1; };

  // 1. Toda célula visitada fica dentro do labirinto.
  for (uint8_t l = 0; l < MAX_LINHAS_EXPLORACAO; ++l) {
    for (uint8_t c = 0; c < MAX_COLUNAS_EXPLORACAO; ++c) {
      if (visitada(l, c) && !dentro(l, c)) {
        return false;
      }
    }
  }

  // 2. Nenhuma parede livre no perímetro.
  for (uint8_t c = c0; c <= c1; ++c) {
    if (exploracao.obterParede({l0, c}, Direcao::Sul) == EstadoParede::Livre ||
        exploracao.obterParede({l1, c}, Direcao::Norte) == EstadoParede::Livre) {
      return false;
    }
  }
  for (uint8_t l = l0; l <= l1; ++l) {
    if (exploracao.obterParede({l, c0}, Direcao::Oeste) == EstadoParede::Livre ||
        exploracao.obterParede({l, c1}, Direcao::Leste) == EstadoParede::Livre) {
      return false;
    }
  }

  if (!comConectividade) {
    return true;
  }

  // 3. Toda célula do labirinto alcançável da largada, sem atravessar parede vista.
  uint32_t alcancadas[MAX_LINHAS_EXPLORACAO] = {};
  PosicaoCelula fila[static_cast<uint16_t>(MAX_LINHAS_LABIRINTO) * MAX_COLUNAS_LABIRINTO];
  uint16_t inicio = 0;
  uint16_t fim = 0;
  fila[fim++] = LARGADA_NA_EXPLORACAO;
  alcancadas[LARGADA_NA_EXPLORACAO.linha] |= (1u << LARGADA_NA_EXPLORACAO.coluna);
  while (inicio < fim) {
    const PosicaoCelula atual = fila[inicio++];
    for (Direcao d : DIRECOES) {
      PosicaoCelula viz = {0, 0};
      if (!vizinha(atual, d, viz) || !dentro(viz.linha, viz.coluna) ||
          exploracao.obterParede(atual, d) == EstadoParede::Parede ||
          ((alcancadas[viz.linha] >> viz.coluna) & 1u)) {
        continue;
      }
      alcancadas[viz.linha] |= (1u << viz.coluna);
      fila[fim++] = viz;
    }
  }
  return fim == static_cast<uint16_t>(LINHAS_LABIRINTO) * h.colunas;
}

void LocalizadorLargada::atualizar(const Mapa& exploracao) {
  if (exploracao.obterDimensao() != DimensaoMapa::Exploracao) {
    return;
  }
  // Com a conectividade, a filtragem é feita numa cópia: se ela zerar as hipóteses, a regra sai.
  HipoteseLargada vivas[MAX_HIPOTESES];
  uint8_t n = 0;
  for (uint8_t i = 0; i < quantidade_; ++i) {
    if (coerente(hipoteses_[i], exploracao, !semConectividade_)) {
      vivas[n++] = hipoteses_[i];
    }
  }
  if (n == 0 && !semConectividade_) {
    semConectividade_ = true;
    for (uint8_t i = 0; i < quantidade_; ++i) {
      if (coerente(hipoteses_[i], exploracao, false)) {
        vivas[n++] = hipoteses_[i];
      }
    }
  }
  for (uint8_t i = 0; i < n; ++i) {
    hipoteses_[i] = vivas[i];
  }
  quantidade_ = n;
}

uint16_t LocalizadorLargada::objetivos(PosicaoCelula* saida, uint16_t capacidade) const {
  uint16_t n = 0;
  for (uint8_t i = 0; i < quantidade_ && n < capacidade; ++i) {
    const PosicaoCelula objetivo = objetivoNaExploracao(hipoteses_[i]);
    bool repetido = false;
    for (uint16_t j = 0; j < n && !repetido; ++j) {
      repetido = mesmaCelula(saida[j], objetivo);
    }
    if (!repetido) {
      saida[n++] = objetivo;
    }
  }
  return n;
}

bool LocalizadorLargada::objetivoDeTodas(PosicaoCelula celula) const {
  if (quantidade_ == 0) {
    return false;
  }
  for (uint8_t i = 0; i < quantidade_; ++i) {
    if (!mesmaCelula(objetivoNaExploracao(hipoteses_[i]), celula)) {
      return false;
    }
  }
  return true;
}

Mapa LocalizadorLargada::mapaReal(const Mapa& exploracao, const HipoteseLargada& h) {
  Mapa real(dimensaoComColunas(h.colunas));
  const uint8_t dl = deslocamentoLinhas(h);
  const uint8_t dc = deslocamentoColunas(h);
  for (uint8_t l = 0; l < real.obterLinhas(); ++l) {
    for (uint8_t c = 0; c < real.obterColunas(); ++c) {
      const PosicaoCelula naExploracao = {static_cast<uint8_t>(l + dl), static_cast<uint8_t>(c + dc)};
      for (Direcao d : DIRECOES) {
        const EstadoParede estado = exploracao.obterParede(naExploracao, d);
        if (estado != EstadoParede::Desconhecido) {
          real.registrarParede({l, c}, d, estado);  // no perímetro, a parede já está lá
        }
      }
    }
  }
  return real;
}

}  // namespace micromouse
