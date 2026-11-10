#include "ReconexaoWifi.h"

namespace micromouse {

namespace {

/// `true` se `agoraMs` já alcançou `alvoMs`, correto mesmo quando o contador de ms dá a volta.
bool chegou(uint32_t agoraMs, uint32_t alvoMs) { return static_cast<int32_t>(agoraMs - alvoMs) >= 0; }

}  // namespace

ReconexaoWifi::ReconexaoWifi(IRedeWifi& rede, uint32_t tempoLimiteConexaoMs, const ConfiguracaoReenvio& esperas)
    : rede_(rede), tempoLimiteConexaoMs_(tempoLimiteConexaoMs), esperas_(esperas) {}

void ReconexaoWifi::comecarTentativa(uint32_t agoraMs) {
  rede_.iniciarConexao();
  ++tentativas_;
  estado_ = EstadoWifi::Conectando;
  instanteMs_ = agoraMs;
}

void ReconexaoWifi::registrarConexao() {
  estado_ = EstadoWifi::Conectado;
  esperas_.registrarSucesso();
  falhasNaRodada_ = 0;
}

void ReconexaoWifi::registrarTentativaEsgotada(uint32_t agoraMs) {
  rede_.desconectar();
  estado_ = EstadoWifi::Aguardando;
  const uint8_t redes = rede_.quantidadeRedes() > 1 ? 2 : 1;
  if (redes > 1) {
    redeAtual_ = static_cast<uint8_t>((redeAtual_ + 1) % redes);
    rede_.escolherRede(redeAtual_);
    ++trocasDeRede_;
  }
  if (++falhasNaRodada_ >= redes) {
    falhasNaRodada_ = 0;
    instanteMs_ = agoraMs + esperas_.registrarFalha();  // todas falharam: espera crescente
  } else {
    instanteMs_ = agoraMs;  // ainda há rede a tentar nesta rodada: tenta já
  }
}

void ReconexaoWifi::reiniciar() {
  rede_.desconectar();
  redeAtual_ = 0;
  rede_.escolherRede(0);
  falhasNaRodada_ = 0;
  esperas_.registrarSucesso();
  estado_ = EstadoWifi::Aguardando;
  primeiraVez_ = true;
}

bool ReconexaoWifi::ciclo(uint32_t agoraMs) {
  switch (estado_) {
    case EstadoWifi::Conectado:
      if (rede_.conectada()) {
        return true;
      }
      ++quedas_;
      comecarTentativa(agoraMs);  // caiu: tenta de novo já
      return false;

    case EstadoWifi::Conectando:
      if (rede_.conectada()) {
        registrarConexao();
        return true;
      }
      if (static_cast<uint32_t>(agoraMs - instanteMs_) >= tempoLimiteConexaoMs_) {
        registrarTentativaEsgotada(agoraMs);
      }
      return false;

    case EstadoWifi::Aguardando:
    default:
      if (rede_.conectada()) {  // conectou sozinho (reconexão automática do próprio Wi-Fi)
        registrarConexao();
        primeiraVez_ = false;
        return true;
      }
      if (primeiraVez_ || chegou(agoraMs, instanteMs_)) {
        primeiraVez_ = false;
        comecarTentativa(agoraMs);
      }
      return false;
  }
}

}  // namespace micromouse
