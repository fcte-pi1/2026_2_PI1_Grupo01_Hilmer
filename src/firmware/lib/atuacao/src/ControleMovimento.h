#pragma once

/// @file ControleMovimento.h
/// @brief Deslocamento em linha reta centralizado no corredor, com parada antes da parede
/// frontal (#191, HU-12).

#include <stdint.h>

#include "ConfiguracaoControle.h"
#include "ControladorCentralizacao.h"
#include "ILidar.h"
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

  /// Lê o LiDAR, calcula e aplica nos motores: `parar()` nos estados parados e
  /// `definirPotencia()` nos demais.
  /// @return O estado depois da atualização.
  EstadoMovimento atualizar(ILidar& lidar, IMotores& motores, uint32_t dtMs);

  /// Estado depois da última atualização.
  EstadoMovimento estado() const { return estado_; }

  /// `true` em `ParadoNaParede` ou `ParadoSemLeitura`.
  bool parado() const {
    return estado_ == EstadoMovimento::ParadoNaParede || estado_ == EstadoMovimento::ParadoSemLeitura;
  }

  /// Destrava a parada e esquece o histórico da centralização, para um novo trecho reto.
  void reiniciar();

  /// Controlador lateral, para consultar o erro e o modo (telemetria e testes).
  const ControladorCentralizacao& centralizacao() const { return centralizacao_; }

 private:
  ConfiguracaoControle config_;
  ControladorCentralizacao centralizacao_;
  PerfilParadaFrontal perfil_;
  EstadoMovimento estado_ = EstadoMovimento::Cruzeiro;
  uint32_t tempoSemLeituraMs_ = 0;
};

}  // namespace micromouse
