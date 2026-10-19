#pragma once

/// @file Odometria.h
/// @brief Estimativa da pose (célula e direção) pela odometria diferencial dos encoders.
///
/// A arquitetura não tem IMU: a rotação vem da diferença entre os deslocamentos das duas rodas, e
/// o avanço, da média. Não depende do Arduino e só usa aritmética de inteiros.

#include <stdint.h>

#include "ConfiguracaoOdometria.h"
#include "Direcao.h"
#include "IEncoder.h"
#include "Tipos.h"

namespace micromouse {

/// Estima onde o robô está na malha, a partir das contagens acumuladas dos dois encoders.
///
/// **Convenções.** A contagem de cada roda aumenta quando ela avança (veja `IEncoder`). Girar para
/// a direita (sentido horário, Norte → Leste) faz a roda esquerda avançar mais que a direita, então
/// `contagemEsquerda - contagemDireita` cresce. Norte aumenta a linha e Leste aumenta a coluna.
///
/// **Orientação.** O giro acumulado é a diferença entre as rodas dividida por
/// `pulsosDiferenciaisPor90Graus`, arredondada para o múltiplo de 90° mais próximo. Por isso uma
/// diferença pequena em linha reta (uma roda um pouco mais rápida) não muda a direção, e o giro só
/// passa para a direção seguinte depois de 45°.
///
/// **Posição.** O avanço de cada atualização (média dos deslocamentos das duas rodas) soma-se à
/// posição na direção atual. A pose inicial é o centro da célula informada, e a célula muda ao
/// cruzar a parede, a meia célula do centro. Um giro no lugar (rodas em sentidos opostos) não muda
/// a célula.
/// Fora do que o `uint8_t` comporta, a célula é limitada a 0..255 e `posicaoValida()` é `false`.
///
/// A odometria acumula erro (patinagem, arredondamento); a correção pelas paredes é a #188.
///
/// Exemplo:
/// @code
/// Odometria odometria;                              // parte em (0, 0), virada para o Norte
/// odometria.atualizar(contagemEsq, contagemDir);    // a cada ciclo, com as contagens acumuladas
/// Pose p = odometria.pose();
/// @endcode
class Odometria {
 public:
  /// @param configuracao Parâmetros de conversão e de giro.
  /// @param inicial Pose de partida (padrão: célula (0, 0) virada para o Norte).
  explicit Odometria(const ConfiguracaoOdometria& configuracao = ConfiguracaoOdometria(),
                     Pose inicial = Pose{{0, 0}, Direcao::Norte});

  /// Atualiza a pose a partir das contagens acumuladas dos encoders.
  /// A primeira chamada considera que os encoders partiram de zero; use `reiniciar` para
  /// sincronizar com contagens que não estão em zero.
  /// @param contagemEsquerda Pulsos acumulados da roda esquerda.
  /// @param contagemDireita Pulsos acumulados da roda direita.
  void atualizar(int32_t contagemEsquerda, int32_t contagemDireita);

  /// Atualiza lendo os dois encoders.
  void atualizar(const IEncoder& esquerdo, const IEncoder& direito) {
    atualizar(esquerdo.lerContagem(), direito.lerContagem());
  }

  /// Define a pose atual e usa as contagens dadas como referência (por exemplo, depois de um
  /// `zerar()` dos encoders, ou ao reposicionar o robô numa célula conhecida).
  void reiniciar(Pose pose, int32_t contagemEsquerda = 0, int32_t contagemDireita = 0);

  /// Célula e direção estimadas.
  Pose pose() const;

  /// `false` se a posição estimada saiu do intervalo 0..255 em linha ou coluna.
  bool posicaoValida() const { return posicaoValida_; }

  /// Posição no eixo Norte-Sul, em µm, a partir do centro da linha 0. O centro da linha `n` fica em
  /// `n × tamanhoCelulaMicrometros`.
  int32_t posicaoNorteMicrometros() const { return static_cast<int32_t>(norteUm_); }

  /// Posição no eixo Leste-Oeste, em µm, a partir do centro da coluna 0.
  int32_t posicaoLesteMicrometros() const { return static_cast<int32_t>(lesteUm_); }

 private:
  void recalcularPose();

  ConfiguracaoOdometria configuracao_;
  Direcao direcaoInicial_;
  PosicaoCelula celula_;
  Direcao direcao_;
  bool posicaoValida_ = true;

  int32_t ultimaEsquerda_ = 0;
  int32_t ultimaDireita_ = 0;
  int64_t diferencaAcumulada_ = 0;  ///< Soma de (esquerda - direita), em pulsos.
  int64_t norteUm_ = 0;             ///< Posição Norte-Sul, em µm.
  int64_t lesteUm_ = 0;             ///< Posição Leste-Oeste, em µm.
};

}  // namespace micromouse
