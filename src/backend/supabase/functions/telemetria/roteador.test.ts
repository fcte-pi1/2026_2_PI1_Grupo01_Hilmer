import test from "node:test";
import assert from "node:assert/strict";
import { CORPO_INVALIDO, ErroDeNegocio, tratarRequisicao } from "./roteador.ts";
import type { Dependencias, Repositorio, Requisicao } from "./roteador.ts";
import type { LoteTelemetria } from "./logica.ts";

const TOKEN = "token-do-robo";
const AMOSTRA = { numero_sequencia: 1, posicao_linha: 0, posicao_coluna: 0, orientacao: "N" };

/** Repositório falso que registra os lotes e devolve o que o teste configurar. */
function repositorioFalso(config: { erro?: ErroDeNegocio | Error } = {}) {
  const lotes: LoteTelemetria[] = [];
  const repositorio: Repositorio = {
    async registrarTelemetria(lote) {
      if (config.erro) throw config.erro;
      lotes.push(lote);
      return { gravadas: lote.amostras.length, existentes: 0, reconsolidada: false };
    },
  };
  return { repositorio, lotes };
}

function requisicao(parcial: Partial<Requisicao>): Requisicao {
  return {
    metodo: "POST",
    caminho: "/functions/v1/telemetria",
    tokenRecebido: TOKEN,
    corpo: { corrida_id: 7, amostras: [AMOSTRA] },
    ...parcial,
  };
}

function dependencias(repositorio: Repositorio): Dependencias {
  return { tokenEsperado: TOKEN, repositorio };
}

test("sem token ou com token errado responde 401 e não toca o banco", async () => {
  const { repositorio, lotes } = repositorioFalso();
  for (const tokenRecebido of [null, "", "outro"]) {
    const r = await tratarRequisicao(requisicao({ tokenRecebido }), dependencias(repositorio));
    assert.equal(r.status, 401);
  }
  assert.equal(lotes.length, 0);
});

test("sem DEVICE_TOKEN configurado, o acesso é aberto e o lote é gravado", async () => {
  const { repositorio, lotes } = repositorioFalso();
  const r = await tratarRequisicao(requisicao({ tokenRecebido: null }), { tokenEsperado: undefined, repositorio });
  assert.equal(r.status, 200);
  assert.equal(lotes.length, 1);
});

test("lote válido responde 200 com o que o banco gravou", async () => {
  const { repositorio, lotes } = repositorioFalso();
  const r = await tratarRequisicao(requisicao({}), dependencias(repositorio));
  assert.equal(r.status, 200);
  assert.deepEqual(r.corpo, { gravadas: 1, existentes: 0, reconsolidada: false });
  assert.deepEqual(lotes, [{ corrida_id: 7, amostras: [AMOSTRA] }]);
});

test("a rota também responde fora do prefixo /functions/v1", async () => {
  const { repositorio, lotes } = repositorioFalso();
  const r = await tratarRequisicao(requisicao({ caminho: "/telemetria" }), dependencias(repositorio));
  assert.equal(r.status, 200);
  assert.equal(lotes.length, 1);
});

test("caminho que não é da telemetria responde 404", async () => {
  const { repositorio, lotes } = repositorioFalso();
  for (const caminho of ["/functions/v1/corridas", "/", "/functions/v1/telemetria/extra"]) {
    const r = await tratarRequisicao(requisicao({ caminho }), dependencias(repositorio));
    assert.equal(r.status, 404, caminho);
  }
  assert.equal(lotes.length, 0);
});

test("método diferente de POST responde 405", async () => {
  const { repositorio, lotes } = repositorioFalso();
  for (const metodo of ["GET", "PUT", "DELETE"]) {
    const r = await tratarRequisicao(requisicao({ metodo }), dependencias(repositorio));
    assert.equal(r.status, 405, metodo);
  }
  assert.equal(lotes.length, 0);
});

test("corpo que não é JSON ou com forma errada responde 400 sem tocar o banco", async () => {
  const { repositorio, lotes } = repositorioFalso();
  const corpos: unknown[] = [CORPO_INVALIDO, undefined, { corrida_id: 7 }, { amostras: [AMOSTRA] }, { corrida_id: 7, amostras: [] }];
  for (const corpo of corpos) {
    const r = await tratarRequisicao(requisicao({ corpo }), dependencias(repositorio));
    assert.equal(r.status, 400);
    assert.equal((r.corpo as { erro: string }).erro, "corpo_invalido");
  }
  assert.equal(lotes.length, 0);
});

test("os erros de negócio do banco viram os códigos HTTP que o firmware espera", async () => {
  const esperados: Array<[ErroDeNegocio["codigo"], number]> = [
    ["corrida_inexistente", 404],
    ["amostras_invalidas", 400],
    ["lote_grande_demais", 413],
  ];
  for (const [codigo, status] of esperados) {
    const { repositorio } = repositorioFalso({ erro: new ErroDeNegocio(codigo) });
    const r = await tratarRequisicao(requisicao({}), dependencias(repositorio));
    assert.equal(r.status, status, codigo);
    assert.equal((r.corpo as { erro: string }).erro, codigo);
  }
});

test("falha inesperada do banco responde 500 sem vazar a mensagem", async () => {
  const { repositorio } = repositorioFalso({ erro: new Error("connection refused: senha=segredo") });
  const r = await tratarRequisicao(requisicao({}), dependencias(repositorio));
  assert.equal(r.status, 500);
  assert.equal(JSON.stringify(r.corpo).includes("segredo"), false);
});
