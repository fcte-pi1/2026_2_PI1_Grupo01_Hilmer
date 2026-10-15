# Lista de Compras — Micromouse PI1

## Lista de componentes

| ID | Categoria | Componente | Qtd. | Especificação / Requisito | Modelo sugerido | Preço (R$) | Status | Observações |
|---:|---|---|---:|---|---|---:|---|---|
| 1 | Alimentação | Bateria Li-ion 2S | 1 | 7,4 V nominal; 8,4 V máximo | Bateria 18650 7,4 V 2200 mAh 2S Li-ion com BMS | 45,00 | 🛒 Comprar | — |
| 3 | Alimentação | Chave de potência | 1 | Liga/desliga; corrente adequada ao sistema | Mini rocker KCD1 — SPST ON/OFF — 2 terminais | 9,40 | 🛒 Comprar | — |
| 4 | Alimentação | Buck converter 6 V | 1 | Entrada 7,4–8,4 V; saída ajustável para 6,0 V | LM2596 Step-Down DC-DC 3 A | 21,90 | 🛒 Comprar | Alimentação dos motores |
| 4 | Alimentação | Buck converter 5 V | 1 | Entrada compatível com 8,4 V; saída 5 V | LM2596 Step-Down DC-DC ajustável | 14,50 | 🛒 Comprar | Alimentação do LiDAR |
| 5 | Alimentação | Buck converter 3,3 V | 1 | Entrada compatível com 8,4 V; saída 3,3 V | MP1584EN — Buck DC-DC ajustável | 17,18 | 🛒 Comprar | Alimentação do ESP32 |
| 6 | Alimentação | Carregador 2S | 1 | Carregador para Li-ion 2S / 8,4 V | Carregador Li-ion 2S — 8,4 V / 1 A | 49,99 | 🛒 Comprar | — |
| 7 | Controle | ESP32-C3-MINI-1 / placa de desenvolvimento | 1 | MCU; alimentação 3,0–3,6 V | ESP32-C3-DevKitM-1 — 4 MB Flash | 58,13 | 🛒 Comprar | Projeto atual usa ESP32-C3-MINI-1 |
| 8 | Controle | Driver de motores | 1 | DRV8833; ponte H dupla | DRV8833 | 19,00 | 🛒 Comprar | Dois motores DC |
| 9 | Atuação | Motor DC com encoder | 2 | 6 V nominal; encoder incremental em quadratura | GA12-N20-S0310D-E / N20 + caixa GA12 | 39,31 | 🛒 Comprar | Corrente e compatibilidade com 8,4 V ainda precisam ser validadas |
| 10 | Atuação | Roda | 2 | Compatível com eixo do motor | — | — | ⚠️ Validar | — |
| 11 | Atuação | Suporte de motor | 2 | Compatível com motores e chassi | Suporte de motor N20 — 2 furos | 12,90 | 🛒 Comprar | — |
| 12 | Sensoriamento | LiDAR | 1 | Alimentação e UART conforme datasheet | ST-L50B2 | 64,59 | ✅ Comprado | Validar alimentação, protocolo e níveis UART |
| 13 | Sensoriamento | Resistor R1 | 1 | Valor a definir conforme interface UART | — | — | ⚠️ Validar | Não comprar valor definitivo antes da validação |
| 14 | Sensoriamento | Resistor R2 | 1 | Valor a definir conforme interface UART | — | — | ⚠️ Validar | Não comprar valor definitivo antes da validação |
| 15 | Sensoriamento | Resistores de pull-up | 1 | Valores a definir conforme encoders | Kit com 10 resistores | 10,00 | 🛒 Comprar | Validar níveis lógicos e necessidade |
| 16 | Filtragem | Capacitor cerâmico | 1 | 100 nF; desacoplamento | Kit com 10 × 100 nF / 50 V | 9,60 | 🛒 Comprar | — |
| 17 | Filtragem | Capacitor eletrolítico | 1 | Valor a dimensionar; filtragem de alimentação | Capacitor 100 µF — kit com 10 | 12,99 | 🛒 Comprar | — |
| 18 | Filtragem | Capacitor eletrolítico de maior capacidade | 1 | Ex.: 470 µF; valor final a dimensionar | Capacitor 470 µF — kit com 10 | 19,92 | ⚠️ Validar | — |
| 21 | Conectores | Conector para LiDAR | 1 | Conforme conector do módulo | Direto à PCB | — | ⏳ Depois | — |
| 22 | Conectores | Headers 2,54 mm | 1 | Macho/fêmea conforme montagem | Barra de pinos 2,54 mm 1×40 | 19,00 | 🛒 Comprar | — |
| 23 | Montagem | Fios para PCB | 1 | Bitola adequada às correntes do projeto | Kit 20 m de fio flexível 0,50 mm | 41,83 | 🛒 Comprar | — |
| 24 | Montagem | Termo-retrátil | 1 | Diversos diâmetros | Kit de tubo termo-retrátil | 19,68 | 🛒 Comprar | — |
| 25 | Montagem | Abraçadeiras pequenas | 1 | Fixação e organização dos cabos | Abraçadeiras de nylon 2,5 × 100 mm | 8,30 | 🛒 Comprar | — |
| 26 | Estrutura | Chassi | 1 | Estrutura do Micromouse | — | 200,00 | 🛒 Comprar/Fabricar | — |
| 27 | PCB | PCB do projeto | 1 | Após validação do esquemático | — | — | ⏳ Depois | Fabricar somente após fechar o esquemático |
| 28 | Consumíveis | Fluxo de solda | 1 | Fluxo para soldagem eletrônica | Pasta para solda 110 g | 16,15 | 🛒 Comprar | Melhorar molhabilidade das soldas |
| 29 | Consumíveis | Malha dessoldadora | 1 | 2–3 mm | Malha de cobre 3 mm × 1,5 m | 18,86 | 🛒 Comprar | Retrabalho e correção de soldas |
| 30 | Consumíveis | Álcool isopropílico | 1 | Preferencialmente ≥ 90% | Álcool isopropílico 99,8% — 1 L | 23,35 | 🛒 Comprar | Limpeza de PCB e resíduos de fluxo |
| 31 | Ferramentas | Alicate de bico | 1 | Pequeno | Mini alicate de bico longo | 21,90 | ⚠️ Validar | Montagem e dobra de terminais |
| 32 | Ferramentas | Alicate de corte | 1 | Pequeno, para corte de fios e terminais | Alicate de corte 6" | 22,13 | ⚠️ Validar | Montagem |
| 33 | Ferramentas | Cabos de teste/jacaré | 1 | Kit de pontas/cabos para multímetro e fonte | Kit com 10 fios garra jacaré | 19,00 | 🛒 Comprar | Medições e testes |
| 34 | Ferramentas | Decapador de fios | 1 | Compatível com bitolas usadas no projeto | Alicate desencapador/crimpador 5 em 1 | 21,90 | ⚠️ Validar | Preparação dos fios |
| 35 | Ferramentas | Fonte de bancada | 1 | Fonte DC ajustável com limitação de corrente | Yaxun 1502DD — 15 V / 5 A | 199,00 | ⚠️ Validar | Testes iniciais com corrente limitada |
| 36 | Ferramentas | Kit de jumpers | 1 | Macho-macho, macho-fêmea e fêmea-fêmea | Kit 120 unidades | 23,96 | 🛒 Comprar | Testes em protoboard |
| 37 | Ferramentas | Multímetro | 1 | Tensão, continuidade, resistência e corrente | Multímetro digital | 59,99 | ⚠️ Validar | Teste elétrico e verificação antes de energizar |
| 38 | Ferramentas | Pinça | 1 | Pinça para eletrônica | Kit de pinças ESD | 19,59 | 🛒 Comprar | Manipulação de componentes |
| 39 | Ferramentas | Protoboard | 1 | Protoboard pequena para testes preliminares | Protoboard MB-102 — 830 pontos | 15,70 | 🛒 Comprar | Protótipo antes da PCB |
| 40 | Ferramentas | Suporte para ferro de solda | 1 | Com suporte e limpeza de ponta | Suporte com mola e esponja | 22,97 | ⚠️ Validar | Segurança na bancada |
| 41 | Ferramentas | Terceira mão | 1 | Suporte para soldagem | Lupa com garras e suporte | 42,79 | ⚠️ Validar | Fixação durante soldagem |
| 42 | Mão de obra | Montagem e soldagem da eletrônica | 1 | Montagem dos componentes, soldagem, cabeamento e organização | Serviço de montagem eletrônica | — | ⏳ Falta estimar horas | Valor da hora definido (R$ 59,03, ver metodologia abaixo); falta a equipe estimar quantas horas a tarefa leva para fechar o custo total |
---

