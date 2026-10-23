#pragma once

/// @file ProtecaoMotorTravado.h
/// @brief Detecção de motor travado sem encoder, pelo tempo em duty alto sem movimento (#341).

#include <stdint.h>

#include "ConfiguracaoProtecaoMotor.h"
#include "IMotores.h"

namespace micromouse {

/// O que o robô mediu no último ciclo, para dizer se ele se moveu.
struct MedidasProtecao {
  bool frenteValida;           ///< `false` se o ToF frontal não deu leitura válida.
  uint16_t frenteMm;           ///< Distância frontal, em mm (só vale se `frenteValida`).
  int16_t taxaGiroDecimosDps;  ///< Taxa de giro do giroscópio, em décimos de °/s, com sinal.
};

/// Situação da proteção depois da última atualização.
enum class EstadoProtecao : uint8_t {
  Normal,       ///< Sem potência alta, ou sem leitura frontal para concluir algo.
  Monitorando,  ///< Potência alta e o tempo limite está correndo.
  Travado       ///< Potência alta por `tempoLimiteMs` sem movimento: cortar os motores.
};

/// Diz se o robô está travado, sem usar encoder.
///
/// Cada ciclo recebe a potência comandada e as medidas do ToF frontal e do giroscópio. Com
/// `|potência|` de alguma roda a partir de `potenciaAltaMinima`, abre-se uma janela de
/// `tempoLimiteMs`. Se, ao fim dela, a leitura frontal variou menos que `variacaoMinimaFrenteMm`
/// **e** o giroscópio nunca passou de `taxaGiroMinimaDecimosDps`, o robô está travado. Andar em
/// linha reta muda o ToF; girar no lugar muda o giroscópio; só o robô preso não muda nenhum.
///
/// Sem leitura frontal válida não dá para concluir nada, então a contagem recomeça e o estado
/// volta a `Normal`. A potência baixa também zera a contagem. `Travado` é travado: só sai com
/// `reiniciar()`, depois que a navegação decidir o que fazer.
///
/// Uso típico, a cada ciclo da navegação:
/// @code
/// ProtecaoMotorTravado protecao;
/// EstadoProtecao estado = protecao.atualizar(motores.ultimaPotencia(), medidas, 100);
/// if (estado == EstadoProtecao::Travado) { motores.parar(); }
/// @endcode
class ProtecaoMotorTravado {
 public:
  explicit ProtecaoMotorTravado(const ConfiguracaoProtecaoMotor& config = ConfiguracaoProtecaoMotor{});

  /// Avança a proteção em um ciclo.
  /// @param comandada Potência aplicada às rodas neste ciclo.
  /// @param medidas ToF frontal e giroscópio medidos neste ciclo.
  /// @param dtMs Tempo desde a chamada anterior, em ms.
  EstadoProtecao atualizar(const PotenciaRodas& comandada, const MedidasProtecao& medidas, uint32_t dtMs);

  /// Estado depois da última atualização.
  EstadoProtecao estado() const { return estado_; }

  /// `true` se o robô foi considerado travado e ainda não houve `reiniciar()`.
  bool travado() const { return estado_ == EstadoProtecao::Travado; }

  /// Tempo, em ms, que a janela atual de monitoramento já correu.
  uint32_t tempoMonitoradoMs() const { return tempoMs_; }

  /// Destrava a proteção e zera a contagem.
  void reiniciar();

 private:
  void abrirJanela(const MedidasProtecao& medidas);
  void fecharJanela();

  ConfiguracaoProtecaoMotor config_;
  EstadoProtecao estado_ = EstadoProtecao::Normal;
  bool janelaAberta_ = false;
  uint32_t tempoMs_ = 0;
  uint16_t menorFrenteMm_ = 0;
  uint16_t maiorFrenteMm_ = 0;
  uint16_t maiorGiroDecimosDps_ = 0;
};

}  // namespace micromouse
