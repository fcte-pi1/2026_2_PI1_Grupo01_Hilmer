#pragma once

/// @file ControleMovimento.h
/// @brief Deslocamento em linha reta centralizado no corredor, com parada antes da parede
/// frontal (#191, HU-12).

#include <stdint.h>

#include "AcompanhadorGiro.h"
#include "ConfiguracaoControle.h"
#include "ControladorCentralizacao.h"
#include "ISensorDistancia.h"
#include "IMotores.h"
#include "PerfilParadaFrontal.h"
#include "Tipos.h"

namespace micromouse {

/// Situação do movimento depois da última atualização.
enum class EstadoMovimento : uint8_t {
  Cruzeiro,          ///< Caminho livre à frente: potência de cruzeiro, com centralização.
  Desacelerando,     ///< Parede à frente se aproximando: rampa de potência, com centralização.
  ParadoNaParede,    ///< Chegou à distância de parada da parede frontal: motores freados.
  SemLeitura,        ///< LiDAR sem leitura válida há pouco tempo: segue reto na potência mínima.
  ParadoSemLeitura   ///< LiDAR sem leitura por `tempoMaximoSemLeituraMs`: motores freados.
};

/// Junta a centralização no corredor (`ControladorCentralizacao`) e a parada frontal
/// (`PerfilParadaFrontal`) num comando para os dois motores.
///
/// A cada leitura nova do LiDAR: a potência base vem da distância frontal e a correção lateral é
/// somada à roda direita e subtraída da esquerda. A correção nunca passa de
/// `correcaoMaximaPercentualDaBase` da base, para nenhuma roda girar para trás.
///
/// Os dois estados parados são travados: o robô só volta a andar depois de `reiniciar()`, quando
/// a navegação decidir o próximo movimento (por exemplo, depois de um giro).
///
/// Uso típico, na tarefa de navegação, a cada leitura nova do LiDAR (cerca de 100 ms):
/// @code
/// ControleMovimento controle;
/// EstadoMovimento estado = controle.atualizar(lidar, motores, 100);
/// if (estado == EstadoMovimento::ParadoNaParede) { /* decidir o próximo passo */ }
/// @endcode
class ControleMovimento {
 public:
  explicit ControleMovimento(const ConfiguracaoControle& config = ConfiguracaoControle{});

  /// Calcula a potência das rodas para uma leitura do LiDAR.
  /// @param leitura Distâncias medidas, ou `nullptr` se o LiDAR não deu leitura válida.
  /// @param dtMs Tempo desde a chamada anterior, em ms.
  /// @return Potência de cada roda. `{0, 0}` nos estados parados.
  PotenciaRodas calcular(const DistanciasLaterais* leitura, uint32_t dtMs);

  /// Calcula a potência das rodas para uma referência de leitura.
  PotenciaRodas calcular(const DistanciasLaterais& leitura, uint32_t dtMs) {
    return calcular(&leitura, dtMs);
  }

  /// Calcula a potência das rodas com correção conjunta de centralização lateral e rumo (#190).
  /// @param leitura Distâncias medidas, ou `nullptr` se o LiDAR não deu leitura válida.
  /// @param dtMs Tempo desde a chamada anterior, em ms.
  /// @param anguloGiroMiligraus Ângulo atual de guinada medido pelo giroscópio, em mili-graus.
  /// @return Potência de cada roda.
  PotenciaRodas calcular(const DistanciasLaterais* leitura, uint32_t dtMs, int32_t anguloGiroMiligraus);

  /// Calcula a potência das rodas para uma referência de leitura e ângulo de rumo (#190).
  PotenciaRodas calcular(const DistanciasLaterais& leitura, uint32_t dtMs, int32_t anguloGiroMiligraus) {
    return calcular(&leitura, dtMs, anguloGiroMiligraus);
  }

  /// Lê o LiDAR, calcula e aplica nos motores: `parar()` nos estados parados e
  /// `definirPotencia()` nos demais.
  /// @return O estado depois da atualização.
  EstadoMovimento atualizar(ISensorDistancia& lidar, IMotores& motores, uint32_t dtMs);

