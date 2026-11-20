#pragma once

/// @file credenciais.exemplo.h
/// @brief Modelo do `include/credenciais.h` local, que o Git ignora. **Este arquivo vai para o Git;
/// o `credenciais.h` de verdade, não.**
///
/// Você normalmente NÃO precisa de `credenciais.h`: o Wi-Fi e a URL do Supabase já vêm de
/// `credenciais_padrao.h`, e a chave e o token são opcionais. Crie o arquivo só para:
/// - usar outro Wi-Fi no seu computador;
/// - informar a chave `anon`/publishable ou o `DEVICE_TOKEN`, se o seu projeto Supabase ainda os exigir.
///
/// Como usar: copie este arquivo para `include/credenciais.h`, descomente e preencha só o que precisa
/// (qualquer `#define` seu vale mais que o padrão). Nunca cole segredos em issue, PR ou conversa, e
/// nunca use a chave `sb_secret_`/`service_role` no firmware. Depois grave: `pio run -e esp32_devkit -t upload`
/// (ou `-e esp32_devkit_ota` pelo Wi-Fi).

// Outro Wi-Fi (2,4 GHz; o ESP32 não conecta em 5 GHz), principal e reserva (#491).
// #define CREDENCIAL_WIFI_SSID "nome-da-rede"
// #define CREDENCIAL_WIFI_SENHA "senha-da-rede"
// #define CREDENCIAL_WIFI_SSID_RESERVA "nome-da-reserva"
// #define CREDENCIAL_WIFI_SENHA_RESERVA "senha-da-reserva"

// Outro projeto Supabase (sem `/` no fim).
// #define CREDENCIAL_SUPABASE_URL "https://SEU-PROJETO.supabase.co"

// OPCIONAL: chave `anon`/publishable, só se o gateway das Edge Functions ainda exigir JWT.
// #define CREDENCIAL_SUPABASE_ANON "cole-aqui-a-chave-anon"

// OPCIONAL: `DEVICE_TOKEN`, só se o segredo existir nas Edge Functions.
// #define CREDENCIAL_DEVICE_TOKEN "cole-aqui-o-device-token"

// OPCIONAL: o HTTPS é validado com as raízes públicas embutidas (`CertificadosRaiz.h`). Só se o
// Supabase mudar de autoridade: PEM da nova raiz. `SEM_VALIDAR_CERTIFICADO 1` aceita qualquer
// servidor (só em bancada; nunca na versão da competição).
// #define CREDENCIAL_CERTIFICADO_RAIZ "-----BEGIN CERTIFICATE-----\n..."
// #define CREDENCIAL_SEM_VALIDAR_CERTIFICADO 1
