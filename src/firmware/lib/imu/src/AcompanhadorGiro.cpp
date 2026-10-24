#include "AcompanhadorGiro.h"

namespace micromouse {

namespace {

/// Taxa menos o viés, limitada à faixa do `int16_t`.
int16_t semVies(int16_t taxa, int16_t vies) {
  const int32_t diferenca = static_cast<int32_t>(taxa) - vies;
  return static_cast<int16_t>(diferenca > INT16_MAX ? INT16_MAX : diferenca < INT16_MIN ? INT16_MIN : diferenca);
}

}  // namespace

AcompanhadorGiro::AcompanhadorGiro(IImu& imu, const ConfiguracaoIntegradorGiro& config, uint8_t falhasParaInvalidar)
    : imu_(imu), integrador_(config), falhasParaInvalidar_(falhasParaInvalidar) {}

void AcompanhadorGiro::iniciarCalibracao() {
  integrador_.iniciarCalibracao();
  integrador_.zerarAngulo();
  leitura_.calibracao = integrador_.estadoCalibracao();
  leitura_.anguloMiligraus = 0;
  leitura_.valida = false;
}

void AcompanhadorGiro::zerarAngulo() {
  integrador_.zerarAngulo();
  leitura_.anguloMiligraus = 0;
}

const LeituraGiro& AcompanhadorGiro::ciclo(uint32_t dtMs) {
  int16_t taxa = 0;
  if (!imu_.lerTaxaGiroZ(taxa)) {
    ++leitura_.falhasLeitura;
    if (falhasSeguidas_ < UINT8_MAX) {
      ++falhasSeguidas_;
    }
    if (falhasSeguidas_ >= falhasParaInvalidar_) {
      leitura_.valida = false;
    }
    return leitura_;
  }
  falhasSeguidas_ = 0;

  if (integrador_.estadoCalibracao() == EstadoCalibracao::Calibrando) {
    if (integrador_.adicionarAmostraCalibracao(taxa) == EstadoCalibracao::Recusada) {
      // O robô se mexeu durante a calibração: recomeça com as próximas amostras.
      ++leitura_.calibracoesRecusadas;
      integrador_.iniciarCalibracao();
    }
    leitura_.calibracao = integrador_.estadoCalibracao();
    leitura_.taxaDecimosDps = semVies(taxa, integrador_.viesDecimosDps());
    // Na última amostra a calibração termina e o ângulo (zero) já vale.
    leitura_.valida = leitura_.calibracao == EstadoCalibracao::Calibrado;
    return leitura_;
  }

  leitura_.anguloMiligraus = integrador_.atualizar(taxa, dtMs);
  leitura_.taxaDecimosDps = semVies(taxa, integrador_.viesDecimosDps());
  leitura_.calibracao = integrador_.estadoCalibracao();
  leitura_.valida = leitura_.calibracao == EstadoCalibracao::Calibrado;
  return leitura_;
}

}  // namespace micromouse
