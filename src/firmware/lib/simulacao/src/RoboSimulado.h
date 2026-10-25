#pragma once

/// @file RoboSimulado.h
/// @brief Robô "real" em posição contínua dentro de um `LabirintoSimulado`, para os testes de
/// integração da execução dos movimentos (#184).

#include <stdint.h>

#include "ConfiguracaoDeslocamento.h"
#include "EstimadorDeslocamento.h"
#include "IMotores.h"
#include "LabirintoSimulado.h"
#include "Tipos.h"

namespace micromouse {

/// Geometria e imperfeições do robô simulado.
struct ParametrosRoboSimulado {
  double bitolaMm = 100.0;           ///< Distância entre as rodas (a mesma da `ConfiguracaoOdometria`).
  double fatorEsquerda = 1.0;        ///< Velocidade real da roda esquerda ÷ a da tabela.
  double fatorDireita = 1.0;         ///< Velocidade real da roda direita ÷ a da tabela.
  double sensorFrontalMm = 50.0;     ///< Do centro do robô até a face do ToF frontal.
  double sensorLateralMm = 30.0;     ///< Do centro do robô até a face de cada ToF lateral.
  double meiaLarguraMm = 60.0;       ///< Metade da largura do chassi (para a colisão).
  double meioComprimentoMm = 50.0;   ///< Metade do comprimento do chassi (para a colisão).
  double espessuraParedeMm = 12.0;   ///< Espessura das paredes e das colunas.
};

/// Robô "real" que anda de verdade (em ponto flutuante) no labirinto: recebe os comandos dos
/// motores, anda com a velocidade da tabela vezes um fator por roda (o erro que o modo estimado
/// não vê), gira pela diferença entre as rodas e mede o que os 3 ToF e o giroscópio veriam.
///
/// Coordenadas: `x` cresce para o Leste e `y` para o Norte, em mm, com o centro da célula
/// (linha, coluna) em (`coluna` × 180, `linha` × 180). O rumo é 0° para o Norte e cresce para a
/// esquerda, como o giroscópio.
///
/// Os ToF são raios retos a partir da face de cada sensor, até a primeira parede (de espessura
/// `espessuraParedeMm`) ou coluna. Sem parede até 1200 mm, a leitura é 8190 (fora de alcance).
/// Qualquer canto ou meio de lado do chassi dentro de uma parede conta como colisão.
class RoboSimulado : public IMotores {
 public:
  RoboSimulado(const LabirintoSimulado& labirinto, const ParametrosRoboSimulado& parametros = ParametrosRoboSimulado{},
               const ConfiguracaoDeslocamento& tabela = ConfiguracaoDeslocamento{});

  /// Coloca o robô no centro da célula, virado para a direção, com um desvio opcional.
  void posicionar(const Pose& pose, double desvioLateralMm = 0.0, double desvioRumoGraus = 0.0);

  /// @copydoc IMotores::definirPotencia
  void definirPotencia(const PotenciaRodas& potencia) override;

  /// @copydoc IMotores::parar
  void parar() override { potencia_ = {0, 0}; }

  /// Anda `dtMs` com a potência atual.
  void avancar(uint32_t dtMs);

  /// Distâncias brutas dos 3 ToF, a partir da face de cada sensor, em mm.
  DistanciasLaterais medirTof() const;

  /// Taxa de giro real, em décimos de °/s (positivo = esquerda), para o `ImuSimulado`.
  int16_t taxaGiroDecimosDps() const;

  double xMm() const { return x_; }
  double yMm() const { return y_; }
  double rumoGraus() const;

  /// Célula que contém o centro do robô.
  PosicaoCelula celula() const;

  /// Distância do centro do robô ao centro da célula em que está, ao longo do rumo (positivo =
  /// passou do centro), em mm.
  double erroAoLongoMm() const;

  /// Distância do centro do robô ao centro da célula, para o lado (positivo = à direita), em mm.
  double erroLateralMm() const;

  /// Diferença do rumo real para o múltiplo de 90° mais próximo, em graus.
  double erroRumoGraus() const;

  /// Distância real percorrida por cada roda desde `posicionar`, em mm (positivo = para a frente):
  /// o que um encoder ideal mediria, para os testes com os encoders reais.
  double distanciaRodaEsquerdaMm() const { return rodaEsquerdaMm_; }
  double distanciaRodaDireitaMm() const { return rodaDireitaMm_; }

  /// `true` se o chassi encostou numa parede em algum momento.
  bool colidiu() const { return colidiu_; }

  /// Tempo total com o chassi encostado, em ms.
  uint32_t msEmContato() const { return msEmContato_; }

  /// Onde foi a primeira colisão (centro do robô, em mm, e rumo, em graus), para o log do teste.
  double xColisaoMm() const { return xColisao_; }
  double yColisaoMm() const { return yColisao_; }
  double rumoColisaoGraus() const { return rumoColisao_; }

  PotenciaRodas potencia() const { return potencia_; }

  /// `true` se o ponto está dentro de uma parede, de uma coluna ou fora do labirinto.
  bool dentroDeParede(double x, double y) const;

 private:
  double raio(double ox, double oy, double ux, double uy) const;
  void conferirColisao();

  const LabirintoSimulado& labirinto_;
  ParametrosRoboSimulado parametros_;
  EstimadorDeslocamento tabela_;
  PotenciaRodas potencia_ = {0, 0};
  double x_ = 0.0;
  double y_ = 0.0;
  double rumoRad_ = 0.0;
  double taxaRadPorS_ = 0.0;
  bool colidiu_ = false;
  uint32_t msEmContato_ = 0;
  double rodaEsquerdaMm_ = 0.0;
  double rodaDireitaMm_ = 0.0;
  double xColisao_ = 0.0;
  double yColisao_ = 0.0;
  double rumoColisao_ = 0.0;
};

}  // namespace micromouse
