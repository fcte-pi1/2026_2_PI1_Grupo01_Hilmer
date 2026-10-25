#include "ExecutorMovimentos.h"

#include "Configuracao.h"

namespace micromouse {

namespace {

constexpr int64_t CELULA_UM = static_cast<int64_t>(TAMANHO_CELULA_MM) * 1000;

/// Centro de célula mais próximo de `posicaoUm` (os centros ficam nos múltiplos de `CELULA_UM`).
int64_t centroMaisProximo(int64_t posicaoUm) {
  int64_t dobro = 2 * posicaoUm + CELULA_UM;
  int64_t quociente = dobro / (2 * CELULA_UM);
  if (dobro % (2 * CELULA_UM) != 0 && dobro < 0) {
    quociente--;
  }
  return quociente * CELULA_UM;
}

/// Ângulos do realinhamento, do menor para o maior, com o seno e o cosseno em milésimos e o quanto a
/// quina do chassi vai, com o robô girado desse ângulo, para o lado (78,1 mm × sen(50,2° + θ)) e ao
/// longo do eixo (78,1 mm × cos(50,2° − θ)), em µm. A quina de 120 × 100 mm fica a 50,2° da frente:
/// sem giro, ela está a 60 mm para o lado (a meia largura) e a 50 mm no eixo (o meio comprimento).
struct AnguloRealinhamento {
  int32_t miligraus;
  int32_t senoPorMil;
  int32_t cossenoPorMil;
  int32_t alcanceLateralUm;
  int32_t alcanceAoLongoUm;
};
constexpr AnguloRealinhamento ANGULOS_REALINHAMENTO[] = {
    {5000, 87, 996, 64129, 55039},   {8000, 139, 990, 66375, 57864},  {10000, 174, 985, 67771, 59659},
    {12000, 208, 978, 69084, 61382}, {15000, 259, 966, 70897, 63825}, {20000, 342, 940, 73483, 67506},
    {25000, 423, 906, 75509, 70672}, {30000, 500, 866, 76962, 73301}};
constexpr uint8_t QUANTIDADE_ANGULOS = sizeof(ANGULOS_REALINHAMENTO) / sizeof(ANGULOS_REALINHAMENTO[0]);
static_assert(MEIA_LARGURA_CHASSI_MM == 60 && MEIO_COMPRIMENTO_CHASSI_MM == 50,
              "a tabela ANGULOS_REALINHAMENTO supõe o chassi de 120 × 100 mm");

/// Soma o recuo do sensor à leitura, sem passar do maior `uint16_t`.
uint16_t somarRecuo(uint16_t leituraMm, uint16_t recuoMm) {
  const uint32_t soma = static_cast<uint32_t>(leituraMm) + recuoMm;
  return soma > UINT16_MAX ? UINT16_MAX : static_cast<uint16_t>(soma);
}

}  // namespace

PlanoRealinhamento planejarRealinhamento(int32_t desvioUm, bool recuarPrimeiro, bool trechoLivre,
                                          uint16_t trechoMaximoMm, uint16_t folgaMinimaMm) {
  PlanoRealinhamento plano;
  const int64_t modulo = desvioUm < 0 ? -static_cast<int64_t>(desvioUm) : desvioUm;
  if (modulo == 0 || trechoMaximoMm == 0) {
    return plano;
  }
  const int64_t paredeComFolgaUm = static_cast<int64_t>(DISTANCIA_CENTRO_A_PAREDE_MM - folgaMinimaMm) * 1000;
  // A quina do lado da parede não pode chegar a ela: com o robô `modulo` fora do centro, sobra
  // 84 mm − modulo − folga para o alcance lateral da quina.
  const int64_t alcanceLateralMaximoUm = paredeComFolgaUm - modulo;
  // O menor ângulo cujo trecho cabe no espaço, entre os que não encostam; se nenhum couber, o maior
  // que não encosta, com o trecho máximo dele (correção parcial: o ciclo realinha de novo).
  const AnguloRealinhamento* escolhido = nullptr;
  int64_t trechoUm = 0;
  for (uint8_t i = 0; i < QUANTIDADE_ANGULOS; i++) {
    const AnguloRealinhamento& a = ANGULOS_REALINHAMENTO[i];
    if (a.alcanceLateralUm > alcanceLateralMaximoUm && escolhido != nullptr) {
      break;
    }
    // Com parede no fim do trecho, a quina girada avança no eixo: o trecho cabe no que sobra até ela.
    int64_t maximoUm = static_cast<int64_t>(trechoMaximoMm) * 1000;
    if (!trechoLivre) {
      const int64_t espacoUm = paredeComFolgaUm - a.alcanceAoLongoUm;
      maximoUm = espacoUm < maximoUm ? espacoUm : maximoUm;
    }
    if (maximoUm <= 0) {
      break;
    }
    escolhido = &a;
    trechoUm = maximoUm;
    const int64_t necessario = modulo * 1000 / a.senoPorMil;
    if (necessario <= maximoUm) {
      trechoUm = necessario;
      break;
    }
  }
  if (escolhido == nullptr) {
    return plano;
  }
  // O robô à direita do centro (desvio positivo) vai para a esquerda. De frente, o giro aponta para
  // onde ele vai; de ré, para o lado oposto (a traseira aponta para lá).
  const int32_t lado = desvioUm > 0 ? 1 : -1;
  const int32_t sentido = recuarPrimeiro ? -1 : 1;
  plano.anguloMiligraus = lado * sentido * escolhido->miligraus;
  plano.trechoUm = static_cast<int32_t>(sentido * trechoUm);
  plano.voltaUm = static_cast<int32_t>(-plano.trechoUm * static_cast<int64_t>(escolhido->cossenoPorMil) / 1000);
  plano.deslocamentoUm = static_cast<int32_t>(lado * trechoUm * escolhido->senoPorMil / 1000);
  return plano;
}

ExecutorMovimentos::ExecutorMovimentos(const ConfiguracaoExecucao& config, const ConfiguracaoControle& controle,
                                       const ConfiguracaoGiro& giro, const ConfiguracaoRealinhamento& realinhamento)
    : config_(config),
      configControle_(controle),
      configGiro_(giro),
      configRealinhamento_(realinhamento),
      controleMovimento_(controle),
      controleGiro_(giro) {}

bool ExecutorMovimentos::iniciarRealinhamento(const PlanoRealinhamento& plano) {
  if (ocupado() || plano.trechoUm == 0) {
    return false;
  }
  iniciar(Movimento::AvancarCelula);  // zera o estado comum
  movimento_ = Movimento::Realinhar;
  plano_ = plano;
  etapa_ = EtapaRealinhamento::GiroIda;
  etapaComecou_ = false;
  tempoEtapaMs_ = 0;
  return true;
}

bool ExecutorMovimentos::iniciar(Movimento movimento) {
  if (ocupado() || movimento == Movimento::Realinhar) {
    return false;
  }
  movimento_ = movimento;
  estado_ = EstadoExecucao::Executando;
  falha_ = FalhaExecucao::Nenhuma;
  comecou_ = false;
  tempoMs_ = 0;
  restanteUm_ = 0;
  semLeituraMs_ = 0;
  desdeCalculoMs_ = 0;
  temPotencia_ = false;
  ultimaPotencia_ = {0, 0};
  etapaComecou_ = false;
  tempoEtapaMs_ = 0;
  rumoAlvoMiligraus_ += variacaoRumoMiligraus(movimento);
  rumoNominalMiligraus_ += variacaoRumoMiligraus(movimento);
  return true;
}

EstadoExecucao ExecutorMovimentos::atualizar(const EntradaExecucao& entrada, IMotores& motores) {
  if (!ocupado()) {
    return estado_;
  }
  if (movimento_ == Movimento::AvancarCelula) {
    return atualizarAvanco(entrada, motores);
  }
  return movimento_ == Movimento::Realinhar ? atualizarRealinhamento(entrada, motores)
                                            : atualizarGiro(entrada, motores);
}

void ExecutorMovimentos::cancelar(IMotores& motores) {
  if (!ocupado()) {
    return;
  }
  controleGiro_.cancelar();
  motores.parar();
  estado_ = EstadoExecucao::Ocioso;
}

DistanciasLaterais ExecutorMovimentos::paraOCentro(const DistanciasLaterais& leitura) const {
  const auto lateral = [this](uint16_t leituraMm) {
    return leituraMm > config_.limiarParedeLateralControleMm
               ? LATERAL_SEM_PAREDE_MM
               : somarRecuo(leituraMm, config_.distanciaSensorLateralAoCentroMm);
  };
  return {somarRecuo(leitura.frenteMm, config_.distanciaSensorFrontalAoCentroMm), lateral(leitura.esquerdaMm),
          lateral(leitura.direitaMm)};
}

int16_t ExecutorMovimentos::tetoParaRestante(int64_t restanteUm) const {
  const int64_t rampaUm = static_cast<int64_t>(config_.distanciaDesaceleracaoMm) * 1000;
  if (rampaUm <= 0 || restanteUm >= rampaUm) {
    return INT16_MAX;
  }
  const int64_t faixa = configControle_.potenciaCruzeiro - configControle_.potenciaMinima;
  const int64_t restante = restanteUm < 0 ? 0 : restanteUm;
  return static_cast<int16_t>(configControle_.potenciaMinima + faixa * restante / rampaUm);
}

EstadoExecucao ExecutorMovimentos::terminar(EstadoExecucao estado, FalhaExecucao falha, IMotores& motores) {
  motores.parar();
  estado_ = estado;
  falha_ = falha;
  return estado_;
}

EstadoExecucao ExecutorMovimentos::atualizarAvanco(const EntradaExecucao& entrada, IMotores& motores) {
  if (!comecou_) {
    comecou_ = true;
    destinoUm_ = centroMaisProximo(entrada.posicaoAvancoUm) + CELULA_UM;
    controleMovimento_.reiniciar(rumoAlvoMiligraus_);
  } else {
    tempoMs_ += entrada.dtMs;
    desdeCalculoMs_ += entrada.dtMs;
  }

  restanteUm_ = destinoUm_ - entrada.posicaoAvancoUm;
  if (restanteUm_ <= static_cast<int64_t>(config_.toleranciaChegadaMm) * 1000) {
    return terminar(EstadoExecucao::Concluido, FalhaExecucao::Nenhuma, motores);
  }
  if (tempoMs_ >= config_.tempoMaximoAvancoMs) {
    return terminar(EstadoExecucao::Falha, FalhaExecucao::TempoEsgotado, motores);
  }

  if (entrada.leituraNova != nullptr) {
    semLeituraMs_ = 0;
    const DistanciasLaterais centro = paraOCentro(*entrada.leituraNova);
    controleMovimento_.definirTetoPotenciaBase(tetoParaRestante(restanteUm_));
    ultimaPotencia_ = entrada.giro.valida
                          ? controleMovimento_.calcular(&centro, desdeCalculoMs_, entrada.giro.anguloMiligraus)
                          : controleMovimento_.calcular(&centro, desdeCalculoMs_);
    desdeCalculoMs_ = 0;
    temPotencia_ = true;
    ajustarRumoPeloCorredor(entrada.giro);
    if (controleMovimento_.estado() == EstadoMovimento::ParadoNaParede) {
      const bool pertoDoDestino = restanteUm_ <= static_cast<int64_t>(config_.folgaParadaNaParedeMm) * 1000;
      return pertoDoDestino ? terminar(EstadoExecucao::Concluido, FalhaExecucao::Nenhuma, motores)
                            : terminar(EstadoExecucao::Falha, FalhaExecucao::ParedeNaFrente, motores);
    }
  } else {
    semLeituraMs_ += entrada.dtMs;
    if (semLeituraMs_ >= configControle_.tempoMaximoSemLeituraMs) {
      return terminar(EstadoExecucao::Falha, FalhaExecucao::SemLeituraTof, motores);
    }
  }

  // Entre duas leituras do ToF (cerca de 33 ms), mantém o último comando.
  if (temPotencia_) {
    motores.definirPotencia(ultimaPotencia_);
  } else {
    motores.parar();
  }
  return estado_;
}

void ExecutorMovimentos::ajustarRumoPeloCorredor(const LeituraGiro& giro) {
  const ControladorCentralizacao& lateral = controleMovimento_.centralizacao();
  const int32_t erro = lateral.erroMm() < 0 ? -lateral.erroMm() : lateral.erroMm();
  const int32_t variacao = lateral.derivadaMmPorS() < 0 ? -lateral.derivadaMmPorS() : lateral.derivadaMmPorS();
  const bool andando = controleMovimento_.estado() == EstadoMovimento::Cruzeiro ||
                       controleMovimento_.estado() == EstadoMovimento::Desacelerando;
  if (configRealinhamento_.ajusteRumoPeloCorredorPorMil == 0 || !giro.valida || !andando ||
      lateral.modo() != ModoCentralizacao::DuasParedes ||
      erro > static_cast<int32_t>(configRealinhamento_.erroLateralMaximoRumoMm) ||
      variacao > static_cast<int32_t>(configRealinhamento_.variacaoLateralMaximaRumoMmPorS)) {
    return;
  }
  // Paralelo às duas paredes: o ângulo atual é o eixo do corredor.
  const int64_t passo = static_cast<int64_t>(giro.anguloMiligraus - rumoAlvoMiligraus_) *
                        configRealinhamento_.ajusteRumoPeloCorredorPorMil / 1000;
  int64_t rumo = rumoAlvoMiligraus_ + passo;
  const int64_t minimo = static_cast<int64_t>(rumoNominalMiligraus_) - configRealinhamento_.ajusteRumoMaximoMiligraus;
  const int64_t maximo = static_cast<int64_t>(rumoNominalMiligraus_) + configRealinhamento_.ajusteRumoMaximoMiligraus;
  rumo = rumo < minimo ? minimo : (rumo > maximo ? maximo : rumo);
  rumoAlvoMiligraus_ = static_cast<int32_t>(rumo);
  controleMovimento_.definirRumoAlvo(rumoAlvoMiligraus_);
}

EstadoExecucao ExecutorMovimentos::passoDeGiro(int32_t alvoMiligraus, const EntradaExecucao& entrada,
                                               IMotores& motores) {
  if (!etapaComecou_) {
    if (!entrada.giro.valida) {
      // Sem ângulo não há como começar: espera parado, dentro do mesmo tempo limite do giro.
      tempoEtapaMs_ += entrada.dtMs;
      motores.parar();
      return tempoEtapaMs_ >= configGiro_.tempoMaximoGiroMs ? EstadoExecucao::Falha : EstadoExecucao::Executando;
    }
    etapaComecou_ = true;
    const int32_t angulo = entrada.giro.anguloMiligraus;
    controleGiro_.iniciar(alvoMiligraus - angulo, angulo);
  }
  tempoEtapaMs_ += entrada.dtMs;
  const EstadoGiro estadoGiro = controleGiro_.atualizar(entrada.giro, motores, entrada.dtMs);
  if (estadoGiro == EstadoGiro::Concluido) {
    return EstadoExecucao::Concluido;
  }
  return estadoGiro == EstadoGiro::Timeout ? EstadoExecucao::Falha : EstadoExecucao::Executando;
}

EstadoExecucao ExecutorMovimentos::passoDeTrecho(int32_t distanciaUm, int32_t rumoMiligraus,
                                                 const EntradaExecucao& entrada, IMotores& motores) {
  if (!etapaComecou_) {
    etapaComecou_ = true;
    inicioTrechoUm_ = entrada.posicaoAvancoUm;
    tempoEtapaMs_ = 0;
  } else {
    tempoEtapaMs_ += entrada.dtMs;
  }
  const int64_t andadoUm = entrada.posicaoAvancoUm - inicioTrechoUm_;
  if ((distanciaUm > 0 && andadoUm >= distanciaUm) || (distanciaUm < 0 && andadoUm <= distanciaUm)) {
    motores.parar();
    return EstadoExecucao::Concluido;
  }
  if (tempoEtapaMs_ >= configRealinhamento_.tempoMaximoTrechoMs) {
    return EstadoExecucao::Falha;
  }
  // Reto, na potência mínima, mantendo o rumo pelo giroscópio (positivo = a direita recebe mais).
  const int16_t base = distanciaUm > 0 ? configRealinhamento_.potencia : static_cast<int16_t>(-configRealinhamento_.potencia);
  int32_t correcao = 0;
  if (entrada.giro.valida) {
    correcao = (rumoMiligraus - entrada.giro.anguloMiligraus) * configRealinhamento_.ganhoRumoDecimos / 10000;
    const int32_t limite = configRealinhamento_.potencia * configControle_.correcaoMaximaPercentualDaBase / 100;
    correcao = correcao > limite ? limite : (correcao < -limite ? -limite : correcao);
  }
  motores.definirPotencia({limitarPotencia(base - correcao), limitarPotencia(base + correcao)});
  return EstadoExecucao::Executando;
}

EstadoExecucao ExecutorMovimentos::atualizarGiro(const EntradaExecucao& entrada, IMotores& motores) {
  tempoMs_ += entrada.dtMs;
  const EstadoExecucao resultado = passoDeGiro(rumoAlvoMiligraus_, entrada, motores);
  if (resultado == EstadoExecucao::Concluido) {
    return terminar(EstadoExecucao::Concluido, FalhaExecucao::Nenhuma, motores);
  }
  if (resultado == EstadoExecucao::Falha) {
    return terminar(EstadoExecucao::Falha, FalhaExecucao::GiroTempoEsgotado, motores);
  }
  return estado_;
}

EstadoExecucao ExecutorMovimentos::atualizarRealinhamento(const EntradaExecucao& entrada, IMotores& motores) {
  tempoMs_ += entrada.dtMs;
  const int32_t rumoInclinado = rumoAlvoMiligraus_ + plano_.anguloMiligraus;
  EstadoExecucao resultado = EstadoExecucao::Executando;
  switch (etapa_) {
    case EtapaRealinhamento::GiroIda:
      resultado = passoDeGiro(rumoInclinado, entrada, motores);
      break;
    case EtapaRealinhamento::Trecho:
      resultado = passoDeTrecho(plano_.trechoUm, rumoInclinado, entrada, motores);
      break;
    case EtapaRealinhamento::GiroVolta:
      resultado = passoDeGiro(rumoAlvoMiligraus_, entrada, motores);
      break;
    case EtapaRealinhamento::Volta:
      resultado = passoDeTrecho(plano_.voltaUm, rumoAlvoMiligraus_, entrada, motores);
      break;
  }
  if (resultado == EstadoExecucao::Falha) {
    const bool giro = etapa_ == EtapaRealinhamento::GiroIda || etapa_ == EtapaRealinhamento::GiroVolta;
    return terminar(EstadoExecucao::Falha, giro ? FalhaExecucao::GiroTempoEsgotado : FalhaExecucao::TempoEsgotado,
                    motores);
  }
  if (resultado == EstadoExecucao::Concluido) {
    if (etapa_ == EtapaRealinhamento::Volta) {
      return terminar(EstadoExecucao::Concluido, FalhaExecucao::Nenhuma, motores);
    }
    etapa_ = static_cast<EtapaRealinhamento>(static_cast<uint8_t>(etapa_) + 1);
    etapaComecou_ = false;
    tempoEtapaMs_ = 0;
  }
  return estado_;
}

const char* nomeDoEstadoExecucao(EstadoExecucao estado) {
  switch (estado) {
    case EstadoExecucao::Ocioso:
      return "ocioso";
    case EstadoExecucao::Executando:
      return "executando";
    case EstadoExecucao::Concluido:
      return "concluido";
    case EstadoExecucao::Falha:
      return "falha";
  }
  return "falha";
}

const char* nomeDaFalhaExecucao(FalhaExecucao falha) {
  switch (falha) {
    case FalhaExecucao::Nenhuma:
      return "nenhuma";
    case FalhaExecucao::ParedeNaFrente:
      return "parede_na_frente";
    case FalhaExecucao::SemLeituraTof:
      return "sem_leitura_tof";
    case FalhaExecucao::TempoEsgotado:
      return "tempo_esgotado";
    case FalhaExecucao::GiroTempoEsgotado:
      return "giro_tempo_esgotado";
  }
  return "nenhuma";
}

}  // namespace micromouse
