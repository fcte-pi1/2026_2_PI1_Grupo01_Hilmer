import { LIMITE_TEMPO_S } from './formatacao';
import type { Corrida, EstadoPainel, MapaCelula, Telemetria } from '../tipos';

/**
 * Estado da tela ao vivo montado a partir das linhas que chegam do Supabase (HU-14, #231; HU-06, #239):
 * a corrida acompanhada (tabela `corridas`), as leituras dela (tabela `telemetria`) e as células
 * mapeadas do labirinto (tabela `mapa_celulas`).
 *
 * As funções deste arquivo são puras: recebem o painel atual e uma linha nova e devolvem o
 * painel seguinte, sem React e sem Supabase. O hook `useTelemetriaAoVivo` só repassa as linhas.
 */
export interface PainelAoVivo {
  estado: EstadoPainel;
  /** Corrida acompanhada; `null` enquanto o robô não larga. */
  corrida: Corrida | null;
  /** Leituras da corrida, em ordem de `numero_sequencia` e sem repetição. */
  leituras: Telemetria[];
  /** Tempo da corrida, em segundos: o `tempo_decorrido` mais recente (congela ao terminar). */
  tempo: number;
  /** Paredes conhecidas do labirinto descobertas pelo robô (`mapa_celulas`). */
  mapa: MapaCelula[];
}

export const PAINEL_INICIAL: PainelAoVivo = {
  estado: 'aguardando',
  corrida: null,
  leituras: [],
  tempo: 0,
  mapa: [],
};

const terminado = (estado: EstadoPainel) => estado === 'concluido' || estado === 'nao_concluido';

/**
 * Aplica uma linha da tabela `corridas` (consulta inicial ou evento do Realtime).
 *
 * - Corrida `em_andamento` nova (outro `id`): começa a acompanhar, com leituras e mapa zerados.
 * - `concluida` ou `nao_concluida` da corrida acompanhada: encerra o painel; o tempo passa a
 *   ser o `tempo_final` gravado no banco, quando houver.
 * - Linhas de outras corridas já encerradas são ignoradas.
 */
export function aplicarCorrida(painel: PainelAoVivo, corrida: Corrida): PainelAoVivo {
  const mesma = painel.corrida?.id === corrida.id;

  if (corrida.status === 'em_andamento') {
    if (mesma) return { ...painel, corrida };
    return { estado: 'em_execucao', corrida, leituras: [], tempo: 0, mapa: [] };
  }

  if (!mesma) return painel;
  return {
    ...painel,
    corrida,
    estado: corrida.status === 'concluida' ? 'concluido' : 'nao_concluido',
    tempo: corrida.tempo_final ?? painel.tempo,
  };
}

/**
 * Aplica uma célula da tabela `mapa_celulas` (#239).
 *
 * - Atualiza as paredes da célula (linha, coluna) ou adiciona se ainda não existir.
 * - Célula de outra corrida é ignorada. Aceita células durante ou após a corrida (mapa final, HU-04).
 */
export function aplicarMapaCelula(
  painel: PainelAoVivo,
  celula: MapaCelula & { corrida_id?: number },
): PainelAoVivo {
  if (celula.corrida_id !== undefined && painel.corrida && celula.corrida_id !== painel.corrida.id) {
    return painel;
  }
  const celulaLimpa: MapaCelula = {
    linha: celula.linha,
    coluna: celula.coluna,
    parede_norte: celula.parede_norte ?? null,
    parede_sul: celula.parede_sul ?? null,
    parede_leste: celula.parede_leste ?? null,
    parede_oeste: celula.parede_oeste ?? null,
  };
  const index = painel.mapa.findIndex((c) => c.linha === celula.linha && c.coluna === celula.coluna);
  if (index >= 0) {
    const novoMapa = [...painel.mapa];
    novoMapa[index] = celulaLimpa;
    return { ...painel, mapa: novoMapa };
  }
  return { ...painel, mapa: [...painel.mapa, celulaLimpa] };
}

/** Aplica uma lista de células do mapa ao painel (#239). */
export function aplicarMapa(
  painel: PainelAoVivo,
  celulas: (MapaCelula & { corrida_id?: number })[],
): PainelAoVivo {
  return celulas.reduce(aplicarMapaCelula, painel);
}

/**
 * Aplica uma linha da tabela `telemetria`.
 *
 * - Só entram leituras da corrida acompanhada e enquanto ela está em execução.
 * - Leitura repetida (mesmo `numero_sequencia`, como num reenvio do buffer do robô, RF16) é
 *   ignorada; leitura atrasada entra na posição certa.
 * - O evento `objetivo_alcancado` conclui o painel (HU-07); passar de 10:00 sem chegar ao
 *   objetivo o encerra como não concluído (RNF-03).
 */
export function aplicarTelemetria(painel: PainelAoVivo, leitura: Telemetria): PainelAoVivo {
  if (!painel.corrida || leitura.corrida_id !== painel.corrida.id || terminado(painel.estado)) {
    return painel;
  }
  if (painel.leituras.some((l) => l.numero_sequencia === leitura.numero_sequencia)) {
    return painel;
  }

  const leituras = [...painel.leituras, leitura].sort((a, b) => a.numero_sequencia - b.numero_sequencia);
  const tempo = Math.max(painel.tempo, leitura.tempo_decorrido);

  if (leitura.evento === 'objetivo_alcancado') {
    return { ...painel, leituras, estado: 'concluido', tempo: leitura.tempo_decorrido };
  }
  if (tempo >= LIMITE_TEMPO_S) {
    return { ...painel, leituras, estado: 'nao_concluido', tempo: LIMITE_TEMPO_S };
  }
  return { ...painel, leituras, tempo };
}

/** Valores dos cards da tela ao vivo, calculados a partir das leituras. */
export interface MetricasPainel {
  /** Tensão da última leitura, em V (`null` sem leituras). */
  tensao: number | null;
  /** Corrente da última leitura, em mA. */
  correnteMa: number;
  /** Média das velocidades instantâneas das leituras, em m/s. */
  velocidadeMedia: number;
  /** Maior velocidade instantânea, em m/s. */
  velocidadePico: number;
}

export function calcularMetricas(leituras: Telemetria[]): MetricasPainel {
  const ultima = leituras.at(-1);
  const soma = leituras.reduce((total, l) => total + l.velocidade, 0);
  return {
    tensao: ultima ? ultima.tensao_bateria : null,
    correnteMa: Math.round((ultima?.corrente_bateria ?? 0) * 1000),
    velocidadeMedia: leituras.length > 0 ? soma / leituras.length : 0,
    velocidadePico: leituras.reduce((maximo, l) => Math.max(maximo, l.velocidade), 0),
  };
}

/** Fração do limite de 10:00 já usada, entre 0 e 1, para o marcador do cronômetro. */
export function fracaoDoLimite(tempo: number): number {
  return Math.min(1, Math.max(0, tempo / LIMITE_TEMPO_S));
}
