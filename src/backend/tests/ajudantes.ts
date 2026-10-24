// Ajudantes dos testes do banco: um PostgreSQL de verdade (PGlite, em WebAssembly) que imita o Supabase,
// com os papéis anon/authenticated/service_role, a publicação do Realtime e as permissões padrão que o
// Supabase concede a anon e authenticated. Usado por `banco.test.ts` e `telemetria.test.ts`.

import { readFileSync, readdirSync } from "node:fs";
import { join } from "node:path";
import { fileURLToPath } from "node:url";
import { PGlite } from "@electric-sql/pglite";

const PASTA_MIGRACOES = fileURLToPath(new URL("../supabase/migrations/", import.meta.url));

/** Cria um banco novo e aplica todas as migrações em ordem. */
export async function criarBanco(): Promise<PGlite> {
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
export async function como<T>(db: PGlite, papel: string, acao: () => Promise<T>): Promise<T> {
  await db.exec(`SET ROLE ${papel}`);
  try {
    return await acao();
  } finally {
    await db.exec("RESET ROLE");
  }
}

/** Mensagem do erro de uma promessa que deve falhar; texto vazio se ela não falhar. */
export async function mensagemDeErro(promessa: Promise<unknown>): Promise<string> {
  try {
    await promessa;
  } catch (e) {
    return (e as Error).message;
  }
  return "";
}
