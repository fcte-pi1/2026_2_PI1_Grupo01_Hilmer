#include "IntegradorGiro.h"

namespace micromouse {

namespace {

constexpr int32_t MEIA_VOLTA_MILIGRAUS = 180000;
constexpr int32_t VOLTA_MILIGRAUS = 360000;

/// Média arredondada para o inteiro mais próximo (também para somas negativas).
int16_t mediaArredondada(int32_t soma, uint16_t quantidade) {
  const int32_t metade = quantidade / 2;
  return static_cast<int16_t>(soma >= 0 ? (soma + metade) / quantidade : (soma - metade) / quantidade);
}

}  // namespace

IntegradorGiro::IntegradorGiro(const ConfiguracaoIntegradorGiro& config) : config_(config) {}

void IntegradorGiro::iniciarCalibracao() {
  estado_ = EstadoCalibracao::Calibrando;
  amostras_ = 0;
  somaAmostras_ = 0;
}

EstadoCalibracao IntegradorGiro::adicionarAmostraCalibracao(int16_t taxaDecimosDps) {
  if (estado_ != EstadoCalibracao::Calibrando) {
    return estado_;
  }
  if (amostras_ == 0) {
    menorAmostra_ = taxaDecimosDps;
    maiorAmostra_ = taxaDecimosDps;
  } else {
    if (taxaDecimosDps < menorAmostra_) {
      menorAmostra_ = taxaDecimosDps;
    }
    if (taxaDecimosDps > maiorAmostra_) {
      maiorAmostra_ = taxaDecimosDps;
    }
  }
  somaAmostras_ += taxaDecimosDps;
  ++amostras_;

  if (amostras_ < config_.amostrasCalibracao) {
    return estado_;
  }
  const int32_t variacao = static_cast<int32_t>(maiorAmostra_) - menorAmostra_;
  if (variacao > config_.variacaoMaximaCalibracaoDecimosDps) {
    estado_ = EstadoCalibracao::Recusada;
  } else {
    vies_ = mediaArredondada(somaAmostras_, amostras_);
    estado_ = EstadoCalibracao::Calibrado;
  }
  return estado_;
}

int32_t IntegradorGiro::atualizar(int16_t taxaDecimosDps, uint32_t dtMs) {
  int32_t corrigida = static_cast<int32_t>(taxaDecimosDps) - vies_;
  const int32_t zona = config_.zonaMortaDecimosDps;
  if (corrigida <= zona && corrigida >= -zona) {
    corrigida = 0;
  }
  acumulado_ += static_cast<int64_t>(corrigida) * dtMs;
  return anguloMiligraus();
}

int32_t IntegradorGiro::anguloMiligraus() const {
  // 1 mili-grau = 10 unidades de "décimos de °/s × ms".
  return static_cast<int32_t>(acumulado_ / 10);
}

int32_t IntegradorGiro::anguloNormalizadoMiligraus() const {
  int32_t angulo = anguloMiligraus() % VOLTA_MILIGRAUS;  // (−360°, 360°)
  if (angulo > MEIA_VOLTA_MILIGRAUS) {
    angulo -= VOLTA_MILIGRAUS;
  } else if (angulo <= -MEIA_VOLTA_MILIGRAUS) {
    angulo += VOLTA_MILIGRAUS;
  }
  return angulo;
}

}  // namespace micromouse
