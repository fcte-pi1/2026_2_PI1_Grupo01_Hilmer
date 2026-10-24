// Testa, num PostgreSQL de verdade (PGlite), a gravação da telemetria em lote, a do mapa final e a
// consolidação da corrida (#229): idempotência, atomicidade, validação, permissões e a corrida
// simulada completa. Os arquivos de `tests/fixtures` são o contrato do lote com o firmware (#236).

import test from "node:test";
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import type { PGlite } from "@electric-sql/pglite";
import { como, criarBanco, mensagemDeErro } from "./ajudantes.ts";

type Amostra = Record<string, unknown>;

function fixture(nome: string): any {
  return JSON.parse(readFileSync(fileURLToPath(new URL(`./fixtures/${nome}`, import.meta.url)), "utf8"));
}

/** Amostra mínima válida no labirinto 4x4, com a sequência dada. */
function amostra(sequencia: number, extra: Amostra = {}): Amostra {
  return { numero_sequencia: sequencia, posicao_linha: 0, posicao_coluna: 0, orientacao: "N", ...extra };
}

async function novaCorrida(db: PGlite, tipo = "4x4"): Promise<number> {
  const r = await como(db, "service_role", () => db.query<{ id: number }>("SELECT id FROM criar_corrida($1)", [tipo]));
  return r.rows[0].id;
}

async function registrar(db: PGlite, id: number, amostras: unknown) {
  const r = await como(db, "service_role", () =>
    db.query<{ r: { gravadas: number; existentes: number; reconsolidada: boolean } }>(
      "SELECT registrar_telemetria($1, $2::jsonb) AS r",
      [id, JSON.stringify(amostras)],
    ),
  );
  return r.rows[0].r;
}

async function mapa(db: PGlite, id: number, celulas: unknown) {
  const r = await como(db, "service_role", () =>
    db.query<{ r: { novas: number; atualizadas: number } }>("SELECT registrar_mapa($1, $2::jsonb) AS r", [
      id,
      JSON.stringify(celulas),
    ]),
  );
  return r.rows[0].r;
}

async function contar(db: PGlite, tabela: string, id: number): Promise<number> {
  const r = await db.query<{ n: number }>(`SELECT COUNT(*)::int AS n FROM ${tabela} WHERE corrida_id = $1`, [id]);
  return r.rows[0].n;
}

async function encerrar(db: PGlite, id: number, ...args: unknown[]) {
  const params = ["$1", ...args.map((_, i) => `$${i + 2}`)].join(", ");
  const r = await como(db, "service_role", () =>
    db.query<Record<string, any>>(`SELECT * FROM encerrar_corrida(${params})`, [id, ...args]),
  );
  return r.rows[0];
}

// ---------------------------------------------------------------- registrar_telemetria

test("um lote é gravado, vinculado à corrida, com os valores e os padrões das colunas", async () => {
  const db = await criarBanco();
  const id = await novaCorrida(db);
  const r = await registrar(db, id, [
    amostra(1, { velocidade: 0.25, tempo_decorrido: 1.5, estado_corrida: "explorando", posicao_linha: 2, posicao_coluna: 3, orientacao: "L" }),
    amostra(2),
  ]);
  assert.deepEqual(r, { gravadas: 2, existentes: 0, reconsolidada: false });

  const linhas = await db.query<Record<string, any>>("SELECT * FROM telemetria WHERE corrida_id = $1 ORDER BY numero_sequencia", [id]);
  assert.equal(linhas.rows.length, 2);
  assert.equal(linhas.rows[0].velocidade, 0.25);
  assert.equal(linhas.rows[0].tempo_decorrido, 1.5);
  assert.equal(linhas.rows[0].posicao_linha, 2);
  assert.equal(linhas.rows[0].posicao_coluna, 3);
  assert.equal(linhas.rows[0].orientacao, "L");
  assert.equal(linhas.rows[0].estado_corrida, "explorando");
  // Campos que o robô não envia ficam com os padrões da coluna.
  assert.equal(linhas.rows[1].velocidade, 0);
  assert.equal(linhas.rows[1].tensao_bateria, 0);
  assert.equal(linhas.rows[1].corrente_bateria, 0);
  assert.equal(linhas.rows[1].tempo_decorrido, 0);
  assert.equal(linhas.rows[1].estado_corrida, null);
  assert.equal(linhas.rows[1].evento, null);
});

