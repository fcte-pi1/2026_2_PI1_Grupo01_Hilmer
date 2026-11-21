// Testes do roteador da Edge Function `robo`: as duas credenciais, as três rotas e os erros.

import test from "node:test";
import assert from "node:assert/strict";
import { CORPO_INVALIDO, ErroDeNegocio, tratarRequisicao } from "./roteador.ts";
import type { Dependencias, Repositorio, Requisicao } from "./roteador.ts";

const TOKEN_ROBO = "token-do-robo";
const TOKEN_PAINEL = "senha-do-painel";

interface Chamadas {
  comandos: string[];
  proximos: number;
  logs: { corrida_id: number | null; linhas: string[] }[];
}

function montar(
  sobrescrever: Partial<Repositorio> = {},
): { deps: Dependencias; chamadas: Chamadas } {
  const chamadas: Chamadas = { comandos: [], proximos: 0, logs: [] };
  const repositorio: Repositorio = {
    async enviarComando(comando) {
      chamadas.comandos.push(comando);
      return { id: 7, comando };
    },
    async proximoComando() {
      chamadas.proximos++;
      return { id: 7, comando: "iniciar" };
    },
    async registrarLogs(lote) {
      chamadas.logs.push(lote);
      return lote.linhas.length;
    },
    ...sobrescrever,
  };
  return {
    chamadas,
    deps: { tokenDispositivoEsperado: TOKEN_ROBO, tokenPainelEsperado: TOKEN_PAINEL, repositorio },
  };
}

function requisicao(caminho: string, sobrescrever: Partial<Requisicao> = {}): Requisicao {
  return {
    metodo: "POST",
    caminho: `/functions/v1/robo/${caminho}`,
    tokenDispositivo: null,
    tokenPainel: null,
    corpo: undefined,
    ...sobrescrever,
  };
}

test("o painel manda um comando com o token do painel e recebe 202", async () => {
  const { deps, chamadas } = montar();
  const r = await tratarRequisicao(
    requisicao("comandos", { tokenPainel: TOKEN_PAINEL, corpo: { comando: "iniciar" } }),
    deps,
  );
  assert.equal(r.status, 202);
  assert.deepEqual(r.corpo, { id: 7, comando: "iniciar" });
  assert.deepEqual(chamadas.comandos, ["iniciar"]);
});

test("os três comandos do painel são aceitos e outros são recusados", async () => {
  const { deps, chamadas } = montar();
  for (const comando of ["iniciar", "parar", "zerar"]) {
    const r = await tratarRequisicao(requisicao("comandos", { tokenPainel: TOKEN_PAINEL, corpo: { comando } }), deps);
    assert.equal(r.status, 202, comando);
  }
  for (const corpo of [{ comando: "apagar" }, { comando: 1 }, {}, [], "iniciar"]) {
    const r = await tratarRequisicao(requisicao("comandos", { tokenPainel: TOKEN_PAINEL, corpo }), deps);
    assert.equal(r.status, 400, JSON.stringify(corpo));
  }
  assert.deepEqual(chamadas.comandos, ["iniciar", "parar", "zerar"]);
});

test("o token do painel não vale nas rotas do robô, e o do robô não vale no painel", async () => {
  const { deps, chamadas } = montar();
  const comTokenDoPainel = await tratarRequisicao(
    requisicao("comandos/proximo", { tokenDispositivo: TOKEN_PAINEL, tokenPainel: TOKEN_PAINEL }),
    deps,
  );
  assert.equal(comTokenDoPainel.status, 401);
  const logsComTokenDoPainel = await tratarRequisicao(
    requisicao("logs", { tokenPainel: TOKEN_PAINEL, corpo: { linhas: ["a"] } }),
    deps,
  );
  assert.equal(logsComTokenDoPainel.status, 401);
  const comandoComTokenDoRobo = await tratarRequisicao(
    requisicao("comandos", { tokenDispositivo: TOKEN_ROBO, tokenPainel: TOKEN_ROBO, corpo: { comando: "iniciar" } }),
    deps,
  );
  assert.equal(comandoComTokenDoRobo.status, 401);
  assert.equal(chamadas.proximos + chamadas.logs.length + chamadas.comandos.length, 0);
});

test("com o segredo configurado, sem token ninguém é autorizado", async () => {
  const { deps } = montar();
  const semToken = await tratarRequisicao(requisicao("comandos", { corpo: { comando: "iniciar" } }), deps);
  assert.equal(semToken.status, 401);
  const semTokenDoRobo = await tratarRequisicao(requisicao("comandos/proximo", {}), deps);
  assert.equal(semTokenDoRobo.status, 401);
});

test("sem o segredo configurado, o acesso é aberto (token opcional)", async () => {
  const { deps } = montar();
  const painelAberto = await tratarRequisicao(requisicao("comandos", { corpo: { comando: "iniciar" } }), {
    ...deps,
    tokenPainelEsperado: undefined,
  });
  assert.equal(painelAberto.status, 202);
  const roboAberto = await tratarRequisicao(requisicao("comandos/proximo", {}), {
    ...deps,
    tokenDispositivoEsperado: undefined,
  });
  assert.equal(roboAberto.status, 200);
});

