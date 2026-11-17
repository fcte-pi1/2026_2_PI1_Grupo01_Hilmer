// Testa `definir_labirinto_corrida` (migração 20261117130000): o robô descobre o tamanho do labirinto
// depois de a corrida nascer e a corrida passa para o labirinto certo, com a tentativa renumerada.

import test from "node:test";
import assert from "node:assert/strict";
import { como, criarBanco, mensagemDeErro } from "./ajudantes.ts";

type Banco = Awaited<ReturnType<typeof criarBanco>>;

async function criar(db: Banco, tipo: string): Promise<number> {
  const r = await como(db, "service_role", () => db.query<{ id: number }>("SELECT id FROM criar_corrida($1)", [tipo]));
  return r.rows[0].id;
}

async function definir(db: Banco, id: number, tipo: string) {
  const r = await como(db, "service_role", () =>
    db.query<{ tipo: string; numero_tentativa: number }>(
      "SELECT l.tipo, c.numero_tentativa FROM definir_labirinto_corrida($1, $2) c JOIN labirintos l ON l.id = c.labirinto_id",
      [id, tipo],
    ),
  );
  return r.rows[0];
}

test("a corrida passa para o labirinto que o robô descobriu, com a tentativa dele", async () => {
  const db = await criarBanco();
  const existente = await criar(db, "8x4"); // já há uma tentativa 1 no 8x4
  const id = await criar(db, "4x4"); // nasce como 4x4, o padrão do firmware
  const r = await definir(db, id, "8x4");
  assert.deepEqual(r, { tipo: "8x4", numero_tentativa: 2 });
  assert.notEqual(existente, id);
});

test("repetir o mesmo tamanho não muda nada (reenvio depois de uma queda de rede)", async () => {
  const db = await criarBanco();
  const id = await criar(db, "4x4");
  assert.deepEqual(await definir(db, id, "4x4"), { tipo: "4x4", numero_tentativa: 1 });
  await definir(db, id, "12x4");
  assert.deepEqual(await definir(db, id, "12x4"), { tipo: "12x4", numero_tentativa: 1 });
});

test("duas corridas que descobrem o mesmo labirinto ficam com tentativas diferentes", async () => {
  const db = await criarBanco();
  const a = await criar(db, "4x4");
  const b = await criar(db, "4x4");
  const ra = await definir(db, a, "12x4");
  const rb = await definir(db, b, "12x4");
  assert.deepEqual([ra.numero_tentativa, rb.numero_tentativa], [1, 2]);
});

test("recusa labirinto desconhecido, corrida inexistente e corrida já encerrada", async () => {
  const db = await criarBanco();
  const id = await criar(db, "4x4");
  const falha = (consulta: string, params: unknown[]) =>
    mensagemDeErro(como(db, "service_role", () => db.query(consulta, params)));
  assert.match(await falha("SELECT * FROM definir_labirinto_corrida($1, $2)", [id, "5x5"]), /labirinto_inexistente/);
  assert.match(await falha("SELECT * FROM definir_labirinto_corrida($1, $2)", [999, "8x4"]), /corrida_inexistente/);
  await como(db, "service_role", () => db.query("SELECT * FROM encerrar_corrida($1, 'nao_concluida')", [id]));
  assert.match(await falha("SELECT * FROM definir_labirinto_corrida($1, $2)", [id, "8x4"]), /corrida_ja_encerrada/);
});

test("não encolhe o labirinto se já há telemetria ou mapa fora da malha nova", async () => {
  const db = await criarBanco();
  const id = await criar(db, "12x4");
  await db.exec(`INSERT INTO mapa_celulas (corrida_id, linha, coluna) VALUES (${id}, 3, 9)`);
  const msg = await mensagemDeErro(
    como(db, "service_role", () => db.query("SELECT * FROM definir_labirinto_corrida($1, '4x4')", [id])),
  );
  assert.match(msg, /telemetria_fora_do_labirinto/);
  // O que cabe nas duas malhas não atrapalha.
  const outro = await criar(db, "12x4");
  await db.exec(`INSERT INTO mapa_celulas (corrida_id, linha, coluna) VALUES (${outro}, 1, 1)`);
  assert.equal((await definir(db, outro, "4x4")).tipo, "4x4");
});

test("anon e authenticated não executam a função", async () => {
  const db = await criarBanco();
  const id = await criar(db, "4x4");
  for (const papel of ["anon", "authenticated"]) {
    const msg = await mensagemDeErro(como(db, papel, () => db.query("SELECT * FROM definir_labirinto_corrida($1, '8x4')", [id])));
    assert.match(msg, /permission denied/i, papel);
  }
});
