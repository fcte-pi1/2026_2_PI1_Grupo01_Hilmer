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

test("sem token recebido ou sem token configurado, nunca autoriza", () => {
  assert.equal(tokenValido(null, "segredo"), false);
  assert.equal(tokenValido(undefined, "segredo"), false);
  assert.equal(tokenValido("", "segredo"), false);
  assert.equal(tokenValido("segredo", undefined), false);
  assert.equal(tokenValido("segredo", ""), false);
  assert.equal(tokenValido("", ""), false);
  assert.equal(tokenValido(null, null), false);
});

test("comparação segura trata acentos (bytes UTF-8)", () => {
  assert.equal(comparacaoSegura("sé", "sé"), true);
  assert.equal(comparacaoSegura("sé", "se"), false);
});
