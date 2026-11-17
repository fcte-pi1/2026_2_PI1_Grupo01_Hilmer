#pragma once

/// @file ContratoTelemetria.h
/// @brief O formato das mensagens trocadas com as Edge Functions do Supabase (#238, cliente da #236).
///
/// Fica fora do cliente HTTP (`EnvioHttpSupabase`, em `drivers_esp32`) para ser testado no
/// computador. O lote segue o contrato de `src/backend/tests/fixtures/lote-telemetria.json`: os nomes
/// dos campos são os das colunas de `telemetria`, e as unidades são as do banco (m/s, V, A e s).
///
/// | Chamada | Corpo enviado | Resposta lida |
/// |---|---|---|
/// | `POST /functions/v1/corridas` | `{"labirinto":"4x4"}` | `id` da corrida criada |
/// | `POST /functions/v1/corridas/{id}/labirinto` | `{"labirinto":"8x4"}` | só o código HTTP |
/// | `POST /functions/v1/telemetria` | `{"corrida_id":7,"amostras":[...]}` | `gravadas` e `existentes` |
/// | `POST /functions/v1/corridas/{id}/encerrar` | `{"status":"concluida"}` | só o código HTTP |
/// | `POST /functions/v1/robo/comandos/proximo` | vazio | `comando` do painel (`iniciar`, `parar`, `zerar`) ou nulo |
/// | `POST /functions/v1/robo/logs` | `{"corrida_id":7,"linhas":["..."]}` | só o código HTTP |

#include <stddef.h>
#include <stdint.h>

#include "AmostraTelemetria.h"
#include "Tipos.h"

namespace micromouse {

/// Escreve o lote em JSON, com as amostras na ordem dada. O campo `evento` só aparece quando existe.
/// Os números com casas decimais saem com três casas (de mm/s, mV, mA e ms para m/s, V, A e s).
/// @return Bytes escritos (sem o `\0`), ou 0 se não couber em `capacidade`.
size_t serializarLote(uint32_t corridaId, const AmostraTelemetria* amostras, uint16_t quantidade, char* saida,
                      size_t capacidade);

/// Lê a resposta do envio de um lote: `{"gravadas": n, "existentes": m, ...}`.
/// @return `false` se o corpo não é um JSON com os dois números inteiros, entre 0 e 65535.
bool lerRespostaLote(const char* corpo, uint16_t& gravadas, uint16_t& existentes);

/// Lê o `id` da corrida devolvida pelo `POST /corridas` (a linha da tabela `corridas`).
/// @return `false` se o corpo não tem um `id` inteiro positivo.
bool lerIdCorrida(const char* corpo, uint32_t& id);

/// Corpo da criação da corrida: `{"labirinto":"4x4"}`.
/// @return Bytes escritos, ou 0 se não couber.
size_t serializarCriacaoCorrida(const char* labirinto, char* saida, size_t capacidade);

/// Corpo do encerramento: `{"status":"concluida"}` ou `{"status":"nao_concluida"}`.
/// @return Bytes escritos, ou 0 se não couber.
size_t serializarEncerramento(bool concluida, char* saida, size_t capacidade);

/// Nome do labirinto no banco (`labirintos.tipo`): `4x4`, `8x4` ou `12x4`; vazio para o mapa de exploração,
/// que não é um labirinto.
const char* nomeDoLabirinto(DimensaoMapa dimensao);

/// Comando que o painel mandou ao robô.
enum class ComandoRobo : uint8_t {
  Nenhum,   ///< Nenhum comando pendente.
  Iniciar,  ///< Larga o robô (o mesmo que o `L`).
  Parar,    ///< Para na hora e volta a esperar a largada (o mesmo que o `X`).
  Zerar     ///< Zera a estimativa e volta a aguardar a largada (o mesmo que o `Z`).
};

/// Lê a resposta de `POST /robo/comandos/proximo`: `{"id": 7, "comando": "iniciar"}`, ou os dois
/// nulos quando não há comando.
/// @param[out] comando O comando, ou `Nenhum` se a resposta traz `"comando": null`.
/// @return `false` se o corpo não é um JSON com `comando` nulo ou um dos três textos conhecidos.
bool lerComandoRobo(const char* corpo, ComandoRobo& comando);

/// Linhas de log por lote enviado ao painel e o maior tamanho de cada uma, com o `\0`: os mesmos
/// limites da função `registrar_logs_robo` no banco (50 linhas de 300 caracteres), com folga para o
/// JSON caber no buffer do cliente.
constexpr size_t LINHAS_POR_LOTE_DE_LOGS = 10;
constexpr size_t TAMANHO_MAX_LINHA_DE_LOG = 200;

/// Corpo de `POST /robo/logs`: `{"corrida_id":7,"linhas":["...", ...]}`. As aspas e as barras das linhas
/// saem escapadas. `corridaId` 0 deixa o campo de fora (o robô ainda não tem corrida).
/// @return Bytes escritos (sem o `\0`), ou 0 se não couber ou se alguma linha for vazia.
size_t serializarLogs(uint32_t corridaId, const char* const* linhas, size_t quantidade, char* saida, size_t capacidade);

/// Monta a amostra da coleta a partir da pose do robô. O `numeroSequencia` fica em zero: quem o dá é
/// o `ControladorEnvio::registrar()`. Tensão e corrente vão zeradas: quem coleta preenche `tensaoBateriaMv`
/// com a leitura da bateria (`BateriaAdc`), e a corrente fica em 0 (a Rev 2.0 não tem sensor de corrente).
/// @param pose Célula e direção do robô.
/// @param tempoDecorridoMs Tempo desde a criação da corrida.
/// @param velocidadeMmPorS Velocidade estimada (sem encoder, #380).
/// @param estado Estado da corrida.
/// @param evento Evento pontual deste instante, ou `Nenhum`.
AmostraTelemetria montarAmostra(const Pose& pose, uint32_t tempoDecorridoMs, int32_t velocidadeMmPorS,
                                EstadoCorrida estado, EventoTelemetria evento);

}  // namespace micromouse
