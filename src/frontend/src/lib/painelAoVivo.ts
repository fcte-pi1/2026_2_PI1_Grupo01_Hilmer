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
 * Valor de `telemetria.estado_corrida` que o robô envia depois de calibrar e antes de largar
 * (Aguardando Largada, #242). Essas leituras mostram o robô parado na célula de partida: não contam
 * para a velocidade, o gráfico nem o cronômetro.
 */
export const ESTADO_AGUARDANDO_LARGADA = 'aguardando';

/** `true` se a leitura foi enviada com o robô aguardando a largada. */
export const ehLeituraDeLargada = (leitura: Telemetria) => leitura.estado_corrida === ESTADO_AGUARDANDO_LARGADA;

/** As leituras da corrida em si, sem as de antes da largada. */
export const leiturasDaCorrida = (leituras: Telemetria[]) => leituras.filter((l) => !ehLeituraDeLargada(l));

/**
 * Aplica uma linha da tabela `corridas` (consulta inicial ou evento do Realtime).
 *
 * - Corrida `em_andamento` nova (outro `id`): começa a acompanhar, com leituras e mapa zerados, em
 *   `aguardando` (o robô criou a corrida depois de calibrar e ainda não largou). O painel só vai
 *   para `em_execucao` quando chegar uma leitura de depois da largada.
 * - `concluida` ou `nao_concluida` da corrida acompanhada: encerra o painel; o tempo passa a
 *   ser o `tempo_final` gravado no banco, quando houver.
 * - Linhas de outras corridas já encerradas são ignoradas.
 */
export function aplicarCorrida(painel: PainelAoVivo, corrida: Corrida): PainelAoVivo {
  const mesma = painel.corrida?.id === corrida.id;

  if (corrida.status === 'em_andamento') {
    if (mesma) return { ...painel, corrida };
    return { estado: 'aguardando', corrida, leituras: [], tempo: 0, mapa: [] };
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
 * - Leitura com `estado_corrida = 'aguardando'` (#242) entra no painel, mas não tira o painel de
 *   `aguardando` nem avança o cronômetro. A primeira leitura de depois da largada o põe em `em_execucao`.
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
  if (ehLeituraDeLargada(leitura) && painel.estado === 'aguardando') {
    return { ...painel, leituras };
  }

  const tempo = Math.max(painel.tempo, leitura.tempo_decorrido);
  // A primeira leitura de depois da largada é o que põe o painel em execução.
  const estado: EstadoPainel = painel.estado === 'aguardando' ? 'em_execucao' : painel.estado;

  if (leitura.evento === 'objetivo_alcancado') {
    return { ...painel, leituras, estado: 'concluido', tempo: leitura.tempo_decorrido };
  }
  if (tempo >= LIMITE_TEMPO_S) {
    return { ...painel, leituras, estado: 'nao_concluido', tempo: LIMITE_TEMPO_S };
  }
  return { ...painel, leituras, estado, tempo };
}

/** Lado de uma célula do labirinto, em metros (18 cm, regra do Micromouse). */
export const LADO_CELULA_M = 0.18;

/**
 * Distância percorrida, em metros: cada troca de célula entre leituras seguidas vale 0,18 m.
 * Uma leitura que pula células (o robô andou duas entre um envio e outro) conta as duas.
 * As leituras de antes da largada entram só como ponto de partida, porque o robô está parado nelas.
 */
export function distanciaPercorrida(leituras: Telemetria[]): number {
  const ordenadas = [...leituras].sort((a, b) => a.numero_sequencia - b.numero_sequencia);
  let celulas = 0;
  for (let i = 1; i < ordenadas.length; i++) {
    const de = ordenadas[i - 1];
    const para = ordenadas[i];
    celulas += Math.abs(para.posicao_linha - de.posicao_linha) + Math.abs(para.posicao_coluna - de.posicao_coluna);
  }
  return celulas * LADO_CELULA_M;
}

/**
 * Velocidade média pela distância (#557): distância percorrida ÷ tempo da corrida, em m/s.
 * Serve quando o robô manda `velocidade = 0`. Devolve `null` sem movimento ou sem tempo.
 *
 * @param tempo Tempo da corrida, em segundos. Sem ele, usa o `tempo_decorrido` da última leitura.
 */
export function velocidadePelaDistancia(leituras: Telemetria[], tempo?: number | null): number | null {
  const distancia = distanciaPercorrida(leituras);
  const duracao = tempo ?? leiturasDaCorrida(leituras).reduce((maior, l) => Math.max(maior, l.tempo_decorrido), 0);
  if (distancia <= 0 || !(duracao > 0)) return null;
  return distancia / duracao;
}

/** Valores dos cards da tela ao vivo, calculados a partir das leituras. */
export interface MetricasPainel {
  /** Tensão da última leitura, em V (`null` sem leituras). */
  tensao: number | null;
  /** Corrente da última leitura, em mA. */
  correnteMa: number;
  /**
   * Velocidade média, em m/s: a média das velocidades instantâneas; se o robô as mandou zeradas,
   * a distância percorrida ÷ tempo. `null` sem dado nenhum (a tela mostra "—", e não "0.00 m/s").
   */
  velocidadeMedia: number | null;
  /** `true` quando a média veio da distância, e não das velocidades enviadas pelo robô. */
  mediaPelaDistancia: boolean;
  /** Maior velocidade instantânea, em m/s; `null` se o robô não mandou nenhuma acima de zero. */
  velocidadePico: number | null;
}

/**
 * @param tempo Tempo da corrida, em segundos, para a média pela distância (padrão: a última leitura).
 */
export function calcularMetricas(leituras: Telemetria[], tempo?: number | null): MetricasPainel {
  const ultima = leituras.at(-1); // a bateria vale também na largada
  const naCorrida = leiturasDaCorrida(leituras); // a velocidade, só depois da largada
  const soma = naCorrida.reduce((total, l) => total + l.velocidade, 0);
  const pico = naCorrida.reduce((maximo, l) => Math.max(maximo, l.velocidade), 0);
  const pelaDistancia = soma > 0 ? null : velocidadePelaDistancia(leituras, tempo);
  return {
    tensao: ultima ? ultima.tensao_bateria : null,
    correnteMa: Math.round((ultima?.corrente_bateria ?? 0) * 1000),
    velocidadeMedia: soma > 0 ? soma / naCorrida.length : pelaDistancia,
    mediaPelaDistancia: pelaDistancia !== null,
    velocidadePico: pico > 0 ? pico : null,
  };
}

/**
 * Pontos do gráfico de velocidade × tempo. Se o robô mandou todas as velocidades zeradas, cada ponto
 * passa a ser a média acumulada pela distância até aquela leitura (o gráfico não fica uma linha no zero).
 *
 * Custa O(n log n): uma ordenação e uma passada acumulando a distância. Os componentes a chamam dentro
 * de `useMemo`, para não refazer a conta a cada render.
 */
export function serieDeVelocidade(leituras: Telemetria[]): { leituras: Telemetria[]; estimada: boolean } {
  const naCorrida = leiturasDaCorrida(leituras);
  if (naCorrida.some((l) => l.velocidade > 0)) return { leituras: naCorrida, estimada: false };
  const ordenadas = [...leituras].sort((a, b) => a.numero_sequencia - b.numero_sequencia);
  const estimadas: Telemetria[] = [];
  let celulas = 0;
  ordenadas.forEach((l, i) => {
    const anterior = ordenadas[i - 1];
    if (anterior) {
      celulas += Math.abs(l.posicao_linha - anterior.posicao_linha) + Math.abs(l.posicao_coluna - anterior.posicao_coluna);
    }
    if (ehLeituraDeLargada(l)) return; // a largada só entra como ponto de partida
    const distancia = celulas * LADO_CELULA_M;
    estimadas.push({ ...l, velocidade: distancia > 0 && l.tempo_decorrido > 0 ? distancia / l.tempo_decorrido : 0 });
  });
  return { leituras: estimadas, estimada: estimadas.some((l) => l.velocidade > 0) };
}

/** Fração do limite de 10:00 já usada, entre 0 e 1, para o marcador do cronômetro. */
export function fracaoDoLimite(tempo: number): number {
  return Math.min(1, Math.max(0, tempo / LIMITE_TEMPO_S));
}
