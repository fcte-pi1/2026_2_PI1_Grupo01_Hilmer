import test from "node:test";
import assert from "node:assert/strict";
import { CORPO_INVALIDO, ErroDeNegocio, tratarRequisicao } from "./roteador.ts";
import type { Dependencias, Repositorio, Requisicao } from "./roteador.ts";
import type { Corrida, DadosEncerramento, TipoLabirinto } from "./logica.ts";

const TOKEN = "token-do-robo";

function corrida(parcial: Partial<Corrida> = {}): Corrida {
  return {
    id: 1,
    labirinto_id: 1,
    numero_tentativa: 1,
    data_hora_inicio: "2026-10-16T12:00:00Z",
    data_hora_fim: null,
    tempo_final: null,
    velocidade_media: null,
    velocidade_pico: null,
    consumo_bateria: null,
    status: "em_andamento",
    ...parcial,
  };
}

/** Repositório falso que registra as chamadas e devolve o que o teste configurar. */
function repositorioFalso(config: { erro?: ErroDeNegocio | Error } = {}) {
  const chamadas: Array<{ tipo: TipoLabirinto } | { id: number; dados: DadosEncerramento }> = [];
  const repositorio: Repositorio = {
    async criarCorrida(tipo) {
      if (config.erro) throw config.erro;
      chamadas.push({ tipo });
      return corrida({ numero_tentativa: chamadas.length });
    },
    async encerrarCorrida(id, dados) {
      if (config.erro) throw config.erro;
      chamadas.push({ id, dados });
      return corrida({ id, status: dados.status, data_hora_fim: "2026-10-16T12:05:00Z" });
    },
  };
  return { repositorio, chamadas };
}

function requisicao(parcial: Partial<Requisicao>): Requisicao {
  return { metodo: "POST", caminho: "/functions/v1/corridas", tokenRecebido: TOKEN, corpo: undefined, ...parcial };
}

function dependencias(repositorio: Repositorio): Dependencias {
  return { tokenEsperado: TOKEN, repositorio };
}

test("sem token ou com token errado responde 401 e não toca o banco", async () => {
  const { repositorio, chamadas } = repositorioFalso();
  for (const tokenRecebido of [null, "", "outro"]) {
    const r = await tratarRequisicao(requisicao({ tokenRecebido, corpo: { labirinto: "4x4" } }), dependencias(repositorio));
    assert.equal(r.status, 401);
  }
  assert.equal(chamadas.length, 0);
});

test("sem DEVICE_TOKEN configurado, ninguém é autorizado", async () => {
  const { repositorio } = repositorioFalso();
  const r = await tratarRequisicao(requisicao({ corpo: { labirinto: "4x4" } }), { tokenEsperado: undefined, repositorio });
  assert.equal(r.status, 401);
});

test("cria corridas nos três labirintos e devolve 201 com a corrida", async () => {
  for (const tipo of ["4x4", "8x4", "12x4"]) {
    const { repositorio, chamadas } = repositorioFalso();
    const r = await tratarRequisicao(requisicao({ corpo: { labirinto: tipo } }), dependencias(repositorio));
    assert.equal(r.status, 201);
    assert.equal((r.corpo as Corrida).status, "em_andamento");
    assert.deepEqual(chamadas, [{ tipo }]);
  }
});

test("criação sem labirinto ou com labirinto inválido responde 400 sem chamar o banco", async () => {
  const { repositorio, chamadas } = repositorioFalso();
  for (const corpo of [{}, { labirinto: "5x5" }, undefined, CORPO_INVALIDO]) {
    const r = await tratarRequisicao(requisicao({ corpo }), dependencias(repositorio));
    assert.equal(r.status, 400);
    assert.equal((r.corpo as { erro: string }).erro, "corpo_invalido");
  }
  assert.equal(chamadas.length, 0);
});

test("labirinto fora do cadastro do banco responde 404", async () => {
  const { repositorio } = repositorioFalso({ erro: new ErroDeNegocio("labirinto_inexistente") });
  const r = await tratarRequisicao(requisicao({ corpo: { labirinto: "4x4" } }), dependencias(repositorio));
  assert.equal(r.status, 404);
  assert.equal((r.corpo as { erro: string }).erro, "labirinto_inexistente");
});

