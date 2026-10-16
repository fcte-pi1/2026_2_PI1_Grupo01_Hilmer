// Testa as migrações e as funções SQL de corrida num PostgreSQL de verdade (PGlite, em WebAssembly).
// Imita o Supabase: papéis anon/authenticated/service_role, publicação do Realtime e as permissões
// padrão que o Supabase concede a anon e authenticated.

import test from "node:test";
import assert from "node:assert/strict";
import { readFileSync, readdirSync } from "node:fs";
import { join } from "node:path";
import { fileURLToPath } from "node:url";
import { PGlite } from "@electric-sql/pglite";

const PASTA_MIGRACOES = fileURLToPath(new URL("../supabase/migrations/", import.meta.url));

async function criarBanco(): Promise<PGlite> {
  const db = new PGlite();
  await db.exec(`
    CREATE ROLE anon NOLOGIN;
    CREATE ROLE authenticated NOLOGIN;
    CREATE ROLE service_role NOLOGIN BYPASSRLS;
    CREATE PUBLICATION supabase_realtime;
    GRANT USAGE ON SCHEMA public TO anon, authenticated, service_role;
    ALTER DEFAULT PRIVILEGES IN SCHEMA public GRANT ALL ON TABLES TO anon, authenticated, service_role;
    ALTER DEFAULT PRIVILEGES IN SCHEMA public GRANT ALL ON SEQUENCES TO anon, authenticated, service_role;
    ALTER DEFAULT PRIVILEGES IN SCHEMA public GRANT ALL ON FUNCTIONS TO anon, authenticated, service_role;
  `);
  const arquivos = readdirSync(PASTA_MIGRACOES).filter((f) => f.endsWith(".sql")).sort();
  for (const arquivo of arquivos) {
    await db.exec(readFileSync(join(PASTA_MIGRACOES, arquivo), "utf8"));
  }
  return db;
}

/** Executa como um papel do Supabase e volta ao administrador no fim. */
async function como<T>(db: PGlite, papel: string, acao: () => Promise<T>): Promise<T> {
  await db.exec(`SET ROLE ${papel}`);
  try {
    return await acao();
  } finally {
    await db.exec("RESET ROLE");
  }
}

async function mensagemDeErro(promessa: Promise<unknown>): Promise<string> {
  try {
    await promessa;
  } catch (e) {
    return (e as Error).message;
  }
  return "";
}

test("as migrações rodam em ordem e o seed traz os três labirintos com linhas e colunas certas", async () => {
  const db = await criarBanco();
  const r = await db.query<{ tipo: string; linhas: number; colunas: number }>(
    "SELECT tipo, linhas, colunas FROM labirintos ORDER BY colunas",
  );
  assert.deepEqual(r.rows, [
    { tipo: "4x4", linhas: 4, colunas: 4 },
    { tipo: "8x4", linhas: 4, colunas: 8 },
    { tipo: "12x4", linhas: 4, colunas: 12 },
  ]);
});

test("a tentativa é numerada por labirinto, nos três tipos", async () => {
  const db = await criarBanco();
  const tentativa = async (tipo: string) => {
    const r = await como(db, "service_role", () =>
      db.query<{ numero_tentativa: number }>("SELECT numero_tentativa FROM criar_corrida($1)", [tipo]),
    );
    return r.rows[0].numero_tentativa;
  };
  assert.equal(await tentativa("4x4"), 1);
  assert.equal(await tentativa("4x4"), 2);
  assert.equal(await tentativa("8x4"), 1);
  assert.equal(await tentativa("12x4"), 1);
  assert.equal(await tentativa("4x4"), 3);
  assert.equal(await tentativa("12x4"), 2);
});

test("a corrida nasce em andamento, com o labirinto e a data de início", async () => {
  const db = await criarBanco();
  const r = await como(db, "service_role", () =>
    db.query<Record<string, unknown>>("SELECT * FROM criar_corrida('8x4')"),
  );
  const c = r.rows[0];
  assert.equal(c.status, "em_andamento");
  assert.equal(c.data_hora_fim, null);
  assert.equal(c.tempo_final, null);
  assert.ok(c.data_hora_inicio instanceof Date);
  const lab = await db.query<{ tipo: string }>("SELECT tipo FROM labirintos WHERE id = $1", [c.labirinto_id]);
  assert.equal(lab.rows[0].tipo, "8x4");
});