test("reenviar o mesmo lote não duplica nenhuma linha", async () => {
  const db = await criarBanco();
  const id = await novaCorrida(db);
  const lote = [amostra(1), amostra(2), amostra(3)];
  await registrar(db, id, lote);
  const de_novo = await registrar(db, id, lote);
  assert.deepEqual(de_novo, { gravadas: 0, existentes: 3, reconsolidada: false });
  assert.equal(await contar(db, "telemetria", id), 3);
});

test("um lote com amostras novas e repetidas grava só as novas", async () => {
  const db = await criarBanco();
  const id = await novaCorrida(db);
  await registrar(db, id, [amostra(1), amostra(2)]);
  const r = await registrar(db, id, [amostra(2), amostra(3), amostra(4)]);
  assert.deepEqual(r, { gravadas: 2, existentes: 1, reconsolidada: false });
  assert.equal(await contar(db, "telemetria", id), 4);
});

test("a mesma sequência duas vezes no mesmo lote conta uma vez só", async () => {
  const db = await criarBanco();
  const id = await novaCorrida(db);
  const r = await registrar(db, id, [amostra(1), amostra(1), amostra(2)]);
  assert.deepEqual(r, { gravadas: 2, existentes: 1, reconsolidada: false });
});

test("a mesma sequência em corridas diferentes é aceita (a chave é corrida mais sequência)", async () => {
  const db = await criarBanco();
  const a = await novaCorrida(db);
  const b = await novaCorrida(db);
  await registrar(db, a, [amostra(1)]);
  assert.equal((await registrar(db, b, [amostra(1)])).gravadas, 1);
});

test("corrida inexistente é recusada", async () => {
  const db = await criarBanco();
  const erro = await mensagemDeErro(registrar(db, 999, [amostra(1)]));
  assert.match(erro, /corrida_inexistente/);
});

test("lote vazio, que não é lista ou com item que não é objeto é recusado", async () => {
  const db = await criarBanco();
  const id = await novaCorrida(db);
  for (const ruim of [[], {}, "texto", null, [1, 2], [amostra(1), "x"]]) {
    assert.match(await mensagemDeErro(registrar(db, id, ruim)), /amostras_invalidas/, JSON.stringify(ruim));
  }
  assert.equal(await contar(db, "telemetria", id), 0);
});

test("100 amostras passam e 101 são recusadas", async () => {
  const db = await criarBanco();
  const id = await novaCorrida(db);
  const lote = (n: number) => Array.from({ length: n }, (_, i) => amostra(i + 1));
  assert.equal((await registrar(db, id, lote(100))).gravadas, 100);
  assert.match(await mensagemDeErro(registrar(db, id, lote(101))), /lote_grande_demais/);
  assert.equal(await contar(db, "telemetria", id), 100);
});

test("amostra inválida recusa o lote inteiro: nada é gravado (atomicidade)", async () => {
  const db = await criarBanco();
  const id = await novaCorrida(db);
  const invalidas: Amostra[] = [
    amostra(3, { orientacao: "X" }),
    amostra(3, { evento: "explosao" }),
    amostra(0),
    amostra(-1),
    amostra(3, { posicao_linha: 4 }),   // o 4x4 vai de 0 a 3
    amostra(3, { posicao_coluna: 4 }),
    amostra(3, { posicao_linha: -1 }),
    amostra(3, { tempo_decorrido: -0.5 }),
    amostra(3, { tensao_bateria: -1 }),
    amostra(3, { velocidade: "rapido" }),
    amostra(3, { numero_sequencia: "abc" }),
    { numero_sequencia: 3, posicao_linha: 0, posicao_coluna: 0 },  // sem orientação
    { posicao_linha: 0, posicao_coluna: 0, orientacao: "N" },      // sem sequência
  ];
  for (const ruim of invalidas) {
    const erro = await mensagemDeErro(registrar(db, id, [amostra(1), amostra(2), ruim]));
    assert.match(erro, /amostras_invalidas/, JSON.stringify(ruim));
  }
  assert.equal(await contar(db, "telemetria", id), 0);
});

