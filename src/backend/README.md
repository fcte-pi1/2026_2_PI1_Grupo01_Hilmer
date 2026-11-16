# _Backend_ - Persistência de Dados e API (Supabase)

Esta pasta armazena os esquemas de banco de dados, migrações SQL, políticas de segurança e configurações do backend do projeto Micromouse.

A arquitetura do backend utiliza o **Supabase (PostgreSQL gerenciado)**, aproveitando os recursos nativos de:
- **PostgREST**: Exposição automática de APIs REST a partir das tabelas.
- **Supabase Realtime**: Notificações e propagação de eventos via WebSockets para atualização do dashboard em tempo real.
- **Row Level Security (RLS)**: Controle granular de acesso para isolar a escrita do firmware e a leitura do frontend.

---

## Estrutura de Pastas

```text
src/backend/
├── .env.example                                  # Modelo de variáveis de ambiente do Supabase
├── README.md                                     # Este documento de instruções
├── package.json                                  # Só para os testes (npm test e o CI)
├── tests/
│   └── banco.test.ts                             # Migrações, funções SQL e RLS num PostgreSQL (PGlite)
└── supabase/
    ├── functions/
    │   ├── _shared/autenticacao.ts               # Confere o x-device-token
    │   ├── _shared/                              # Token do dispositivo e validações comuns
    │   ├── corridas/                             # Edge Function: criar e encerrar corridas e gravar o mapa final
    │   └── telemetria/                           # Edge Function: gravar os lotes de telemetria do robô
    └── migrations/
        ├── 20261014180000_esquema_inicial.sql    # DDL das 4 tabelas, restrições e seed de labirintos
        ├── 20261014180500_seguranca_rls.sql      # Políticas RLS e ativação do Realtime
        ├── 20261018232250_remote_schema.sql      # criar_corrida e encerrar_corrida (só service_role)
        ├── 20261023100000_telemetria_mapa_e_consolidacao.sql  # Telemetria em lote, mapa final e consolidação (#229)
        └── 20261024120000_realtime_corridas.sql  # Publica `corridas` no Realtime (#231)
```
---

## Variáveis de Ambiente

Para conectar o frontend ou o firmware ao Supabase, copie o arquivo de exemplo:

```bash
cp .env.example .env
```

Preencha as seguintes variáveis com as credenciais do seu projeto:

* `SUPABASE_URL`: URL base do projeto (encontrada em *Project Settings > API* no dashboard do Supabase).
* `SUPABASE_ANON_KEY`: Chave pública `anon` com acesso controlado por RLS.

> [!WARNING]
> **Nunca versione arquivos `.env` com chaves reais.** O `.env` já deve constar no `.gitignore`.

---

## Como Aplicar as Migrações no Supabase

As migrações seguem o formato de versionamento cronológico aceito pelo Supabase CLI (`AAAAMMDDHHMMSS_nome.sql`):