test("labirinto inexistente é recusado e não cria corrida", async () => {
  const db = await criarBanco();
  const msg = await mensagemDeErro(
    como(db, "service_role", () => db.query("SELECT * FROM criar_corrida('5x5')")),
  );
  assert.match(msg, /labirinto_inexistente/);
  const n = await db.query<{ n: number }>("SELECT count(*)::int AS n FROM corridas");
  assert.equal(n.rows[0].n, 0);
});

test("encerrar muda o status, preenche data_hora_fim e guarda as métricas", async () => {
  const db = await criarBanco();
  for (const status of ["concluida", "nao_concluida"]) {
    const criada = await como(db, "service_role", () =>
      db.query<{ id: number }>("SELECT id FROM criar_corrida('4x4')"),
    );
    const id = criada.rows[0].id;
    const r = await como(db, "service_role", () =>
      db.query<Record<string, unknown>>("SELECT * FROM encerrar_corrida($1, $2, 61.5, 0.31, 0.8, 12)", [id, status]),
    );
    const c = r.rows[0];
    assert.equal(c.status, status);
    assert.ok(c.data_hora_fim instanceof Date);
    assert.equal(c.tempo_final, 61.5);
    assert.equal(c.velocidade_media, 0.31);
    assert.equal(c.velocidade_pico, 0.8);
    assert.equal(c.consumo_bateria, 12);
  }
});

test("sem tempo_final, o tempo decorrido desde o início é usado", async () => {
  const db = await criarBanco();
  const criada = await db.query<{ id: number }>("SELECT id FROM criar_corrida('4x4')");
  await db.exec(`UPDATE corridas SET data_hora_inicio = now() - interval '90 seconds' WHERE id = ${criada.rows[0].id}`);
  const r = await db.query<{ tempo_final: number }>("SELECT tempo_final FROM encerrar_corrida($1, 'concluida')", [criada.rows[0].id]);
  assert.ok(r.rows[0].tempo_final >= 90 && r.rows[0].tempo_final < 95, String(r.rows[0].tempo_final));
});

test("encerrar de novo com o mesmo status é idempotente e não mexe na corrida", async () => {
  const db = await criarBanco();
  const criada = await db.query<{ id: number }>("SELECT id FROM criar_corrida('4x4')");
  const id = criada.rows[0].id;
  const primeira = await db.query<Record<string, unknown>>("SELECT * FROM encerrar_corrida($1, 'concluida', 10)", [id]);
  const segunda = await db.query<Record<string, unknown>>("SELECT * FROM encerrar_corrida($1, 'concluida', 99)", [id]);
  assert.deepEqual(segunda.rows[0], primeira.rows[0]);
  assert.equal(segunda.rows[0].tempo_final, 10);
});

test("encerrar com outro status uma corrida já encerrada é recusado", async () => {
  const db = await criarBanco();
  const criada = await db.query<{ id: number }>("SELECT id FROM criar_corrida('4x4')");
  const id = criada.rows[0].id;
  await db.query("SELECT * FROM encerrar_corrida($1, 'concluida')", [id]);
  const msg = await mensagemDeErro(db.query("SELECT * FROM encerrar_corrida($1, 'nao_concluida')", [id]));
  assert.match(msg, /corrida_ja_encerrada/);
  const r = await db.query<{ status: string }>("SELECT status FROM corridas WHERE id = $1", [id]);
  assert.equal(r.rows[0].status, "concluida");
});

