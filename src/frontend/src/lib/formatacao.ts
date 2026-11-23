import type { StatusCorrida, TipoLabirinto } from '../tipos';

/** Tempo máximo por desafio, em segundos (RNF-03: 10 minutos). */
export const LIMITE_TEMPO_S = 600;

/** Tentativas permitidas por labirinto. */
export const MAXIMO_TENTATIVAS = 3;

/** Faixa da bateria LiPo 2S (energia.md): 8,4 V em plena carga e corte (UVLO) em 6,6 V. */
export const TENSAO_CHEIA_V = 8.4;
export const TENSAO_CORTE_V = 6.6;

/** Ordem fixa dos labirintos, usada nos nomes "Labirinto 1", "Labirinto 2" e "Labirinto 3". */
export const TIPOS_LABIRINTO: readonly TipoLabirinto[] = ['4x4', '8x4', '12x4'];

/** "Labirinto 2 · 8x4" (seletor do cabeçalho). */
export function nomeLabirinto(tipo: TipoLabirinto): string {
  return `Labirinto ${TIPOS_LABIRINTO.indexOf(tipo) + 1} · ${tipo}`;
}

/** "Labirinto 2 (8×4)" (filtros e tabela do histórico). */
export function nomeLabirintoHistorico(tipo: TipoLabirinto): string {
  return `Labirinto ${TIPOS_LABIRINTO.indexOf(tipo) + 1} (${tipo.replace('x', '×')})`;
}

/** Segundos em "mm:ss.mmm", como no cronômetro do protótipo (ex.: 68.45 → "01:08.450"). */
export function formatarTempo(segundos: number): string {
  const milissegundos = Math.max(0, Math.round(segundos * 1000));
  const minutos = Math.floor(milissegundos / 60000);
  const resto = milissegundos % 60000;
  const s = Math.floor(resto / 1000);
  const ms = resto % 1000;
  return `${String(minutos).padStart(2, '0')}:${String(s).padStart(2, '0')}.${String(ms).padStart(3, '0')}`;
}

/** Segundos inteiros em "m:ss" (ex.: 600 → "10:00"). */
export function formatarMinutos(segundos: number): string {
  return `${Math.floor(segundos / 60)}:${String(Math.floor(segundos % 60)).padStart(2, '0')}`;
}

/** "0.18 m/s". */
export function formatarVelocidade(metrosPorSegundo: number): string {
  return `${metrosPorSegundo.toFixed(2)} m/s`;
}

/** Traço das células sem dado. */
export const TRACO = '—';

/**
 * Velocidade para a tela, sem nunca mostrar "0.00 m/s" (#557):
 * - `null`, zero ou valor inválido viram "—" (sem dado);
 * - um valor positivo que arredondaria para 0.00 (a média de uma corrida em que o robô andou pouco e
 *   ficou parado até o tempo esgotar, por exemplo) vira "< 0.01 m/s": houve movimento.
 */
export function formatarVelocidadeOuTraco(metrosPorSegundo: number | null | undefined): string {
  if (metrosPorSegundo == null || !(metrosPorSegundo > 0)) return TRACO;
  return metrosPorSegundo < 0.005 ? '< 0.01 m/s' : formatarVelocidade(metrosPorSegundo);
}

/**
 * Bateria restante ao fim da corrida, a partir de `corridas.consumo_bateria`.
 *
 * O esquema (#228) admite "mAh ou % consumida", mas quem preenche a coluna é o banco: a função
 * `metricas_da_telemetria` (migração 20261023100000) grava o **percentual consumido** da LiPo 2S,
 * de 0 a 100, pela primeira e pela última tensão. O firmware não envia esse campo. Se algum dia vier
 * um valor fora de 0 a 100 (mAh, por exemplo), aparece "—" em vez de uma porcentagem errada.
 */
export function bateriaRestante(consumoBateria: number | null | undefined): string {
  if (consumoBateria == null || consumoBateria < 0 || consumoBateria > 100) return TRACO;
  return `${Math.round(100 - consumoBateria)}%`;
}

/** "Sim", "Não" ou "—" (em andamento) para a coluna "Desafio cumprido (S/N)" do histórico. */
export function desafioCumprido(status: StatusCorrida): string {
  return status === 'concluida' ? 'Sim' : status === 'nao_concluida' ? 'Não' : TRACO;
}

/**
 * Carga restante (0 a 100 %) estimada pela tensão, linear entre o corte e a carga plena.
 * Provisório: a curva real da LiPo não é linear; a HU-08 define o cálculo final.
 */
export function percentualBateria(tensao: number): number {
  const fracao = (tensao - TENSAO_CORTE_V) / (TENSAO_CHEIA_V - TENSAO_CORTE_V);
  return Math.round(Math.min(1, Math.max(0, fracao)) * 100);
}

/** Nota do labirinto pelo número da tentativa em que o desafio foi cumprido (Tema PI1). */
export function notaDaTentativa(numeroTentativa: number): number {
  const notas: Record<number, number> = { 1: 10, 2: 7.5, 3: 5 };
  return notas[numeroTentativa] ?? 0;
}

/** "1ª", "2ª", "3ª". */
export function ordinal(numero: number): string {
  return `${numero}ª`;
}

const formatoDataHora = new Intl.DateTimeFormat('pt-BR', {
  day: '2-digit',
  month: '2-digit',
  year: 'numeric',
  hour: '2-digit',
  minute: '2-digit',
});

/** ISO 8601 em "27/09/2026 14:32", no fuso do navegador. */
export function formatarDataHora(iso: string): string {
  return formatoDataHora.format(new Date(iso)).replace(',', '');
}

/** Texto da coluna "Tentativa" do histórico (ex.: "1ª Tentativa (Nota 10)"). */
export function descreverTentativa(numeroTentativa: number, status: StatusCorrida): string {
  const detalhe =
    status === 'concluida'
      ? `Nota ${notaDaTentativa(numeroTentativa)}`
      : status === 'nao_concluida'
        ? 'Não concluída'
        : 'Em andamento';
  return `${ordinal(numeroTentativa)} Tentativa (${detalhe})`;
}
