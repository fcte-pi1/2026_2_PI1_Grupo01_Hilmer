// Edge Function `robo`: o canal entre o painel e o robô, sem cabo nem botão de largada.
//
//   POST /functions/v1/robo/comandos           { "comando": "iniciar" | "parar" | "zerar" }   (painel, x-painel-token)
//   POST /functions/v1/robo/comandos/proximo   (corpo vazio)                                  (robô, x-device-token)
//   POST /functions/v1/robo/logs               { "corrida_id": 7 | null, "linhas": ["..."] }  (robô, x-device-token)
//
// O painel grava o comando; o robô o busca a cada poucos segundos, executa e manda de volta os logs dos
// eventos, que o painel recebe pelo Realtime. A regra de negócio fica nas funções SQL
// `enviar_comando_robo`, `proximo_comando_robo` e `registrar_logs_robo` (veja a migração
// 20261117120000); a chave `anon` não escreve nas tabelas.
//
// Segredos: DEVICE_TOKEN (o mesmo das outras funções) e PAINEL_TOKEN (a senha do operador no painel),
// definidos com `supabase secrets set`. Sem o PAINEL_TOKEN, ninguém manda comando.
// SUPABASE_URL e SUPABASE_SERVICE_ROLE_KEY já são fornecidos pelo Supabase às Edge Functions.

import { createClient } from "npm:@supabase/supabase-js@2.45.4";
import { CORPO_INVALIDO, ErroDeNegocio, tratarRequisicao } from "./roteador.ts";
import type { CodigoErro, Repositorio } from "./roteador.ts";
import type { Comando, ComandoEntregue, LoteLogs } from "./logica.ts";

const CODIGOS: CodigoErro[] = ["comando_invalido", "logs_invalidos", "logs_grande_demais"];

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
  async enviarComando(comando: Comando) {
    const { data, error } = await obterCliente().rpc("enviar_comando_robo", { p_comando: comando });
    if (error) throw converterErro(error.message);
    return data as { id: number; comando: Comando };
  },
  async proximoComando(): Promise<ComandoEntregue> {
    const { data, error } = await obterCliente().rpc("proximo_comando_robo", {});
    if (error) throw converterErro(error.message);
    return data as ComandoEntregue;
  },
  async registrarLogs(lote: LoteLogs): Promise<number> {
    const { data, error } = await obterCliente().rpc("registrar_logs_robo", {
      p_corrida_id: lote.corrida_id,
      p_linhas: lote.linhas,
    });
    if (error) throw converterErro(error.message);
    return data as number;
  },
};

// O painel chama a função do navegador (outra origem), então ela responde ao CORS.
const CORS = {
  "Access-Control-Allow-Origin": "*",
  "Access-Control-Allow-Methods": "POST, OPTIONS",
  "Access-Control-Allow-Headers": "authorization, apikey, content-type, x-painel-token, x-device-token",
};

Deno.serve(async (req: Request) => {
  if (req.method === "OPTIONS") {
    return new Response(null, { status: 204, headers: CORS });
  }
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
      tokenDispositivo: req.headers.get("x-device-token"),
      tokenPainel: req.headers.get("x-painel-token"),
      corpo,
    },
    {
      tokenDispositivoEsperado: Deno.env.get("DEVICE_TOKEN") ?? undefined,
      tokenPainelEsperado: Deno.env.get("PAINEL_TOKEN") ?? undefined,
      repositorio,
    },
  );

  return new Response(JSON.stringify(resposta.corpo), {
    status: resposta.status,
    headers: { "Content-Type": "application/json", ...CORS },
  });
});