test("status inválido e corrida inexistente são recusados", async () => {
  const db = await criarBanco();
  const criada = await db.query<{ id: number }>("SELECT id FROM criar_corrida('4x4')");
  for (const status of ["em_andamento", "ok", ""]) {
    const msg = await mensagemDeErro(db.query("SELECT * FROM encerrar_corrida($1, $2)", [criada.rows[0].id, status]));
    assert.match(msg, /status_invalido/, status);
  }
  const msg = await mensagemDeErro(db.query("SELECT * FROM encerrar_corrida(9999, 'concluida')"));
  assert.match(msg, /corrida_inexistente/);
});

test("a restrição única impede duas corridas com a mesma tentativa no mesmo labirinto", async () => {
  const db = await criarBanco();
  await db.query("SELECT * FROM criar_corrida('4x4')");
  const msg = await mensagemDeErro(
    db.query("INSERT INTO corridas (labirinto_id, numero_tentativa) SELECT id, 1 FROM labirintos WHERE tipo = '4x4'"),
  );
  assert.match(msg, /uq_corridas_labirinto_tentativa/);
});

test("anon e authenticated não executam as funções de corrida", async () => {
  const db = await criarBanco();
  const criada = await db.query<{ id: number }>("SELECT id FROM criar_corrida('4x4')");
  for (const papel of ["anon", "authenticated"]) {
    const m1 = await mensagemDeErro(como(db, papel, () => db.query("SELECT * FROM criar_corrida('4x4')")));
    assert.match(m1, /permission denied/, `${papel} criar`);
    const m2 = await mensagemDeErro(
      como(db, papel, () => db.query("SELECT * FROM encerrar_corrida($1, 'concluida')", [criada.rows[0].id])),
    );
    assert.match(m2, /permission denied/, `${papel} encerrar`);
  }
});

test("anon lê as tabelas mas não escreve nelas (RLS só com SELECT)", async () => {
  const db = await criarBanco();
  await db.query("SELECT * FROM criar_corrida('4x4')");
  const lidas = await como(db, "anon", () => db.query<{ n: number }>("SELECT count(*)::int AS n FROM corridas"));
  assert.equal(lidas.rows[0].n, 1);
  const labs = await como(db, "anon", () => db.query<{ n: number }>("SELECT count(*)::int AS n FROM labirintos"));
  assert.equal(labs.rows[0].n, 3);

  const insercoes = [
    "INSERT INTO corridas (labirinto_id) VALUES (1)",
    "INSERT INTO telemetria (corrida_id, numero_sequencia, posicao_linha, posicao_coluna, orientacao) VALUES (1, 1, 0, 0, 'N')",
    "INSERT INTO mapa_celulas (corrida_id, linha, coluna) VALUES (1, 0, 0)",
  ];
  for (const papel of ["anon", "authenticated"]) {
    for (const sql of insercoes) {
      const msg = await mensagemDeErro(como(db, papel, () => db.query(sql)));
      assert.match(msg, /row-level security|permission denied/, `${papel}: ${sql}`);
    }
  }

  // UPDATE e DELETE sem política afetam zero linhas, e o status segue intacto.
  await como(db, "anon", () => db.exec("UPDATE corridas SET status = 'concluida'"));
  await como(db, "anon", () => db.exec("DELETE FROM corridas"));
  const depois = await db.query<{ status: string }>("SELECT status FROM corridas");
  assert.deepEqual(depois.rows, [{ status: "em_andamento" }]);
});

test("service_role ignora o RLS e consegue gravar telemetria e mapa", async () => {
  const db = await criarBanco();
  const criada = await db.query<{ id: number }>("SELECT id FROM criar_corrida('4x4')");
  const id = criada.rows[0].id;
  await como(db, "service_role", async () => {
    await db.query(
      "INSERT INTO telemetria (corrida_id, numero_sequencia, posicao_linha, posicao_coluna, orientacao) VALUES ($1, 1, 0, 0, 'N')",
      [id],
    );
    await db.query("INSERT INTO mapa_celulas (corrida_id, linha, coluna, parede_norte) VALUES ($1, 0, 0, NULL)", [id]);
  });
  const t = await db.query<{ n: number }>("SELECT count(*)::int AS n FROM telemetria");
  assert.equal(t.rows[0].n, 1);
});
