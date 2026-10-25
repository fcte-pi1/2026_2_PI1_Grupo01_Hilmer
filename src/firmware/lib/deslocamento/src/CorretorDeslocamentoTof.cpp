#include "CorretorDeslocamentoTof.h"

#include "ConfiguracaoControle.h"

namespace micromouse {

namespace {

constexpr int64_t CELULA_UM = static_cast<int64_t>(TAMANHO_CELULA_MM) * 1000;
constexpr int64_t MEIA_ESPESSURA_PAREDE_UM = static_cast<int64_t>(ESPESSURA_PAREDE_MM) * 1000 / 2;

/// Divisão inteira arredondada para o inteiro mais próximo (meio arredonda para cima).
int64_t divisaoArredondada(int64_t numerador, int64_t denominador) {
  int64_t dobro = 2 * numerador + denominador;
  int64_t divisor = 2 * denominador;
  int64_t quociente = dobro / divisor;
  if ((dobro % divisor != 0) && ((dobro < 0) != (divisor < 0))) {
    quociente--;
  }
  return quociente;
}

/// Fronteira de célula mais próxima de `posicaoUm`. As fronteiras ficam a meia célula dos
/// centros, que estão nos múltiplos de `CELULA_UM`.
int64_t fronteiraMaisProxima(int64_t posicaoUm) {
  return divisaoArredondada(posicaoUm - CELULA_UM / 2, CELULA_UM) * CELULA_UM + CELULA_UM / 2;
}

}  // namespace

int64_t posicaoAoLongoDoAvancoUm(const Odometria& odometria) {
  switch (odometria.pose().direcao) {
    case Direcao::Norte:
      return odometria.posicaoNorteMicrometros();
    case Direcao::Sul:
      return -static_cast<int64_t>(odometria.posicaoNorteMicrometros());
    case Direcao::Leste:
      return odometria.posicaoLesteMicrometros();
    case Direcao::Oeste:
      return -static_cast<int64_t>(odometria.posicaoLesteMicrometros());
  }
  return 0;
}

CorretorDeslocamentoTof::CorretorDeslocamentoTof(const ConfiguracaoDeslocamento& config)
    : config_(config) {}

void CorretorDeslocamentoTof::esquecerLaterais() {
  esquerda_ = EstadoLado::Desconhecido;
  direita_ = EstadoLado::Desconhecido;
}

CorretorDeslocamentoTof::EstadoLado CorretorDeslocamentoTof::classificarLado(
    EstadoLado anterior, uint16_t leituraMm) const {
  if (leituraMm <= config_.limiarParedeLateralMm) {
    return EstadoLado::Parede;
  }
  if (leituraMm >= config_.limiarParedeLateralMm + config_.histereseParedeLateralMm) {
    return EstadoLado::Livre;
  }
  // Na faixa da histerese: mantém o anterior; sem anterior, ainda não dá para dizer.
  return anterior;
}

bool CorretorDeslocamentoTof::dentroDoLimite(int64_t deltaUm) const {
  const int64_t modulo = deltaUm < 0 ? -deltaUm : deltaUm;
  return modulo <= static_cast<int64_t>(config_.correcaoMaximaMm) * 1000;
}

bool CorretorDeslocamentoTof::corrigirPelaFrente(int64_t posicaoUm, uint16_t frenteMm,
                                                 int64_t& deltaUm) const {
  if (frenteMm > config_.distanciaFrontalMaximaParaCorrigirMm) {
    return false;
  }
  // Distância do centro do robô até a face da parede da frente.
  const int64_t centroAParedeUm =
      (static_cast<int64_t>(frenteMm) + config_.distanciaSensorFrontalAoCentroMm) * 1000;
  // A face fica meia parede antes de uma fronteira de célula: a mais próxima da medida.
  const int64_t faceMedidaUm = posicaoUm + centroAParedeUm;
  const int64_t faceUm = fronteiraMaisProxima(faceMedidaUm + MEIA_ESPESSURA_PAREDE_UM) -
                         MEIA_ESPESSURA_PAREDE_UM;
  deltaUm = (faceUm - centroAParedeUm) - posicaoUm;
  return true;
}

bool CorretorDeslocamentoTof::corrigirPelaLateral(int64_t posicaoUm, EstadoLado anterior,
                                                  EstadoLado atual, int64_t& deltaUm) const {
  if (anterior == EstadoLado::Desconhecido || atual == EstadoLado::Desconhecido ||
      anterior == atual) {
    return false;
  }
  // Ponto do sensor lateral ao longo do eixo de avanço.
  const int64_t avancoSensorUm = static_cast<int64_t>(config_.avancoSensoresLateraisMm) * 1000;
  const int64_t sensorUm = posicaoUm + avancoSensorUm;
  // Avançando, a quina da saída de uma parede fica depois da coluna da fronteira, e a da entrada,
  // antes dela.
  const int64_t fronteiraUm = fronteiraMaisProxima(sensorUm);
  const int64_t quinaUm = atual == EstadoLado::Livre ? fronteiraUm + MEIA_ESPESSURA_PAREDE_UM
                                                     : fronteiraUm - MEIA_ESPESSURA_PAREDE_UM;
  deltaUm = (quinaUm - avancoSensorUm) - posicaoUm;
  return true;
}

Correcao CorretorDeslocamentoTof::avaliar(const Odometria& odometria,
                                          const DistanciasLaterais& leitura,
                                          const EstimadorDeslocamento& estimador) {
  return avaliar(odometria, leitura, estimador.potencia());
}

Correcao CorretorDeslocamentoTof::avaliar(const Odometria& odometria, const DistanciasLaterais& leitura,
                                          const PotenciaRodas& potencia) {
  Correcao correcao;
  const bool girando = (potencia.esquerda > 0 && potencia.direita < 0) || (potencia.esquerda < 0 && potencia.direita > 0);
  if (girando) {
    esquecerLaterais();
    correcao.resultado = ResultadoCorrecao::Girando;
    return correcao;
  }
  const int64_t posicaoUm = posicaoAoLongoDoAvancoUm(odometria);

  const EstadoLado esquerdaAnterior = esquerda_;
  const EstadoLado direitaAnterior = direita_;
  esquerda_ = classificarLado(esquerda_, leitura.esquerdaMm);
  direita_ = classificarLado(direita_, leitura.direitaMm);

  int64_t deltaUm = 0;
  ResultadoCorrecao encontrada = ResultadoCorrecao::SemReferencia;
  // A frente é a referência mais precisa; a lateral só vale com o robô avançando.
  const bool avancando = potencia.esquerda >= 0 && potencia.direita >= 0 &&
                         (potencia.esquerda > 0 || potencia.direita > 0);
  if (corrigirPelaFrente(posicaoUm, leitura.frenteMm, deltaUm)) {
    encontrada = ResultadoCorrecao::PelaFrente;
  } else if (avancando && (corrigirPelaLateral(posicaoUm, esquerdaAnterior, esquerda_, deltaUm) ||
                           corrigirPelaLateral(posicaoUm, direitaAnterior, direita_, deltaUm))) {
    encontrada = ResultadoCorrecao::PelaLateral;
  }

  if (encontrada == ResultadoCorrecao::SemReferencia) {
    return correcao;
  }
  if (!dentroDoLimite(deltaUm)) {
    correcao.resultado = ResultadoCorrecao::Descartada;
    return correcao;
  }
  correcao.resultado = encontrada;
  correcao.deltaUm = static_cast<int32_t>(deltaUm);
  return correcao;
}

}  // namespace micromouse
