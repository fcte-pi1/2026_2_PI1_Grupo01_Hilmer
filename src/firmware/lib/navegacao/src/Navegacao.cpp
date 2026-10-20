/// @file Navegacao.cpp
/// @brief Implementação da navegação e resolução do labirinto (Flood Fill).

#include "Navegacao.h"

#include <cstddef>

namespace micromouse {

namespace {

bool mesmaCelula(PosicaoCelula a, PosicaoCelula b) {
  return a.linha == b.linha && a.coluna == b.coluna;
}

}  // namespace

Navegacao::Navegacao(const Mapa& mapa)
    : mapa_(mapa),
      posicao_atual_({0, 0}),
      direcao_atual_(Direcao::Norte),
      objetivo_({static_cast<uint8_t>(mapa.obterLinhas() > 0 ? mapa.obterLinhas() - 1 : 0),
                 static_cast<uint8_t>(mapa.obterColunas() > 0 ? mapa.obterColunas() - 1 : 0)}) {
  recalcularDistancias();
}

Navegacao::Navegacao(const Mapa& mapa, PosicaoCelula objetivo, Pose poseInicial)
    : mapa_(mapa),
      posicao_atual_(poseInicial.celula),
      direcao_atual_(poseInicial.direcao),
      objetivo_(objetivo) {
  recalcularDistancias();
}

void Navegacao::definirPose(const Pose& pose) {
  posicao_atual_ = pose.celula;
  direcao_atual_ = pose.direcao;
  passo_pendente_ = false;
}

void Navegacao::moverPara(uint8_t linha, uint8_t coluna, Direcao direcao) {
  posicao_atual_ = PosicaoCelula{linha, coluna};
  direcao_atual_ = direcao;
  passo_pendente_ = false;
}

void Navegacao::definirObjetivo(PosicaoCelula objetivo) {
  objetivo_ = objetivo;
  recalcularDistancias();
}

bool Navegacao::atingiuObjetivo() const {
  return posicao_atual_.linha == objetivo_.linha && posicao_atual_.coluna == objetivo_.coluna;
}

void Navegacao::recalcularDistancias() {
  for (uint8_t l = 0; l < MAX_LINHAS_LABIRINTO; ++l) {
    for (uint8_t c = 0; c < MAX_COLUNAS_LABIRINTO; ++c) {
      distancias_[l][c] = DISTANCIA_INFINITA;
    }
  }

  if (!mapa_.posValida(objetivo_)) {
    return;
  }

  // Fila estática para BFS
  constexpr size_t TAMANHO_FILA = MAX_LINHAS_LABIRINTO * MAX_COLUNAS_LABIRINTO;
  PosicaoCelula fila[TAMANHO_FILA];
  size_t inicio = 0;
  size_t fim = 0;

  distancias_[objetivo_.linha][objetivo_.coluna] = 0;
  fila[fim++] = objetivo_;

  const Direcao direcoes[4] = {Direcao::Norte, Direcao::Leste, Direcao::Sul, Direcao::Oeste};

  while (inicio < fim) {
    const PosicaoCelula atual = fila[inicio++];
    const uint8_t distAtual = distancias_[atual.linha][atual.coluna];

    for (Direcao d : direcoes) {
      // Parede saindo da célula atual
      if (mapa_.obterParede(atual, d) == EstadoParede::Parede) {
        continue;
      }

      PosicaoCelula viz = {};
      if (!vizinha(atual, d, viz) || !mapa_.posValida(viz)) {
        continue;
      }

      // Parede na vizinha apontando para a atual
      if (mapa_.obterParede(viz, oposta(d)) == EstadoParede::Parede) {
        continue;
      }

      if (distancias_[viz.linha][viz.coluna] > distAtual + 1) {
        distancias_[viz.linha][viz.coluna] = static_cast<uint8_t>(distAtual + 1);
        fila[fim++] = viz;
      }
    }
  }
}

uint8_t Navegacao::obterDistancia(PosicaoCelula celula) const {
  if (!mapa_.posValida(celula)) {
    return DISTANCIA_INFINITA;
  }
  return distancias_[celula.linha][celula.coluna];
}

bool Navegacao::decidirProximaDirecao(Direcao& proximaDirecao) {
  if (atingiuObjetivo()) {
    return false;
  }

  recalcularDistancias();

  if (distancias_[posicao_atual_.linha][posicao_atual_.coluna] == DISTANCIA_INFINITA) {
    return false;
  }

  // Ordem de preferência de candidatos em caso de empate:
  // 1. Frente (continua na direção atual, sem rotação)
  // 2. Direita (+90°)
  // 3. Esquerda (-90°)
  // 4. Trás / Ré (180°)
  const Direcao candidatos[4] = {
      direcao_atual_,
      girarDireita(direcao_atual_),
      girarEsquerda(direcao_atual_),
      oposta(direcao_atual_)};

  uint8_t menorDistancia = DISTANCIA_INFINITA;
  Direcao melhorDir = direcao_atual_;
  bool encontrou = false;

  for (Direcao d : candidatos) {
    if (mapa_.obterParede(posicao_atual_, d) == EstadoParede::Parede) {
      continue;
    }

    PosicaoCelula viz = {};
    if (!vizinha(posicao_atual_, d, viz) || !mapa_.posValida(viz)) {
      continue;
    }

    if (mapa_.obterParede(viz, oposta(d)) == EstadoParede::Parede) {
      continue;
    }

    const uint8_t dist = distancias_[viz.linha][viz.coluna];
    // Estritamente menor (<) preserva a primeira direção na lista de prioridade em caso de empate
    if (dist < menorDistancia) {
      menorDistancia = dist;
      melhorDir = d;
      encontrou = true;
    }
  }

  if (encontrou) {
    proximaDirecao = melhorDir;
    return true;
  }

  return false;
}

bool Navegacao::darPasso(Direcao& direcaoMovimento) {
  if (passo_pendente_) {
    return false;
  }

  Direcao direcao = direcao_atual_;
  if (!decidirProximaDirecao(direcao)) {
    return false;
  }

  PosicaoCelula esperada = {};
  if (!vizinha(posicao_atual_, direcao, esperada) || !mapa_.posValida(esperada)) {
    return false;
  }

  celula_esperada_ = esperada;
  passo_pendente_ = true;
  direcaoMovimento = direcao;
  return true;
}

ResultadoConfirmacao Navegacao::confirmarPose(const Pose& poseMedida) {
  if (!mapa_.posValida(poseMedida.celula)) {
    return ResultadoConfirmacao::Invalida;
  }

  const PosicaoCelula origem = posicao_atual_;
  posicao_atual_ = poseMedida.celula;
  direcao_atual_ = poseMedida.direcao;

  if (!passo_pendente_) {
    return ResultadoConfirmacao::Confirmada;
  }

  if (mesmaCelula(posicao_atual_, celula_esperada_)) {
    passo_pendente_ = false;
    return ResultadoConfirmacao::Confirmada;
  }

  if (mesmaCelula(posicao_atual_, origem)) {
    return ResultadoConfirmacao::EmAndamento;
  }

  passo_pendente_ = false;
  return ResultadoConfirmacao::Divergente;
}

}  // namespace micromouse
