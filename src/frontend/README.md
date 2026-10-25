# _Frontend_ - Dashboard Web do Micromouse

Esta pasta armazena o dashboard web do micromouse (HU-14): a tela de **telemetria ao vivo** e a tela de **histórico de consultas**, conforme o [protótipo navegável no Figma](https://www.figma.com/proto/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=7-121&p=f&scaling=min-zoom&content-scaling=fixed&page-id=0%3A1&starting-point-node-id=7%3A121).

Tecnologias: **React 18** + **Vite** + **TypeScript**, **Recharts** para gráficos, **supabase-js** para ler o banco e CSS simples (sem framework de estilos).

> [!NOTE]
> O dashboard web suporta tanto **dados reais via Supabase** (leitura de `labirintos`, acompanhamento em tempo real de `corridas`, `telemetria` e `mapa_celulas` via Supabase Realtime, HUs 06, 09, 10, 14 e 15) quanto o modo de **dados de exemplo** com simulação navegável quando o Supabase não estiver configurado.

---

## Como rodar

Requer **Node.js 18 ou mais recente** (com `npm`).

```bash
cd src/frontend
npm install
cp .env.example .env.local   # depois preencha com os dados do Supabase
npm run dev                  # abre em http://localhost:5173
```

| Comando | O que faz |
|---|---|
| `npm run dev` | Servidor de desenvolvimento com recarga automática, em `http://localhost:5173`. |
| `npm test` | Executa todos os testes unitários e de integração com Vitest. |
| `npm run test:cobertura` | Executa os testes e gera o relatório de cobertura de código. |
| `npm run build` | Confere os tipos (`tsc --noEmit`) e gera a versão final em `dist/`. |
| `npm run preview` | Serve a pasta `dist/` para conferir o build. |

Sem o `.env.local` o dashboard funciona do mesmo jeito, só com os dados de exemplo; o rodapé avisa "Supabase não configurado".

---

## Variáveis de ambiente

Copie `.env.example` para `.env.local` e preencha (valores em *Project Settings > API* no Supabase):

* `VITE_SUPABASE_URL`: URL base do projeto.
* `VITE_SUPABASE_ANON_KEY`: chave pública `anon`.

> [!WARNING]
> **O `.env.local` nunca vai para o Git** (está no `.gitignore`). Tudo o que começa com `VITE_` é embutido no código que roda no navegador, então aqui entra **só a chave `anon`**, que pelas políticas de RLS apenas lê. A chave `service_role` e o token do dispositivo nunca devem aparecer no frontend.

Com tudo certo, o rodapé mostra "Supabase conectado: labirintos lidos do banco".

---

## Estrutura de pastas

```text
src/frontend/
├── .env.example               # Modelo das variáveis de ambiente
├── index.html                 # Página base do Vite
├── package.json               # Dependências e scripts
├── tsconfig.json              # Configuração do TypeScript
├── vite.config.ts             # Configuração do Vite
├── public/
│   └── favicon.svg
└── src/
    ├── main.tsx               # Ponto de entrada
    ├── App.tsx                # Cabeçalho, abas e rodapé; guarda aba, labirinto e tentativa
    ├── tipos.ts               # Tipos das tabelas do Supabase (mesmos nomes de colunas)
    ├── index.css              # Cores e estilos base
    ├── componentes/           # Cabecalho, GradeLabirinto, CartaoMetrica, BadgeDesafio,
    │                          # GraficoVelocidade, FiltroLabirinto, TabelaHistorico (+ .css)
    ├── paginas/               # TelaAoVivo (Frames 01–03) e TelaHistorico (Frame 04)
    ├── hooks/                 # useLabirintos, useCorridas, useTelemetriaAoVivo e useReproducaoCorrida
    ├── lib/                   # supabase.ts (cliente), formatacao.ts, labirinto.ts, painelAoVivo.ts
    ├── dados/                 # Dados de exemplo: labirintos, corrida e histórico
    └── test/                  # setup.ts e cliente Supabase simulado reutilizável (supabaseSimulado.ts)
```

---

## Telas

### Telemetria ao Vivo (Frames 01, 02 e 03)

* **Mapa** (`GradeLabirinto`, HU-06 / #239): malha dimensional escura com as proporções do labirinto em execução (4×4, 8×4 ou 12×4), largada verde no canto inferior esquerdo (0, 0), objetivo vermelho no canto oposto (`linhas - 1, colunas - 1`), paredes conhecidas desenhadas a partir de `mapa_celulas` ou simulação, traçado laranja `Robot Path` e marcador `Robot` atualizados em tempo real conforme as leituras de telemetria chegam, mantendo o traçado completo visível após a conclusão da corrida. A célula (0, 0) é a largada; Norte aumenta a linha e Leste aumenta a coluna, em conformidade com o firmware.
* **Cards**: bateria (% estimado pela tensão, tensão e corrente), velocidade média e pico, cronômetro com o limite de 10:00 e a tentativa (1 de 3).
* **Badge** no cabeçalho: `DESAFIO CUMPRIDO: NÃO`, `NÃO (EM ANDAMENTO)` ou `SIM`.
* **Botão**: `INICIAR CORRIDA` → `CORRIDA EM ANDAMENTO` → `RESETAR / NOVA TENTATIVA` (no modo exemplo) ou avisos de estado sincronizados com o robô (no modo ao vivo).
* **Gráfico de exemplo** (Recharts): velocidade × tempo da tentativa.

Ao clicar em `INICIAR CORRIDA`, a corrida de exemplo é reproduzida 4× mais rápido que o tempo real, passando pelos três estados. Os labirintos de exemplo são os mesmos de `src/firmware/test/test_labirinto_simulado/LabirintosExemplo.h`, e o trajeto é gerado respeitando as paredes. Com o Supabase configurado, a tela acompanha a corrida real do robô via canais Realtime de `telemetria`, `mapa_celulas` e `corridas`.

### Histórico de Consultas (Frame 04 e filtros M1–M3)

Tabela integrada ao banco de dados via Supabase (`supabase-js`), consultando as corridas reais com Data/Hora, Labirinto, Tentativa (com a nota: 1ª = 10, 2ª = 7,5, 3ª = 5), Tempo final, Velocidade média, Bateria e Status, ordenadas da corrida mais recente para a mais antiga (`data_hora_inicio` decrescente). Suporta filtro por labirinto (`Todos os Labirintos` / `Labirinto 1 (4×4)` / `Labirinto 2 (8×4)` / `Labirinto 3 (12×4)`), atualização sob demanda e tratamento de estado vazio.

---

## Testes

O frontend conta com testes unitários e de integração utilizando **Vitest** e **Testing Library** (com ambiente `jsdom`).

### Como rodar os testes

```bash
cd src/frontend
npm test                 # Roda todos os testes uma vez
npm run test:watch       # Modo contínuo (recarrega ao salvar arquivos)
npm run test:cobertura   # Gera relatório de cobertura com V8
```

### O que é testado

* **Grade do Labirinto (`src/componentes/GradeLabirinto.test.tsx`)**: testes unitários da malha dimensional (4×4, 8×4 e 12×4), renderização de partida em verde e chegada em vermelho, posicionamento e rotação angular do marcador `Robot`, vetor do traçado `Robot Path` atualizado em tempo real, renderização de paredes mapeadas e preservação do percurso completo ao término da corrida (HU-06 / #239).
* **Painel e Telemetria ao Vivo (`src/lib/painelAoVivo.test.ts` e `src/hooks/useTelemetriaAoVivo.test.ts`)**: testes das funções puras de atualização de estado e da integração com canais Supabase Realtime para `telemetria`, `corridas` e `mapa_celulas`, recepção de atualizações de paredes em tempo real e mapa final (#231, #239).
* **Formatação (`src/lib/formatacao.test.ts`)**: testes unitários das regras de exibição e conversão (tempo em `mm:ss.mmm` e `m:ss`, velocidade, percentual linear de bateria LiPo 2S, notas de 10 a 5 por tentativa e formatação de datas).
* **Labirinto (`src/lib/labirinto.test.ts`)**: testes unitários do algoritmo de parsing de labirintos em ASCII, verificação de paredes cardeais, percurso com retorno de becos sem saída e mapa de células exploradas.
* **Hook de reprodução (`src/hooks/useReproducaoCorrida.test.ts`)**: testes unitários do cronômetro acelerado (4×), avanço e filtragem temporal de telemetria, transição para o estado concluído ao atingir o objetivo e corte pelo limite de 10:00 (RNF-03).
* **Integração da Tela ao Vivo (`src/paginas/TelaAoVivo.test.tsx`)**: testes de integração da tela ao vivo com o cliente Supabase simulado, verificando o carregamento da corrida, atualização dos cards de telemetria, traçado do percurso e mapa em tempo real (#231, #239).
* **Integração do Histórico (`src/paginas/TelaHistorico.test.tsx`)**: teste de integração da tela de histórico montada com o cliente Supabase simulado, verificando o carregamento assíncrono das corridas do banco, aplicação dos filtros por tipo de labirinto, botão de atualização sob demanda e tratamento de erro de conexão.
* **Cliente Supabase simulado (`src/test/supabaseSimulado.ts`)**: implementação reutilizável em memória para os testes do frontend, compatível com consultas encadeadas (`.from().select().order().eq()`), inserção de registros e canais Realtime (`.channel().on('postgres_changes', ...).subscribe()`).

### Integração contínua (CI)

O workflow [`.github/workflows/ci-frontend.yml`](../../.github/workflows/ci-frontend.yml) executa o check **"Testes do frontend"** em todos os Pull Requests e pushes para a branch `main`. O job realiza a instalação limpa (`npm ci`), executa todos os testes com cobertura (`npm run test:cobertura`) e confere os tipos e compilação de produção (`npm run build`).

---

## Pontos em aberto

* **Unidade de `corridas.consumo_bateria`**: a migração diz "mAh ou % consumida". O histórico assume **% consumida** e mostra a carga restante (`100 − consumo`). Precisa ser fechado na HU-09.
* **Percentual da bateria**: estimado de forma linear entre 6,6 V (corte) e 8,4 V (carga plena) da LiPo 2S. A curva real não é linear; o cálculo final é da HU-08.
