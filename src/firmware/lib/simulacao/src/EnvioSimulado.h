#pragma once

/// @file EnvioSimulado.h
/// @brief Servidor de telemetria falso para testes: imita a idempotência do backend (#229, #238).

#include <stdint.h>

#include "IEnvioTelemetria.h"
#include "TravaNula.h"

namespace micromouse {

/// Faz o papel do cliente HTTP e do servidor ao mesmo tempo.
///
/// Como o backend (`registrar_telemetria`): grava cada amostra pela chave (corrida, sequência) e
/// ignora as que já tem, devolvendo quantas gravou e quantas já existiam; recusa lote com mais de
/// 100 amostras (`400`) e corrida inexistente (`404`).
///
/// O teste controla a rede: `definirOffline` (nenhuma chamada chega ao servidor),
/// `perderProximasRespostas` (o servidor grava, mas a resposta não volta, o caso difícil) e
/// `responderProximasCom` (o servidor responde um código de erro sem gravar).
///
/// Exemplo:
/// @code
/// EnvioSimulado servidor;
/// servidor.definirOffline(true);
/// // ... o controlador falha e guarda as amostras ...
/// servidor.definirOffline(false);
/// // ... o controlador reenvia, e servidor.amostrasArmazenadas() tem todas, sem duplicata.
/// @endcode
class EnvioSimulado : public IEnvioTelemetria {
 public:
  /// Quantas amostras o servidor falso guarda.
  static constexpr uint16_t CAPACIDADE_SERVIDOR = 1024;

  // ---- controle da rede (o teste decide) ----
  void definirOffline(bool offline) { offline_ = offline; }
  void perderProximasRespostas(uint32_t chamadas) { respostasPerdidas_ = chamadas; }
  void responderProximasCom(int codigoHttp, uint32_t chamadas) {
    codigoForcado_ = codigoHttp;
    chamadasForcadas_ = chamadas;
  }
  /// Faz o servidor tratar esta corrida como inexistente (`404`).
  void marcarCorridaInexistente(uint32_t corridaId) {
    corridaInexistente_ = corridaId;
    temCorridaInexistente_ = true;
  }
  /// Passa a conferir, em toda chamada, que a trava não está segura (a rede nunca roda com trava).
  void vigiarTrava(const TravaNula* trava) { trava_ = trava; }

  // ---- IEnvioTelemetria ----
  ResultadoEnvio enviarLote(uint32_t corridaId, const AmostraTelemetria* amostras, uint16_t quantidade) override {
    registrarChamada();
    ++chamadasLote_;
    ultimoTamanhoLote_ = quantidade;
    if (quantidade > maiorLote_) {
      maiorLote_ = quantidade;
    }
    ResultadoEnvio semResposta;  // código 0
    if (offline_) {
      return semResposta;
    }
    if (chamadasForcadas_ > 0) {
      --chamadasForcadas_;
      return ResultadoEnvio{codigoForcado_, 0, 0};
    }
    if (quantidade > 100) {
      return ResultadoEnvio{400, 0, 0};
    }
    if (temCorridaInexistente_ && corridaId == corridaInexistente_) {
      return ResultadoEnvio{404, 0, 0};
    }
    uint16_t gravadas = 0;
    uint16_t existentes = 0;
    for (uint16_t i = 0; i < quantidade; ++i) {
      if (jaTem(corridaId, amostras[i].numeroSequencia)) {
        ++existentes;
        ++duplicatasIgnoradas_;
      } else if (armazenadas_ < CAPACIDADE_SERVIDOR) {
        corridas_[armazenadas_] = corridaId;
        amostrasGuardadas_[armazenadas_] = amostras[i];
        ++armazenadas_;
        ++gravadas;
      }
    }
    if (respostasPerdidas_ > 0) {
      --respostasPerdidas_;
      return semResposta;  // gravou, mas o robô não recebe a resposta
    }
    return ResultadoEnvio{200, gravadas, existentes};
  }

  ResultadoEnvio encerrarCorrida(uint32_t corridaId) override {
    registrarChamada();
    ++chamadasEncerramento_;
    ResultadoEnvio semResposta;
    if (offline_) {
      return semResposta;
    }
    if (chamadasForcadas_ > 0) {
      --chamadasForcadas_;
      return ResultadoEnvio{codigoForcado_, 0, 0};
    }
    if (temCorridaInexistente_ && corridaId == corridaInexistente_) {
      return ResultadoEnvio{404, 0, 0};
    }
    encerrada_ = true;
    amostrasAoEncerrar_ = armazenadas_;
    return ResultadoEnvio{200, 0, 0};
  }

  // ---- o que o servidor guardou ----
  uint16_t amostrasArmazenadas() const { return armazenadas_; }
  /// Amostra `i`, na ordem em que o servidor as gravou.
  const AmostraTelemetria& armazenada(uint16_t i) const { return amostrasGuardadas_[i]; }
  uint32_t duplicatasIgnoradas() const { return duplicatasIgnoradas_; }
  uint32_t chamadasLote() const { return chamadasLote_; }
  uint32_t chamadasEncerramento() const { return chamadasEncerramento_; }
  uint16_t ultimoTamanhoLote() const { return ultimoTamanhoLote_; }
  uint16_t maiorLote() const { return maiorLote_; }
  bool corridaEncerrada() const { return encerrada_; }
  /// Quantas amostras o servidor tinha quando recebeu o encerramento.
  uint16_t amostrasAoEncerrar() const { return amostrasAoEncerrar_; }
  /// Quantas chamadas chegaram com a trava segura (deve ser zero).
  uint32_t chamadasComTravaSegura() const { return chamadasComTravaSegura_; }

 private:
  void registrarChamada() {
    if (trava_ != nullptr && trava_->profundidade() != 0) {
      ++chamadasComTravaSegura_;
    }
  }

  bool jaTem(uint32_t corridaId, uint32_t sequencia) const {
    for (uint16_t i = 0; i < armazenadas_; ++i) {
      if (corridas_[i] == corridaId && amostrasGuardadas_[i].numeroSequencia == sequencia) {
        return true;
      }
    }
    return false;
  }

  bool offline_ = false;
  uint32_t respostasPerdidas_ = 0;
  int codigoForcado_ = 0;
  uint32_t chamadasForcadas_ = 0;
  uint32_t corridaInexistente_ = 0;
  bool temCorridaInexistente_ = false;
  const TravaNula* trava_ = nullptr;

  uint32_t corridas_[CAPACIDADE_SERVIDOR];
  AmostraTelemetria amostrasGuardadas_[CAPACIDADE_SERVIDOR];
  uint16_t armazenadas_ = 0;
  uint32_t duplicatasIgnoradas_ = 0;
  uint32_t chamadasLote_ = 0;
  uint32_t chamadasEncerramento_ = 0;
  uint16_t ultimoTamanhoLote_ = 0;
  uint16_t maiorLote_ = 0;
  bool encerrada_ = false;
  uint16_t amostrasAoEncerrar_ = 0;
  uint32_t chamadasComTravaSegura_ = 0;
};

}  // namespace micromouse