### 1. Criar o Projeto no Supabase
1. Acesse [supabase.com](https://supabase.com) e faça login.
2. Crie um novo projeto no plano gratuito (*Free Tier*).
3. Em *Project Settings > Members*, convide os membros da equipe de software.

### 2. Executar os Scripts SQL
No menu lateral esquerdo do dashboard do Supabase:
1. Abra o **SQL Editor**.
2. Crie uma nova query e cole o conteúdo de [`20261014180000_esquema_inicial.sql`](supabase/migrations/20261014180000_esquema_inicial.sql).
3. Clique em **Run** para criar as tabelas (`labirintos`, `corridas`, `telemetria`, `mapa_celulas`) e inserir o seed dos 3 labirintos padrão (4x4, 8x4 e 12x4).
4. Em seguida, cole o conteúdo de [`20261014180500_seguranca_rls.sql`](supabase/migrations/20261014180500_seguranca_rls.sql).
5. Clique em **Run** para habilitar o RLS, criar as políticas de acesso e ativar o Realtime na tabela `telemetria`.

## Arquitetura de Segurança e Ingestão

O backend adota uma estratégia de **segurança em camadas**:

1. **Leitura Pública Controlada (`anon`):**
   - O dashboard web conecta-se usando a `SUPABASE_ANON_KEY` para listar os labirintos disponíveis, consultar o histórico de corridas e acompanhar a telemetria e o mapa gerado em tempo real via Supabase Realtime.
   - As políticas de Row Level Security (RLS) concedem **exclusivamente permissão de `SELECT`** para as roles `anon` e `authenticated`.

2. **Escrita Restrita e Mediada (Edge Functions):**
   - **Bloqueio Total de Escrita Direta:** Nenhuma chave pública (`anon`) pode executar `INSERT`, `UPDATE` ou `DELETE` diretamente nas tabelas (`corridas`, `telemetria`, `mapa_celulas`). Isso impede que usuários forjem leituras ou adulterem dados no banco inspecionando a chave do frontend.
   - **Ingestão Protegida:** As gravações do robô (ESP32) passam por uma **Edge Function** do Supabase. A função autentica a requisição através de um cabeçalho de token de dispositivo secreto (`x-device-token`), valida a consistência do payload e efetua as operações de banco com a chave interna de serviço (`service_role`), que ignora o RLS.

---

## Edge Function `corridas` (criar e encerrar corridas, gravar o mapa)

A criação e o encerramento de corridas passam por uma Edge Function (`supabase/functions/corridas`), que usa a chave de serviço (`service_role`). A regra de negócio fica em duas funções SQL (`criar_corrida` e `encerrar_corrida`, na migração `20261018232250_remote_schema.sql`, gerada pelo `supabase db pull` e completada com o `REVOKE` e o `GRANT`), executáveis só pela chave de serviço: `anon` e `authenticated` não têm permissão de `EXECUTE` nelas nem de escrita nas tabelas. Os testes do banco (`tests/banco.test.ts`) conferem essa restrição.

| Requisição | Corpo | Resposta |
|---|---|---|
| `POST /functions/v1/corridas` | `{"labirinto": "4x4"}` (ou `"8x4"`, `"12x4"`); o labirinto é obrigatório | `201` com a corrida em andamento, com `numero_tentativa` contado por labirinto |
| `POST /functions/v1/corridas/{id}/mapa` | `{"celulas": [...]}`, o `MapaJson` do firmware (os campos `dimensao`, `linhas` e `colunas` são ignorados); cada célula traz `linha`, `coluna` e as quatro paredes (`true`, `false` ou `null`) | `200` com `{"novas": n, "atualizadas": m}`. Reenviar atualiza as células, sem duplicar |
| `POST /functions/v1/corridas/{id}/encerrar` | **Tudo opcional:** `"status"` (`"concluida"` ou `"nao_concluida"`), `tempo_final`, `velocidade_media`, `velocidade_pico` e `consumo_bateria`. O que faltar o banco calcula pela telemetria já gravada (`concluida` se houve o evento `objetivo_alcancado`, senão `nao_concluida`) | `200` com a corrida encerrada e `data_hora_fim` preenchida |

Regras:
- **Tentativa por labirinto:** a primeira corrida do 4x4 é a tentativa 1, e a primeira do 8x4 também. Um bloqueio no banco serializa criações simultâneas do mesmo labirinto, e `UNIQUE (labirinto_id, numero_tentativa)` é a segunda barreira.
- **Encerramento idempotente:** repetir o pedido com o mesmo status (por exemplo, depois de uma queda de rede) devolve a corrida como está. Com outro status, responde `409`. Sem `tempo_final`, o banco usa o tempo desde `data_hora_inicio`.
- **Encerramento sem status:** repetir o pedido sem status em uma corrida já encerrada devolve a corrida como está. O robô pode, então, encerrar com `{}` e deixar o banco decidir.
- **Mapa:** o banco recusa o mapa inteiro se uma célula for inválida (fora do labirinto da corrida) e responde `413` se houver mais células do que o labirinto tem.
- **Erros:** `400` (corpo, status, id ou células inválidos), `401` (token), `404` (labirinto ou corrida inexistente), `405` (método), `409` (já encerrada com outro status), `413` (mapa com células demais) e `500` (falha do banco, sem expor a mensagem).

**Autenticação.** O gateway do Supabase exige um JWT (`Authorization: Bearer <SUPABASE_ANON_KEY>`), e a função exige também o cabeçalho `x-device-token`, comparado com o segredo `DEVICE_TOKEN`. Sem o segredo configurado, ninguém é autorizado. A chave `service_role` fica só no servidor.

```bash
# Publicação (uma vez por projeto Supabase: teste e produção)
supabase link --project-ref <ID-DO-PROJETO>
supabase db push
supabase secrets set DEVICE_TOKEN="$(openssl rand -hex 24)"
supabase functions deploy corridas
supabase functions deploy telemetria

# Exemplo: criar uma corrida no labirinto 8x4 e encerrá-la
curl -s -X POST "${SUPABASE_URL}/functions/v1/corridas" \
  -H "Authorization: Bearer ${SUPABASE_ANON_KEY}" -H "x-device-token: ${DEVICE_TOKEN}" \
  -H "Content-Type: application/json" -d '{"labirinto": "8x4"}'

curl -s -X POST "${SUPABASE_URL}/functions/v1/corridas/1/encerrar" \
  -H "Authorization: Bearer ${SUPABASE_ANON_KEY}" -H "x-device-token: ${DEVICE_TOKEN}" \
  -H "Content-Type: application/json" -d '{"status": "concluida", "tempo_final": 61.5}'
```

**Testes.** Em `src/backend`, `npm install` e `npm test` (Node 22.18 ou superior). A suíte tem testes da validação, da autenticação e do roteamento (com um banco falso) e testes das funções SQL e do RLS num PostgreSQL de verdade (PGlite, em WebAssembly), com os papéis `anon`, `authenticated` e `service_role` imitando o Supabase. Os testes da Edge Function dentro do Supabase (com `supabase functions serve`, que precisa do Docker) e a chamada real com `curl` continuam sendo feitos no projeto de teste.


## Edge Function `telemetria` (lotes de amostras do robô)

O robô envia a telemetria em lotes para `supabase/functions/telemetria`, que chama a função SQL `registrar_telemetria` com a chave de serviço. É o contrato que o firmware usa (`ContratoTelemetria`, #238).

| Requisição | Corpo | Resposta |
|---|---|---|
| `POST /functions/v1/telemetria` | `{"corrida_id": 7, "amostras": [...]}`: de 1 a 100 amostras, com os nomes das colunas de `telemetria` (exemplo em `tests/fixtures/lote-telemetria.json`) | `200` com `{"gravadas": n, "existentes": m, "reconsolidada": bool}` |

Regras:
- **Idempotente:** uma amostra com o mesmo `numero_sequencia` na mesma corrida é ignorada e conta em `existentes`. Reenviar um lote cuja resposta se perdeu responde `200` sem duplicar.
- **Atômico:** se uma amostra do lote for inválida, nenhuma é gravada.
- **Corrida encerrada:** continua aceitando amostras (o robô pode reenviar depois de a rede voltar), e as métricas da corrida são refeitas (`reconsolidada`).
- **Erros e o que o firmware faz com cada um:** `400` (corpo ou amostra inválidos), `404` (corrida inexistente) e `413` (lote com mais de 100 amostras) descartam o lote; `401` e `403` pausam o envio; `405` (método) e `5xx` repetem ou pausam conforme a política de reenvio.

A autenticação, os segredos e a publicação são os da função `corridas`: o mesmo `DEVICE_TOKEN`. Para publicar as duas: `supabase functions deploy corridas` e `supabase functions deploy telemetria`.

```bash
curl -s -X POST "${SUPABASE_URL}/functions/v1/telemetria" \
  -H "Authorization: Bearer ${SUPABASE_ANON_KEY}" -H "x-device-token: ${DEVICE_TOKEN}" \
  -H "Content-Type: application/json" \
  -d '{"corrida_id": 1, "amostras": [{"numero_sequencia": 1, "posicao_linha": 0, "posicao_coluna": 0, "orientacao": "N"}]}'

curl -s -X POST "${SUPABASE_URL}/functions/v1/corridas/1/mapa" \
  -H "Authorization: Bearer ${SUPABASE_ANON_KEY}" -H "x-device-token: ${DEVICE_TOKEN}" \
  -H "Content-Type: application/json" -d @tests/fixtures/mapa-final-4x4.json
```

## CI (testes automáticos do backend)

O workflow `.github/workflows/ci-backend.yml` roda o check **Testes do backend** em todo PR para a `main` e em todo push nela, sem filtro de caminhos (como o CI do firmware). Ele é independente do `ci-firmware.yml`: um não espera o outro.

O que ele faz, em ordem:

1. `npm ci` em `src/backend`, com Node 22 e cache do npm.
2. `npm run test:cobertura`: os mesmos testes do `npm test` (validação, autenticação e roteamento da Edge Function, e as migrações, as funções SQL e o RLS num PostgreSQL em WebAssembly, o PGlite), com a medição de cobertura. O CI exige no mínimo **80% das linhas** (`--test-coverage-lines=80`, Node 22.8 ou superior) e mostra a tabela de cobertura no resumo do workflow, que serve de evidência para o registro de testes. Só entram na medição os arquivos carregados pelos testes (`autenticacao.ts`, `logica.ts` e `roteador.ts`); o `index.ts` é coberto pelo passo seguinte.
3. `deno check --node-modules-dir=none supabase/functions/corridas/index.ts supabase/functions/telemetria/index.ts` (Deno 2.9.8, versão fixa no workflow, como o PlatformIO e o gcovr no CI do firmware): checagem de tipos do `index.ts`, o único arquivo da Edge Function que os testes do Node não executam (ele usa `Deno.serve`). A opção `--node-modules-dir=none` é necessária porque o `package.json` desta pasta faria o Deno procurar o `@supabase/supabase-js` em `node_modules`, e no Supabase o pacote vem do cache do Deno.

**Como ler o resultado.** Na página do PR, abra a aba *Checks* (ou clique em *Details* ao lado de "Testes do backend"). O resumo do workflow mostra as contagens (`tests`, `pass`, `fail`), a tabela de cobertura e, se algum teste falhar, o nome dele e a mensagem. Qualquer falha, nos testes ou na checagem de tipos, reprova o PR.

**Antes de abrir o PR**, rode o mesmo que o CI:

```bash
cd src/backend
npm ci                  # Node 22.18 ou superior
npm test                # rápido, sem cobertura
npm run test:cobertura  # com cobertura e o mínimo de 80% das linhas, como no CI
deno check --node-modules-dir=none supabase/functions/corridas/index.ts supabase/functions/telemetria/index.ts   # opcional localmente; precisa do Deno 2
```

Mexeu em uma migração ou em uma função SQL? O `npm test` aplica as migrações em um banco novo, então ele avisa se algo quebrou. A publicação automática das migrações e das funções no Supabase (`supabase db push` e `functions deploy`) não faz parte do CI.

---

## Validação Prática das Políticas RLS e Idempotência

### 1. Teste de Permissões via REST (Chave `anon`)

Você pode validar que a chave pública só tem permissão de leitura executando no terminal:

```bash
# 1. Leitura com chave anon (DEVE FUNCIONAR - HTTP 200)
curl -s -X GET "${SUPABASE_URL}/rest/v1/labirintos?select=*" \
  -H "apikey: ${SUPABASE_ANON_KEY}" \
  -H "Authorization: Bearer ${SUPABASE_ANON_KEY}"

# 2. Tentativa de inserção direta com chave anon (DEVE SER BLOQUEADA PELO RLS)
curl -s -X POST "${SUPABASE_URL}/rest/v1/telemetria" \
  -H "apikey: ${SUPABASE_ANON_KEY}" \
  -H "Authorization: Bearer ${SUPABASE_ANON_KEY}" \
  -H "Content-Type: application/json" \
  -d '{"corrida_id": 1, "numero_sequencia": 1, "posicao_linha": 0, "posicao_coluna": 0, "orientacao": "N"}'
```

O comando de inserção retornará um erro de permissão ou resposta vazia devido ao RLS, demonstrando que escritas diretas estão bloqueadas.

---

### 2. Testando o Upsert Idempotente no Banco

Para validar a restrição de unicidade `UNIQUE (corrida_id, numero_sequencia)` e o comportamento de *upsert* com `ON CONFLICT DO NOTHING`, execute no SQL Editor do Supabase:

```sql
-- 1. Cria uma corrida de teste no labirinto 1 (4x4)
INSERT INTO corridas (id, labirinto_id, numero_tentativa, status)
VALUES (999, 1, 999, 'em_andamento')
ON CONFLICT (id) DO NOTHING;

-- 2. Primeira inserção do pacote de telemetria número 1
INSERT INTO telemetria (corrida_id, numero_sequencia, posicao_linha, posicao_coluna, orientacao)
VALUES (999, 1, 0, 0, 'N')
ON CONFLICT (corrida_id, numero_sequencia) DO NOTHING;

-- 3. Segunda inserção IDÊNTICA (simula retransmissão por instabilidade de Wi-Fi)
INSERT INTO telemetria (corrida_id, numero_sequencia, posicao_linha, posicao_coluna, orientacao)
VALUES (999, 1, 0, 0, 'N')
ON CONFLICT (corrida_id, numero_sequencia) DO NOTHING;

-- 4. Verificação: deve existir apenas 1 registro para a sequência 1
SELECT count(*) FROM telemetria WHERE corrida_id = 999 AND numero_sequencia = 1;

-- 5. Limpeza do teste
DELETE FROM corridas WHERE id = 999;
```

---

## Rastreabilidade
- **Issue**: #228 - Configurar o Supabase e criar o esquema do banco (DER)
- **História de Usuário Pai**: HU-09 - Persistência de Dados e Corridas
- **Requisitos Associados**: RF-8, RF-9, RF-14, RF-15, RNF-5