## Metodologia de custo de mão de obra

O custo da mão de obra considera o valor da hora de um estudante da UnB, em vez de um
salário de mercado. O valor é obtido a partir do orçamento público investido por
estudante, proporcional aos créditos da disciplina e às horas de dedicação previstas no
TAP.

| Variável | Valor | Fonte |
|---|---:|---|
| Dotação atualizada da UnB em 2026 | R$ 2.700.943.579,00 | Painel Gestão UnB |
| Total oficial de estudantes regulares | 50.843 | Anuário Estatístico 2025 da UnB (tabela 2.18), DPO/UnB |
| Créditos anuais de referência | 40 créditos | — |
| Créditos da disciplina | 4 créditos | — |
| Horas de dedicação por aluno no projeto | 90 h | [TAP do projeto](01-tap.md), seção 4.3 |

```
Orçamento por estudante/ano   = R$ 2.700.943.579,00 ÷ 50.843            = R$ 53.123,21
Orçamento da disciplina/aluno = R$ 53.123,21 ÷ 40 créditos × 4 créditos = R$ 5.312,32
Valor da hora do estudante    = R$ 5.312,32 ÷ 90 h                      = R$ 59,03/hora
```

**Valor da hora do estudante da UnB: R$ 59,03.**

> **Nota sobre as fontes:** os valores de dotação orçamentária e de total de estudantes
> não foram reconferidos por nós diretamente nas fontes primárias. Antes de uma entrega
> final, vale a pena confirmar esses dois números direto no Painel Gestão UnB e no
> Anuário Estatístico vigente.

Para fechar o custo de qualquer item de mão de obra do orçamento (como o item 42, acima),
basta multiplicar as horas estimadas da tarefa por R$ 59,03.

# Resumo

| Informação | Quantidade |
|---|---:|
| Total de itens | 39 |
| Itens para comprar | — |
| Itens para validar | — |
| Itens já comprados | 1 |
| Itens para comprar/fabricar | 1 |
| Itens para fazer depois | 2 |
| Custo estimado dos itens com preço informado | **R$ 1.184,56** |
