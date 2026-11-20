#pragma once

/// @file credenciais_padrao.h
/// @brief Valores padrão do Wi-Fi e da URL do Supabase, versionados, para quem compila não precisar
/// de nenhum arquivo: com eles o robô e a bancada conectam no Wi-Fi do grupo e aceitam gravação pelo
/// Wi-Fi (OTA) logo de cara.
///
/// Não há mais segredo para o robô enviar: o `DEVICE_TOKEN` e a chave do Supabase são **opcionais**.
/// As Edge Functions publicadas com `--no-verify-jwt` e sem o segredo `DEVICE_TOKEN` aceitam o robô só
/// com a URL. Se o seu projeto ainda exige a chave `anon`/publishable ou o token, ponha-os em
/// `include/credenciais.h` (ignorado pelo Git; modelo em `credenciais.exemplo.h`): eles **nunca** vão
/// para o Git, e a chave `sb_secret_` não pode ser usada aqui de jeito nenhum.
///
/// Um `credenciais.h` local vale mais que este arquivo: cada `#define` abaixo só vale se o
/// `credenciais.h` não o definiu. Para trocar a rede só no seu computador, defina-a lá.

#ifndef CREDENCIAL_WIFI_SSID
#define CREDENCIAL_WIFI_SSID "Teste_Esp"
#define CREDENCIAL_WIFI_SENHA "87654321"
#endif

#ifndef CREDENCIAL_WIFI_SSID_RESERVA
#define CREDENCIAL_WIFI_SSID_RESERVA "iPhone"
#define CREDENCIAL_WIFI_SENHA_RESERVA "claudio123"
#endif

#ifndef CREDENCIAL_SUPABASE_URL
#define CREDENCIAL_SUPABASE_URL "https://xjcduvoedyqqczgethgn.supabase.co"
#endif

/// Opcionais e sem valor no repositório: vazios até o `credenciais.h` local defini-los.
#ifndef CREDENCIAL_SUPABASE_ANON
#define CREDENCIAL_SUPABASE_ANON ""
#endif
#ifndef CREDENCIAL_DEVICE_TOKEN
#define CREDENCIAL_DEVICE_TOKEN ""
#endif
