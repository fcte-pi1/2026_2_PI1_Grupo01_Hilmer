#include "GatilhoLargada.h"

namespace micromouse {

GatilhoLargada::GatilhoLargada(const ConfiguracaoGatilhoLargada& config) : config_(config) {}

void GatilhoLargada::reiniciar() {
  estado_ = EstadoGatilho::Desarmado;
  tempoMs_ = 0;
  temFrente_ = false;
}

bool GatilhoLargada::atualizar(const DistanciasLaterais* leituraNova, uint32_t dtMs) {
  if (estado_ == EstadoGatilho::Disparado) {
    return true;
  }
  if (leituraNova != nullptr) {
    frenteMm_ = leituraNova->frenteMm;
    temFrente_ = true;
  }
  if (!temFrente_) {
    return false;
  }
  const bool livre = frenteMm_ >= config_.limiarFrenteLivreMm;
  const bool coberta = frenteMm_ <= config_.limiarFrenteCobertaMm;

  switch (estado_) {
    case EstadoGatilho::Desarmado:
      if (livre) {
        estado_ = EstadoGatilho::Armado;
      }
      break;
    case EstadoGatilho::Armado:
      if (coberta) {
        estado_ = EstadoGatilho::Coberto;
        tempoMs_ = 0;
      }
      break;
    case EstadoGatilho::Coberto:
      if (coberta) {
        tempoMs_ += dtMs;
      } else if (livre) {
        // A mão saiu: vale como gesto só se ficou o tempo mínimo.
        estado_ = tempoMs_ >= config_.tempoMinimoCobertoMs ? EstadoGatilho::Contando : EstadoGatilho::Armado;
        tempoMs_ = 0;
      }
      // Entre os dois limiares, a mão está saindo: espera a frente ficar livre.
      break;
    case EstadoGatilho::Contando:
      if (coberta) {
        estado_ = EstadoGatilho::Coberto;
        tempoMs_ = 0;
        break;
      }
      tempoMs_ += dtMs;
      if (tempoMs_ >= config_.esperaAposGestoMs) {
        estado_ = EstadoGatilho::Disparado;
      }
      break;
    case EstadoGatilho::Disparado:
      break;
  }
  return disparado();
}

const char* nomeDoEstadoGatilho(EstadoGatilho estado) {
  switch (estado) {
    case EstadoGatilho::Desarmado:
      return "desarmado";
    case EstadoGatilho::Armado:
      return "armado";
    case EstadoGatilho::Coberto:
      return "coberto";
    case EstadoGatilho::Contando:
      return "contando";
    case EstadoGatilho::Disparado:
      return "disparado";
  }
  return "desarmado";
}

}  // namespace micromouse
