#include "ControleMovimento.h"

namespace micromouse {

ControleMovimento::ControleMovimento(const ConfiguracaoControle& config)
    : config_(config), centralizacao_(config), perfil_(config) {}

PotenciaRodas ControleMovimento::calcular(const DistanciasLaterais* leitura, uint32_t dtMs) {
  return calcularInterno(leitura, dtMs, false, 0);
}

PotenciaRodas ControleMovimento::calcular(const DistanciasLaterais* leitura, uint32_t dtMs,
                                          int32_t anguloGiroMiligraus) {
  return calcularInterno(leitura, dtMs, true, anguloGiroMiligraus);
}

PotenciaRodas ControleMovimento::calcularInterno(const DistanciasLaterais* leitura, uint32_t dtMs, bool temAngulo,
                                                 int32_t anguloGiroMiligraus) {
  if (parado()) {
    return {0, 0};
  }

  if (leitura == nullptr) {
    tempoSemLeituraMs_ += dtMs;
    centralizacao_.reiniciar();
    temErroRumoAnterior_ = false;
    temRumoValido_ = false;
    correcaoRumo_ = 0;
    if (tempoSemLeituraMs_ >= config_.tempoMaximoSemLeituraMs) {
      estado_ = EstadoMovimento::ParadoSemLeitura;
      return {0, 0};
    }
    estado_ = EstadoMovimento::SemLeitura;
    return {config_.potenciaMinima, config_.potenciaMinima};
  }
  tempoSemLeituraMs_ = 0;

  int16_t base = perfil_.potenciaBase(leitura->frenteMm);
  if (base > tetoPotenciaBase_) {
    base = tetoPotenciaBase_ < config_.potenciaMinima ? config_.potenciaMinima : tetoPotenciaBase_;
  }
  if (base == 0) {
    estado_ = EstadoMovimento::ParadoNaParede;
    centralizacao_.reiniciar();
    temErroRumoAnterior_ = false;
    temRumoValido_ = false;
    correcaoRumo_ = 0;
    return {0, 0};
  }
  estado_ = base < config_.potenciaCruzeiro ? EstadoMovimento::Desacelerando : EstadoMovimento::Cruzeiro;

  const int32_t correcaoLateral = centralizacao_.calcular(*leitura, dtMs);

  int32_t correcaoRumo = 0;
  if (temAngulo && config_.habilitarCorrecaoRumo) {
    temRumoValido_ = true;
    desvioRumoMiligraus_ = rumoAlvoMiligraus_ - anguloGiroMiligraus;

    // Termo proporcional: ganhoRumoProporcionalDecimos em décimos de potência por grau (ex.: 80 = 8,0 pot/grau).
    // desvioRumoMiligraus_ em mili-graus (1000 = 1°).
    // P = (desvioRumoMiligraus_ * ganhoRumoProporcionalDecimos) / 10000;
    const int32_t p = (desvioRumoMiligraus_ * config_.ganhoRumoProporcionalDecimos) / 10000;

    // Termo derivativo: variação da guinada no tempo (amortecimento de oscilação).
    if (temErroRumoAnterior_ && dtMs > 0) {
      const int32_t derivadaBruta = (desvioRumoMiligraus_ - desvioRumoAnterior_) * 1000 / static_cast<int32_t>(dtMs);
      derivadaRumoMiliGrausPorS_ +=
          (derivadaBruta - derivadaRumoMiliGrausPorS_) * config_.filtroDerivadaPercentual / 100;
    } else {
      derivadaRumoMiliGrausPorS_ = 0;
    }
    desvioRumoAnterior_ = desvioRumoMiligraus_;
    temErroRumoAnterior_ = true;

    const int32_t d = (config_.ganhoRumoDerivativoDecimos * (derivadaRumoMiliGrausPorS_ / 100)) / 1000;
    correcaoRumo = p + d;

    if (correcaoRumo > config_.correcaoRumoMaxima) {
      correcaoRumo = config_.correcaoRumoMaxima;
    } else if (correcaoRumo < -config_.correcaoRumoMaxima) {
      correcaoRumo = -config_.correcaoRumoMaxima;
    }
    correcaoRumo_ = static_cast<int16_t>(correcaoRumo);
  } else {
    temRumoValido_ = false;
    correcaoRumo_ = 0;
    desvioRumoMiligraus_ = 0;
    temErroRumoAnterior_ = false;
  }

  int32_t correcaoTotal = 0;
  if (centralizacao_.modo() == ModoCentralizacao::SemReferencia) {
    // Sem paredes laterais: o rumo pelo giroscópio é a única referência para avançar reto (#190).
    correcaoTotal = correcaoRumo;
  } else {
    // Com parede(s): a centralização lateral posiciona no centro e o giroscópio amortece o rumo.
    correcaoTotal = correcaoLateral + correcaoRumo;
  }

  const int32_t limite = static_cast<int32_t>(base) * config_.correcaoMaximaPercentualDaBase / 100;
  if (correcaoTotal > limite) {
    correcaoTotal = limite;
  } else if (correcaoTotal < -limite) {
    correcaoTotal = -limite;
  }

  return {limitarPotencia(base - correcaoTotal), limitarPotencia(base + correcaoTotal)};
}

EstadoMovimento ControleMovimento::atualizar(ISensorDistancia& lidar, IMotores& motores, uint32_t dtMs) {
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

EstadoMovimento ControleMovimento::atualizar(ISensorDistancia& lidar, IMotores& motores, uint32_t dtMs,
                                             int32_t anguloGiroMiligraus) {
  DistanciasLaterais leitura = {};
  const bool valida = lidar.lerDistanciasLaterais(leitura);
  const PotenciaRodas potencia = calcular(valida ? &leitura : nullptr, dtMs, anguloGiroMiligraus);
  if (parado()) {
    motores.parar();
  } else {
    motores.definirPotencia(potencia);
  }
  return estado_;
}

EstadoMovimento ControleMovimento::atualizar(ISensorDistancia& lidar, IMotores& motores, uint32_t dtMs,
                                             const LeituraGiro& giro) {
  DistanciasLaterais leitura = {};
  const bool valida = lidar.lerDistanciasLaterais(leitura);
  const PotenciaRodas potencia =
      giro.valida ? calcular(valida ? &leitura : nullptr, dtMs, giro.anguloMiligraus)
                  : calcular(valida ? &leitura : nullptr, dtMs);
  if (parado()) {
    motores.parar();
  } else {
    motores.definirPotencia(potencia);
  }
  return estado_;
}

void ControleMovimento::definirRumoAlvo(int32_t rumoAlvoMiligraus) {
  rumoAlvoMiligraus_ = rumoAlvoMiligraus;
  temErroRumoAnterior_ = false;
  desvioRumoMiligraus_ = 0;
  derivadaRumoMiliGrausPorS_ = 0;
  correcaoRumo_ = 0;
}

void ControleMovimento::reiniciar() {
  estado_ = EstadoMovimento::Cruzeiro;
  tempoSemLeituraMs_ = 0;
  centralizacao_.reiniciar();
  temErroRumoAnterior_ = false;
  desvioRumoMiligraus_ = 0;
  derivadaRumoMiliGrausPorS_ = 0;
  correcaoRumo_ = 0;
  temRumoValido_ = false;
}

}  // namespace micromouse
