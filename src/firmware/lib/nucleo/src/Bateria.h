#pragma once

/// @file Bateria.h
/// @brief Leitura da bateria pelo divisor no GPIO34: conversão do pino para a bateria, carga em % e o
/// monitor que avisa quando a bateria fica fraca. Sem Arduino: testado no computador (`test_bateria`).
///
/// O divisor é R1 (100 kΩ, no positivo) e R2 (33 kΩ, no GND): a tensão no pino é a da bateria vezes
/// R2 / (R1 + R2), e só existe com a chave geral ligada. Os valores vêm do esquemático e foram
/// conferidos na bancada geral (tecla `a`).

#include <stdint.h>

namespace micromouse {

constexpr uint32_t BATERIA_R1_OHMS = 100000;
constexpr uint32_t BATERIA_R2_OHMS = 33000;

/// Leituras do ADC que entram na média de cada medida da bateria.
constexpr uint32_t BATERIA_LEITURAS = 16;

/// Abaixo disto a LiPo 2S está descarregada: os motores perdem força e o robô erra mais o deslocamento.
constexpr uint32_t BATERIA_CORTE_MV = 6600;

/// Carga plena da LiPo 2S.
constexpr uint32_t BATERIA_PLENA_MV = 8400;

/// Acima disto a leitura não é de uma LiPo 2S (divisor errado ou outra fonte).
constexpr uint32_t BATERIA_MAXIMA_VALIDA_MV = 8700;

/// Abaixo disto o pino está sem o divisor ligado ou a chave geral está desligada (o robô só na USB).
constexpr uint32_t BATERIA_MINIMA_MEDIDA_MV = 1000;

/// Quanto a bateria precisa subir acima do corte para o aviso de bateria fraca sair (evita o aviso
/// ligar e desligar com a queda de tensão dos motores).
constexpr uint32_t BATERIA_HISTERESE_MV = 200;

/// Tensão da bateria, em mV, a partir da tensão lida no pino, em mV.
constexpr uint32_t bateriaMvDoPino(uint32_t pinoMv) {
  return pinoMv * (BATERIA_R1_OHMS + BATERIA_R2_OHMS) / BATERIA_R2_OHMS;
}

/// Carga da LiPo 2S, de 0 a 100, entre o corte e a carga plena.
constexpr uint8_t bateriaPercentual(uint32_t mv) {
  return mv <= BATERIA_CORTE_MV   ? 0
         : mv >= BATERIA_PLENA_MV ? 100
                                  : static_cast<uint8_t>((mv - BATERIA_CORTE_MV) * 100 / (BATERIA_PLENA_MV - BATERIA_CORTE_MV));
}

/// Situação da bateria para o aviso.
enum class SituacaoBateria : uint8_t {
  SemMedida,  ///< Pino sem tensão: chave geral desligada (só USB) ou divisor não ligado.
  Ok,         ///< Acima do corte.
  Fraca       ///< Abaixo do corte: recarregue antes de continuar.
};

/// Nome da situação para o log: `sem_medida`, `ok` ou `fraca`.
constexpr const char* nomeDaSituacaoBateria(SituacaoBateria situacao) {
  return situacao == SituacaoBateria::Ok ? "ok" : situacao == SituacaoBateria::Fraca ? "fraca" : "sem_medida";
}

/// Acompanha a bateria e diz quando a situação muda, com histerese na volta de `Fraca` para `Ok`.
///
/// @code
/// MonitorBateria monitor;
/// if (monitor.atualizar(bateriaMv)) { /* mudou: avisar */ }
/// @endcode
class MonitorBateria {
 public:
  /// Registra uma medida, em mV.
  /// @return `true` se a situação mudou com ela.
  bool atualizar(uint32_t mv) {
    const SituacaoBateria antes = situacao_;
    if (mv < BATERIA_MINIMA_MEDIDA_MV) {
      situacao_ = SituacaoBateria::SemMedida;
    } else if (mv < BATERIA_CORTE_MV) {
      situacao_ = SituacaoBateria::Fraca;
    } else if (situacao_ != SituacaoBateria::Fraca || mv >= BATERIA_CORTE_MV + BATERIA_HISTERESE_MV) {
      situacao_ = SituacaoBateria::Ok;
    }
    return situacao_ != antes;
  }

  SituacaoBateria situacao() const { return situacao_; }

 private:
  SituacaoBateria situacao_ = SituacaoBateria::SemMedida;
};

}  // namespace micromouse
