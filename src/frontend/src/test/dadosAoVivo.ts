import type { Corrida, MapaCelula, Telemetria } from '../tipos';

/**
 * Linhas simuladas de `corridas`, `telemetria` e `mapa_celulas` para os testes da tela ao vivo (#231, #239).
 * Não vêm do robô: os valores são inventados, só com o formato real das tabelas.
 */
export function corridaSimulada(parcial: Partial<Corrida> = {}): Corrida {
  return {
    id: 1,
    labirinto_id: 1,
    numero_tentativa: 1,
    data_hora_inicio: '2026-10-24T14:00:00Z',
    data_hora_fim: null,
    tempo_final: null,
    velocidade_media: null,
    velocidade_pico: null,
    consumo_bateria: null,
    status: 'em_andamento',
    ...parcial,
  };
}

export function leituraSimulada(numeroSequencia: number, parcial: Partial<Telemetria> = {}): Telemetria {
  return {
    id: numeroSequencia,
    corrida_id: 1,
    numero_sequencia: numeroSequencia,
    timestamp: '2026-10-24T14:00:00Z',
    posicao_linha: 0,
    posicao_coluna: 0,
    orientacao: 'N',
    velocidade: 0.2,
    tensao_bateria: 8.0,
    corrente_bateria: 0.5,
    tempo_decorrido: numeroSequencia,
    estado_corrida: 'explorando',
    evento: null,
    ...parcial,
  };
}

export function celulaSimulada(
  linha: number,
  coluna: number,
  parcial: Partial<MapaCelula & { corrida_id: number }> = {},
): MapaCelula & { corrida_id: number } {
  return {
    linha,
    coluna,
    parede_norte: false,
    parede_sul: true,
    parede_leste: false,
    parede_oeste: true,
    corrida_id: 1,
    ...parcial,
  };
}
