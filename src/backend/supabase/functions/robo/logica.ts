/// Regras da Edge Function `robo`: validação dos corpos do comando do painel e do lote de logs.
/// Sem dependência de Deno nem do Supabase, para rodar também nos testes com Node.

import { ehIdValido, ehObjeto } from "../_shared/validacao.ts";
import type { Validacao } from "../_shared/validacao.ts";

/** Comandos que o painel pode mandar ao robô. */
export const COMANDOS = ["iniciar", "parar", "zerar"] as const;
export type Comando = (typeof COMANDOS)[number];

/** Limites dos logs, iguais aos de `registrar_logs_robo` no banco. */
export const MAXIMO_LINHAS_POR_LOTE = 50;
export const MAXIMO_CARACTERES_POR_LINHA = 300;

export interface LoteLogs {
  corrida_id: number | null;
  linhas: string[];
}

/** Resposta de `proximo_comando_robo`: o comando e o `id`, ou os dois nulos. */
export interface ComandoEntregue {
  id: number | null;
  comando: Comando | null;
}

/** Corpo de `POST /robo/comandos`: `{ "comando": "iniciar" | "parar" | "zerar" }`. */
export function validarComando(corpo: unknown): Validacao<Comando> {
  if (!ehObjeto(corpo)) {
    return { ok: false, mensagem: "o corpo deve ser um objeto JSON" };
  }
  const comando = corpo.comando;
  if (typeof comando !== "string" || !(COMANDOS as readonly string[]).includes(comando)) {
    return { ok: false, mensagem: "o campo 'comando' deve ser 'iniciar', 'parar' ou 'zerar'" };
  }
  return { ok: true, valor: comando as Comando };
}

/** Corpo de `POST /robo/logs`: `{ "corrida_id": 7 | null, "linhas": ["...", ...] }`. */
export function validarLogs(corpo: unknown): Validacao<LoteLogs> {
  if (!ehObjeto(corpo)) {
    return { ok: false, mensagem: "o corpo deve ser um objeto JSON" };
  }
  let corridaId: number | null = null;
  if (corpo.corrida_id !== undefined && corpo.corrida_id !== null) {
    if (!ehIdValido(corpo.corrida_id)) {
      return { ok: false, mensagem: "o campo 'corrida_id' deve ser um inteiro positivo ou ficar de fora" };
    }
    corridaId = corpo.corrida_id;
  }
  const linhas = corpo.linhas;
  if (!Array.isArray(linhas) || linhas.length === 0) {
    return { ok: false, mensagem: "o campo 'linhas' deve ser uma lista com pelo menos uma linha" };
  }
  if (linhas.length > MAXIMO_LINHAS_POR_LOTE) {
    return { ok: false, mensagem: `o lote tem mais de ${MAXIMO_LINHAS_POR_LOTE} linhas` };
  }
  const valida = (l: unknown): l is string =>
    typeof l === "string" && l.length >= 1 && l.length <= MAXIMO_CARACTERES_POR_LINHA;
  if (!linhas.every(valida)) {
    return { ok: false, mensagem: `cada linha deve ser um texto de 1 a ${MAXIMO_CARACTERES_POR_LINHA} caracteres` };
  }
  return { ok: true, valor: { corrida_id: corridaId, linhas } };
}
