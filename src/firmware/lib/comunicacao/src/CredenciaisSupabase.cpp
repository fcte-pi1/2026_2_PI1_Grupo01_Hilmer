#include "CredenciaisSupabase.h"

#include <stdio.h>
#include <string.h>

namespace micromouse {

namespace {

bool preenchido(const char* texto) { return texto != nullptr && texto[0] != '\0'; }

}  // namespace

bool credenciaisValidas(const CredenciaisSupabase& c) {
  if (!preenchido(c.ssidWifi) || c.senhaWifi == nullptr || !preenchido(c.urlProjeto)) {
    return false;
  }
  if (strncmp(c.urlProjeto, "https://", 8) != 0 || c.urlProjeto[8] == '\0') {
    return false;  // só HTTPS: o que o robô manda não pode viajar aberto
  }
  return true;  // sem certificado próprio, o cliente usa os certificados raiz embutidos (CertificadosRaiz.h)
}

size_t montarUrlFuncao(const char* urlProjeto, const char* caminho, char* saida, size_t capacidade) {
  if (urlProjeto == nullptr || caminho == nullptr || saida == nullptr || capacidade == 0) {
    return 0;
  }
  size_t base = strlen(urlProjeto);
  while (base > 0 && urlProjeto[base - 1] == '/') {
    --base;
  }
  while (*caminho == '/') {
    ++caminho;
  }
  const int escritos = snprintf(saida, capacidade, "%.*s/functions/v1/%s", static_cast<int>(base), urlProjeto, caminho);
  if (escritos < 0 || static_cast<size_t>(escritos) >= capacidade) {
    saida[0] = '\0';
    return 0;
  }
  return static_cast<size_t>(escritos);
}

}  // namespace micromouse