test("a posição é conferida com o labirinto da corrida: coluna 4 vale no 8x4 e não no 4x4", async () => {
  const db = await criarBanco();
  const quatro = await novaCorrida(db, "4x4");
  const oito = await novaCorrida(db, "8x4");
  const doze = await novaCorrida(db, "12x4");
  const na_coluna = (c: number) => [amostra(1, { posicao_coluna: c })];
  assert.match(await mensagemDeErro(registrar(db, quatro, na_coluna(4))), /amostras_invalidas/);
  assert.equal((await registrar(db, oito, na_coluna(7))).gravadas, 1);
  assert.match(await mensagemDeErro(registrar(db, oito, [amostra(2, { posicao_coluna: 8 })])), /amostras_invalidas/);
  assert.equal((await registrar(db, doze, na_coluna(11))).gravadas, 1);
});

test("os eventos colisao e objetivo_alcancado e as quatro orientações são aceitos", async () => {
  const db = await criarBanco();
  const id = await novaCorrida(db);
  const lote = [
    amostra(1, { orientacao: "N" }),
    amostra(2, { orientacao: "S" }),
    amostra(3, { orientacao: "L" }),
    amostra(4, { orientacao: "O", evento: "colisao" }),
    amostra(5, { evento: "objetivo_alcancado" }),
  ];
  assert.equal((await registrar(db, id, lote)).gravadas, 5);
});

// ---------------------------------------------------------------- consolidação e encerramento

/** Uma corrida com telemetria: a velocidade média é (0 + 0.2 + 0.4) / 3 = 0.2 e o pico, 0.4. */
async function corridaComTelemetria(db: PGlite, evento?: string) {
  const id = await novaCorrida(db);
  await registrar(db, id, [
    amostra(1, { velocidade: 0.0, tempo_decorrido: 0 }),
    amostra(2, { velocidade: 0.2, tempo_decorrido: 1.5 }),
    amostra(3, { velocidade: 0.4, tempo_decorrido: 3.25, evento }),
  ]);
  return id;
}

test("encerrar sem informar nada calcula as métricas e o status a partir da telemetria", async () => {
  const db = await criarBanco();
  const id = await corridaComTelemetria(db, "objetivo_alcancado");
  const c = await encerrar(db, id);
  assert.equal(c.status, "concluida");
  assert.equal(c.tempo_final, 3.25);
  assert.ok(Math.abs(c.velocidade_media - 0.2) < 1e-9);
  assert.equal(c.velocidade_pico, 0.4);
  assert.equal(c.consumo_bateria, null);  // a bateria não foi medida (tensão zerada)
  assert.ok(c.data_hora_fim instanceof Date);
});

test("sem o evento objetivo_alcancado, a corrida encerra como nao_concluida", async () => {
  const db = await criarBanco();
  const id = await corridaComTelemetria(db);
  assert.equal((await encerrar(db, id)).status, "nao_concluida");
});

test("o que o robô informa prevalece sobre o que a telemetria calcula", async () => {
  const db = await criarBanco();
  const id = await corridaComTelemetria(db, "objetivo_alcancado");
  const c = await encerrar(db, id, "nao_concluida", 61.5, 0.9, 1.1, 42);
  assert.equal(c.status, "nao_concluida");
  assert.equal(c.tempo_final, 61.5);
  assert.equal(c.velocidade_media, 0.9);
  assert.equal(c.velocidade_pico, 1.1);
  assert.equal(c.consumo_bateria, 42);
});

test("sem telemetria nem métricas, o encerramento mantém o comportamento anterior", async () => {
  const db = await criarBanco();
  const id = await novaCorrida(db);
  const c = await encerrar(db, id);
  assert.equal(c.status, "nao_concluida");
  assert.ok(c.tempo_final >= 0 && c.tempo_final < 5);  // tempo desde o início
  assert.equal(c.velocidade_media, null);
  assert.equal(c.velocidade_pico, null);
  assert.equal(c.consumo_bateria, null);
});

test("o consumo da bateria sai da primeira e da última tensão medida, de forma linear", async () => {
  const db = await criarBanco();
  const id = await novaCorrida(db);
  await registrar(db, id, [
    amostra(1, { tensao_bateria: 0 }),    // sem medida: ignorada
    amostra(2, { tensao_bateria: 8.4 }),
    amostra(3, { tensao_bateria: 8.0 }),
    amostra(4, { tensao_bateria: 7.5 }),  // 0,9 V de 1,8 V: metade
    amostra(5, { tensao_bateria: 0 }),
  ]);
  const c = await encerrar(db, id);
  assert.ok(Math.abs(c.consumo_bateria - 50) < 1e-9, String(c.consumo_bateria));
});

