/// Roteamento da Edge Function `robo`, sem Deno nem Supabase: recebe os dados da requisição e um
/// repositório, e devolve status e corpo. O `index.ts` só liga isto ao servidor HTTP.
///
/// Três rotas, com duas credenciais diferentes:
/// - `POST /robo/comandos` (o **painel**, com `x-painel-token`): grava um comando para o robô; 202.
/// - `POST /robo/comandos/proximo` (o **robô**, com `x-device-token`): busca o próximo comando pendente.
///   200 sempre que o token vale, com `{ "id": null, "comando": null }` se não há comando.
/// - `POST /robo/logs` (o **robô**): grava um lote de linhas de log; 200 com `{ "gravadas": n }`.

import { tokenValido } from "../_shared/autenticacao.ts";
import { validarComando, validarLogs } from "./logica.ts";
import type { Comando, ComandoEntregue, LoteLogs } from "./logica.ts";

export type CodigoErro = "comando_invalido" | "logs_invalidos" | "logs_grande_demais";

/** Erro de regra de negócio vindo do banco. */
export class ErroDeNegocio extends Error {
  codigo: CodigoErro;
  constructor(codigo: CodigoErro) {
    super(codigo);
    this.codigo = codigo;
  }
}

/** Acesso ao banco. A implementação real chama as funções SQL com a chave de serviço. */
export interface Repositorio {
  enviarComando(comando: Comando): Promise<{ id: number; comando: Comando }>;
  proximoComando(): Promise<ComandoEntregue>;
  registrarLogs(lote: LoteLogs): Promise<number>;
}

export interface Requisicao {
  metodo: string;
  caminho: string;
  tokenDispositivo: string | null;
  tokenPainel: string | null;
  /** Corpo já lido como JSON; `undefined` se estiver vazio e `CORPO_INVALIDO` se não for JSON. */
  corpo: unknown;
}

export interface Resposta {
  status: number;
  corpo: unknown;
}

export interface Dependencias {
  tokenDispositivoEsperado: string | undefined;
  tokenPainelEsperado: string | undefined;
  repositorio: Repositorio;
}

/** Marca o corpo que não pôde ser lido como JSON. */
export const CORPO_INVALIDO = Symbol("corpo_invalido");

function erro(status: number, codigo: string, mensagem: string): Resposta {
  return { status, corpo: { erro: codigo, mensagem } };
}

const STATUS_POR_ERRO: Record<CodigoErro, { status: number; mensagem: string }> = {
  comando_invalido: { status: 400, mensagem: "comando inválido" },
  logs_invalidos: { status: 400, mensagem: "alguma linha de log é inválida; nenhuma foi gravada" },
  logs_grande_demais: { status: 413, mensagem: "o lote tem linhas demais" },
};

/** As partes do caminho depois de `robo` (por exemplo, `/functions/v1/robo/comandos` vira `["comandos"]`). */
function partesDaRota(caminho: string): string[] | null {
  const partes = caminho.split("/").filter((p) => p.length > 0);
  const i = partes.lastIndexOf("robo");
  return i === -1 ? null : partes.slice(i + 1);
}

async function tratarBanco(acao: () => Promise<Resposta>): Promise<Resposta> {
  try {
    return await acao();
  } catch (e) {
    if (e instanceof ErroDeNegocio) {
      const { status, mensagem } = STATUS_POR_ERRO[e.codigo];
      return erro(status, e.codigo, mensagem);
    }
    return erro(500, "erro_interno", "erro ao acessar o banco de dados");
  }
}

export async function tratarRequisicao(req: Requisicao, deps: Dependencias): Promise<Resposta> {
  const rota = partesDaRota(req.caminho);
  const chave = rota === null ? "" : rota.join("/");
  const rotasDoRobo = ["comandos/proximo", "logs"];
  const rotasDoPainel = ["comandos"];
  if (!rotasDoRobo.includes(chave) && !rotasDoPainel.includes(chave)) {
    return erro(404, "rota_inexistente", "rota não encontrada");
  }

  // Cada rota tem a sua credencial: o token do painel nunca vale nas rotas do robô, e vice-versa.
  if (rotasDoPainel.includes(chave)) {
    if (!tokenValido(req.tokenPainel, deps.tokenPainelEsperado)) {
      return erro(401, "nao_autorizado", "token do painel ausente ou inválido");
    }
  } else if (!tokenValido(req.tokenDispositivo, deps.tokenDispositivoEsperado)) {
    return erro(401, "nao_autorizado", "token de dispositivo ausente ou inválido");
  }
  if (req.metodo !== "POST") {
    return erro(405, "metodo_nao_permitido", "use POST");
  }

  if (chave === "comandos/proximo") {
    return tratarBanco(async () => ({ status: 200, corpo: await deps.repositorio.proximoComando() }));
  }

  if (req.corpo === CORPO_INVALIDO) {
    return erro(400, "corpo_invalido", "o corpo não é um JSON válido");
  }
  if (chave === "comandos") {
    const validacao = validarComando(req.corpo);
    if (!validacao.ok) {
      return erro(400, "corpo_invalido", validacao.mensagem);
    }
    return tratarBanco(async () => ({ status: 202, corpo: await deps.repositorio.enviarComando(validacao.valor) }));
  }
  const validacao = validarLogs(req.corpo);
  if (!validacao.ok) {
    return erro(400, "corpo_invalido", validacao.mensagem);
  }
  return tratarBanco(async () => ({ status: 200, corpo: { gravadas: await deps.repositorio.registrarLogs(validacao.valor) } }));
}
