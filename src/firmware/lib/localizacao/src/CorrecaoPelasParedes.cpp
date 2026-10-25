#include "CorrecaoPelasParedes.h"

#include "ConfiguracaoControle.h"

namespace micromouse {

namespace {

constexpr int64_t CELULA_UM = static_cast<int64_t>(TAMANHO_CELULA_MM) * 1000;
constexpr int64_t CENTRO_A_PAREDE_UM = static_cast<int64_t>(DISTANCIA_CENTRO_A_PAREDE_MM) * 1000;

/// Centro de célula mais próximo de `posicaoUm` (os centros ficam nos múltiplos de `CELULA_UM`).
int64_t centroMaisProximo(int64_t posicaoUm) {
  const int64_t dobro = 2 * posicaoUm + CELULA_UM;
  int64_t quociente = dobro / (2 * CELULA_UM);
  if (dobro % (2 * CELULA_UM) != 0 && dobro < 0) {
    quociente--;
  }
  return quociente * CELULA_UM;
}

}  // namespace

int64_t posicaoLateralUm(const Odometria& odometria) {
  switch (odometria.pose().direcao) {
    case Direcao::Norte:
      return odometria.posicaoLesteMicrometros();
    case Direcao::Sul:
      return -static_cast<int64_t>(odometria.posicaoLesteMicrometros());
    case Direcao::Leste:
      return -static_cast<int64_t>(odometria.posicaoNorteMicrometros());
    case Direcao::Oeste:
      return odometria.posicaoNorteMicrometros();
  }
  return 0;
}

DesvioLateral medirDesvioLateral(const DistanciasLaterais& leitura, uint16_t limiarParedeMm, uint16_t recuoSensorMm) {
  DesvioLateral desvio;
  const bool esquerda = leitura.esquerdaMm <= limiarParedeMm;
  const bool direita = leitura.direitaMm <= limiarParedeMm;
  const int64_t esquerdaUm = (static_cast<int64_t>(leitura.esquerdaMm) + recuoSensorMm) * 1000;
  const int64_t direitaUm = (static_cast<int64_t>(leitura.direitaMm) + recuoSensorMm) * 1000;
  if (esquerda && direita) {
    desvio.referencia = ReferenciaLateral::DuasParedes;
    desvio.desvioUm = static_cast<int32_t>((esquerdaUm - direitaUm) / 2);
  } else if (esquerda) {
    desvio.referencia = ReferenciaLateral::SoEsquerda;
    desvio.desvioUm = static_cast<int32_t>(esquerdaUm - CENTRO_A_PAREDE_UM);
  } else if (direita) {
    desvio.referencia = ReferenciaLateral::SoDireita;
    desvio.desvioUm = static_cast<int32_t>(CENTRO_A_PAREDE_UM - direitaUm);
  }
  return desvio;
}

CorrecaoPelasParedes::CorrecaoPelasParedes(const ConfiguracaoCorrecaoParedes& config)
    : config_(config), avanco_(config.deslocamento) {}

ResultadoCorrecaoParedes CorrecaoPelasParedes::aplicar(Odometria& odometria, const DistanciasLaterais& leitura,
                                                       MovimentoRobo movimento, bool corrigirAvanco) {
  ResultadoCorrecaoParedes resultado;
  if (movimento == MovimentoRobo::Manobrando) {
    avanco_.esquecerLaterais();
    resultado.lateral = ReferenciaLateral::Manobrando;
    resultado.avanco = ResultadoCorrecao::Girando;
    ultima_ = resultado;
    return resultado;
  }

  // Ao longo do avanço primeiro: a lateral não depende dele, mas a célula sim.
  if (corrigirAvanco) {
    const PotenciaRodas sinal = movimento == MovimentoRobo::Avancando ? PotenciaRodas{1, 1} : PotenciaRodas{0, 0};
    const Correcao correcao = avanco_.avaliar(odometria, leitura, sinal);
    resultado.avanco = correcao.resultado;
    if (correcao.resultado == ResultadoCorrecao::PelaFrente || correcao.resultado == ResultadoCorrecao::PelaLateral) {
      odometria.ajustarPosicao(correcao.deltaUm, 0);
      resultado.deltaAvancoUm = correcao.deltaUm;
    }
  }

  const DesvioLateral desvio =
      medirDesvioLateral(leitura, config_.limiarParedeLateralMm, config_.distanciaSensorLateralAoCentroMm);
  resultado.lateral = desvio.referencia;
  if (desvio.referencia != ReferenciaLateral::SemParede) {
    const int64_t atualUm = posicaoLateralUm(odometria);
    const int64_t alvoUm = centroMaisProximo(atualUm) + desvio.desvioUm;
    const int64_t diferencaUm = alvoUm - atualUm;
    const int64_t modulo = diferencaUm < 0 ? -diferencaUm : diferencaUm;
    if (modulo > static_cast<int64_t>(config_.correcaoLateralMaximaMm) * 1000) {
      resultado.lateral = ReferenciaLateral::Descartada;
    } else {
      const int32_t deltaUm = static_cast<int32_t>(diferencaUm * config_.ganhoCorrecaoLateralPercentual / 100);
      odometria.ajustarPosicao(0, deltaUm);
      resultado.deltaLateralUm = deltaUm;
      correcoesLaterais_++;
    }
  }
  ultima_ = resultado;
  return resultado;
}

const char* nomeDaReferenciaLateral(ReferenciaLateral referencia) {
  switch (referencia) {
    case ReferenciaLateral::SemParede:
      return "sem_parede";
    case ReferenciaLateral::DuasParedes:
      return "duas_paredes";
    case ReferenciaLateral::SoEsquerda:
      return "so_esquerda";
    case ReferenciaLateral::SoDireita:
      return "so_direita";
    case ReferenciaLateral::Descartada:
      return "descartada";
    case ReferenciaLateral::Manobrando:
      return "manobrando";
  }
  return "sem_parede";
}

}  // namespace micromouse
