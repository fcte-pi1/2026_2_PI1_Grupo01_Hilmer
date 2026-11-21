#pragma once

/// @file DetectorPatinagem.h
/// @brief Detecta rodas patinando: a odometria anda, mas o ToF frontal não muda.
///
/// Quando uma roda cai num vinco ou o chassi prende numa quina, as rodas continuam girando e os encoders
/// contam, mas o robô não sai do lugar. Dentro de uma janela de tempo, se o percurso medido pela odometria
/// foi grande e a leitura do ToF frontal quase não variou, o robô está patinando. A proteção do motor travado
/// não pega esse caso quando o giroscópio ainda vê algum giro (o robô pivota contra o obstáculo).
///
/// Não depende do Arduino e não aloca memória: testado no ambiente `native`.

#include <stdint.h>

namespace micromouse {

struct ConfiguracaoPatinagem {
  uint32_t janelaMs = 1500;               ///< Tempo observado, em ms, antes de decidir.
  int64_t percursoMinimoUm = 150000;      ///< Percurso mínimo da odometria na janela, em µm (150 mm).
  uint16_t variacaoMaximaFrenteMm = 20;   ///< Variação máxima do ToF frontal, em mm, que ainda conta como parado.
  uint16_t frenteMaximaValidaMm = 700;    ///< Acima disso o ToF frontal quase não muda com o avanço: não vale.
};

class DetectorPatinagem {
 public:
  explicit DetectorPatinagem(ConfiguracaoPatinagem configuracao = ConfiguracaoPatinagem{});

  /// Chamar a cada ciclo da navegação.
  /// @param ativo `true` enquanto o robô avança em linha reta, conduzido pelo ciclo.
  /// @param posicaoAvancoUm Posição ao longo do avanço, em µm, pela odometria.
  /// @param frenteValida `true` se o ToF frontal tem leitura válida.
  /// @param frenteMm Leitura do ToF frontal, em mm.
  /// @param dtMs Tempo desde a chamada anterior, em ms.
  /// @return `true` no instante em que uma patinagem é detectada (uma vez por janela).
  bool atualizar(bool ativo, int64_t posicaoAvancoUm, bool frenteValida, uint16_t frenteMm, uint32_t dtMs);

  /// Descarta a janela em andamento.
  void reiniciar();

  uint32_t deteccoes() const { return deteccoes_; }

 private:
  ConfiguracaoPatinagem configuracao_;
  bool temBase_ = false;
  int64_t posicaoBaseUm_ = 0;
  uint16_t frenteBaseMm_ = 0;
  uint32_t decorridoMs_ = 0;
  uint32_t deteccoes_ = 0;
};

}  // namespace micromouse
