/// Roteamento da Edge Function `corridas`, sem Deno nem Supabase: recebe os dados da requisição e
/// um repositório, e devolve status e corpo. O `index.ts` só liga isto ao servidor HTTP.

import { tokenValido } from "../_shared/autenticacao.ts";
import { analisarId, validarCriacao, validarEncerramento } from "./logica.ts";
import type { Corrida, DadosEncerramento, TipoLabirinto } from "./logica.ts";

export type CodigoErro = "labirinto_inexistente" | "corrida_inexistente" | "corrida_ja_encerrada" | "status_invalido";

/** Erro de regra de negócio vindo do banco (por exemplo, encerrar uma corrida já encerrada com outro status). */
export class ErroDeNegocio extends Error {
  codigo: CodigoErro;
  constructor(codigo: CodigoErro) {
    super(codigo);
    this.codigo = codigo;
  }
}

/** Acesso ao banco. A implementação real chama as funções SQL com a chave de serviço. */
export interface Repositorio {
  criarCorrida(tipo: TipoLabirinto): Promise<Corrida>;
  encerrarCorrida(id: number, dados: DadosEncerramento): Promise<Corrida>;
}

export interface Requisicao {
  metodo: string;
  caminho: string;
  tokenRecebido: string | null;
  /** Corpo já lido como JSON; `undefined` se estiver vazio e `invalido` se não for JSON. */
  corpo: unknown;
}

export interface Resposta {
  status: number;
  corpo: unknown;
}

export interface Dependencias {
  tokenEsperado: string | undefined;
  repositorio: Repositorio;
}

/** Marca o corpo que não pôde ser lido como JSON. */
export const CORPO_INVALIDO = Symbol("corpo_invalido");

function erro(status: number, codigo: string, mensagem: string): Resposta {
  return { status, corpo: { erro: codigo, mensagem } };
}

const STATUS_POR_ERRO: Record<CodigoErro, { status: number; mensagem: string }> = {
  labirinto_inexistente: { status: 404, mensagem: "labirinto não cadastrado" },
  corrida_inexistente: { status: 404, mensagem: "corrida não encontrada" },
  corrida_ja_encerrada: { status: 409, mensagem: "a corrida já foi encerrada com outro status" },
  status_invalido: { status: 400, mensagem: "status inválido" },
};

/** Segmentos do caminho depois de `corridas`: `/functions/v1/corridas/12/encerrar` vira `["12", "encerrar"]`. */
function segmentosDepoisDeCorridas(caminho: string): string[] | null {
  const partes = caminho.split("/").filter((p) => p.length > 0);
  const indice = partes.indexOf("corridas");
  return indice === -1 ? null : partes.slice(indice + 1);
}

export async function tratarRequisicao(req: Requisicao, deps: Dependencias): Promise<Resposta> {
  if (!tokenValido(req.tokenRecebido, deps.tokenEsperado)) {
    return erro(401, "nao_autorizado", "token de dispositivo ausente ou inválido");
  }

  const segmentos = segmentosDepoisDeCorridas(req.caminho);
  if (segmentos === null) {
    return erro(404, "rota_inexistente", "rota não encontrada");
  }

  try {
    if (segmentos.length === 0) {
      if (req.metodo !== "POST") {
        return erro(405, "metodo_nao_permitido", "use POST em /corridas");
      }
      if (req.corpo === CORPO_INVALIDO) {
        return erro(400, "corpo_invalido", "o corpo não é um JSON válido");
      }
      const validacao = validarCriacao(req.corpo);
      if (!validacao.ok) {
        return erro(400, "corpo_invalido", validacao.mensagem);
      }
      const corrida = await deps.repositorio.criarCorrida(validacao.valor.tipo);
      return { status: 201, corpo: corrida };
    }

    if (segmentos.length === 2 && segmentos[1] === "encerrar") {
      if (req.metodo !== "POST") {
        return erro(405, "metodo_nao_permitido", "use POST em /corridas/{id}/encerrar");
      }
      const id = analisarId(segmentos[0]);
      if (id === null) {
        return erro(400, "id_invalido", "o identificador da corrida deve ser um inteiro positivo");
      }
      if (req.corpo === CORPO_INVALIDO) {
        return erro(400, "corpo_invalido", "o corpo não é um JSON válido");
      }
      const validacao = validarEncerramento(req.corpo);
      if (!validacao.ok) {
        return erro(400, "corpo_invalido", validacao.mensagem);
      }
      const corrida = await deps.repositorio.encerrarCorrida(id, validacao.valor);
      return { status: 200, corpo: corrida };
    }

    return erro(404, "rota_inexistente", "rota não encontrada");
  } catch (e) {
    if (e instanceof ErroDeNegocio) {
      const { status, mensagem } = STATUS_POR_ERRO[e.codigo];
      return erro(status, e.codigo, mensagem);
    }
    return erro(500, "erro_interno", "erro ao acessar o banco de dados");
  }
}
