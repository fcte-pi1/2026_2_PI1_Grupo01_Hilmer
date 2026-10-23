#include "ProtecaoMotorTravado.h"

namespace micromouse {

namespace {

constexpr int32_t modulo(int32_t valor) { return valor < 0 ? -valor : valor; }

}  // namespace

ProtecaoMotorTravado::ProtecaoMotorTravado(const ConfiguracaoProtecaoMotor& config) : config_(config) {}

void ProtecaoMotorTravado::abrirJanela(const MedidasProtecao& medidas) {
  janelaAberta_ = true;
  tempoMs_ = 0;
  menorFrenteMm_ = medidas.frenteMm;
  maiorFrenteMm_ = medidas.frenteMm;
  maiorGiroDecimosDps_ = static_cast<uint16_t>(modulo(medidas.taxaGiroDecimosDps));
}

void ProtecaoMotorTravado::fecharJanela() {
  janelaAberta_ = false;
  tempoMs_ = 0;
}

EstadoProtecao ProtecaoMotorTravado::atualizar(const PotenciaRodas& comandada, const MedidasProtecao& medidas,
                                               uint32_t dtMs) {
  if (estado_ == EstadoProtecao::Travado) {
    return estado_;
  }

  const int32_t maiorPotencia = modulo(comandada.esquerda) > modulo(comandada.direita)
                                    ? modulo(comandada.esquerda)
                                    : modulo(comandada.direita);
  if (maiorPotencia < config_.potenciaAltaMinima || !medidas.frenteValida) {
    fecharJanela();
    estado_ = EstadoProtecao::Normal;
    return estado_;
  }

  if (!janelaAberta_) {
    abrirJanela(medidas);
  } else {
    if (medidas.frenteMm < menorFrenteMm_) {
      menorFrenteMm_ = medidas.frenteMm;
    }
    if (medidas.frenteMm > maiorFrenteMm_) {
      maiorFrenteMm_ = medidas.frenteMm;
    }
    const uint16_t giro = static_cast<uint16_t>(modulo(medidas.taxaGiroDecimosDps));
    if (giro > maiorGiroDecimosDps_) {
      maiorGiroDecimosDps_ = giro;
    }
  }
  tempoMs_ += dtMs;
  estado_ = EstadoProtecao::Monitorando;

  if (tempoMs_ >= config_.tempoLimiteMs) {
    const uint16_t variacaoMm = static_cast<uint16_t>(maiorFrenteMm_ - menorFrenteMm_);
    const bool semMovimentoFrontal = variacaoMm < config_.variacaoMinimaFrenteMm;
    const bool semGiro = maiorGiroDecimosDps_ < config_.taxaGiroMinimaDecimosDps;
    if (semMovimentoFrontal && semGiro) {
      estado_ = EstadoProtecao::Travado;
    } else {
      // O robô se mexeu: recomeça uma janela nova a partir da próxima leitura.
      fecharJanela();
    }
  }
  return estado_;
}

void ProtecaoMotorTravado::reiniciar() {
  fecharJanela();
  estado_ = EstadoProtecao::Normal;
}

}  // namespace micromouse
