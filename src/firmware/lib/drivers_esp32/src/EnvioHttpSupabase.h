#pragma once

/// @file EnvioHttpSupabase.h
/// @brief `IEnvioTelemetria` real: as Edge Functions do Supabase por HTTPS (#238, cliente da #236).

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "ConfiguracaoRede.h"
#include "ContratoTelemetria.h"
#include "CredenciaisSupabase.h"
#include "IEnvioTelemetria.h"

namespace micromouse {

/// Cliente HTTPS das Edge Functions `corridas` e `telemetria`.
///
/// Toda chamada leva os cabeçalhos `apikey` e `Authorization: Bearer` com a chave `anon` (exigidos
/// pelo gateway) e o `x-device-token` (conferido pela função). O HTTPS é cifrado, mas não confere o
/// certificado do servidor (sem certificado para configurar).
///
/// As chamadas **bloqueiam** até a resposta ou `TEMPO_LIMITE_REDE_MS`: só a tarefa de rede pode
/// usá-las. O corpo do lote é montado num espaço fixo de `TAMANHO_MAXIMO_LOTE_JSON` bytes (sem
/// alocação dinâmica para o lote).
class EnvioHttpSupabase : public IEnvioTelemetria {
 public:
  /// @param credenciais Lidas de `include/credenciais.h` no `main.cpp`. Precisam viver até o fim.
  /// @param tempoLimiteMs Tempo máximo de cada chamada.
  explicit EnvioHttpSupabase(const CredenciaisSupabase& credenciais, uint32_t tempoLimiteMs = TEMPO_LIMITE_REDE_MS);

  /// Configura o certificado do HTTPS. Chame uma vez, na tarefa de rede, antes da primeira chamada.
  void iniciar();

  /// `POST /functions/v1/corridas` com `{"labirinto": ...}`.
  /// @param[out] corridaId Recebe o `id` da corrida criada (só vale com resposta `2xx`).
  /// @return O código HTTP; `2xx` sem `id` legível volta com `codigoHttp` 0 (sem resposta útil).
  ResultadoEnvio criarCorrida(const char* labirinto, uint32_t& corridaId);

  /// `POST /functions/v1/corridas/{id}/labirinto`: informa o tamanho do labirinto que o robô descobriu
  /// (`4x4`, `8x4` ou `12x4`; a corrida nasce como 4x4). Repetir o mesmo tamanho é inofensivo.
  /// @return O código HTTP: `2xx` informado; `409` a corrida já tem posições fora da malha nova ou já
  ///         foi encerrada (não adianta repetir); outro código ou negativo, tente de novo.
  ResultadoEnvio definirLabirintoCorrida(uint32_t corridaId, const char* labirinto);

  /// `POST /functions/v1/robo/comandos/proximo`: busca o próximo comando que o painel mandou ao robô.
  /// @param[out] comando O comando, ou `Nenhum` se não há (só vale com resposta `2xx`).
  /// @return O código HTTP; `2xx` com corpo ilegível volta com `codigoHttp` 0 (sem resposta útil).
  ResultadoEnvio buscarComando(ComandoRobo& comando);

  /// `POST /functions/v1/robo/logs`: envia linhas de log ao painel. `corridaId` 0 envia sem corrida.
  /// Até `LINHAS_POR_LOTE_DE_LOGS` linhas por chamada.
  ResultadoEnvio enviarLogs(uint32_t corridaId, const char* const* linhas, size_t quantidade);

  /// Status mandado no encerramento: `concluida` se o robô chegou ao objetivo, senão `nao_concluida`.
  void definirResultadoCorrida(bool concluida) { concluida_ = concluida; }

  /// Corpo da resposta da última chamada que o servidor recusou (código 400 ou maior; cortado em 160
  /// caracteres), para a bancada mostrar o motivo do erro. Vazio se não houve erro nem resposta.
  const String& ultimaResposta() const { return ultimaResposta_; }

  /// @copydoc IEnvioTelemetria::enviarLote
  ResultadoEnvio enviarLote(uint32_t corridaId, const AmostraTelemetria* amostras, uint16_t quantidade) override;

  /// @copydoc IEnvioTelemetria::encerrarCorrida
  /// Manda `{"status": ...}` com o resultado de `definirResultadoCorrida()`.
  ResultadoEnvio encerrarCorrida(uint32_t corridaId) override;

 private:
  /// Faz um POST e devolve o código HTTP (negativo: sem resposta). Com `resposta`, guarda o corpo.
  int postar(const char* caminho, const char* corpo, size_t tamanho, String* resposta);

  const CredenciaisSupabase& credenciais_;
  uint32_t tempoLimiteMs_;
  bool concluida_ = false;
  String ultimaResposta_;
  WiFiClientSecure cliente_;
  HTTPClient http_;
  char lote_[TAMANHO_MAXIMO_LOTE_JSON];
  /// Corpo do lote de logs: até 10 linhas de 200 caracteres, com as aspas escapadas.
  char logs_[LINHAS_POR_LOTE_DE_LOGS * TAMANHO_MAX_LINHA_DE_LOG * 2 + 64];
};

}  // namespace micromouse
