#pragma once

/// @file CorretorDeslocamentoTof.h
/// @brief Corrige a posição estimada ao longo do eixo de avanço pelas paredes vistas no ToF (#380).

#include <stdint.h>

#include "ConfiguracaoDeslocamento.h"
#include "EstimadorDeslocamento.h"
#include "Odometria.h"
#include "Tipos.h"

namespace micromouse {

/// O que o corretor fez com uma leitura.
enum class ResultadoCorrecao : uint8_t {
  SemReferencia,  ///< Nenhuma parede serviu de referência nesta leitura.
  PelaFrente,     ///< Posição ajustada pela distância à parede da frente.
  PelaLateral,    ///< Posição ajustada na quina de uma parede lateral (fronteira de célula).
  Descartada,     ///< Havia referência, mas a correção passava de `correcaoMaximaMm`.
  Girando         ///< Rodas em sentidos opostos: as leituras não servem de referência.
};

/// Correção calculada para uma leitura: o resultado e quanto mover a posição, em µm, ao longo do
/// eixo de avanço (positivo = para a frente).
struct Correcao {
  ResultadoCorrecao resultado = ResultadoCorrecao::SemReferencia;
  int32_t deltaUm = 0;
};

/// Calcula, a partir de uma leitura do ToF, quanto a posição da `Odometria` está errada ao longo
/// do eixo de avanço. Só corrige esse eixo (quanto o robô já andou); o desvio para os lados e o
/// rumo são de outras tarefas (#188 e #190).
///
/// **Pela frente.** Com o sensor frontal a até `distanciaFrontalMaximaParaCorrigirMm` de uma
/// parede, a parede está na face de uma fronteira de célula (meia parede antes dela). A fronteira
/// mais próxima do que a leitura indica dá a posição real do centro do robô.
///
/// **Pela lateral.** Quando um lado passa de parede para livre (ou o contrário) com o robô
/// avançando, o sensor lateral está na quina de uma parede: a fronteira de célula mais a meia
/// espessura da coluna (ao sair da parede) ou menos ela (ao entrar). Uma histerese evita quinas
/// inventadas pelo ruído.
///
/// Uma correção maior que `correcaoMaximaMm` é descartada (leitura errada). O corretor só calcula:
/// quem aplica é o `DeslocamentoEstimado`, por `EstimadorDeslocamento::corrigirAvanco`.
class CorretorDeslocamentoTof {
 public:
  explicit CorretorDeslocamentoTof(const ConfiguracaoDeslocamento& config = ConfiguracaoDeslocamento{});

  /// Avalia uma leitura NOVA do ToF (chame uma vez por leitura, não por ciclo).
  /// @param odometria Posição atual estimada.
  /// @param leitura Distâncias medidas, em mm, a partir da face de cada sensor.
  /// @param estimador Potência atual: diz se o robô está girando ou avançando.
  Correcao avaliar(const Odometria& odometria, const DistanciasLaterais& leitura,
                   const EstimadorDeslocamento& estimador);

  /// O mesmo, com a potência comandada às rodas no lugar do estimador: serve para qualquer fonte de
  /// deslocamento, inclusive os encoders reais (#188).
  Correcao avaliar(const Odometria& odometria, const DistanciasLaterais& leitura, const PotenciaRodas& potencia);

  /// Esquece o estado parede/livre dos lados (depois de um giro ou de reposicionar o robô).
  void esquecerLaterais();

 private:
  enum class EstadoLado : uint8_t { Desconhecido, Parede, Livre };

  /// Novo estado do lado para uma leitura, com histerese.
  EstadoLado classificarLado(EstadoLado anterior, uint16_t leituraMm) const;

  /// Correção pela parede da frente, se houver uma perto o bastante.
  bool corrigirPelaFrente(int64_t posicaoUm, uint16_t frenteMm, int64_t& deltaUm) const;

  /// Correção pela quina de uma parede lateral, na transição de `anterior` para `atual`.
  bool corrigirPelaLateral(int64_t posicaoUm, EstadoLado anterior, EstadoLado atual,
                           int64_t& deltaUm) const;

  bool dentroDoLimite(int64_t deltaUm) const;

  ConfiguracaoDeslocamento config_;
  EstadoLado esquerda_ = EstadoLado::Desconhecido;
  EstadoLado direita_ = EstadoLado::Desconhecido;
};

/// Posição do robô, em µm, ao longo do eixo para onde ele está virado, na convenção da
/// `Odometria` (centro da célula 0 em 0). Cresce quando o robô avança.
int64_t posicaoAoLongoDoAvancoUm(const Odometria& odometria);

}  // namespace micromouse
