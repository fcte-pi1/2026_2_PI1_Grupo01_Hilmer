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
      detector_(mapa.obterLinhas(), mapa.obterColunas()) {
  objetivos_[0] = detector_.celulaObjetivo();
  quantidadeObjetivos_ = 1;
  recalcularDistancias();
}

Navegacao::Navegacao(const Mapa& mapa, PosicaoCelula objetivo, Pose poseInicial)
    : mapa_(mapa),
      posicao_atual_(mapa.posValida(poseInicial.celula) ? poseInicial.celula : PosicaoCelula{0, 0}),
      direcao_atual_(poseInicial.direcao),
      detector_(objetivo) {
  objetivos_[0] = objetivo;
  quantidadeObjetivos_ = 1;
  recalcularDistancias();
}

bool Navegacao::definirPose(const Pose& pose) {
  if (!mapa_.posValida(pose.celula)) {
    return false;
  }
  posicao_atual_ = pose.celula;
  direcao_atual_ = pose.direcao;
  passo_pendente_ = false;
  return true;
}

bool Navegacao::moverPara(uint8_t linha, uint8_t coluna, Direcao direcao) {
  return definirPose({PosicaoCelula{linha, coluna}, direcao});
}

void Navegacao::definirObjetivo(PosicaoCelula objetivo) {
  definirObjetivos(&objetivo, 1);
}

void Navegacao::definirObjetivos(const PosicaoCelula* objetivos, uint16_t quantidade) {
  if (objetivos == nullptr) {
    quantidade = 0;
  }
  quantidadeObjetivos_ = quantidade > CAPACIDADE ? CAPACIDADE : quantidade;
  for (uint16_t i = 0; i < quantidadeObjetivos_; ++i) {
    objetivos_[i] = objetivos[i];
  }
  if (quantidadeObjetivos_ > 0) {
    detector_.definirObjetivo(objetivos_[0]);
  }
  recalcularDistancias();
}

bool Navegacao::atingiuObjetivo() const {
  for (uint16_t i = 0; i < quantidadeObjetivos_; ++i) {
    if (mesmaCelula(objetivos_[i], posicao_atual_)) {
      return true;
    }
  }
  return false;
}

void Navegacao::recalcularDistancias() {
  for (uint8_t l = 0; l < MAX_LINHAS_EXPLORACAO; ++l) {
    for (uint8_t c = 0; c < MAX_COLUNAS_EXPLORACAO; ++c) {
      distancias_[l][c] = DISTANCIA_INFINITA;
    }
  }

  // Fila estática para BFS; cada célula entra no máximo uma vez.
  constexpr size_t TAMANHO_FILA = CAPACIDADE;
  PosicaoCelula fila[TAMANHO_FILA];
  size_t inicio = 0;
  size_t fim = 0;

  // Vários objetivos: BFS com várias origens, cada célula fica com a distância ao mais próximo.
  for (uint16_t i = 0; i < quantidadeObjetivos_; ++i) {
    const PosicaoCelula objetivo = objetivos_[i];
    if (!mapa_.posValida(objetivo) || distancias_[objetivo.linha][objetivo.coluna] == 0) {
      continue;
    }
    distancias_[objetivo.linha][objetivo.coluna] = 0;
    fila[fim++] = objetivo;
  }

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
  // A pose pode ficar fora do mapa se o mapa for trocado por outro menor; nunca ler o vetor assim.
  if (!mapa_.posValida(posicao_atual_)) {
    return false;
  }

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
  direcao_passo_ = direcao;
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
    // O sensor pode registrar a parede depois do planejamento: com ela no caminho, o passo nunca
    // seria concluído. Cancela para o próximo darPasso() replanejar.
    if (mapa_.obterParede(origem, direcao_passo_) == EstadoParede::Parede ||
        mapa_.obterParede(celula_esperada_, oposta(direcao_passo_)) == EstadoParede::Parede) {
      passo_pendente_ = false;
      return ResultadoConfirmacao::Bloqueado;
    }
    return ResultadoConfirmacao::EmAndamento;
  }

  passo_pendente_ = false;
  return ResultadoConfirmacao::Divergente;
}

}  // namespace micromouse
