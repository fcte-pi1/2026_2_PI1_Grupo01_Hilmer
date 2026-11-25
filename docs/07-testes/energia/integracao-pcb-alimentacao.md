# Integração de Energia na PCB, Segurança Elétrica e Alimentação do Sistema

Este documento consolida os ensaios e verificações executados entre **17/11/2026 e 19/11/2026** no robô montado com a placa de circuito impresso (PCB) física, cobrindo de forma integrada as issues **[#212](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/212)**, **[#537](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/537)** e **[#103](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/103)**.

| Item | Registro |
| :--- | :--- |
| **Issues Cobertas** | `#212` (Barramentos na PCB), `#537` (Polaridade, proteções e checklist), `#103` (Alimentação → Sistema) |
| **Período de Execução** | 17/11/2026 a 19/11/2026 |
| **Responsáveis / Equipe** | Elias Oliveira, Cláudio Henrique, Bruno Duarte, Aline, Ana Carolina e Paulo Melo |
| **Hardware Avaliado** | PCB física com 3× Bucks `MP1584EN`, `ESP32 DevKit V1` em soquete removível, `MPU6050`, `DRV8833`, `XT60`, conectores de 6 pinos e motores N20 |
| **Tensões Reguladas** | **$3{,}3\text{ V}$** (lógica/sensores) · **$5{,}1\text{ V}$** (`ESP32 VIN`) · **$6{,}6\text{ V}$** (`DRV8833 VM`) |

---

## 1. Soldagem e Integração Física dos Barramentos na PCB (Issue #212)

### 1.1. Escopo Executado e Adaptações na Montagem Física

Para viabilizar a manutenção rápida em bancada e acomodar os componentes reais disponíveis no robô montado, a integração na PCB seguiu a arquitetura abaixo:

1. **Soquete removível para a placa microcontroladora (`ESP32 DevKit V1`):**
   * Foram soldados **dois barramentos de conectores fêmea** na PCB para deixar a `ESP32 DevKit V1` removível, permitindo inspeções a frio nos pinos, medições de tensão antes do encaixe e trocas rápidas sem dessoldagem.
2. **Chicote de força com conector polarizado `XT60`:**
   * A entrada da bateria LiPo 2S foi montada com um **chicote `XT60`**, assegurando acoplamento mecânico firme para corrente de potência e proteção física contra inversão de polaridade.
3. **Componentes soldados e conectores modulares de sensores:**
   * Os **três conversores Step-Down Buck (`MP1584EN`)**, a **IMU (`MPU6050`)** e o **driver Ponte H dupla (`DRV8833`)** foram soldados diretamente na PCB.
   * Os **sensores periféricos** foram interligados através de **conectores fêmea de 6 pinos**, permitindo acoplar e remover os chicotes com facilidade durante manutenções.
4. **Desvios documentados em relação ao escopo original da issue (#212):**
   * **Padronização sem `LM2596`:** toda a regulação foi feita com os três módulos `MP1584EN` (`3,3 V`, `5,1 V` e `6,6 V`).
   * **Ausência de `BMS` externo e fusível (`PPTC`):** nesta montagem física não foram instalados BMS externo nem fusível PPTC na entrada (mitigação detalhada na [Seção 2](#2-verificacao-de-polaridade-protecoes-e-checklist-de-energizacao-issue-537)).
   * **Sensor `INA226` desconectado:** o módulo `INA226` foi mantido desconectado após problemas identificados durante os testes de integração, sem comprometer a operação dos demais barramentos.

### 1.2. Inspeção Visual e Teste de Continuidade entre Barramentos

Antes de encaixar a `ESP32 DevKit V1` nos dois barramentos fêmea e antes de energizar o sistema com a bateria, foram realizados os ensaios a frio com multímetro:

| Verificação Executada | Pontos Aferidos | Critério de Aceitação | Resultado Obtido |
| :--- | :--- | :--- | :--- |
| **Inspeção visual das soldas** | `XT60`, chave geral, pinos dos `MP1584EN`, `DRV8833`, `MPU6050`, soquetes da ESP32 e conectores de 6 pinos | Ausência de solda fria, pontes de estanho ou fios soltos | **Aprovado** |
| **Isolamento entre barramentos positivos e terra** | `VBAT` $\times$ `GND`; `+6V6` $\times$ `GND`; `+5V1` $\times$ `GND`; `+3V3` $\times$ `GND` | Circuito aberto (ausência de curto-circuito) | **Aprovado (sem curtos)** |
| **Continuidade do retorno comum (`GND`)** | Negativo do `XT60` $\times$ `GND` dos Bucks, `DRV8833`, conectores e soquete da ESP32 | Continuidade plena ($\approx 0\ \Omega$) em todos os nós | **Aprovado** |
| **Isolamento dos pinos de sinal (soquete ESP32)** | Pinos de GPIO dos dois barramentos fêmea $\times$ `VBAT` e `VM` | Nenhuma trilha lógica em curto com linhas de potência | **Aprovado** |

> **Evidência fotográfica (#212):** adicionar as fotos da PCB integrada (soquetes, `XT60` e conectores de 6 pinos) em `docs/figs/` e referenciá-las aqui utilizando `![PCB integrada com alimentação](../../figs/<nome-da-foto>.jpg)`.

---

## 2. Verificação de Polaridade, Proteções e Checklist de Energização (Issue #537)

### 2.1. Conferências no Robô Montado

Com a PCB integrada ao chassi e os chicotes conectados, foram verificados os critérios de segurança elétrica:

| Critério da Issue #537 | Verificação Realizada | Resultado | Observações |
| :--- | :--- | :---: | :--- |
| **Polaridade e travamento do conector** | Conferência da polaridade (`+` vermelho / `-` preto) no chicote `XT60` da bateria e da PCB; teste de acoplamento | **Aprovado** | O corpo chanfrado do `XT60` impede fisicamente o encaixe invertido |
| **Continuidade e isolamento da chave geral e caminho de potência** | Medição no robô montado com chave em `ON` (fechada) e `OFF` (aberta) | **Aprovado** | Continuidade plena em `ON` e circuito aberto em `OFF`, sem fuga entre barramentos |
| **Inspeção das soldas de alimentação** | Revisão visual ponto a ponto das entradas de bateria, saídas dos Bucks e conectores | **Aprovado** | Nenhum fio invertido ou em posição incorreta |
| **Proteções (`PPTC` e `BMS`)** | Conferência da presença física na montagem atual | **Não instalado nesta revisão** | Sem `PPTC` e sem `BMS` nesta PCB; a proteção contra polaridade reversa é mecânica (`XT60`) e a prevenção contra subtensão/falhas segue o **Checklist de Energização** |

### 2.2. Checklist de Energização Pré-Corrida

Checklist operacional padronizado para todas as baterias de testes e corridas do robô montado:

1. **[ ] Chave Geral em `OFF`:** confirmar que a chave geral está desligada antes de aproximar o conector da bateria.
2. **[ ] Estado da Bateria LiPo 2S:** inspecionar visualmente (sem inchaço) e confirmar tensão segura acima de $7{,}0\text{ V}$ antes da sessão (nunca operar abaixo de $6{,}60\text{ V}$).
3. **[ ] Inspeção de Encaixes e Isolamento:** conferir que a `ESP32 DevKit V1` está assentada corretamente nos dois barramentos fêmea, que o `INA226` permanece desconectado/isolado e que os chicotes nos conectores de 6 pinos estão firmes.
4. **[ ] Alinhamento do `XT60`:** verificar a correspondência de polos antes de acoplar o `XT60` sem forçar.
5. **[ ] Conferência de Tensão Pós-Manutenção:** após qualquer soldagem ou ajuste nos Bucks, medir as saídas (**$3{,}3\text{ V}$**, **$5{,}1\text{ V}$** e **$6{,}6\text{ V}$**) **sem a ESP32 no soquete** antes de encaixá-la.
6. **[ ] Ligação Segura e Desconexão Pós-Uso:** ligar a chave geral (`ON`) com o robô apoiado na pista (ou com rodas suspensas na bancada) e **desplugar o `XT60` ao encerrar os testes**, evitando descarga lenta da LiPo 2S na ausência de BMS.

---

## 3. Alimentação Integrada do Sistema Completo (Issue #103)

### 3.1. Condição do Ensaio Integrado

Com a bateria LiPo 2S alimentando todo o robô montado simultaneamente:

* **Microcontrolador sem cabo USB:** a `ESP32 DevKit V1` operou encaixada nos barramentos fêmea da PCB, alimentada exclusivamente pelo pino `VIN` através do Buck de `5,1 V`.
* **Carga completa conectada:** `IMU MPU6050`, `DRV8833`, sensores nos conectores de 6 pinos e ambos os micromotores DC N20 em funcionamento conjunto.

### 3.2. Tensões Ajustadas e Comportamento Observado

Os três reguladores `MP1584EN` foram calibrados com pequena folga positiva nas linhas de `5 V` e de potência (`VM`), visando compensar eventuais quedas sob carga e entregar maior torque/velocidade aos micromotores N20 — medida alinhada aos achados do ensaio isolado de motores ([Procedimento 3.3](../hardware.md)), que havia identificado zona morta até `PWM 140` quando alimentados com apenas `5 V`.

| Regulador (`MP1584EN`) | Subsistema Alimentado | Tensão Ajustada / Aferida | Comportamento sob Operação Integrada | Avaliação |
| :--- | :--- | :---: | :--- | :---: |
| **Buck Lógica (`U5`)** | Sensores (`3,3 V`) e barramento lógico | **$3{,}3\text{ V}$** | Pequenas variações dentro do esperado; sensores e comunicação estáveis | **Aprovado** |
| **Buck MCU (`U2`)** | `ESP32 DevKit V1` via pino `VIN` (sem USB) | **$5{,}1\text{ V}$** | Pequenas variações dentro do esperado; sem reinícios da placa | **Aprovado** |
| **Buck Motores (`U4` — `VM`)** | Ponte H `DRV8833` e motores N20 | **$6{,}6\text{ V}$** | Folga intencional acima de $6{,}0\text{ V}$ para maior potência nos motores; pequenas variações dentro do esperado com os motores girando | **Aprovado** |


### 3.3. Evidências em Vídeo (Substituir pelos Links Após Upload)

* **Vídeo 1 — Teste de Integração na PCB / Robô Completo:** [Link para o Vídeo 1 do teste de integração (substituir pela URL após upload)](#)
* **Vídeo 2 — Validação de Alimentação e Funcionamento Integrado:** [Link para o Vídeo 2 do teste de integração (substituir pela URL após upload)](#)