#include "Odometria.h"

namespace micromouse {

namespace {

/// Divisão inteira arredondada para baixo (a de C++ arredonda para o zero).
int64_t divisaoPiso(int64_t numerador, int64_t denominador) {
  int64_t quociente = numerador / denominador;
  if ((numerador % denominador != 0) && ((numerador < 0) != (denominador < 0))) {
    quociente--;
  }
  return quociente;
}

/// Giro acumulado em quartos de volta, arredondado para o múltiplo de 90° mais próximo.
int64_t quartosDeVolta(int64_t diferencaPulsos, int64_t pulsosPor90) {
  return divisaoPiso(2 * diferencaPulsos + pulsosPor90, 2 * pulsosPor90);
}

/// Direção depois de `quartos` giros de 90° para a direita (negativo gira para a esquerda).
Direcao girar(Direcao inicial, int64_t quartos) {
  const int64_t indice = (static_cast<int64_t>(inicial) + quartos) % 4;
  return static_cast<Direcao>(indice < 0 ? indice + 4 : indice);
}

/// Índice da célula que contém a coordenada, com o centro da célula `n` em `n × celula`.
int64_t indiceDaCelula(int64_t posicaoUm, int64_t celulaUm) {
  return divisaoPiso(posicaoUm + celulaUm / 2, celulaUm);
}

}  // namespace

Odometria::Odometria(const ConfiguracaoOdometria& configuracao, Pose inicial)
    : configuracao_(configuracao), direcaoInicial_(inicial.direcao), celula_(inicial.celula),
      direcao_(inicial.direcao) {
  reiniciar(inicial);
}

void Odometria::reiniciar(Pose pose, int32_t contagemEsquerda, int32_t contagemDireita) {
  direcaoInicial_ = pose.direcao;
  direcao_ = pose.direcao;
  celula_ = pose.celula;
  ultimaEsquerda_ = contagemEsquerda;
  ultimaDireita_ = contagemDireita;
  diferencaAcumulada_ = 0;
  norteUm_ = static_cast<int64_t>(pose.celula.linha) * configuracao_.tamanhoCelulaMicrometros;
  lesteUm_ = static_cast<int64_t>(pose.celula.coluna) * configuracao_.tamanhoCelulaMicrometros;
  posicaoValida_ = true;
}

void Odometria::definirDirecao(Direcao direcao) {
  direcaoInicial_ = direcao;
  direcao_ = direcao;
  diferencaAcumulada_ = 0;
}

void Odometria::ajustarPosicao(int32_t deltaAvancoUm, int32_t deltaLateralUm) {
  // A direita de cada direção: Norte → Leste, Leste → Sul, Sul → Oeste, Oeste → Norte.
  switch (direcao_) {
    case Direcao::Norte:
      norteUm_ += deltaAvancoUm;
      lesteUm_ += deltaLateralUm;
      break;
    case Direcao::Sul:
      norteUm_ -= deltaAvancoUm;
      lesteUm_ -= deltaLateralUm;
      break;
    case Direcao::Leste:
      lesteUm_ += deltaAvancoUm;
      norteUm_ -= deltaLateralUm;
      break;
    case Direcao::Oeste:
      lesteUm_ -= deltaAvancoUm;
      norteUm_ += deltaLateralUm;
      break;
  }
  recalcularPose();
}

void Odometria::atualizar(int32_t contagemEsquerda, int32_t contagemDireita) {
  const int64_t deltaEsquerda = static_cast<int64_t>(contagemEsquerda) - ultimaEsquerda_;
  const int64_t deltaDireita = static_cast<int64_t>(contagemDireita) - ultimaDireita_;
  ultimaEsquerda_ = contagemEsquerda;
  ultimaDireita_ = contagemDireita;

  // Primeiro a rotação: o avanço desta atualização já é na direção depois do giro.
  diferencaAcumulada_ += deltaEsquerda - deltaDireita;
  direcao_ = girar(direcaoInicial_, quartosDeVolta(diferencaAcumulada_, configuracao_.pulsosDiferenciaisPor90Graus));

  // Avanço do centro do robô: média dos deslocamentos das duas rodas, em µm.
  const int64_t avancoUm = (deltaEsquerda + deltaDireita) * configuracao_.micrometrosPorPulso / 2;
  switch (direcao_) {
    case Direcao::Norte:
      norteUm_ += avancoUm;
      break;
    case Direcao::Sul:
      norteUm_ -= avancoUm;
      break;
    case Direcao::Leste:
      lesteUm_ += avancoUm;
      break;
    case Direcao::Oeste:
      lesteUm_ -= avancoUm;
      break;
  }
  recalcularPose();
}

void Odometria::recalcularPose() {
  const int64_t linha = indiceDaCelula(norteUm_, configuracao_.tamanhoCelulaMicrometros);
  const int64_t coluna = indiceDaCelula(lesteUm_, configuracao_.tamanhoCelulaMicrometros);
  posicaoValida_ = linha >= 0 && linha <= UINT8_MAX && coluna >= 0 && coluna <= UINT8_MAX;
  celula_.linha = static_cast<uint8_t>(linha < 0 ? 0 : (linha > UINT8_MAX ? UINT8_MAX : linha));
  celula_.coluna = static_cast<uint8_t>(coluna < 0 ? 0 : (coluna > UINT8_MAX ? UINT8_MAX : coluna));
}

Pose Odometria::pose() const { return Pose{celula_, direcao_}; }

}  // namespace micromouse
