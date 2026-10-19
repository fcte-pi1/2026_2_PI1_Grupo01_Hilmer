#include "ControleMovimento.h"

namespace micromouse {

ControleMovimento::ControleMovimento(const ConfiguracaoControle& config)
    : config_(config), centralizacao_(config), perfil_(config) {}

PotenciaRodas ControleMovimento::calcular(const DistanciasLaterais* leitura, uint32_t dtMs) {
  if (parado()) {
    return {0, 0};
  }

  if (leitura == nullptr) {
    tempoSemLeituraMs_ += dtMs;
    centralizacao_.reiniciar();
    if (tempoSemLeituraMs_ >= config_.tempoMaximoSemLeituraMs) {
      estado_ = EstadoMovimento::ParadoSemLeitura;
      return {0, 0};
    }
    estado_ = EstadoMovimento::SemLeitura;
    return {config_.potenciaMinima, config_.potenciaMinima};
  }
  tempoSemLeituraMs_ = 0;

  const int16_t base = perfil_.potenciaBase(leitura->frenteMm);
  if (base == 0) {
    estado_ = EstadoMovimento::ParadoNaParede;
    centralizacao_.reiniciar();
    return {0, 0};
  }
  estado_ = base < config_.potenciaCruzeiro ? EstadoMovimento::Desacelerando : EstadoMovimento::Cruzeiro;

  int32_t correcao = centralizacao_.calcular(*leitura, dtMs);
  const int32_t limite = static_cast<int32_t>(base) * config_.correcaoMaximaPercentualDaBase / 100;
  if (correcao > limite) {
    correcao = limite;
  } else if (correcao < -limite) {
    correcao = -limite;
  }

  return {limitarPotencia(base - correcao), limitarPotencia(base + correcao)};
}

EstadoMovimento ControleMovimento::atualizar(ILidar& lidar, IMotores& motores, uint32_t dtMs) {
  DistanciasLaterais leitura = {};
  const bool valida = lidar.lerDistanciasLaterais(leitura);
  const PotenciaRodas potencia = calcular(valida ? &leitura : nullptr, dtMs);
  if (parado()) {
    motores.parar();
  } else {
    motores.definirPotencia(potencia);
  }
  return estado_;
}

void ControleMovimento::reiniciar() {
  estado_ = EstadoMovimento::Cruzeiro;
  tempoSemLeituraMs_ = 0;
  centralizacao_.reiniciar();
}

}  // namespace micromouse
