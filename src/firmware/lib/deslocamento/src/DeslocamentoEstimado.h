#pragma once

/// @file DeslocamentoEstimado.h
/// @brief Junta as peças do modo sem encoder (#380): estimador, encoders estimados, motores e
/// correção pelo ToF, num ciclo que a tarefa de navegação chama a cada 10 ms.

#include <stdint.h>

#include "ConfiguracaoDeslocamento.h"
#include "CorretorDeslocamentoTof.h"
#include "EncoderEstimado.h"
#include "EstimadorDeslocamento.h"
#include "MotoresComEstimativa.h"
#include "Odometria.h"

namespace micromouse {

/// Modo de deslocamento sem encoder.
///
/// - `motores()` substitui o driver para quem aciona as rodas, e assim o estimador sabe a potência.
/// - `encoderEsquerdo()` e `encoderDireito()` substituem os encoders para a `Odometria` e o
///   `MedidorDeslocamento`.
/// - `ciclo()` avança a estimativa, atualiza a `Odometria` e aplica a correção do ToF.
///
/// @code
/// DeslocamentoEstimado deslocamento(motoresDrv8833);
/// Odometria odometria;
/// // a cada 10 ms, na tarefa de navegação:
/// const DistanciasLaterais* nova = temLeituraNova ? &leitura : nullptr;
/// deslocamento.ciclo(10, odometria, nova);
/// bool poseValida = odometria.posicaoValida() && deslocamento.poseConfiavel();
/// @endcode
class DeslocamentoEstimado {
 public:
  /// @param reais Driver dos motores (precisa continuar existindo).
  /// @param config Tabela de velocidade, geometria dos sensores e limites das correções.
  explicit DeslocamentoEstimado(IMotores& reais,
                                const ConfiguracaoDeslocamento& config = ConfiguracaoDeslocamento{});

  /// Motores para quem aciona as rodas: repassa ao driver e informa o estimador.
  IMotores& motores() { return motores_; }

  IEncoder& encoderEsquerdo() { return encoderEsquerdo_; }
  IEncoder& encoderDireito() { return encoderDireito_; }

  /// Um ciclo da tarefa de navegação: estima o avanço de `dtMs`, atualiza a `odometria` e, se
  /// houver uma leitura nova do ToF, corrige a posição por ela.
  /// @param leituraNova Leitura válida e ainda não usada, ou `nullptr` se não houver.
  /// @return O que a correção fez (`SemReferencia` quando não havia leitura).
  ResultadoCorrecao ciclo(uint32_t dtMs, Odometria& odometria, const DistanciasLaterais* leituraNova);

  /// Reposiciona o robô numa pose conhecida (por exemplo, na partida): zera as contagens, reinicia
  /// a `odometria` e esquece o estado das laterais. A pose volta a ser confiável.
  void reiniciar(Odometria& odometria, Pose pose);

  /// Esquece o estado parede/livre das laterais. Chame durante uma manobra em que as leituras não são
  /// passadas ao `ciclo()` (o realinhamento, #188): sem isso, a primeira leitura depois dela poderia
  /// parecer uma quina.
  void esquecerLaterais() { corretor_.esquecerLaterais(); }

  /// `false` depois de `distanciaMaximaSemCorrecaoMm` sem correção pelo ToF.
  bool poseConfiavel() const { return estimador_.poseConfiavel(); }

  /// Resultado e tamanho da correção do último `ciclo()` que recebeu leitura (o `deltaUm` é zero
  /// quando nada foi aplicado). Serve para registrar as correções na telemetria e na bancada.
  const Correcao& ultimaCorrecao() const { return ultimaCorrecao_; }

  /// Distância percorrida desde a última correção, em mm.
  uint32_t distanciaSemCorrecaoMm() const { return estimador_.distanciaSemCorrecaoMm(); }

  const EstimadorDeslocamento& estimador() const { return estimador_; }

 private:
  EstimadorDeslocamento estimador_;
  MotoresComEstimativa motores_;
  EncoderEstimado encoderEsquerdo_;
  EncoderEstimado encoderDireito_;
  CorretorDeslocamentoTof corretor_;
  Correcao ultimaCorrecao_;
};

}  // namespace micromouse