  /// Lê o LiDAR, calcula com correção de rumo (#190) e aplica nos motores.
  /// @param lidar Sensor de distância.
  /// @param motores Interface dos motores.
  /// @param dtMs Tempo decorrido em ms.
  /// @param anguloGiroMiligraus Ângulo atual de guinada do robô.
  /// @return O estado depois da atualização.
  EstadoMovimento atualizar(ISensorDistancia& lidar, IMotores& motores, uint32_t dtMs, int32_t anguloGiroMiligraus);

  /// Lê o LiDAR, calcula com leitura completa do giroscópio (#190) e aplica nos motores.
  /// @param lidar Sensor de distância.
  /// @param motores Interface dos motores.
  /// @param dtMs Tempo decorrido em ms.
  /// @param giro Estrutura com leitura do giroscópio e indicador de validade.
  /// @return O estado depois da atualização.
  EstadoMovimento atualizar(ISensorDistancia& lidar, IMotores& motores, uint32_t dtMs, const LeituraGiro& giro);

  /// Estado depois da última atualização.
  EstadoMovimento estado() const { return estado_; }

  /// `true` em `ParadoNaParede` ou `ParadoSemLeitura`.
  bool parado() const {
    return estado_ == EstadoMovimento::ParadoNaParede || estado_ == EstadoMovimento::ParadoSemLeitura;
  }

  /// Destrava a parada e esquece o histórico da centralização e do rumo, para um novo trecho reto.
  void reiniciar();

  /// Destrava a parada, esquece o histórico e define um novo rumo alvo em linha reta (#190).
  void reiniciar(int32_t novoRumoAlvoMiligraus) {
    reiniciar();
    definirRumoAlvo(novoRumoAlvoMiligraus);
  }

  /// Define o rumo alvo (em mili-graus) que o robô deve manter durante o avanço.
  void definirRumoAlvo(int32_t rumoAlvoMiligraus);

  /// Limita a potência base, para desacelerar antes de um ponto que não é uma parede (por exemplo,
  /// o centro da próxima célula, #184). A base fica entre `potenciaMinima` e o teto; a parada
  /// frontal continua valendo. `INT16_MAX` (o padrão) desliga o limite.
  void definirTetoPotenciaBase(int16_t teto) { tetoPotenciaBase_ = teto; }

  /// Teto atual da potência base.
  int16_t tetoPotenciaBase() const { return tetoPotenciaBase_; }

  /// Rumo alvo atualmente configurado, em mili-graus.
  int32_t rumoAlvoMiligraus() const { return rumoAlvoMiligraus_; }

  /// Último desvio angular de rumo calculado (rumoAlvo - anguloAtual), em mili-graus.
  int32_t desvioRumoMiligraus() const { return desvioRumoMiligraus_; }

  /// Última correção de rumo aplicada aos motores, em unidades de potência.
  int16_t correcaoRumo() const { return correcaoRumo_; }

  /// `true` se houve um ângulo de giro válido na última chamada a `calcular`.
  bool temRumoValido() const { return temRumoValido_; }

  /// Controlador lateral, para consultar o erro e o modo (telemetria e testes).
  const ControladorCentralizacao& centralizacao() const { return centralizacao_; }

 private:
  PotenciaRodas calcularInterno(const DistanciasLaterais* leitura, uint32_t dtMs, bool temAngulo,
                                int32_t anguloGiroMiligraus);

  ConfiguracaoControle config_;
  ControladorCentralizacao centralizacao_;
  PerfilParadaFrontal perfil_;
  EstadoMovimento estado_ = EstadoMovimento::Cruzeiro;
  uint32_t tempoSemLeituraMs_ = 0;
  int16_t tetoPotenciaBase_ = INT16_MAX;

  // Controle de rumo pelo giroscópio (#190)
  int32_t rumoAlvoMiligraus_ = 0;
  int32_t desvioRumoMiligraus_ = 0;
  int32_t desvioRumoAnterior_ = 0;
  int32_t derivadaRumoMiliGrausPorS_ = 0;
  int16_t correcaoRumo_ = 0;
  bool temErroRumoAnterior_ = false;
  bool temRumoValido_ = false;
};

}  // namespace micromouse
