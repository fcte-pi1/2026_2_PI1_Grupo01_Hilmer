#include "ControladorEnvio.h"

namespace micromouse {

namespace {

/// `true` se `agoraMs` já alcançou `alvoMs`, correto mesmo quando o contador de ms dá a volta.
bool chegouAoInstante(uint32_t agoraMs, uint32_t alvoMs) {
  return static_cast<int32_t>(agoraMs - alvoMs) >= 0;
}

}  // namespace

ControladorEnvio::ControladorEnvio(IEnvioTelemetria& envio, ITrava& trava, const ConfiguracaoReenvio& config)
    : envio_(envio), trava_(trava), config_(config), politica_(config) {}

void ControladorEnvio::iniciarCorrida(uint32_t corridaId) {
  trava_.travar();
  buffer_.limpar();
  proximaSequencia_ = 1;
  trava_.destravar();

  corridaId_ = corridaId;
  corridaDefinida_ = true;
  contadores_ = ContadoresEnvio{};
  politica_.registrarSucesso();
  esperando_ = false;
  temUltimoInicio_ = false;
  encerramentoSolicitado_ = false;
  encerrada_ = false;
  pausado_ = false;
}

uint32_t ControladorEnvio::registrar(AmostraTelemetria amostra) {
  trava_.travar();
  amostra.numeroSequencia = proximaSequencia_++;
  buffer_.adicionar(amostra);
  trava_.destravar();
  return amostra.numeroSequencia;
}

ResultadoCiclo ControladorEnvio::ciclo(uint32_t agoraMs) {
  if (!corridaDefinida_ || encerrada_) {
    return encerrada_ ? ResultadoCiclo::Encerrada : ResultadoCiclo::Ocioso;
  }
  if (pausado_) {
    return ResultadoCiclo::Pausado;
  }
  if (esperando_ && !chegouAoInstante(agoraMs, proximaTentativaMs_)) {
    return ResultadoCiclo::Aguardando;
  }
  esperando_ = false;

  uint64_t indiceInicial = 0;
  trava_.travar();
  const uint16_t quantidade = buffer_.copiar(lote_, config_.tamanhoMaximoLote, indiceInicial);
  trava_.destravar();

  if (quantidade > 0) {
    return enviarLote(indiceInicial, quantidade, agoraMs);
  }
  if (encerramentoSolicitado_) {
    return tentarEncerrar(agoraMs);
  }
  return ResultadoCiclo::Ocioso;
}

ResultadoCiclo ControladorEnvio::enviarLote(uint64_t indiceInicial, uint16_t quantidade, uint32_t agoraMs) {
  ++contadores_.tentativas;
  if (temUltimoInicio_ && indiceInicial == ultimoInicioTentado_) {
    ++contadores_.reenvios;
  }
  temUltimoInicio_ = true;
  ultimoInicioTentado_ = indiceInicial;

  // A chamada de rede bloqueia; a trava não está segura aqui. Enquanto ela dura, a coleta pode
  // sobrescrever as amostras mais antigas, por isso a confirmação é por índice absoluto.
  const ResultadoEnvio resposta = envio_.enviarLote(corridaId_, lote_, quantidade);

  switch (PoliticaReenvio::classificar(resposta.codigoHttp)) {
    case ClasseResposta::Sucesso: {
      if (static_cast<uint32_t>(resposta.gravadas) + resposta.existentes != quantidade) {
        ++contadores_.respostasInconsistentes;
        return registrarFalha(agoraMs);  // não dá para saber o que o servidor guardou: tenta de novo
      }
      trava_.travar();
      buffer_.removerAte(indiceInicial + quantidade);
      trava_.destravar();
      contadores_.amostrasConfirmadas += resposta.gravadas;
      contadores_.amostrasJaExistentes += resposta.existentes;
      politica_.registrarSucesso();  // com atraso acumulado, segue já para o próximo lote
      return ResultadoCiclo::Enviado;
    }
    case ClasseResposta::Repetir:
      return registrarFalha(agoraMs);
    case ClasseResposta::DescartarLote: {
      trava_.travar();
      const uint16_t removidas = buffer_.removerAte(indiceInicial + quantidade);
      trava_.destravar();
      ++contadores_.falhas;
      contadores_.amostrasDescartadas += removidas;
      politica_.registrarSucesso();  // o servidor respondeu: não é falta de rede
      return ResultadoCiclo::Descartado;
    }
    case ClasseResposta::Pausar:
    default:
      ++contadores_.falhas;
      pausado_ = true;
      return ResultadoCiclo::Pausado;
  }
}

ResultadoCiclo ControladorEnvio::tentarEncerrar(uint32_t agoraMs) {
  const ResultadoEnvio resposta = envio_.encerrarCorrida(corridaId_);
  switch (PoliticaReenvio::classificar(resposta.codigoHttp)) {
    case ClasseResposta::Sucesso:
      encerrada_ = true;
      return ResultadoCiclo::Encerrada;
    case ClasseResposta::Repetir:
      return registrarFalha(agoraMs);
    case ClasseResposta::DescartarLote:
      // O servidor não conhece a corrida (ou recusou o pedido): insistir não adianta.
      ++contadores_.falhas;
      encerrada_ = true;
      return ResultadoCiclo::Descartado;
    case ClasseResposta::Pausar:
    default:
      ++contadores_.falhas;
      pausado_ = true;
      return ResultadoCiclo::Pausado;
  }
}

ResultadoCiclo ControladorEnvio::registrarFalha(uint32_t agoraMs) {
  ++contadores_.falhas;
  esperando_ = true;
  proximaTentativaMs_ = agoraMs + politica_.registrarFalha();
  return ResultadoCiclo::Falhou;
}

void ControladorEnvio::solicitarEncerramento(uint32_t agoraMs) {
  if (encerramentoSolicitado_) {
    return;
  }
  instanteSolicitacaoMs_ = agoraMs;
  encerramentoSolicitado_ = true;
}

bool ControladorEnvio::encerramentoAtrasado(uint32_t agoraMs) const {
  return encerramentoSolicitado_ && !encerrada_ &&
         static_cast<uint32_t>(agoraMs - instanteSolicitacaoMs_) > config_.tempoLimiteEncerramentoMs;
}

void ControladorEnvio::retomar() {
  pausado_ = false;
  politica_.registrarSucesso();
  esperando_ = false;
}

}  // namespace micromouse
