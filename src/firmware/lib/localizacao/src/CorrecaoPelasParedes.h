#pragma once

/// @file CorrecaoPelasParedes.h
/// @brief Corrige o erro acumulado da odometria pelas paredes vistas nos ToF (#188), com qualquer
/// fonte de deslocamento: a posição lateral pelas paredes laterais e, com os encoders, a posição
/// ao longo do avanço pela parede da frente e pelas quinas (a regra da #380).

#include <stdint.h>

#include "ConfiguracaoLocalizacao.h"
#include "CorretorDeslocamentoTof.h"
#include "Odometria.h"
#include "Tipos.h"

namespace micromouse {

/// O que o robô está fazendo, para saber se a leitura serve de referência.
enum class MovimentoRobo : uint8_t {
  Parado,     ///< Parado (por exemplo, percebendo no centro da célula).
  Avancando,  ///< Andando para a frente em linha reta.
  Manobrando  ///< Girando ou realinhando: a leitura não serve de referência.
};

/// Quais paredes laterais deram a referência.
enum class ReferenciaLateral : uint8_t {
  SemParede,     ///< Nenhum lado com parede perto: nada a corrigir.
  DuasParedes,   ///< As duas: o desvio é a metade da diferença entre elas.
  SoEsquerda,    ///< Só a esquerda: o desvio vem da distância a ela.
  SoDireita,     ///< Só a direita.
  Descartada,    ///< Havia referência, mas a diferença passava de `correcaoLateralMaximaMm`.
  Manobrando     ///< Giro ou realinhamento em andamento.
};

/// Resultado de uma leitura.
struct ResultadoCorrecaoParedes {
  ReferenciaLateral lateral = ReferenciaLateral::SemParede;
  int32_t deltaLateralUm = 0;   ///< Quanto a posição foi movida para o lado (positivo = direita).
  ResultadoCorrecao avanco = ResultadoCorrecao::SemReferencia;  ///< Só com `corrigirAvanco`.
  int32_t deltaAvancoUm = 0;    ///< Quanto a posição foi movida ao longo do avanço.
};

/// Desvio do centro do robô em relação ao centro do corredor, medido pelas paredes laterais.
struct DesvioLateral {
  ReferenciaLateral referencia = ReferenciaLateral::SemParede;
  int32_t desvioUm = 0;  ///< Positivo = o robô está à direita do centro.
};

/// Mede o desvio lateral numa leitura: com as duas paredes, `(esquerda − direita) / 2`; só com a
/// esquerda, `esquerda + recuo − 84 mm`; só com a direita, `84 mm − (direita + recuo)`. As leituras
/// são a partir da face de cada sensor. Supõe o robô alinhado ao corredor (alguns graus de rumo
/// mudam a leitura em menos de 1%).
DesvioLateral medirDesvioLateral(const DistanciasLaterais& leitura, uint16_t limiarParedeMm, uint16_t recuoSensorMm);

/// Corrige a `Odometria` pelas paredes, uma leitura nova do ToF por vez.
///
/// - **Lateral (as duas fontes):** a odometria só soma o avanço na direção atual e nunca vê o robô
///   escorregar para o lado. Com parede lateral, a posição transversal vai para o centro da
///   célula mais o desvio medido (`ganhoCorrecaoLateralPercentual` por leitura, contra o ruído). Só
///   com o robô parado ou avançando: durante uma manobra, as laterais não servem de referência.
/// - **Ao longo do avanço (só com `corrigirAvanco`):** a mesma regra da #380 (`CorretorDeslocamentoTof`),
///   aplicada direto na `Odometria`. No modo estimado, quem a aplica é o `DeslocamentoEstimado`, nos
///   pulsos do estimador, e aqui ela fica desligada.
///
/// Não depende do Arduino e não aloca memória: testada no ambiente `native`.
///
/// @code
/// CorrecaoPelasParedes correcao;
/// // a cada leitura nova do ToF, depois de atualizar a odometria:
/// correcao.aplicar(odometria, leitura, MovimentoRobo::Avancando, ENCODERS_INSTALADOS);
/// @endcode
class CorrecaoPelasParedes {
 public:
  explicit CorrecaoPelasParedes(const ConfiguracaoCorrecaoParedes& config = ConfiguracaoCorrecaoParedes{});

  /// Corrige a odometria por uma leitura NOVA do ToF (chame uma vez por leitura).
  /// @param corrigirAvanco `true` para também corrigir ao longo do avanço (com os encoders reais).
  ResultadoCorrecaoParedes aplicar(Odometria& odometria, const DistanciasLaterais& leitura, MovimentoRobo movimento,
                                   bool corrigirAvanco);

  /// Esquece o estado das quinas (depois de reposicionar o robô à mão).
  void reiniciar() { avanco_.esquecerLaterais(); }

  /// Resultado da última leitura.
  const ResultadoCorrecaoParedes& ultima() const { return ultima_; }

  /// Correções laterais aplicadas desde o início (para o log e os testes).
  uint32_t correcoesLaterais() const { return correcoesLaterais_; }

  const ConfiguracaoCorrecaoParedes& configuracao() const { return config_; }

 private:
  ConfiguracaoCorrecaoParedes config_;
  CorretorDeslocamentoTof avanco_;
  ResultadoCorrecaoParedes ultima_;
  uint32_t correcoesLaterais_ = 0;
};

/// Posição do robô, em µm, no eixo transversal (para a direita da direção atual), na convenção da
/// `Odometria` (centro da célula 0 em 0).
int64_t posicaoLateralUm(const Odometria& odometria);

/// Nome da referência lateral para o log: `sem_parede`, `duas_paredes`, `so_esquerda`,
/// `so_direita`, `descartada` ou `manobrando`.
const char* nomeDaReferenciaLateral(ReferenciaLateral referencia);

}  // namespace micromouse