test("o consumo fica entre 0 e 100 mesmo com tensões fora da faixa", async () => {
  const db = await criarBanco();
  const sobe = await novaCorrida(db);
  await registrar(db, sobe, [amostra(1, { tensao_bateria: 7.0 }), amostra(2, { tensao_bateria: 8.4 })]);
  assert.equal((await encerrar(db, sobe)).consumo_bateria, 0);
  const cai = await novaCorrida(db);
  await registrar(db, cai, [amostra(1, { tensao_bateria: 8.4 }), amostra(2, { tensao_bateria: 5.0 })]);
  assert.equal((await encerrar(db, cai)).consumo_bateria, 100);
});

test("encerrar de novo sem status devolve a corrida como está; com outro status é recusado", async () => {
  const db = await criarBanco();
  const id = await corridaComTelemetria(db, "objetivo_alcancado");
  const primeira = await encerrar(db, id);
  const segunda = await encerrar(db, id);
  assert.equal(segunda.status, "concluida");
  assert.equal(segunda.tempo_final, primeira.tempo_final);
  assert.deepEqual(segunda.data_hora_fim, primeira.data_hora_fim);
  assert.match(await mensagemDeErro(encerrar(db, id, "nao_concluida")), /corrida_ja_encerrada/);
  assert.match(await mensagemDeErro(encerrar(db, id, "talvez")), /status_invalido/);
});

test("amostras que chegam depois de a corrida encerrar são aceitas e as métricas são refeitas", async () => {
  const db = await criarBanco();
  const id = await novaCorrida(db);
  await registrar(db, id, [amostra(1, { velocidade: 0.1, tempo_decorrido: 1 })]);
  const fechada = await encerrar(db, id);
  assert.equal(fechada.status, "nao_concluida");
  assert.equal(fechada.tempo_final, 1);

  // A rede voltou e o robô reenviou o resto, com o evento de chegada.
  const r = await registrar(db, id, [
    amostra(2, { velocidade: 0.5, tempo_decorrido: 4 }),
    amostra(3, { velocidade: 0.3, tempo_decorrido: 5, evento: "objetivo_alcancado" }),
  ]);
  assert.deepEqual(r, { gravadas: 2, existentes: 0, reconsolidada: true });
  const c = (await db.query<Record<string, any>>("SELECT * FROM corridas WHERE id = $1", [id])).rows[0];
  assert.equal(c.status, "concluida");   // promovida pelo evento
  assert.equal(c.tempo_final, 5);
  assert.equal(c.velocidade_pico, 0.5);
  assert.ok(Math.abs(c.velocidade_media - 0.3) < 1e-9);
});

test("a consolidação nunca rebaixa uma corrida concluída", async () => {
  const db = await criarBanco();
  const id = await novaCorrida(db);
  await encerrar(db, id, "concluida");
  await registrar(db, id, [amostra(1, { velocidade: 0.2, tempo_decorrido: 2 })]);  // sem o evento
  const c = (await db.query<Record<string, any>>("SELECT status FROM corridas WHERE id = $1", [id])).rows[0];
  assert.equal(c.status, "concluida");
});

test("consolidar uma corrida em andamento atualiza as métricas e não muda o status", async () => {
  const db = await criarBanco();
  const id = await corridaComTelemetria(db, "objetivo_alcancado");
  const r = await como(db, "service_role", () => db.query<Record<string, any>>("SELECT * FROM consolidar_corrida($1)", [id]));
  assert.equal(r.rows[0].status, "em_andamento");
  assert.equal(r.rows[0].data_hora_fim, null);
  assert.equal(r.rows[0].tempo_final, 3.25);
  assert.equal(r.rows[0].velocidade_pico, 0.4);
});

test("consolidar uma corrida inexistente é recusado", async () => {
  const db = await criarBanco();
  const erro = await mensagemDeErro(como(db, "service_role", () => db.query("SELECT * FROM consolidar_corrida(999)")));
  assert.match(erro, /corrida_inexistente/);
});

// ---------------------------------------------------------------- registrar_mapa

