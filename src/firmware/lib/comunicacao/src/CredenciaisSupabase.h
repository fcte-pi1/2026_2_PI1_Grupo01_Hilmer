#pragma once

/// @file CredenciaisSupabase.h
/// @brief Credenciais do envio ao Supabase e a montagem das URLs das Edge Functions (#238, #236).
///
/// As credenciais **não ficam no repositório**: o `main.cpp` as lê de `include/credenciais.h`, que o
/// Git ignora (o modelo versionado é `include/credenciais.exemplo.h`). Sem esse arquivo, ou com algum
/// campo vazio, `credenciaisValidas()` é falso e o firmware funciona sem enviar nada.

#include <stddef.h>

namespace micromouse {

/// O que o cliente HTTP precisa para falar com o Supabase.
struct CredenciaisSupabase {
  const char* ssidWifi = nullptr;           ///< Rede Wi-Fi.
  const char* senhaWifi = nullptr;          ///< Senha da rede (pode ser vazia numa rede aberta).
  /// Rede reserva (#491), tentada quando a principal não conecta. Vazia ou nula: só a principal.
  const char* ssidWifiReserva = nullptr;
  const char* senhaWifiReserva = nullptr;   ///< Senha da rede reserva.
  const char* urlProjeto = nullptr;         ///< `https://<projeto>.supabase.co`, sem `/` no fim.
  /// Chave `anon`/publishable, opcional: só vai nos cabeçalhos se existir. As funções publicadas com
  /// `--no-verify-jwt` não a exigem.
  const char* chaveAnon = nullptr;
  /// `DEVICE_TOKEN`, opcional: só vai no cabeçalho se existir. As funções só o exigem se o segredo
  /// `DEVICE_TOKEN` estiver configurado no Supabase.
  const char* tokenDispositivo = nullptr;
  /// Certificado raiz (PEM) próprio, opcional. Nulo ou vazio: vale `CERTIFICADOS_RAIZ_PADRAO`
  /// (`CertificadosRaiz.h`), embutido no firmware, e ninguém precisa colar certificado.
  const char* certificadoRaiz = nullptr;
  /// `true` só na bancada: aceita qualquer certificado (`setInsecure`), sem validar o servidor.
  bool semValidarCertificado = false;
};

/// `true` se dá para enviar: SSID e URL `https://` preenchidos. A chave, o token e o certificado são
/// opcionais (sem certificado próprio, o cliente valida o HTTPS com as raízes embutidas).
bool credenciaisValidas(const CredenciaisSupabase& credenciais);

/// Monta a URL de uma Edge Function: `urlProjeto` + `/functions/v1/` + `caminho` (sem barra dupla
/// mesmo que `urlProjeto` termine em `/`).
/// @param caminho Por exemplo `"telemetria"` ou `"corridas/12/encerrar"`.
/// @return Bytes escritos (sem o `\0`), ou 0 se não couber.
size_t montarUrlFuncao(const char* urlProjeto, const char* caminho, char* saida, size_t capacidade);

}  // namespace micromouse
