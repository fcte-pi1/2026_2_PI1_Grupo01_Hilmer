// Testa o canal painel -> robô no banco (migração 20261117120000): comandos entregues uma vez e com
// validade, `parar` na frente e cancelando o `iniciar` pendente, logs em lote e permissões do RLS.

import test from "node:test";
import assert from "node:assert/strict";
import { como, criarBanco, mensagemDeErro } from "./ajudantes.ts";

type Entrega = { id: number | null; comando: string | null };

async function enviar(db: Awaited<ReturnType<typeof criarBanco>>, comando: string) {
  const r = await como(db, "service_role", () =>
    db.query<{ r: { id: number; comando: string } }>("SELECT enviar_comando_robo($1) AS r", [comando]),
  );
  return r.rows[0].r;
}

async function proximo(db: Awaited<ReturnType<typeof criarBanco>>, validade = 30): Promise<Entrega> {
  const r = await como(db, "service_role", () =>
    db.query<{ r: Entrega }>("SELECT proximo_comando_robo($1) AS r", [validade]),
  );
  return r.rows[0].r;
}

test("o comando enviado pelo painel é entregue ao robô uma vez só", async () => {
  const db = await criarBanco();
  const enviado = await enviar(db, "iniciar");
  assert.equal(enviado.comando, "iniciar");
  const primeiro = await proximo(db);
  assert.deepEqual(primeiro, { id: enviado.id, comando: "iniciar" });
  assert.deepEqual(await proximo(db), { id: null, comando: null });
});

test("sem comando pendente, o robô recebe comando nulo", async () => {
  const db = await criarBanco();
  assert.deepEqual(await proximo(db), { id: null, comando: null });
});

test("os comandos saem na ordem em que foram enviados", async () => {
  const db = await criarBanco();
  await enviar(db, "zerar");
  await enviar(db, "iniciar");
  assert.equal((await proximo(db)).comando, "zerar");
  assert.equal((await proximo(db)).comando, "iniciar");
});

test("um parar cancela o iniciar que o robô ainda não buscou", async () => {
  const db = await criarBanco();
  await enviar(db, "iniciar");
  await enviar(db, "parar");
  assert.equal((await proximo(db)).comando, "parar");
  // O iniciar foi cancelado: o robô nunca o recebe, mesmo depois do parar.
  assert.deepEqual(await proximo(db), { id: null, comando: null });
});

test("parar passa na frente de um comando pendente enviado depois dele", async () => {
  const db = await criarBanco();
  await enviar(db, "parar");
  await enviar(db, "iniciar");
  assert.equal((await proximo(db)).comando, "parar");
  assert.equal((await proximo(db)).comando, "iniciar");
});

test("um comando velho não é entregue: o iniciar esquecido no painel não larga o robô depois", async () => {
  const db = await criarBanco();
  await enviar(db, "iniciar");
  await db.exec("UPDATE comandos_robo SET criado_em = now() - interval '31 seconds'");
  assert.deepEqual(await proximo(db), { id: null, comando: null });
  // Um comando novo, depois do velho, é entregue normalmente.
  await enviar(db, "iniciar");
  assert.equal((await proximo(db)).comando, "iniciar");
});

test("comando fora da lista é recusado e não é gravado", async () => {
  const db = await criarBanco();
  for (const comando of ["apagar", "", "INICIAR"]) {
    const msg = await mensagemDeErro(como(db, "service_role", () => db.query("SELECT enviar_comando_robo($1)", [comando])));
    assert.match(msg, /comando_invalido/, comando);
  }
  const msgNulo = await mensagemDeErro(como(db, "service_role", () => db.query("SELECT enviar_comando_robo(NULL)")));
  assert.match(msgNulo, /comando_invalido/);
  const n = await db.query<{ n: number }>("SELECT count(*)::int AS n FROM comandos_robo");
  assert.equal(n.rows[0].n, 0);
});

test("os logs do robô são gravados em ordem, com a corrida quando ela existe", async () => {
  const db = await criarBanco();
  const corrida = await como(db, "service_role", () => db.query<{ id: number }>("SELECT id FROM criar_corrida('4x4')"));
  const id = corrida.rows[0].id;
  const r = await como(db, "service_role", () =>
    db.query<{ n: number }>("SELECT registrar_logs_robo($1, ARRAY['primeira', 'segunda', 'terceira']) AS n", [id]),
  );
  assert.equal(r.rows[0].n, 3);
  const linhas = await db.query<{ mensagem: string; corrida_id: number }>(
    "SELECT mensagem, corrida_id FROM logs_robo ORDER BY id",
  );
  assert.deepEqual(linhas.rows.map((l) => l.mensagem), ["primeira", "segunda", "terceira"]);
  assert.ok(linhas.rows.every((l) => l.corrida_id === id));
});

