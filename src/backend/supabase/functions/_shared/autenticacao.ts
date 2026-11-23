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
 * `true` se a requisição pode seguir. O token de dispositivo é opcional: sem o segredo `DEVICE_TOKEN`
 * configurado na função, qualquer requisição passa (modo aberto, para o projeto da disciplina; o robô
 * e o dashboard não precisam de segredo). Configurado o segredo, só passa quem manda o mesmo token.
 */
export function tokenValido(recebido: string | null | undefined, esperado: string | null | undefined): boolean {
  if (!esperado) {
    return true;
  }
  if (!recebido) {
    return false;
  }
  return comparacaoSegura(recebido, esperado);
}
