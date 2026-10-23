#pragma once

/// @file EncoderEstimado.h
/// @brief `IEncoder` provisório que estima a contagem de uma roda pela potência comandada (#380).

#include <stdint.h>

#include "ConfiguracaoEstimador.h"
#include "IEncoder.h"

namespace micromouse {

/// Encoder sem hardware: soma, a cada ciclo, a distância que a roda deve ter andado (velocidade da
/// tabela para a potência comandada, com a resposta do motor, vezes o tempo) e a entrega como contagem
/// de pulsos. Assim `Odometria` e `ControleMovimento` seguem usando só `IEncoder`: quando o encoder
/// físico for integrado, troca-se a implementação e nada mais muda.
///
/// A contagem aumenta quando a roda avança, como em todo `IEncoder`: potência positiva avança. A
/// distância é guardada em nanômetros (inteiro de 64 bits), então nenhum ciclo perde fração.
///
/// Limites: o estimador não vê patinagem, obstáculo nem queda da bateria. Um robô preso continua
/// "andando" na estimativa; a `ProtecaoMotorTravado` (#341) e a correção pelos ToF cuidam disso.
///
/// Exemplo:
/// @code
/// EncoderEstimado roda;
/// roda.atualizar(400, 100);        // 100 ms em potência 400
/// int32_t pulsos = roda.lerContagem();
/// @endcode
class EncoderEstimado : public IEncoder {
 public:
  explicit EncoderEstimado(const ConfiguracaoEstimador& config = ConfiguracaoEstimador{});

  /// Avança a estimativa em um ciclo.
  /// @param potencia Potência comandada à roda neste ciclo (de `-POTENCIA_MAXIMA` a `POTENCIA_MAXIMA`).
  /// @param dtMs Tempo desde a chamada anterior, em ms. Zero não muda nada.
  void atualizar(int16_t potencia, uint32_t dtMs);

  /// @copydoc IEncoder::lerContagem
  int32_t lerContagem() const override;

  /// @copydoc IEncoder::zerar
  void zerar() override;

  /// Soma uma correção ao deslocamento estimado, em µm (positivo: a roda andou mais que o estimado).
  /// É por aqui que a leitura dos ToF corrige a deriva (parede à frente, abertura lateral).
  void corrigirMicrometros(int32_t micrometros);

  /// Deslocamento acumulado, em µm, com sinal.
  int64_t micrometrosAcumulados() const { return acumuladoNm_ / 1000; }

  /// Velocidade estimada agora, em mm/s, com sinal (já com a resposta do motor).
  int32_t velocidadeMmPorS() const { return static_cast<int32_t>(velocidadeUmPorS_ / 1000); }

 private:
  ConfiguracaoEstimador config_;
  int64_t velocidadeUmPorS_ = 0;  ///< Velocidade estimada, em µm/s.
  int64_t acumuladoNm_ = 0;       ///< Deslocamento, em nm (µm/s × ms = nm), sem perder fração.
};

}  // namespace micromouse
