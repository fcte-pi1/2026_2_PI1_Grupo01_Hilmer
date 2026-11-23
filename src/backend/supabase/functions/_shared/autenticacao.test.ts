import test from "node:test";
import assert from "node:assert/strict";
import { comparacaoSegura, tokenValido } from "./autenticacao.ts";

test("token igual ao esperado é válido", () => {
  assert.equal(tokenValido("segredo-123", "segredo-123"), true);
});

test("token diferente, de outro tamanho ou com prefixo igual é recusado", () => {
  assert.equal(tokenValido("segredo-124", "segredo-123"), false);
  assert.equal(tokenValido("segredo", "segredo-123"), false);
  assert.equal(tokenValido("segredo-1234", "segredo-123"), false);
});

test("com token configurado, sem token recebido nunca autoriza", () => {
  assert.equal(tokenValido(null, "segredo"), false);
  assert.equal(tokenValido(undefined, "segredo"), false);
  assert.equal(tokenValido("", "segredo"), false);
});

test("sem token configurado, o acesso é aberto (token opcional)", () => {
  assert.equal(tokenValido("segredo", undefined), true);
  assert.equal(tokenValido("segredo", ""), true);
  assert.equal(tokenValido("", ""), true);
  assert.equal(tokenValido(null, null), true);
});

test("comparação segura trata acentos (bytes UTF-8)", () => {
  assert.equal(comparacaoSegura("sé", "sé"), true);
  assert.equal(comparacaoSegura("sé", "se"), false);
});