test("o mapa final é gravado, com true, false e null nas paredes", async () => {
  const db = await criarBanco();
  const id = await novaCorrida(db);
  const { celulas } = fixture("mapa-final-4x4.json");
  assert.deepEqual(await mapa(db, id, celulas), { novas: 16, atualizadas: 0 });
  assert.equal(await contar(db, "mapa_celulas", id), 16);

  const desconhecida = (await db.query<Record<string, any>>("SELECT * FROM mapa_celulas WHERE corrida_id = $1 AND linha = 2 AND coluna = 2", [id])).rows[0];
  assert.equal(desconhecida.parede_norte, null);
  assert.equal(desconhecida.parede_oeste, null);
  const canto = (await db.query<Record<string, any>>("SELECT * FROM mapa_celulas WHERE corrida_id = $1 AND linha = 0 AND coluna = 0", [id])).rows[0];
  assert.equal(canto.parede_sul, true);
  assert.equal(canto.parede_oeste, true);
  assert.equal(canto.parede_norte, false);
});

test("reenviar o mapa atualiza as células e não duplica", async () => {
  const db = await criarBanco();
  const id = await novaCorrida(db);
  const { celulas } = fixture("mapa-final-4x4.json");
  await mapa(db, id, celulas);
  const corrigida = celulas.map((c: Amostra) => (c.linha === 2 && c.coluna === 2 ? { ...c, parede_norte: true, parede_sul: false } : c));
  assert.deepEqual(await mapa(db, id, corrigida), { novas: 0, atualizadas: 16 });
  assert.equal(await contar(db, "mapa_celulas", id), 16);
  const c = (await db.query<Record<string, any>>("SELECT parede_norte, parede_sul FROM mapa_celulas WHERE corrida_id = $1 AND linha = 2 AND coluna = 2", [id])).rows[0];
  assert.deepEqual(c, { parede_norte: true, parede_sul: false });
});

test("um mapa parcial grava só as células enviadas e um reenvio completa o resto", async () => {
  const db = await criarBanco();
  const id = await novaCorrida(db);
  const { celulas } = fixture("mapa-final-4x4.json");
  assert.deepEqual(await mapa(db, id, celulas.slice(0, 6)), { novas: 6, atualizadas: 0 });
  assert.deepEqual(await mapa(db, id, celulas), { novas: 10, atualizadas: 6 });
});

test("o mapa é conferido com o labirinto da corrida", async () => {
  const db = await criarBanco();
  const id = await novaCorrida(db, "4x4");
  const celula = (extra: Amostra = {}) => ({ linha: 0, coluna: 0, parede_norte: false, parede_sul: true, parede_leste: false, parede_oeste: true, ...extra });
  const invalidos: unknown[] = [
    [],
    {},
    [celula({ linha: 4 })],
    [celula({ coluna: 4 })],
    [celula({ linha: -1 })],
    [celula({ parede_norte: "sim" })],
    [celula({ parede_sul: 1 })],
    [celula(), celula()],                    // a mesma célula duas vezes
    [1],
  ];
  for (const ruim of invalidos) {
    assert.match(await mensagemDeErro(mapa(db, id, ruim)), /celulas_invalidas/, JSON.stringify(ruim));
  }
  const demais = Array.from({ length: 17 }, (_, i) => celula({ linha: i % 4, coluna: Math.floor(i / 4) % 4 }));
  assert.match(await mensagemDeErro(mapa(db, id, demais)), /mapa_grande_demais/);
  assert.match(await mensagemDeErro(mapa(db, 999, [celula()])), /corrida_inexistente/);
  assert.equal(await contar(db, "mapa_celulas", id), 0);
});

test("o mapa de um labirinto maior aceita as células de que precisa", async () => {
  const db = await criarBanco();
  const id = await novaCorrida(db, "12x4");
  const todas = Array.from({ length: 48 }, (_, i) => ({ linha: i % 4, coluna: Math.floor(i / 4), parede_norte: null, parede_sul: null, parede_leste: null, parede_oeste: null }));
  assert.deepEqual(await mapa(db, id, todas), { novas: 48, atualizadas: 0 });
});

// ---------------------------------------------------------------- permissões