test("encerra a corrida e repassa status e métricas", async () => {
  const { repositorio, chamadas } = repositorioFalso();
  const r = await tratarRequisicao(
    requisicao({ caminho: "/functions/v1/corridas/7/encerrar", corpo: { status: "concluida", tempo_final: 61.2 } }),
    dependencias(repositorio),
  );
  assert.equal(r.status, 200);
  assert.equal((r.corpo as Corrida).status, "concluida");
  assert.equal((r.corpo as Corrida).data_hora_fim, "2026-10-16T12:05:00Z");
  assert.deepEqual(chamadas, [{ id: 7, dados: { status: "concluida", tempo_final: 61.2 } }]);
});

test("encerramento com id inválido, status inválido ou corpo quebrado responde 400", async () => {
  const { repositorio, chamadas } = repositorioFalso();
  const casos: Array<[string, unknown]> = [
    ["/corridas/abc/encerrar", { status: "concluida" }],
    ["/corridas/0/encerrar", { status: "concluida" }],
    ["/corridas/7/encerrar", { status: "em_andamento" }],
    ["/corridas/7/encerrar", {}],
    ["/corridas/7/encerrar", CORPO_INVALIDO],
    ["/corridas/7/encerrar", { status: "concluida", tempo_final: -3 }],
  ];
  for (const [caminho, corpo] of casos) {
    const r = await tratarRequisicao(requisicao({ caminho, corpo }), dependencias(repositorio));
    assert.equal(r.status, 400, caminho + JSON.stringify(corpo));
  }
  assert.equal(chamadas.length, 0);
});

test("encerrar corrida inexistente responde 404", async () => {
  const { repositorio } = repositorioFalso({ erro: new ErroDeNegocio("corrida_inexistente") });
  const r = await tratarRequisicao(requisicao({ caminho: "/corridas/99/encerrar", corpo: { status: "concluida" } }), dependencias(repositorio));
  assert.equal(r.status, 404);
});

test("encerrar com outro status uma corrida já encerrada responde 409", async () => {
  const { repositorio } = repositorioFalso({ erro: new ErroDeNegocio("corrida_ja_encerrada") });
  const r = await tratarRequisicao(requisicao({ caminho: "/corridas/7/encerrar", corpo: { status: "nao_concluida" } }), dependencias(repositorio));
  assert.equal(r.status, 409);
  assert.equal((r.corpo as { erro: string }).erro, "corrida_ja_encerrada");
});

test("status inválido apontado pelo banco responde 400", async () => {
  const { repositorio } = repositorioFalso({ erro: new ErroDeNegocio("status_invalido") });
  const r = await tratarRequisicao(requisicao({ caminho: "/corridas/7/encerrar", corpo: { status: "concluida" } }), dependencias(repositorio));
  assert.equal(r.status, 400);
});

test("erro inesperado do banco responde 500 sem vazar a mensagem", async () => {
  const { repositorio } = repositorioFalso({ erro: new Error("connection refused: senha=123") });
  const r = await tratarRequisicao(requisicao({ corpo: { labirinto: "4x4" } }), dependencias(repositorio));
  assert.equal(r.status, 500);
  assert.equal(JSON.stringify(r.corpo).includes("senha"), false);
});

test("método diferente de POST responde 405", async () => {
  const { repositorio } = repositorioFalso();
  for (const caminho of ["/functions/v1/corridas", "/functions/v1/corridas/7/encerrar"]) {
    for (const metodo of ["GET", "PUT", "DELETE", "PATCH"]) {
      const r = await tratarRequisicao(requisicao({ metodo, caminho }), dependencias(repositorio));
      assert.equal(r.status, 405, metodo + caminho);
    }
  }
});

test("rotas desconhecidas respondem 404", async () => {
  const { repositorio } = repositorioFalso();
  for (const caminho of ["/functions/v1/outra", "/corridas/7", "/corridas/7/apagar", "/corridas/7/encerrar/extra"]) {
    const r = await tratarRequisicao(requisicao({ caminho, corpo: { status: "concluida" } }), dependencias(repositorio));
    assert.equal(r.status, 404, caminho);
  }
});

test("a autenticação vem antes da validação da rota", async () => {
  const { repositorio } = repositorioFalso();
  const r = await tratarRequisicao(requisicao({ caminho: "/qualquer", tokenRecebido: null }), dependencias(repositorio));
  assert.equal(r.status, 401);
});
