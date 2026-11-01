#pragma once

/// @file credenciais.exemplo.h
/// @brief Modelo das credenciais do envio da telemetria (#238, #236). **Este arquivo vai para o Git;
/// as credenciais de verdade, não.**
///
/// Como usar:
/// 1. Copie este arquivo para `include/credenciais.h` (o Git ignora esse nome).
/// 2. Preencha os campos abaixo. Peça a chave `anon`, a URL e o `DEVICE_TOKEN` a quem administra o
///    Supabase do grupo; nunca cole esses valores em issue, PR ou conversa.
/// 3. Grave a placa: `pio run -e esp32c3 -t upload`.
///
/// Sem `include/credenciais.h`, o firmware compila, inicia e não envia nada (o log mostra
/// `envio_desligado`).

/// Rede Wi-Fi (2,4 GHz: o ESP32-C3 não conecta em 5 GHz).
#define CREDENCIAL_WIFI_SSID "nome-da-rede"
#define CREDENCIAL_WIFI_SENHA "senha-da-rede"

/// Projeto do Supabase: Settings → API → Project URL (sem `/` no fim).
#define CREDENCIAL_SUPABASE_URL "https://SEU-PROJETO.supabase.co"

/// Chave `anon` (pública, só leitura pelo RLS), exigida pelo gateway das Edge Functions.
#define CREDENCIAL_SUPABASE_ANON "cole-aqui-a-chave-anon"

/// `DEVICE_TOKEN`: o mesmo segredo configurado nas Edge Functions (Settings → Edge Functions → Secrets).
#define CREDENCIAL_DEVICE_TOKEN "cole-aqui-o-device-token"

/// Certificado raiz (PEM) que valida o HTTPS do projeto. Para obter: abra a URL do projeto no
/// navegador, clique no cadeado → certificado → o primeiro da cadeia (a raiz) → exportar em PEM, e
/// cole o conteúdo inteiro aqui, uma linha do PEM por linha abaixo, cada uma entre aspas e terminada em
/// `\n"` e `\` (o `#define` não aceita texto de várias linhas sem isso). Na bancada, com
/// `CREDENCIAL_SEM_VALIDAR_CERTIFICADO 1`, este valor não é usado.
#define CREDENCIAL_CERTIFICADO_RAIZ \
  "-----BEGIN CERTIFICATE-----\n" \
  "cole aqui o certificado raiz\n" \
  "-----END CERTIFICATE-----\n"

/// 1 só na bancada, para testar sem o certificado: aceita qualquer servidor (`setInsecure`). Nunca
/// deixe 1 na versão da competição.
#define CREDENCIAL_SEM_VALIDAR_CERTIFICADO 0
