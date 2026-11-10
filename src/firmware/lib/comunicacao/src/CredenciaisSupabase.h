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
  const char* chaveAnon = nullptr;          ///< Chave `anon` (JWT), exigida pelo gateway das Edge Functions.
  const char* tokenDispositivo = nullptr;   ///< `DEVICE_TOKEN`, conferido pelas Edge Functions.
  const char* certificadoRaiz = nullptr;    ///< Certificado raiz (PEM) que valida o HTTPS do Supabase.
  /// `true` só na bancada: aceita qualquer certificado (`setInsecure`), sem validar o servidor.
  bool semValidarCertificado = false;
};

/// `true` se dá para enviar: SSID, URL `https://`, chave `anon` e token preenchidos, e um
/// certificado raiz (ou a exceção de bancada `semValidarCertificado`).
bool credenciaisValidas(const CredenciaisSupabase& credenciais);

/// Monta a URL de uma Edge Function: `urlProjeto` + `/functions/v1/` + `caminho` (sem barra dupla
/// mesmo que `urlProjeto` termine em `/`).
/// @param caminho Por exemplo `"telemetria"` ou `"corridas/12/encerrar"`.
/// @return Bytes escritos (sem o `\0`), ou 0 se não couber.
size_t montarUrlFuncao(const char* urlProjeto, const char* caminho, char* saida, size_t capacidade);

}  // namespace micromouse
