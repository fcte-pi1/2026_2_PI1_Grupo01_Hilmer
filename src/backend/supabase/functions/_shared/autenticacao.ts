/// Autenticação do robô por token de dispositivo (cabeçalho `x-device-token`).

/** Compara dois textos sem parar no primeiro caractere diferente, para não vazar o token pelo tempo de resposta. */
export function comparacaoSegura(a: string, b: string): boolean {
  const codificador = new TextEncoder();
  const bytesA = codificador.encode(a);
  const bytesB = codificador.encode(b);
  let diferenca = bytesA.length ^ bytesB.length;
  const tamanho = Math.max(bytesA.length, bytesB.length);
  for (let i = 0; i < tamanho; i++) {
    diferenca |= (bytesA[i] ?? 0) ^ (bytesB[i] ?? 0);
  }
  return diferenca === 0;
}

/**
 * `true` se o token recebido é igual ao esperado. Sem token esperado configurado (segredo
 * `DEVICE_TOKEN` ausente) ou sem token recebido, nunca autoriza.
 */
export function tokenValido(recebido: string | null | undefined, esperado: string | null | undefined): boolean {
  if (!esperado || !recebido) {
    return false;
  }
  return comparacaoSegura(recebido, esperado);
}
