/// Regras da Edge Function `corridas`: validação dos corpos e tipos compartilhados.
/// Sem dependência de Deno nem do Supabase, para rodar também nos testes com Node.

export const TIPOS_LABIRINTO = ["4x4", "8x4", "12x4"] as const;
export type TipoLabirinto = (typeof TIPOS_LABIRINTO)[number];

export const STATUS_FINAIS = ["concluida", "nao_concluida"] as const;
export type StatusFinal = (typeof STATUS_FINAIS)[number];

/** Linha da tabela `corridas`, como devolvida pelo banco. */
export interface Corrida {
  id: number;
  labirinto_id: number;
  numero_tentativa: number;
  data_hora_inicio: string;
  data_hora_fim: string | null;
  tempo_final: number | null;
  velocidade_media: number | null;
  velocidade_pico: number | null;
  consumo_bateria: number | null;
  status: string;
}

export interface DadosEncerramento {
  status: StatusFinal;
  tempo_final?: number;
  velocidade_media?: number;
  velocidade_pico?: number;
  consumo_bateria?: number;
}

export type Validacao<T> = { ok: true; valor: T } | { ok: false; mensagem: string };

function ehObjeto(corpo: unknown): corpo is Record<string, unknown> {
  return typeof corpo === "object" && corpo !== null && !Array.isArray(corpo);
}

/** Corpo de `POST /corridas`: `{ "labirinto": "4x4" | "8x4" | "12x4" }`. O labirinto é obrigatório. */
export function validarCriacao(corpo: unknown): Validacao<{ tipo: TipoLabirinto }> {
  if (!ehObjeto(corpo)) {
    return { ok: false, mensagem: "o corpo deve ser um objeto JSON" };
  }
  const tipo = corpo.labirinto;
  if (tipo === undefined || tipo === null) {
    return { ok: false, mensagem: "o campo 'labirinto' é obrigatório" };
  }
  if (!(TIPOS_LABIRINTO as readonly unknown[]).includes(tipo)) {
    return { ok: false, mensagem: `labirinto inválido; use um de: ${TIPOS_LABIRINTO.join(", ")}` };
  }
  return { ok: true, valor: { tipo: tipo as TipoLabirinto } };
}

const CAMPOS_NUMERICOS = ["tempo_final", "velocidade_media", "velocidade_pico", "consumo_bateria"] as const;

/**
 * Corpo de `POST /corridas/{id}/encerrar`: `status` obrigatório (`concluida` ou `nao_concluida`) e
 * métricas numéricas opcionais, finitas e não negativas.
 */
export function validarEncerramento(corpo: unknown): Validacao<DadosEncerramento> {
  if (!ehObjeto(corpo)) {
    return { ok: false, mensagem: "o corpo deve ser um objeto JSON" };
  }
  const status = corpo.status;
  if (status === undefined || status === null) {
    return { ok: false, mensagem: "o campo 'status' é obrigatório" };
  }
  if (!(STATUS_FINAIS as readonly unknown[]).includes(status)) {
    return { ok: false, mensagem: `status inválido; use um de: ${STATUS_FINAIS.join(", ")}` };
  }
  const dados: DadosEncerramento = { status: status as StatusFinal };
  for (const campo of CAMPOS_NUMERICOS) {
    const valor = corpo[campo];
    if (valor === undefined || valor === null) {
      continue;
    }
    if (typeof valor !== "number" || !Number.isFinite(valor) || valor < 0) {
      return { ok: false, mensagem: `o campo '${campo}' deve ser um número finito e não negativo` };
    }
    dados[campo] = valor;
  }
  return { ok: true, valor: dados };
}

/** Identificador da corrida na URL: só dígitos, maior que zero e dentro do inteiro de 32 bits do banco. */
export function analisarId(texto: string): number | null {
  if (!/^[0-9]+$/.test(texto)) {
    return null;
  }
  const id = Number(texto);
  return id > 0 && id <= 2147483647 ? id : null;
}
