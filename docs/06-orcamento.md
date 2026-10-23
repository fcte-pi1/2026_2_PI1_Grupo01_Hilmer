# Orçamento e Plano de Custos

Este documento apresenta o plano de custos e o orçamento analítico do projeto de desenvolvimento do robô autônomo **Micromouse com Sistema de Telemetria**, desenvolvido no escopo da disciplina de **Projeto Integrador 1 (PI1)** da Faculdade de Ciências e Tecnologias em Engenharia da Universidade de Brasília ([FCTE/UnB](file:///home/efaria/UnB/PI1/docs/01-tap.md)).

O plano abrange a estimativa completa de custos tangíveis e intangíveis necessários para o ciclo de vida do projeto:
1. **Custos de Aquisição de Hardware e Estrutura (BOM — Bill of Materials)**: Componentes físicos embarcados no robô micromouse;
2. **Insumos e Consumíveis de Montagem**: Materiais utilizados durante os procedimentos de fabricação, montagem de PCB e integração;
3. **Equipamentos e Ferramental de Bancada**: Instrumentos de medição, fontes de alimentação e ferramental laboratorial reutilizáveis;
4. **Mão de Obra e Capital Intelectual (Horas de Desenvolvimento e Extensão)**: Valoração econômica do investimento público por aluno despendido pela Universidade de Brasília durante a execução da disciplina (60h formais em sala + 30h de extensão acadêmica).

---

## 1. Lista de Dispositivos e Materiais

A tabela abaixo detalha todos os itens inventariados para o projeto, especificando categorias funcionais, especificações técnicas, modelo recomendado, quantidades e preços de referência de mercado.

### 1.1 Tabela Geral de Componentes e Equipamentos

| ID | Subsistema / Categoria | Componente | Qtd. | Especificação / Requisito | Modelo sugerido | Preço Estimado (R$) | Status | Observações Técnicas |
|---:|---|---|---:|---|---|---:|---|---|
| 1 | Alimentação | Bateria Li-ion 2S | 1 | 7,4 V nominal; 8,4 V máximo | Bateria 18650 7,4 V 2200 mAh 2S Li-ion com BMS | 45,00 | 🛒 Comprar | Fonte primária de energia do robô |
| 2 | Alimentação | Chave de potência | 1 | Liga/desliga geral; corrente nominal adequada | Mini rocker KCD1 — SPST ON/OFF — 2 terminais | 9,40 | 🛒 Comprar | Interrupção segura do barramento de bateria |
| 3 | Alimentação | Buck converter 6 V | 1 | Entrada 7,4–8,4 V; saída ajustável para 6,0 V | LM2596 Step-Down DC-DC 3 A | 21,90 | 🛒 Comprar | Alimentação regulada dos micromotores |
| 4 | Alimentação | Buck converter 5 V | 1 | Entrada compatível com 8,4 V; saída regulada 5,0 V | LM2596 Step-Down DC-DC ajustável | 14,50 | 🛒 Comprar | Alimentação do sensor LiDAR |
| 5 | Alimentação | Buck converter 3,3 V | 1 | Entrada compatível com 8,4 V; saída regulada 3,3 V | MP1584EN — Buck DC-DC ajustável | 17,18 | 🛒 Comprar | Alimentação do microcontrolador ESP32-C3 |
| 6 | Alimentação | Carregador 2S | 1 | Carregador balanceador Li-ion 2S / 8,4 V | Carregador Li-ion 2S — 8,4 V / 1 A | 49,99 | 🛒 Comprar | Recarga externa das células |
| 7 | Processamento / Controle | Placa de Desenvolvimento MCU | 1 | Microcontrolador Wi-Fi/BLE; 3,0–3,6 V | ESP32-C3-DevKitM-1 — 4 MB Flash | 58,13 | 🛒 Comprar | Processamento embarcado e telemetria web |
| 8 | Atuação / Potência | Driver de motores | 1 | Ponte H dupla; DRV8833 | Módulo DRV8833 | 19,00 | 🛒 Comprar | Controle bidirecional PWM de 2 motores DC |
| 9 | Atuação | Motor DC com encoder | 2 | 6 V nominal; encoder incremental em quadratura | GA12-N20-S0310D-E / N20 + caixa GA12 | 39,31 | 🛒 Comprar | Tração e odometria por efeito Hall |
| 10 | Atuação | Roda de alta aderência | 2 | Compatível com o diâmetro do eixo D do motor N20 | Roda emborrachada N20 | — | ⚠️ Validar | Dimensionar raio para velocidade/torque |
| 11 | Atuação | Suporte de motor | 2 | Compatível com motores N20 e fixação ao chassi | Suporte de motor N20 — 2 furos | 12,90 | 🛒 Comprar | Fixação mecânica rígida |
| 12 | Sensoriamento | Sensor de Distância LiDAR | 1 | Comunicação UART 3,3 V; varredura ToF | ST-L50B2 | 64,59 | ✅ Comprado | Mapeamento e percepção de obstáculos |
| 13 | Sensoriamento | Resistor R1 (Ajuste UART) | 1 | Divisor/proteção de níveis lógicos UART | Resistor 1/4 W (valor a definir) | — | ⚠️ Validar | Validar níveis lógicos com osciloscópio |
| 14 | Sensoriamento | Resistor R2 (Ajuste UART) | 1 | Divisor/proteção de níveis lógicos UART | Resistor 1/4 W (valor a definir) | — | ⚠️ Validar | Validar níveis lógicos com osciloscópio |
| 15 | Sensoriamento | Resistores de pull-up | 1 | Resistores de pull-up para canais de encoder | Kit com 10 resistores 10 kΩ | 10,00 | 🛒 Comprar | Estabilização dos sinais lógicos digitais |
| 16 | Filtragem / Desacoplamento | Capacitor cerâmico | 1 | Desacoplamento de alta frequência em CIs | Kit com 10 × 100 nF / 50 V | 9,60 | 🛒 Comprar | Supressão de ruídos de comutação |
| 17 | Filtragem | Capacitor eletrolítico | 1 | Filtragem da linha de alimentação | Kit 10 × 100 µF / 25 V | 12,99 | 🛒 Comprar | Filtragem de barramento DC |
| 18 | Filtragem | Capacitor eletrolítico de alta capacidade | 1 | Suporte a picos transitórios de corrente dos motores | Kit 10 × 470 µF / 16 V | 19,92 | ⚠️ Validar | Validar espaço físico na PCB |
| 19 | Conexões | Conector para LiDAR | 1 | Conector de interface para chicote do sensor | Conexão direta ou terminal Molex/JST | — | ⏳ Depois | Definir modelo pós-layout da placa |
| 20 | Conexões | Barra de Headers 2,54 mm | 1 | Pinos macho e fêmea para conexões modulares | Barra de pinos 2,54 mm 1×40 | 19,00 | 🛒 Comprar | Conexão de módulos e jumpers de teste |
| 21 | Fiação e Conexão | Fios flexíveis para montagem | 1 | Bitola compatível com condução de corrente (AWG 22/24) | Kit 20 m de fio flexível 0,50 mm² | 41,83 | 🛒 Comprar | Distribuição de alimentação e sinais |
| 22 | Isolamento | Tubo termo-retrátil | 1 | Isolamento de emendas e terminais elétricos | Kit de tubos termo-retráteis sortidos | 19,68 | 🛒 Comprar | Segurança contra curto-circuitos |
| 23 | Fixação / Chicote | Abraçadeiras plásticas | 1 | Organização de fiação e amarração de módulos | Abraçadeiras de nylon 2,5 × 100 mm (100 un.) | 8,30 | 🛒 Comprar | Manutenção e fixação mecânica leve |
| 24 | Estrutura Mecânica | Chassi do robô | 1 | Estrutura suporte leve, compacta e resistente | Impressão 3D (PLA/PETG) ou usinagem | 200,00 | 🛒 Comprar/Fabricar | Modelagem própria da equipe de Estrutura |
| 25 | Eletrônica Embarcada | Placa de Circuito Impresso (PCB) | 1 | PCB customizada de 2 camadas para integração | Fabricação própria/laboratório ou prototipagem rápida | — | ⏳ Depois | Fabricar após validação do esquemático |
| 26 | Consumíveis de Solda | Pasta/Fluxo de solda | 1 | Melhoria de contato térmico e molhabilidade | Pasta para solda 110 g | 16,15 | 🛒 Comprar | Soldagem de componentes SMD e PTH |
| 27 | Consumíveis de Solda | Malha dessoldadora | 1 | Remoção de excesso de solda e retrabalho | Malha de cobre 3 mm × 1,5 m | 18,86 | 🛒 Comprar | Correção de soldas na montagem da PCB |
| 28 | Limpeza / Manutenção | Álcool isopropílico | 1 | Limpeza de placas e remoção de fluxo | Álcool isopropílico 99,8% — 1 L | 23,35 | 🛒 Comprar | Limpeza e prevenção de condutividade residual |
| 29 | Ferramentas | Alicate de bico longo | 1 | Conformação de terminais e fixação precisa | Mini alicate de bico longo | 21,90 | ⚠️ Validar | Ferramenta de laboratório |
| 30 | Ferramentas | Alicate de corte rente | 1 | Corte limpo de terminais de componentes e cabos | Alicate de corte diagonal 6" | 22,13 | ⚠️ Validar | Ferramenta de laboratório |
| 31 | Ferramentas | Cabos com garra jacaré | 1 | Conjunto de pontas de prova e cabos de teste | Kit com 10 cabos jacaré | 19,00 | 🛒 Comprar | Conexão rápida de bancada e medições |
| 32 | Ferramentas | Alicate decapador de fios | 1 | Decapagem padronizada sem danificar filamentos | Alicate desencapador/crimpador automático | 21,90 | ⚠️ Validar | Preparação de chicotes |
| 33 | Instrumentação | Fonte de alimentação de bancada | 1 | Fonte DC regulável com limitação de corrente ativa | Yaxun 1502DD — 15 V / 5 A (ou equivalente de bancada) | 199,00 | ⚠️ Validar | Teste seguro e proteção contra sobrecorrente |
| 34 | Prototipagem | Cabos Jumpers flexíveis | 1 | Jumpers M-M, M-F e F-F para ensaios em protoboard | Kit com 120 jumpers | 23,96 | 🛒 Comprar | Validação em bancada pré-PCB |
| 35 | Instrumentação | Multímetro digital | 1 | Medição de tensão, corrente, resistência e continuidade | Multímetro digital True RMS | 59,99 | ⚠️ Validar | Diagnóstico elétrico e verificação de nós |
| 36 | Ferramentas | Kit de pinças antiestáticas | 1 | Manipulação precisa de componentes pequenos | Kit de pinças ESD (ESD-10 a ESD-15) | 19,59 | 🛒 Comprar | Manipulação de componentes sensíveis |
| 37 | Prototipagem | Matriz de contatos (Protoboard) | 1 | Montagem experimental dos circuitos preliminares | Protoboard MB-102 — 830 pontos | 15,70 | 🛒 Comprar | Prototipagem antes da placa final |
| 38 | Segurança de Soldagem | Suporte para ferro de solda | 1 | Base com mola protetora e esponja metálica/vegetal | Suporte de ferro com esponja de limpeza | 22,97 | ⚠️ Validar | Segurança física na bancada de eletrônica |
| 39 | Montagem e Solda | Suporte Terceira Mão | 1 | Suporte com garras articuladas e lupa de bancada | Terceira mão com garras jacaré | 42,79 | ⚠️ Validar | Fixação de placas durante a soldagem |

---

### 1.2 Consolidação dos Custos de Hardware e Materiais por Categoria

| Categoria | Descrição | Valor Estimado (R$) | Percentual sobre Materiais |
|---|---|---:|---:|
| **Componentes Embarcados (BOM Robô)** | Sensores (LiDAR, Encoders), Motores, ESP32, Drivers, Bateria, Conversores Buck, Componentes Passivos e Chassi | R$ 502,76 | 42,44% |
| **Consumíveis e Insumos de Montagem** | Fios flexíveis, tubos termo-retráteis, abraçadeiras, fluxo de solda, malha dessoldadora e álcool isopropílico | R$ 128,19 | 10,82% |
| **Equipamentos e Ferramental de Bancada** | Fonte de bancada, multímetro digital, alicates, pinças ESD, jumpers, protoboard e suportes de bancada | R$ 553,61 | 46,74% |
| **Total de Materiais e Equipamentos** | **Soma consolidada dos itens inventariados com cotação informada** | **R$ 1.184,56** | **100,00%** |

> **Nota Técnica sobre Ferramental:** Os itens de ferramental e instrumentação (R$ 553,61) constituem bens não-consumíveis de infraestrutura. Podem ser disponibilizados pelos laboratórios da FCTE/UnB ou ferramentas e utilitários que membros do grupo já dispõem para o uso no projeto, mas que também serão precificados para uma ideia e mensuração completa dos custos envolvidos no projeto como um todo.

---

## 2. Estimativa de Custos de Mão de Obra e Capital Intelectual

Em projetos acadêmicos e de engenharia aplicada desenvolvidos em universidades públicas, os custos tangíveis de componentes eletrônicos representam apenas uma fração do investimento global. O principal aporte financeiro e econômico reside no **capital intelectual**, consubstanciado no custeio das horas de pesquisa, concepção, projeto de engenharia, programação, experimentação e extensão realizadas pelos alunos da instituição.

### 2.1 Metodologia e Fundamentação das Fontes de Dados

Para precificar a hora de trabalho dos estudantes de forma fundamentada e auditável, utilizam-se dados orçamentários oficiais e levantamentos de fontes de referência:

1. **Decanato de Planejamento, Orçamento e Avaliação Institucional (DPO/UnB)**:
   - Segundo o *Anuário Estatístico da UnB 2024 (ano-base 2023)* e o *Relatório de Gestão Integrado da UnB*, o orçamento total executado da Fundação Universidade de Brasília (FUB) superou **R$ 1,94 bilhão** (PLOA 2023: R$ 1.941.652.930,00), mantendo uma comunidade acadêmica de aproximadamente **39.770 estudantes regulares de graduação** (2º semestre de 2023) e cerca de 48.000 alunos considerando a pós-graduação *stricto sensu*.
   - Em conformidade com as diretrizes do Tribunal de Contas da União (TCU) estabelecidas na **Decisão Plenária nº 408/2002** e no **Acórdão nº 1.043/2006**, as Instituições Federais de Ensino Superior (IFES) acompanham o indicador de **Custo Corrente por Aluno Equivalente**, que apura o volume de recursos públicos aplicados por estudante na formação universitária.

2. **Levantamento Nacional do Ministério da Educação (MEC / Poder360)**:
   - Levantamento analítico realizado pelo veículo de jornalismo econômico *Poder360* com base nos dados do Ministério da Educação (MEC) e no Sistema Integrado de Administração Financeira (SIAFI) apontou que o **custo médio anual por estudante nas universidades federais brasileiras é de R$ 52.533,00**.
   - Esse montante consolida o investimento público com corpo docente, técnicos administrativos, infraestrutura predial, laboratórios e custeio das atividades de ensino, pesquisa e extensão.

---

### 2.2 Memória de Cálculo do Valor da Hora-Aluno

Para transpor o custo anual por estudante para a taxa horária de mão de obra (**Homem-Hora Acadêmica**) aplicável ao projeto, adota-se a seguinte memória de cálculo:

1. **Custo Semestral por Aluno**:
   $$\text{Custo Semestral} = \frac{\text{Custo Anual Médio}}{2} = \frac{\text{R\$}~52.533,00}{2} = \text{R\$}~26.266,50$$

2. **Carga Horária Semestral de Referência**:
   - Um semestre acadêmico regular na Universidade de Brasília possui **16 semanas letivas**.
   - A dedicação acadêmica esperada para um estudante de graduação em Engenharia (considerando uma média de 24 créditos presenciais somados às horas complementares de estudo extraclasse, preparação e tarefas laboratoriais) totaliza aproximadamente **35 horas semanais**, resultando em:
   $$\text{Horas Semestrais de Dedicação} = 16\,\text{semanas} \times 35\,\text{horas/semana} = 560\,\text{horas}$$

3. **Taxa Horária de Investimento Público (Valor da Hora-Aluno)**:
   $$\text{Valor da Hora-Aluno (VHA)} = \frac{\text{R\$}~26.266,50}{560\,\text{horas}} \approx \mathbf{R\$~46,90\,/\,\text{hora}}$$

---

### 2.3 Carga Horária da Disciplina e Esforço da Equipe

A disciplina de **Projeto Integrador 1 (PI1)** da FCTE/UnB possui carga horária oficial total de **90 horas** por estudante ao longo do semestre letivo, distribuída em dois componentes estruturais:

* **60 Horas Formais (Plano de Aulas)**: Aulas teóricas, alinhamentos semanais com o professor orientador, apresentações de pontos de controle (P1 e P2), dinâmicas laboratoriais e orientações metodológicas em sala de aula;
* **30 Horas de Extensão (Atividades Extraclasse)**: Aplicação prática da engenharia na comunidade, trabalho de bancada fora de sala, reuniões internas dos subsistemas, fabricação do protótipo, montagem física do labirinto de testes, validação experimental e eventos de demonstração tecnológica.

Conforme registrado no [Termo de Abertura do Projeto (01-tap.md)](file:///home/efaria/UnB/PI1/docs/01-tap.md), a equipe multidisciplinar é composta por **18 alunos** distribuídos entre os cursos de Engenharia de Software, Engenharia Eletrônica, Engenharia Aeroespacial e Engenharia de Energia.

$$\text{Carga Horária Total da Equipe} = 18\,\text{integrantes} \times 90\,\text{horas} = \mathbf{1.620\,\text{horas-aluno}}$$

* **Horas Formais Presenciais (60h × 18)**: $1.080\,\text{horas}$
* **Horas de Extensão e Desenvolvimento Prático (30h × 18)**: $540\,\text{horas}$

---

### 2.4 Tabela de Custos de Mão de Obra por Área de Atuação

A valoração da dedicação horária de cada área técnica do projeto é sintetizada na tabela a seguir:

| Subsistema / Especialidade | Integrantes Alocados | Horas Formais (60h) | Horas Extensão (30h) | Carga Total (h) | Taxa Horária (R$/h) | Custo Total de Mão de Obra (R$) |
|---|:---:|:---:|:---:|:---:|:---:|---:|
| **Gerência Geral do Projeto** | 1 | 60 | 30 | 90 | 46,90 | R$ 4.221,00 |
| **Engenharia de Software e Telemetria** | 6 | 360 | 180 | 540 | 46,90 | R$ 25.326,00 |
| **Engenharia Eletrônica e Sensoriamento** | 4 | 240 | 120 | 360 | 46,90 | R$ 16.884,00 |
| **Engenharia de Energia e Alimentação** | 3 | 180 | 90 | 270 | 46,90 | R$ 12.663,00 |
| **Engenharia Aeroespacial / Mecânica e Estrutura** | 4 | 240 | 120 | 360 | 46,90 | R$ 16.884,00 |
| **Total Geral de Mão de Obra** | **18** | **1.080** | **540** | **1.620** | **46,90** | **R$ 75.978,00** |

#### Distribuição dos Custos de Recursos Humanos por Natureza Acadêmica:
* **Mão de Obra Formal em Sala de Aula (1.080 horas)**: $\text{R\$}~50.652,00$ ($66,67\%$)
* **Mão de Obra de Extensão Universitária (540 horas)**: $\text{R\$}~25.326,00$ ($33,33\%$)

---

## 3. Orçamento Geral Consolidado

A composição global dos custos associados ao desenvolvimento do projeto Micromouse Autônomo com Telemetria encontra-se consolidada abaixo:

| Natureza da Despesa | Descrição da Rubrica | Valor (R$) | Percentual (%) |
|---|---|---:|---:|
| **Hardware Embarcado** | Componentes eletroeletrônicos e peças estruturais do robô (BOM) | R$ 502,76 | 0,65% |
| **Consumíveis de Montagem** | Fios, solda, termo-retráteis e produtos químicos de limpeza | R$ 128,19 | 0,17% |
| **Ferramental e Instrumentação** | Equipamentos de medição, proteção e ferramentas de bancada | R$ 553,61 | 0,72% |
| **Mão de Obra — Ensino Formal** | 1.080 horas dedicadas em sala de aula e orientação técnica | R$ 50.652,00 | 65,64% |
| **Mão de Obra — Extensão** | 540 horas dedicadas a projeto prático, testes e extensão | R$ 25.326,00 | 32,82% |
| **Total Geral do Projeto** | **Investimento total estimado (Materiais + Infraestrutura + Recursos Humanos)** | **R$ 77.162,56** | **100,00%** |

---

## 4. Referências Bibliográficas

> [1] UNIVERSIDADE DE BRASÍLIA (UnB). **Anuário Estatístico 2024: ano-base 2023**. Brasília, DF: Decanato de Planejamento, Orçamento e Avaliação Institucional (DPO/UnB), nov. 2024. Disponível em: <https://anuario2024.netlify.app/>. Acesso em: 11 out. 2026.
> 
> [2] UNIVERSIDADE DE BRASÍLIA (UnB). **Relatório de Gestão Integrado da Universidade de Brasília**. Brasília, DF: DPO/DAF, 2023. Disponível em: <https://dpo.unb.br/>. Acesso em: 11 out. 2026.
> 
> [3] PODER360. **Universidades federais gastam R$ 52.533 por aluno ao ano**. Brasília, DF: Poder360 / Ministério da Educação (MEC) / SIAFI, 2023/2024. Disponível em: <https://www.poder360.com.br/>. Acesso em: 11 out. 2026.
> 
> [4] TRIBUNAL DE CONTAS DA UNIÃO (TCU). **Decisão Plenária nº 408/2002 e Acórdão nº 1.043/2006**: Indicadores de Gestão e Custo Corrente por Aluno Equivalente nas Instituições Federais de Ensino Superior (IFES). Brasília: TCU, 2002/2006.
> 
> [5] PROJECT MANAGEMENT INSTITUTE (PMI). **Um Guia do Conhecimento em Gerenciamento de Projetos (Guia PMBOK)**. 7. ed. Newtown Square, PA: Project Management Institute, 2021.
> 
> [6] FACULDADE DE CIÊNCIAS E TECNOLOGIAS EM ENGENHARIA (FCTE/UnB). **Plano de Ensino e Critérios de Extensão da Disciplina de Projeto Integrador 1 (PI1)**. Semestre 2026/2, Prof. Dr. Hilmer Rodrigues Neri.
> 
> [7] CONCUR / SAP. **Plano de gerenciamento de custos: como os CFOs devem atuar?**. Disponível em: <https://www.concur.com.br/blog/article/plano-de-gerenciamento-de-custos>. Acesso em: 18 abr. 2026.

---

## 5. Histórico de Versão

| Versão | Data | Descrição | Autor(es) | Revisor(es) |
|:---:|:---:|---|---|---|
| 1.1 | 11/10/2026 | Reestruturação completa do plano de custos: categorização de hardware/ferramentas, memória de cálculo da hora-aluno com fontes oficiais (DPO/UnB e MEC/Poder360), detalhamento das 90h da equipe (60h formais + 30h extensão) e consolidação do orçamento executivo | [Elias Oliveira](https://github.com/EliasOliver21) |  |