test("o robô busca o próximo comando e recebe 200, com ou sem comando", async () => {
  const { deps } = montar();
  const com = await tratarRequisicao(requisicao("comandos/proximo", { tokenDispositivo: TOKEN_ROBO }), deps);
  assert.equal(com.status, 200);
  assert.deepEqual(com.corpo, { id: 7, comando: "iniciar" });

  const { deps: semComando } = montar({ proximoComando: async () => ({ id: null, comando: null }) });
  const sem = await tratarRequisicao(requisicao("comandos/proximo", { tokenDispositivo: TOKEN_ROBO }), semComando);
  assert.equal(sem.status, 200);
  assert.deepEqual(sem.corpo, { id: null, comando: null });
});

test("o robô grava logs com e sem corrida", async () => {
  const { deps, chamadas } = montar();
  const comCorrida = await tratarRequisicao(
    requisicao("logs", { tokenDispositivo: TOKEN_ROBO, corpo: { corrida_id: 3, linhas: ['{"status":"largada"}', "b"] } }),
    deps,
  );
  assert.equal(comCorrida.status, 200);
  assert.deepEqual(comCorrida.corpo, { gravadas: 2 });
  const semCorrida = await tratarRequisicao(
    requisicao("logs", { tokenDispositivo: TOKEN_ROBO, corpo: { linhas: ["a"] } }),
    deps,
  );
  assert.equal(semCorrida.status, 200);
  assert.deepEqual(chamadas.logs.map((l) => l.corrida_id), [3, null]);
});

test("logs inválidos são recusados com 400 antes de chegar ao banco", async () => {
  const { deps, chamadas } = montar();
  const casos: unknown[] = [
    undefined,
    {},
    { linhas: [] },
    { linhas: "texto" },
    { linhas: [""] },
    { linhas: [1] },
    { linhas: ["x".repeat(301)] },
    { linhas: Array.from({ length: 51 }, () => "a") },
    { corrida_id: 0, linhas: ["a"] },
    { corrida_id: "3", linhas: ["a"] },
  ];
  for (const corpo of casos) {
    const r = await tratarRequisicao(requisicao("logs", { tokenDispositivo: TOKEN_ROBO, corpo }), deps);
    assert.equal(r.status, 400, JSON.stringify(corpo));
  }
  assert.equal(chamadas.logs.length, 0);
});

test("o limite de 50 linhas de 300 caracteres é aceito", async () => {
  const { deps } = montar();
  const r = await tratarRequisicao(
    requisicao("logs", {
      tokenDispositivo: TOKEN_ROBO,
      corpo: { linhas: Array.from({ length: 50 }, () => "x".repeat(300)) },
    }),
    deps,
  );
  assert.equal(r.status, 200);
});

test("rota inexistente é 404, método errado é 405 e corpo que não é JSON é 400", async () => {
  const { deps } = montar();
  assert.equal((await tratarRequisicao(requisicao("outra", { tokenDispositivo: TOKEN_ROBO }), deps)).status, 404);
  assert.equal(
    (await tratarRequisicao({ ...requisicao("comandos/proximo", { tokenDispositivo: TOKEN_ROBO }), metodo: "GET" }, deps)).status,
    405,
  );
  const invalido = await tratarRequisicao(requisicao("logs", { tokenDispositivo: TOKEN_ROBO, corpo: CORPO_INVALIDO }), deps);
  assert.equal(invalido.status, 400);
});

test("erros de negócio do banco viram 400 e 413, e o resto vira 500 sem expor a mensagem", async () => {
  const { deps: invalido } = montar({
    registrarLogs: async () => {
      throw new ErroDeNegocio("logs_invalidos");
    },
  });
  const r400 = await tratarRequisicao(requisicao("logs", { tokenDispositivo: TOKEN_ROBO, corpo: { linhas: ["a"] } }), invalido);
  assert.equal(r400.status, 400);

  const { deps: grande } = montar({
    registrarLogs: async () => {
      throw new ErroDeNegocio("logs_grande_demais");
    },
  });
  const r413 = await tratarRequisicao(requisicao("logs", { tokenDispositivo: TOKEN_ROBO, corpo: { linhas: ["a"] } }), grande);
  assert.equal(r413.status, 413);

  const { deps: quebrado } = montar({
    enviarComando: async () => {
      throw new Error("senha do banco: 1234");
    },
  });
  const r500 = await tratarRequisicao(
    requisicao("comandos", { tokenPainel: TOKEN_PAINEL, corpo: { comando: "iniciar" } }),
    quebrado,
  );
  assert.equal(r500.status, 500);
  assert.ok(!JSON.stringify(r500.corpo).includes("1234"));
});
