#include "ParDeEncoders.h"

namespace micromouse {

namespace {

constexpr int32_t TETO_MOVIMENTO = 1000000000;

int32_t modulo(int32_t v) { return v < 0 ? -v : v; }

int32_t somaLimitada(int32_t a, int32_t b) { return a > TETO_MOVIMENTO - b ? TETO_MOVIMENTO : a + b; }

}  // namespace

ParDeEncoders::ParDeEncoders(IEncoder& esquerdo, IEncoder& direito, ParametrosParDeEncoders parametros)
    : realEsquerdo_(esquerdo),
      realDireito_(direito),
      parametros_(parametros),
      adaptadorEsquerdo_(*this, true),
      adaptadorDireito_(*this, false),
      ultimaEsquerda_(esquerdo.lerContagem()),
      ultimaDireita_(direito.lerContagem()) {}

void ParDeEncoders::atualizar(bool girandoNoLugar) {
  const int32_t esquerda = realEsquerdo_.lerContagem();
  const int32_t direita = realDireito_.lerContagem();
  const int32_t deltaEsquerda = esquerda - ultimaEsquerda_;
  const int32_t deltaDireita = direita - ultimaDireita_;
  ultimaEsquerda_ = esquerda;
  ultimaDireita_ = direita;

  movimentoEsquerda_ = somaLimitada(movimentoEsquerda_, modulo(deltaEsquerda));
  movimentoDireita_ = somaLimitada(movimentoDireita_, modulo(deltaDireita));

  // Um lado é mudo quando o outro já provou que o robô andou e ele quase não contou.
  if (!semSinalEsquerdo_ && movimentoDireita_ >= parametros_.pulsosParaDeclarar &&
      movimentoEsquerda_ <= parametros_.pulsosNoLadoMudo) {
    semSinalEsquerdo_ = true;
    avisoEsquerdo_ = true;
  }
  if (!semSinalDireito_ && movimentoEsquerda_ >= parametros_.pulsosParaDeclarar &&
      movimentoDireita_ <= parametros_.pulsosNoLadoMudo) {
    semSinalDireito_ = true;
    avisoDireito_ = true;
  }

  // O lado mudo repete o outro (com o sinal trocado nos giros no lugar). Os dois mudos: nada a repetir.
  const int32_t paraEsquerda = semSinalEsquerdo_ && !semSinalDireito_ ? (girandoNoLugar ? -deltaDireita : deltaDireita)
                                                                      : deltaEsquerda;
  const int32_t paraDireita = semSinalDireito_ && !semSinalEsquerdo_ ? (girandoNoLugar ? -deltaEsquerda : deltaEsquerda)
                                                                     : deltaDireita;
  contagemEsquerda_ += paraEsquerda;
  contagemDireita_ += paraDireita;
}

bool ParDeEncoders::consumirAvisoEsquerdo() {
  const bool aviso = avisoEsquerdo_;
  avisoEsquerdo_ = false;
  return aviso;
}

bool ParDeEncoders::consumirAvisoDireito() {
  const bool aviso = avisoDireito_;
  avisoDireito_ = false;
  return aviso;
}

}  // namespace micromouse
