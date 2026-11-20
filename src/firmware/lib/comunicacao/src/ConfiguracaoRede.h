#pragma once

/// @file ConfiguracaoRede.h
/// @brief Parâmetros do Wi-Fi e das chamadas HTTPS ao Supabase (#238, cliente da #236).
///
/// ATENÇÃO: os valores abaixo são PROVISÓRIOS. Nada foi medido com o Wi-Fi do laboratório. Ajuste na
/// bancada com a rede real; para mudar um valor, altere só este arquivo.

#include <stddef.h>
#include <stdint.h>

#include "ConfiguracaoReenvio.h"

namespace micromouse {

/// Tempo máximo de uma chamada HTTPS (conexão, TLS e resposta), em ms. Passado esse tempo, a chamada
/// conta como "sem resposta" e o lote é tentado de novo, com espera crescente.
constexpr uint32_t TEMPO_LIMITE_REDE_MS = 5000;

/// Intervalo entre dois ciclos da tarefa de rede, em ms. Cada ciclo envia no máximo um lote.
constexpr uint32_t PERIODO_CICLO_REDE_MS = 100;

/// Tempo para o Wi-Fi conectar depois de um `WiFi.begin()`, em ms. Passado esse tempo, a tentativa
/// conta como falha e a próxima espera a espera crescente.
constexpr uint32_t TEMPO_LIMITE_CONEXAO_WIFI_MS = 10000;

/// Espera depois da primeira falha de conexão do Wi-Fi, em ms; dobra a cada falha seguida.
constexpr uint32_t ESPERA_MINIMA_WIFI_MS = 1000;

/// Teto da espera entre tentativas de conectar o Wi-Fi, em ms.
constexpr uint32_t ESPERA_MAXIMA_WIFI_MS = 30000;

/// Intervalo entre duas buscas de comando do painel (`POST /robo/comandos/proximo`), em ms: é o atraso
/// típico entre o clique no painel e o robô agir (mais a chamada HTTPS, de 0,1 a 1 s). Cada busca é
/// uma chamada HTTPS a mais, na mesma conexão reaproveitada do envio da telemetria.
constexpr uint32_t PERIODO_CONSULTA_COMANDO_MS = 1500;

/// Intervalo entre dois envios de logs de eventos ao painel (`POST /robo/logs`), em ms. Cada envio leva
/// até `LINHAS_POR_LOTE_DE_LOGS` linhas; sem linhas pendentes, não há chamada.
constexpr uint32_t PERIODO_ENVIO_LOGS_MS = 2000;

/// Intervalo entre duas linhas de log com os contadores do envio, em ms.
constexpr uint32_t PERIODO_LOG_ENVIO_MS = 10000;

/// Labirinto informado na criação da corrida (`POST /corridas`). Provisório, como o 4×4 fixo do
/// `main.cpp`: a escolha do labirinto ainda não chega ao robô.
constexpr const char* LABIRINTO_CORRIDA_PADRAO = "4x4";

/// Maior JSON de uma amostra no lote, em bytes, com folga. O teste `test_envio_supabase` confere
/// que a pior amostra possível (todos os campos no maior tamanho) cabe.
constexpr size_t TAMANHO_MAXIMO_AMOSTRA_JSON = 280;

/// Maior JSON de um lote (`{"corrida_id":...,"amostras":[...]}`), em bytes, com o `\0`. O cliente
/// HTTP reserva esse espaço uma vez só, sem alocação dinâmica: cerca de 28 KB de RAM.
constexpr size_t TAMANHO_MAXIMO_LOTE_JSON = 64 + TAMANHO_MAXIMO_LOTE * (TAMANHO_MAXIMO_AMOSTRA_JSON + 1);

/// Esperas da reconexão do Wi-Fi, no formato da `PoliticaReenvio` (que também serve para ela).
inline ConfiguracaoReenvio configuracaoEsperaWifi() {
  ConfiguracaoReenvio config;
  config.esperaMinimaMs = ESPERA_MINIMA_WIFI_MS;
  config.esperaMaximaMs = ESPERA_MAXIMA_WIFI_MS;
  return config;
}

}  // namespace micromouse