test("só a service_role executa as funções novas: anon e authenticated não", async () => {
  const db = await criarBanco();
  const id = await novaCorrida(db);
  const chamadas: [string, unknown[]][] = [
    ["SELECT registrar_telemetria($1, '[]'::jsonb)", [id]],
    ["SELECT registrar_mapa($1, '[]'::jsonb)", [id]],
    ["SELECT consolidar_corrida($1)", [id]],
    ["SELECT * FROM metricas_da_telemetria($1)", [id]],
  ];
  for (const papel of ["anon", "authenticated"]) {
    for (const [sql, params] of chamadas) {
      const erro = await mensagemDeErro(como(db, papel, () => db.query(sql, params)));
      assert.match(erro, /permission denied/i, `${papel}: ${sql}`);
    }
  }
  assert.equal(await contar(db, "telemetria", id), 0);
});

test("anon não escreve na telemetria nem no mapa direto, mas lê o que a service_role gravou", async () => {
  const db = await criarBanco();
  const id = await novaCorrida(db);
  await registrar(db, id, [amostra(1)]);
  await mapa(db, id, [{ linha: 0, coluna: 0, parede_norte: false, parede_sul: true, parede_leste: false, parede_oeste: true }]);
  const lidas = await como(db, "anon", () => db.query<{ n: number }>("SELECT COUNT(*)::int AS n FROM telemetria WHERE corrida_id = $1", [id]));
  assert.equal(lidas.rows[0].n, 1);
  const erro = await mensagemDeErro(
    como(db, "anon", () => db.query("INSERT INTO telemetria (corrida_id, numero_sequencia, posicao_linha, posicao_coluna, orientacao) VALUES ($1, 9, 0, 0, 'N')", [id])),
  );
  assert.match(erro, /row-level security|permission denied/i);
});

test("as linhas da telemetria chegam ao Realtime: a tabela está na publicação do dashboard", async () => {
  const db = await criarBanco();
  const r = await db.query<{ tablename: string }>(
    "SELECT tablename FROM pg_publication_tables WHERE pubname = 'supabase_realtime' ORDER BY tablename",
  );
  assert.ok(r.rows.some((l) => l.tablename === "telemetria"));
});

// ---------------------------------------------------------------- contrato e corrida completa

test("o lote de exemplo do contrato com o firmware é aceito como está", async () => {
  const db = await criarBanco();
  const id = await novaCorrida(db);
  const lote = fixture("lote-telemetria.json");
  assert.equal(lote.corrida_id, id);
  const r = await registrar(db, lote.corrida_id, lote.amostras);
  assert.deepEqual(r, { gravadas: lote.amostras.length, existentes: 0, reconsolidada: false });
});

test("corrida simulada completa: cria, grava em lotes, reenvia um lote, grava o mapa e encerra", async () => {
  const db = await criarBanco();
  const id = await novaCorrida(db, "4x4");
  const lote = fixture("lote-telemetria.json").amostras as Amostra[];

  // O robô manda em dois lotes e reenvia o primeiro porque a resposta se perdeu.
  assert.equal((await registrar(db, id, lote.slice(0, 3))).gravadas, 3);
  assert.equal((await registrar(db, id, lote.slice(0, 3))).gravadas, 0);
  assert.equal((await registrar(db, id, lote.slice(3))).gravadas, 2);
  assert.deepEqual(await mapa(db, id, fixture("mapa-final-4x4.json").celulas), { novas: 16, atualizadas: 0 });

  // Encerra sem informar nada: tudo sai da telemetria.
  const c = await encerrar(db, id);
  assert.equal(c.status, "concluida");        // houve o evento objetivo_alcancado
  assert.equal(c.tempo_final, 4);
  assert.equal(c.velocidade_pico, 0.3);
  assert.ok(Math.abs(c.velocidade_media - (0 + 0.12 + 0.18 + 0.3 + 0.06) / 5) < 1e-9);
  assert.equal(c.consumo_bateria, null);

  // Sem duplicatas e na ordem, para o dashboard e o histórico.
  assert.equal(await contar(db, "telemetria", id), 5);
  assert.equal(await contar(db, "mapa_celulas", id), 16);
  const sequencias = await db.query<{ numero_sequencia: number }>("SELECT numero_sequencia FROM telemetria WHERE corrida_id = $1 ORDER BY numero_sequencia", [id]);
  assert.deepEqual(sequencias.rows.map((l) => l.numero_sequencia), [1, 2, 3, 4, 5]);

  // O histórico enxerga a corrida pelo papel anon.
  const historico = await como(db, "anon", () => db.query<{ status: string }>("SELECT status FROM corridas WHERE id = $1", [id]));
  assert.equal(historico.rows[0].status, "concluida");
});
