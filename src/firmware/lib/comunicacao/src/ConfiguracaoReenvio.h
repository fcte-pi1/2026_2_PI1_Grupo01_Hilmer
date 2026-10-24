#pragma once

/// @file ConfiguracaoReenvio.h
/// @brief Parâmetros do buffer de telemetria e do reenvio (#238).
///
/// ATENÇÃO: os valores abaixo são PROVISÓRIOS. Nada foi medido com o Wi-Fi real. A capacidade vem
/// do tempo máximo de uma corrida (RNF-03: 10 minutos) dividido pelo período da telemetria. As
/// esperas do reenvio são um ponto de partida: ajuste na bancada com o Wi-Fi realmente caindo.
/// Para mudar um valor, altere só este arquivo.

#include <stdint.h>

#include "AmostraTelemetria.h"
#include "Configuracao.h"

namespace micromouse {

/// Capacidade do buffer, em amostras: 10 minutos a 1 amostra por segundo = 600. Cada amostra ocupa
/// `sizeof(AmostraTelemetria)` bytes (24), então o buffer usa cerca de 14 KB de RAM, de 320 KB.
constexpr uint16_t CAPACIDADE_BUFFER_AMOSTRAS =
    static_cast<uint16_t>(LIMITE_TEMPO_CORRIDA_MS / PERIODO_TELEMETRIA_MS);

static_assert(CAPACIDADE_BUFFER_AMOSTRAS == 600, "10 min a 1 amostra/s");
static_assert(sizeof(AmostraTelemetria) <= 24, "o buffer deve caber em cerca de 14 KB");

/// Maior lote por requisição, igual ao limite do servidor (`registrar_telemetria` aceita até 100).
constexpr uint16_t TAMANHO_MAXIMO_LOTE = 100;

/// Espera antes de tentar de novo depois da primeira falha, em ms. Volta a este valor depois de um sucesso.
constexpr uint32_t ESPERA_MINIMA_REENVIO_MS = 1000;

/// Teto da espera crescente entre tentativas, em ms.
constexpr uint32_t ESPERA_MAXIMA_REENVIO_MS = 30000;

/// A espera é multiplicada por este fator a cada falha seguida.
constexpr uint8_t FATOR_ESPERA_REENVIO = 2;

/// Tempo, em ms, depois do pedido de encerramento, em que o buffer deveria ter esvaziado. Passado
/// esse tempo, o encerramento é marcado como atrasado (o robô pode seguir sem esperar), mas o envio
/// continua tentando até esvaziar e encerrar a corrida.
constexpr uint32_t TEMPO_LIMITE_ENCERRAMENTO_MS = 10000;

/// Parâmetros usados pela `PoliticaReenvio` e pelo `ControladorEnvio`.
struct ConfiguracaoReenvio {
  uint16_t tamanhoMaximoLote = TAMANHO_MAXIMO_LOTE;
  uint32_t esperaMinimaMs = ESPERA_MINIMA_REENVIO_MS;
  uint32_t esperaMaximaMs = ESPERA_MAXIMA_REENVIO_MS;
  uint8_t fatorEspera = FATOR_ESPERA_REENVIO;
  uint32_t tempoLimiteEncerramentoMs = TEMPO_LIMITE_ENCERRAMENTO_MS;
};

}  // namespace micromouse
