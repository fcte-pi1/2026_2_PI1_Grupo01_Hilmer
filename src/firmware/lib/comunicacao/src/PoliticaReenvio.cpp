#include "PoliticaReenvio.h"

namespace micromouse {

PoliticaReenvio::PoliticaReenvio(const ConfiguracaoReenvio& config)
    : config_(config), proximaEsperaMs_(config.esperaMinimaMs) {}

ClasseResposta PoliticaReenvio::classificar(int codigoHttp) {
  if (codigoHttp <= 0) {
    return ClasseResposta::Repetir;  // sem resposta: Wi-Fi fora, tempo esgotado, falha de TLS
  }
  if (codigoHttp >= 200 && codigoHttp < 300) {
    return ClasseResposta::Sucesso;
  }
  if (codigoHttp == 408 || codigoHttp == 429 || (codigoHttp >= 500 && codigoHttp < 600)) {
    return ClasseResposta::Repetir;
  }
  if (codigoHttp == 400 || codigoHttp == 404 || codigoHttp == 413 || codigoHttp == 422) {
    return ClasseResposta::DescartarLote;
  }
  return ClasseResposta::Pausar;  // 401, 403, 3xx e qualquer outro: não se resolve sozinho
}

uint32_t PoliticaReenvio::registrarFalha() {
  const uint32_t espera = proximaEsperaMs_;
  ++falhasSeguidas_;
  const uint64_t dobro = static_cast<uint64_t>(proximaEsperaMs_) * config_.fatorEspera;
  proximaEsperaMs_ = dobro > config_.esperaMaximaMs ? config_.esperaMaximaMs : static_cast<uint32_t>(dobro);
  return espera > config_.esperaMaximaMs ? config_.esperaMaximaMs : espera;
}

void PoliticaReenvio::registrarSucesso() {
  falhasSeguidas_ = 0;
  proximaEsperaMs_ = config_.esperaMinimaMs;
}

}  // namespace micromouse
