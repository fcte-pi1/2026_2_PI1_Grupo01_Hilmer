#pragma once

/// @file ControladorEnvio.h
/// @brief Buffer de telemetria e reenvio sem perda nem duplicação (#238).

#include <stdint.h>

#include "AmostraTelemetria.h"
#include "BufferCircular.h"
#include "ConfiguracaoReenvio.h"
#include "IEnvioTelemetria.h"
#include "ITrava.h"
#include "PoliticaReenvio.h"

namespace micromouse {

/// Contadores para o log e o dashboard.
struct ContadoresEnvio {
  uint32_t amostrasConfirmadas = 0;       ///< Gravadas pelo servidor.
  uint32_t amostrasJaExistentes = 0;      ///< O servidor já tinha (reenvio de um lote cuja resposta se perdeu).
  uint32_t amostrasDescartadas = 0;       ///< Perdidas porque o servidor recusou o lote (`400`, `404`...).
  uint32_t tentativas = 0;                ///< Chamadas de envio de lote.
  uint32_t falhas = 0;                    ///< Tentativas que não confirmaram o lote.
  uint32_t reenvios = 0;                  ///< Tentativas de um lote que já tinha sido tentado antes.
  uint32_t respostasInconsistentes = 0;   ///< `2xx` cujas contagens não fecham com o tamanho do lote.
};

/// O que o `ciclo()` fez.
enum class ResultadoCiclo : uint8_t {
  Ocioso,      ///< Nada a enviar.
  Aguardando,  ///< Esperando o tempo da próxima tentativa.
  Enviado,     ///< Um lote foi confirmado e saiu do buffer.
  Falhou,      ///< Sem confirmação: o lote continua no buffer e será tentado de novo.
  Descartado,  ///< O servidor recusou o lote: ele saiu do buffer, e a fila segue.
  Pausado,     ///< Resposta que não se resolve sozinha (`401`...): o envio parou.
  Encerrada    ///< A corrida foi encerrada no servidor.
};

/// Guarda a telemetria num buffer circular e a entrega ao servidor, sem perder amostras quando a rede
/// cai e sem duplicá-las quando ela volta.
///
/// **Duas tarefas.** A coleta chama `registrar()` (rápido, nunca usa a rede). A tarefa de rede chama
/// `ciclo()` de tempos em tempos; é ela que faz as chamadas bloqueantes. As duas compartilham o
/// buffer pela `ITrava`, que só é segurada em cópias curtas, nunca durante uma chamada de rede.
///
/// **Sem perda.** Um lote só sai do buffer depois de resposta de sucesso cujas contagens fecham
/// (`gravadas + existentes == tamanho`). Sem isso, ele fica e é reenviado, em ordem, com espera
/// crescente (`PoliticaReenvio`). Se o buffer encher, a amostra mais antiga é sobrescrita e contada
/// em `amostrasPerdidas()`.
///
/// **Sem duplicata.** O `numeroSequencia` é dado em `registrar()` e nunca muda. Se a resposta se
/// perde, o lote é reenviado com os mesmos números, e o servidor ignora o que já tem.
///
/// **Encerramento.** `solicitarEncerramento()` marca o fim da corrida. A chamada de encerramento só
/// sai quando o buffer esvazia, porque o servidor consolida as métricas pela telemetria gravada. Se
/// o buffer não esvaziar a tempo, `encerramentoAtrasado()` fica verdadeiro e o robô pode seguir sem
/// esperar, mas as tentativas continuam até esvaziar e encerrar.
///
/// Uso típico:
/// @code
/// ControladorEnvio envio(clienteHttp, trava);
/// envio.iniciarCorrida(corridaId);               // depois do POST /corridas
/// envio.registrar(amostra);                      // coleta, a cada 1 s
/// // tarefa de rede, a cada 100 ms:
/// ResultadoCiclo r = envio.ciclo(millis());
/// @endcode
class ControladorEnvio {
 public:
  ControladorEnvio(IEnvioTelemetria& envio, ITrava& trava, const ConfiguracaoReenvio& config = ConfiguracaoReenvio{});

  /// Começa a telemetria de uma corrida: esvazia o buffer, zera os contadores e recomeça a
  /// numeração em 1.
  void iniciarCorrida(uint32_t corridaId);

  /// Guarda uma amostra no buffer. Dá a ela o próximo `numeroSequencia` (o que vier na amostra é
  /// ignorado). Nunca bloqueia e nunca usa a rede.
  /// @return O número de sequência dado.
  uint32_t registrar(AmostraTelemetria amostra);

  /// Faz uma tentativa de envio, se já for hora. Bloqueia pelo tempo da chamada de rede.
  /// @param agoraMs Relógio em ms (`millis()`), com volta do contador tratada.
  ResultadoCiclo ciclo(uint32_t agoraMs);

  /// Marca o fim da corrida: depois que o buffer esvaziar, a corrida é encerrada no servidor.
  /// Registre antes todas as amostras da corrida.
  void solicitarEncerramento(uint32_t agoraMs);

  bool encerramentoSolicitado() const { return encerramentoSolicitado_; }
  /// `true` quando o servidor confirmou o encerramento.
  bool encerrada() const { return encerrada_; }
  /// `true` se o encerramento foi pedido há mais de `tempoLimiteEncerramentoMs` e ainda não saiu.
  bool encerramentoAtrasado(uint32_t agoraMs) const;

  /// `true` se o envio parou por uma resposta que exige intervenção (`401`...).
  bool pausado() const { return pausado_; }
  /// Volta a tentar depois de resolver o que pausou o envio.
  void retomar();

  /// Amostras no buffer, esperando confirmação.
  uint16_t ocupacao() const { return buffer_.tamanho(); }
  /// Amostras sobrescritas por falta de espaço, antes de confirmadas.
  uint32_t amostrasPerdidas() const { return buffer_.perdidas(); }
  const ContadoresEnvio& contadores() const { return contadores_; }
  /// Espera, em ms, que a próxima falha vai impor.
  uint32_t proximaEsperaMs() const { return politica_.proximaEsperaMs(); }

 private:
  using Buffer = BufferCircular<AmostraTelemetria, CAPACIDADE_BUFFER_AMOSTRAS>;

  ResultadoCiclo enviarLote(uint64_t indiceInicial, uint16_t quantidade, uint32_t agoraMs);
  ResultadoCiclo tentarEncerrar(uint32_t agoraMs);
  ResultadoCiclo registrarFalha(uint32_t agoraMs);

  IEnvioTelemetria& envio_;
  ITrava& trava_;
  ConfiguracaoReenvio config_;
  PoliticaReenvio politica_;
  Buffer buffer_;
  AmostraTelemetria lote_[TAMANHO_MAXIMO_LOTE];  ///< Cópia do lote em envio (só a tarefa de rede usa).
  ContadoresEnvio contadores_;

  uint32_t corridaId_ = 0;
  bool corridaDefinida_ = false;
  uint32_t proximaSequencia_ = 1;
  bool esperando_ = false;          ///< `true` depois de uma falha, até chegar `proximaTentativaMs_`.
  uint32_t proximaTentativaMs_ = 0;
  bool temUltimoInicio_ = false;
  uint64_t ultimoInicioTentado_ = 0;

  bool encerramentoSolicitado_ = false;
  bool encerrada_ = false;
  bool pausado_ = false;
  uint32_t instanteSolicitacaoMs_ = 0;
};

}  // namespace micromouse
