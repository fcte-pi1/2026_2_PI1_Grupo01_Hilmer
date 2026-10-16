// Edge Function `corridas`: cria e encerra corridas no banco, com a chave de serviço.
//
// O robô (ou o dashboard, quando houver login de operador) chama:
//   POST /functions/v1/corridas                 { "labirinto": "4x4" | "8x4" | "12x4" }
//   POST /functions/v1/corridas/{id}/encerrar   { "status": "concluida" | "nao_concluida", ...métricas }
// com o cabeçalho `x-device-token`. A regra de negócio fica nas funções SQL `criar_corrida` e
// `encerrar_corrida`; a chave `anon` não tem permissão de escrita direta nas tabelas.
//
// Segredos: DEVICE_TOKEN (definido com `supabase secrets set`). SUPABASE_URL e
// SUPABASE_SERVICE_ROLE_KEY já são fornecidos pelo Supabase às Edge Functions.

import { createClient } from "npm:@supabase/supabase-js@2.45.4";
import { CORPO_INVALIDO, ErroDeNegocio, tratarRequisicao } from "./roteador.ts";
import type { CodigoErro, Repositorio } from "./roteador.ts";
import type { Corrida, DadosEncerramento, TipoLabirinto } from "./logica.ts";

const CODIGOS: CodigoErro[] = ["labirinto_inexistente", "corrida_inexistente", "corrida_ja_encerrada", "status_invalido"];

/** Converte a mensagem de erro do banco em erro de negócio, quando for um dos conhecidos. */
function converterErro(mensagem: string): Error {
  const codigo = CODIGOS.find((c) => mensagem.includes(c));
  return codigo ? new ErroDeNegocio(codigo) : new Error(mensagem);
}

/** Cliente com a chave de serviço, criado na primeira chamada que precisa do banco. */
let cliente: ReturnType<typeof createClient> | undefined;
function obterCliente() {
  cliente ??= createClient(Deno.env.get("SUPABASE_URL")!, Deno.env.get("SUPABASE_SERVICE_ROLE_KEY")!, {
    auth: { persistSession: false },
  });
  return cliente;
}

const repositorio: Repositorio = {
  async criarCorrida(tipo: TipoLabirinto): Promise<Corrida> {
    const { data, error } = await obterCliente().rpc("criar_corrida", { p_tipo: tipo });
    if (error) throw converterErro(error.message);
    return data as Corrida;
  },
  async encerrarCorrida(id: number, dados: DadosEncerramento): Promise<Corrida> {
    const { data, error } = await obterCliente().rpc("encerrar_corrida", {
      p_id: id,
      p_status: dados.status,
      p_tempo_final: dados.tempo_final ?? null,
      p_velocidade_media: dados.velocidade_media ?? null,
      p_velocidade_pico: dados.velocidade_pico ?? null,
      p_consumo_bateria: dados.consumo_bateria ?? null,
    });
    if (error) throw converterErro(error.message);
    return data as Corrida;
  },
};

Deno.serve(async (req: Request) => {
  let corpo: unknown = undefined;
  const texto = await req.text();
  if (texto.length > 0) {
    try {
      corpo = JSON.parse(texto);
    } catch {
      corpo = CORPO_INVALIDO;
    }
  }

  const resposta = await tratarRequisicao(
    {
      metodo: req.method,
      caminho: new URL(req.url).pathname,
      tokenRecebido: req.headers.get("x-device-token"),
      corpo,
    },
    { tokenEsperado: Deno.env.get("DEVICE_TOKEN") ?? undefined, repositorio },
  );

  return new Response(JSON.stringify(resposta.corpo), {
    status: resposta.status,
    headers: { "Content-Type": "application/json" },
  });
});
