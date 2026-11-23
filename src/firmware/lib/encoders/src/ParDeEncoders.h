#pragma once

/// @file ParDeEncoders.h
/// @brief Par de encoders com contingência: se um deles não conta, a odometria usa o outro.
///
/// Fica entre os encoders reais e a odometria. Enquanto os dois contam, devolve a contagem real. Se um
/// lado fica mudo (por exemplo, um fio solto) enquanto o outro conta, o lado mudo passa a repetir o outro:
/// com o robô em linha reta, a mesma contagem; girando no lugar, a contagem com o sinal trocado (as duas
/// rodas andam em sentidos opostos). O giroscópio é quem conduz as curvas, então a odometria só precisa
/// da distância. Não depende do Arduino: testado no ambiente `native`.

#include <stdint.h>

#include "IEncoder.h"

namespace micromouse {

/// Quando um lado conta como mudo.
struct ParametrosParDeEncoders {
  int32_t pulsosParaDeclarar = 800;  ///< Movimento acumulado do outro lado (em pulsos) que já prova que o robô andou.
  int32_t pulsosNoLadoMudo = 20;     ///< Até quantos pulsos o lado mudo pode ter contado nesse tempo (ruído).
};

class ParDeEncoders {
 public:
  ParDeEncoders(IEncoder& esquerdo, IEncoder& direito, ParametrosParDeEncoders parametros = ParametrosParDeEncoders{});

  /// Encoder que a odometria e a telemetria devem usar no lado esquerdo e no direito.
  IEncoder& esquerdo() { return adaptadorEsquerdo_; }
  IEncoder& direito() { return adaptadorDireito_; }

  /// Lê os encoders reais e atualiza as contagens. Chamar a cada ciclo da navegação, antes da odometria.
  /// @param girandoNoLugar `true` durante o giro no lugar (as rodas andam em sentidos opostos).
  void atualizar(bool girandoNoLugar);

  bool esquerdoSemSinal() const { return semSinalEsquerdo_; }
  bool direitoSemSinal() const { return semSinalDireito_; }

  /// `true` uma única vez depois de o lado ser declarado mudo, para o chamador avisar no log.
  bool consumirAvisoEsquerdo();
  bool consumirAvisoDireito();

 private:
  class Adaptador : public IEncoder {
   public:
    Adaptador(ParDeEncoders& pai, bool esquerdo) : pai_(pai), esquerdo_(esquerdo) {}
    int32_t lerContagem() const override { return esquerdo_ ? pai_.contagemEsquerda_ : pai_.contagemDireita_; }
    void zerar() override { (esquerdo_ ? pai_.contagemEsquerda_ : pai_.contagemDireita_) = 0; }

   private:
    ParDeEncoders& pai_;
    bool esquerdo_;
  };

  IEncoder& realEsquerdo_;
  IEncoder& realDireito_;
  ParametrosParDeEncoders parametros_;
  Adaptador adaptadorEsquerdo_;
  Adaptador adaptadorDireito_;

  int32_t ultimaEsquerda_;
  int32_t ultimaDireita_;
  int32_t contagemEsquerda_ = 0;
  int32_t contagemDireita_ = 0;
  int32_t movimentoEsquerda_ = 0;  ///< Soma de |variação| do lado esquerdo desde o início.
  int32_t movimentoDireita_ = 0;
  bool semSinalEsquerdo_ = false;
  bool semSinalDireito_ = false;
  bool avisoEsquerdo_ = false;
  bool avisoDireito_ = false;
};

}  // namespace micromouse