test("log sem corrida, ou de uma corrida que não existe, fica com corrida nula", async () => {
  const db = await criarBanco();
  await como(db, "service_role", () => db.query("SELECT registrar_logs_robo(NULL, ARRAY['antes da corrida'])"));
  await como(db, "service_role", () => db.query("SELECT registrar_logs_robo(999, ARRAY['corrida apagada'])"));
  const linhas = await db.query<{ corrida_id: number | null }>("SELECT corrida_id FROM logs_robo ORDER BY id");
  assert.deepEqual(linhas.rows.map((l) => l.corrida_id), [null, null]);
});

test("o lote de logs inválido é recusado inteiro", async () => {
  const db = await criarBanco();
  const casos: [string, string][] = [
    ["ARRAY[]::text[]", "logs_invalidos"],
    ["ARRAY['ok', '']", "logs_invalidos"],
    ["ARRAY['ok', NULL]", "logs_invalidos"],
    [`ARRAY['ok', '${"x".repeat(301)}']`, "logs_invalidos"],
    ["(SELECT array_agg('a'::text) FROM generate_series(1, 51))", "logs_grande_demais"],
  ];
  for (const [expressao, esperado] of casos) {
    const msg = await mensagemDeErro(
      como(db, "service_role", () => db.query(`SELECT registrar_logs_robo(NULL, ${expressao})`)),
    );
    assert.match(msg, new RegExp(esperado), expressao);
  }
  const n = await db.query<{ n: number }>("SELECT count(*)::int AS n FROM logs_robo");
  assert.equal(n.rows[0].n, 0);
});

test("o limite de 50 linhas de 300 caracteres é aceito", async () => {
  const db = await criarBanco();
  const r = await como(db, "service_role", () =>
    db.query<{ n: number }>(
      `SELECT registrar_logs_robo(NULL, (SELECT array_agg(repeat('x', 300)) FROM generate_series(1, 50))) AS n`,
    ),
  );
  assert.equal(r.rows[0].n, 50);
});

test("logs com mais de 7 dias são apagados quando entra um lote novo", async () => {
  const db = await criarBanco();
  await como(db, "service_role", () => db.query("SELECT registrar_logs_robo(NULL, ARRAY['velho'])"));
  await db.exec("UPDATE logs_robo SET criado_em = now() - interval '8 days'");
  await como(db, "service_role", () => db.query("SELECT registrar_logs_robo(NULL, ARRAY['novo'])"));
  const linhas = await db.query<{ mensagem: string }>("SELECT mensagem FROM logs_robo");
  assert.deepEqual(linhas.rows.map((l) => l.mensagem), ["novo"]);
});

test("anon só lê: não executa as funções nem escreve nas tabelas", async () => {
  const db = await criarBanco();
  await enviar(db, "iniciar");
  const lidos = await como(db, "anon", () => db.query("SELECT * FROM comandos_robo"));
  assert.equal(lidos.rows.length, 1);
  await como(db, "anon", () => db.query("SELECT * FROM logs_robo"));

  await como(db, "service_role", () => db.query("SELECT registrar_logs_robo(NULL, ARRAY['existente'])"));
  for (const papel of ["anon", "authenticated"]) {
    for (const consulta of [
      "SELECT enviar_comando_robo('iniciar')",
      "SELECT proximo_comando_robo(30)",
      "SELECT registrar_logs_robo(NULL, ARRAY['x'])",
      "INSERT INTO comandos_robo (comando) VALUES ('iniciar')",
      "INSERT INTO logs_robo (mensagem) VALUES ('x')",
    ]) {
      const msg = await mensagemDeErro(como(db, papel, () => db.query(consulta)));
      assert.match(msg, /permission denied|row-level security|must be owner/i, `${papel}: ${consulta}`);
    }
    // Sem política de UPDATE e DELETE, o RLS esconde as linhas: o comando roda e não muda nada.
    const atualizadas = await como(db, papel, () => db.query("UPDATE comandos_robo SET entregue_em = now()"));
    const apagadas = await como(db, papel, () => db.query("DELETE FROM logs_robo"));
    assert.equal(atualizadas.affectedRows ?? 0, 0, `${papel}: UPDATE`);
    assert.equal(apagadas.affectedRows ?? 0, 0, `${papel}: DELETE`);
  }
  const intactos = await db.query<{ pendentes: number; logs: number }>(
    "SELECT (SELECT count(*) FROM comandos_robo WHERE entregue_em IS NULL)::int AS pendentes, (SELECT count(*) FROM logs_robo)::int AS logs",
  );
  assert.deepEqual(intactos.rows[0], { pendentes: 1, logs: 1 });
});

test("as duas tabelas estão no Realtime", async () => {
  const db = await criarBanco();
  const r = await db.query<{ tablename: string }>(
    "SELECT tablename FROM pg_publication_tables WHERE pubname = 'supabase_realtime' AND tablename IN ('comandos_robo', 'logs_robo') ORDER BY tablename",
  );
  assert.deepEqual(r.rows.map((x) => x.tablename), ["comandos_robo", "logs_robo"]);
});
