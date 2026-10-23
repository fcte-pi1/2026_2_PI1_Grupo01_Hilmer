# Procedimento Operacional Padrão (POP) — Recarga Balanceada da Bateria LiPo 2S

Este documento estabelece o **Procedimento Operacional Padrão (POP)** e o protocolo de segurança homologado pela equipe para a inspeção, conexão, recarga balanceada, monitoramento térmico, desconexão, armazenamento e contingência de emergências do conjunto de baterias **LiPo 2S (7,4 V nominal / 8,4 V máximo)** do Micromouse, atendendo integralmente aos critérios de aceitação da **Issue #210** (*Atividade: Procedimento de Recarga — #144*).

---

## 1. Objetivo e Escopo de Aplicação

Baterias de Polímero de Lítio (**LiPo**) de alta taxa de descarga oferecem excelente densidade de potência e baixíssima resistência interna, porém possuem química sensível a **sobretensão ($> 4{,}20\text{ V/célula}$)**, **descarga profunda ($< 3{,}00\text{ V/célula}$)**, **desbalanceamento entre células** e **estresse térmico**. Como o pack adotado no projeto (**Tattu 650 mAh 2S1P 75C**) não possui placa de BMS interna no encapsulamento, toda a proteção contra sobretensão e equalização entre as células $C_1$ e $C_2$ é executada externamente pelo **carregador balanceador inteligente** durante o processo de recarga.

> **Regra de Ouro (Obrigatoriedade):** Nenhuma bateria LiPo 2S do projeto poderá ser recarregada sem a conexão simultânea do cabo de balanceamento (**JST-XHR-3P**), fora de uma bolsa antichama (**LiPo Safe Bag**) ou sem a presença física de um integrante da equipe na bancada durante todo o ciclo de carga.

---

## 2. Especificações Técnicas do Conjunto Bateria e Carregador

A tabela abaixo consolida os parâmetros nominais do pack homologado em [4.2 Energia](energia.md) e a parametrização obrigatória no carregador de bancada:

| Parâmetro Técnico | Especificação Homologada no Projeto | Observação Operacional |
| :--- | :--- | :--- |
| **Modelo da Bateria** | **Tattu 650mAh 2S1P 75C 7.4V LiPo Battery Pack** | Química LiPo (*Lithium Polymer*) de alta descarga |
| **Configuração de Células** | **2S1P** (2 células em série: $C_1$ e $C_2$) | Sem circuito BMS interno |
| **Tensão Nominal / Plena Carga** | **$7{,}40\text{ V}$** ($3{,}70\text{ V/cél}$) / **$8{,}40\text{ V}$** ($4{,}20\text{ V/cél}$) | Teto absoluto de $4{,}20\text{ V}$ por célula |
| **Tensão de Corte Operacional (UVLO)** | **$6{,}60\text{ V}$** ($3{,}30\text{ V/cél}$) | Monitorado via ADC (`GPIO 2`) no robô |
| **Tensão de Armazenamento (*Storage*)** | **$7{,}60\text{ V a }7{,}70\text{ V}$** ($3{,}80\text{ V a }3{,}85\text{ V/cél}$) | Obrigatório quando inativa por $> 48\text{ h}$ |
| **Conector Principal de Potência** | **AMASS XT-30** (Amarelo, polarizado) | Fio vermelho (`+`) chanfrado / Fio preto (`-`) |
| **Conector de Balanceamento** | **JST-XHR-3P** (Branco, 3 vias, passo $2{,}54\text{ mm}$) | Pino 1: `GND` / Pino 2: Nó central $3{,}7\text{ V}$ / Pino 3: `+7,4 V` |
| **Carregador Homologado** | **B6 V3 Smart Charger (80W)** (ou dedicado $8{,}4\text{ V} / 1\text{ A}$) | Modo obrigatório: **`LiPo BALANCE`** |
| **Corrente de Recarga Padrão (1C)** | **$0{,}6\text{ A}$** (exato: $0{,}65\text{ A}$ para $650\text{ mAh}$) | **Atenção:** Embora a fonte/carregador suporte $\ge 1{,}0\text{ A}$, para células de $650\text{ mAh}$ deve-se parametrizar **$0{,}6\text{ A}$** ($1\text{C}$), evitando aquecimento e degradação precoce |
| **Tempo Estimado de Recarga (20% → 100%)** | **$50\text{ a }65\text{ minutos}$** (Perfil CC/CV) | Estágio CC até $8{,}40\text{ V}$ + estágio CV até $I < 0{,}06\text{ A}$ |

---

## 3. Limites Térmicos e Elétricos de Segurança

Durante qualquer operação de recarga ou manuseio, o operador deve monitorar os seguintes limites de controle:

| Variável Monitorada | Faixa Normal (Verde) | Faixa de Alerta (Amarelo) | Limite Crítico / Aborto Imediato (Vermelho) |
| :--- | :---: | :---: | :---: |
| **Tensão Individual por Célula ($V_{C1}, V_{C2}$)** | $3{,}30\text{ V a }4{,}20\text{ V}$ | $3{,}00\text{ V a }3{,}29\text{ V}$ | **$< 3{,}00\text{ V}$** (Subtensão severa) ou **$> 4{,}22\text{ V}$** (Sobrecarga) |
| **Diferença entre Células ($\Delta V = |V_{C1} - V_{C2}|$)** | $\le 0{,}02\text{ V}$ ($20\text{ mV}$) | $0{,}03\text{ V a }0{,}09\text{ V}$ | **$\ge 0{,}10\text{ V}$ ($100\text{ mV}$)** — Célula desbalanceada/danificada |
| **Temperatura Superficial da Bateria ($T_{\text{bat}}$)** | $20\text{ °C a }35\text{ °C}$ (fria/morna) | $36\text{ °C a }44\text{ °C}$ | **$\ge 45\text{ °C}$** (Quente ao toque) — **Interromper carga!** |
| **Temperatura Ambiente de Bancada ($T_{\text{amb}}$)** | $15\text{ °C a }30\text{ °C}$ | $31\text{ °C a }35\text{ °C}$ | **$< 10\text{ °C}$ ou $> 40\text{ °C}$** — Proibido iniciar recarga |
| **Integridade Física do Encapsulamento (*Pouch*)** | Firme, plano e sem odor | Leve amassado externo sem furo | **Estufamento (*puffing*), odor adocicado ou vazamento** |

---

## 4. Guia Passo a Passo: Conexão, Monitoramento e Desconexão Segura

A Figura 1 ilustra a topologia de conexão dupla obrigatória no carregador inteligente **B6 V3** e a ordem correta de acoplamento dos cabos.

<figure markdown>

![Diagrama Operacional de Conexão Dupla para Recarga Balanceada LiPo 2S](../figs/diagrama-pop-recarga-b6v3.svg){ width="900" }

<figcaption>

**Figura 1.** Esquema operacional de conexão dupla para recarga balanceada da bateria Tattu LiPo 2S 650 mAh no carregador B6 V3 Smart Charger, destacando a ordem de conexão e o uso da bolsa antichama (*LiPo Safe Bag*).

</figcaption>

</figure>

### Fluxo Resumido do Procedimento

```mermaid
flowchart LR
    A["1. Inspeção Visual e<br/>Medição Prévia (≥ 3,0V/cél)"] --> B["2. Ligar Fonte DC e<br/>Bananas no Carregador"]
    B --> C["3. Acoplar XT-30 +<br/>JST-XHR-3P (Porta 2S)"]
    C --> D["4. Fechar LiPo Safe Bag e<br/>Iniciar LiPo BALANCE 0.6A"]
    D --> E["5. Monitorar Tensão/Temp<br/>a cada 15 min (CC/CV)"]
    E --> F["6. Alarme FULL (8,40V):<br/>Desconectar na Ordem Inversa"]
```

---

### 4.1 Fase A — Inspeção Pré-Recarga (Antes de Conectar)

1. **Resfriamento Pós-Corrida:** Se a bateria acabou de ser utilizada no Micromouse, aguarde no mínimo **15 a 20 minutos** até que ela retorne à temperatura ambiente antes de iniciar a recarga.
2. **Inspeção Visual e Tátil:** Verifique se o invólucro termo-retrátil não apresenta cortes, perfurações, deformações nas bordas ou **estufamento (*puffing*)**. Verifique se os fios do conector **XT-30** e do cabo **JST-XHR-3P** estão íntegros.
3. **Aferição Preliminar de Tensão:** Com um multímetro digital ou *Cell Checker*, meça a tensão total e individual das células:
   * Se $V_{\text{total}} \ge 6{,}60\text{ V}$ ($V_{\text{cell}} \ge 3{,}30\text{ V}$): **Apta para recarga normal.**
   * Se $6{,}00\text{ V} \le V_{\text{total}} < 6{,}60\text{ V}$ ($3{,}00\text{ V} \le V_{\text{cell}} < 3{,}30\text{ V}$): Recarregar inicialmente em corrente reduzida de **$0{,}2\text{ A}$** sob supervisão contínua até atingir $6{,}80\text{ V}$.
   * Se qualquer célula apresentar **$V_{\text{cell}} < 3{,}00\text{ V}$**: **PROIBIDO RECARREGAR.** Acione imediatamente a Contingência 5.3.

---

### 4.2 Fase B — Sequência Obrigatória de Conexão (Prevenção de Curto-Circuito)

> **PERIGO DE CURTO-CIRCUITO:** **NUNCA** conecte o plugue XT-30 na bateria enquanto os pinos banana de $4\text{ mm}$ na outra ponta do cabo estiverem soltos! Se os pinos banana encostarem um no outro com a bateria plugada, ocorrerá um curto-circuito direto superior a **48 A**, derretendo os cabos instantaneamente.

Siga estritamente esta ordem numerada:

1. **Passo 1 — Energizar o Carregador:** Conecte a fonte de alimentação DC ($11\text{ V a }18\text{ V}$, mín. $1\text{ A}$) à entrada `DC IN` do carregador B6 V3 e aguarde a inicialização do visor LCD.
2. **Passo 2 — Conectar os Pinos Banana no Carregador (Sem a Bateria):** Com o chicote de carga **desconectado da bateria**, insira o pino banana **Vermelho (`+`)** no borne `OUTPUT +` e o pino banana **Preto (`-`)** no borne `OUTPUT -` do carregador.
3. **Passo 3 — Acoplar o Conector de Potência (`XT-30`):** Conecte o plugue amarelo **XT-30** da bateria ao conector XT-30 do chicote de carga, observando o chanfro mecânico de polaridade.
4. **Passo 4 — Acoplar o Conector de Balanceamento (`JST-XHR-3P`):** Insira o conector branco de 3 vias **JST-XHR-3P** da bateria na porta lateral dedicada **`2S` (3 pinos)** do carregador B6 V3, alinhando as guias plásticas sem forçar.
5. **Passo 5 — Acondicionamento em Bolsa Antichama:** Posicione a bateria no interior da **LiPo Safe Bag** sobre superfície incombustível (bancada limpa, longe de papel, solventes ou álcool isopropílico) e feche a aba de velcro da bolsa.

---

### 4.3 Fase C — Parametrização e Partida no Carregador

1. Pressione `BATT TYPE / STOP` até selecionar o programa **`PROGRAM SELECT: LiPo BATT`** e confirme em `START / ENTER`.
2. Utilize as teclas `INC (+)` / `DEC (-)` até exibir a tela **`LiPo BALANCE`** *(nunca utilize o modo `LiPo CHARGE` sem balanceamento)*.
3. Pressione `START / ENTER`: o valor da corrente começará a piscar. Ajuste com `DEC (-)` / `INC (+)` para **`0.6A`** (taxa segura de $\approx 1\text{C}$ para $650\text{ mAh}$).
4. Pressione `START / ENTER`: o valor da tensão começará a piscar. Ajuste para **`7.4V (2S)`**.
5. Mantenha o botão **`START / ENTER` pressionado por 3 segundos**:
   * O carregador executará a checagem automática de polaridade e contagem de células, exibindo **`R: 2SER S: 2SER CONFIRM (ENTER)`**.
   * Verifique se ambos os números indicam `2SER` e pressione `START / ENTER` uma última vez para iniciar a carga.

---

### 4.4 Fase D — Monitoramento Ativo durante o Ciclo CC/CV

O processo de recarga balanceada divide-se automaticamente em dois estágios:
* **Estágio 1 — Corrente Constante (CC):** O carregador injeta **$0{,}60\text{ A}$ constantes** enquanto a tensão sobe gradualmente de $\approx 7{,}4\text{ V}$ até atingir **$8{,}40\text{ V}$**.
* **Estágio 2 — Tensão Constante e Equalização (CV):** Ao atingir $8{,}40\text{ V}$, o carregador trava a tensão máxima em $4{,}20\text{ V/célula}$ e reduz progressivamente a corrente ($0{,}5\text{ A} \to 0{,}2\text{ A} \to 0{,}06\text{ A}$) enquanto drena pequenos pulsos da célula mais alta pela porta `JST-XHR-3P` até que ambas estejam perfeitamente equalizadas em **$4{,}20\text{ V} \pm 0{,}01\text{ V}$**.

**Ações obrigatórias do operador a cada 15 minutos:**
1. Pressione a tecla **`INC (+)`** durante a carga para abrir a tela de telemetria individual de células e verifique se $V_{C1}$ e $V_{C2}$ estão convergindo ($\Delta V \le 0{,}03\text{ V}$).
2. Abra brevemente a *LiPo Safe Bag* e toque na superfície da bateria com o dorso dos dedos: ela deve estar em **temperatura ambiente ou levemente morna ($< 35\text{ °C}$)**.

---

### 4.5 Fase E — Encerramento e Sequência Segura de Desconexão

Quando o carregador emitir o aviso sonoro e exibir **`FULL`** no visor LCD ($V = 8{,}40\text{ V}$, corrente zerada):

1. **1º Pressione `BATT TYPE / STOP`** para silenciar o alarme e garantir que a saída de potência esteja desativada.
2. **2º Desconecte o cabo de balanceamento (`JST-XHR-3P`)** da porta lateral `2S` do carregador, puxando pelo corpo plástico do conector (nunca pelos fios).
3. **3º Desconecte o plugue de potência (`XT-30`)** separando a bateria do chicote do carregador.
4. **4º Somente após a bateria estar totalmente solta**, retire os pinos banana do carregador (se for guardá-lo) e desligue a fonte DC da tomada.

---

### 4.6 Fase F — Procedimento para Armazenamento (*Storage Mode*)

Baterias LiPo **não devem ser guardadas totalmente carregadas ($8{,}40\text{ V}$)** nem descarregadas ($6{,}60\text{ V}$) por períodos superiores a **48 horas**, pois a manutenção em $4{,}20\text{ V/célula}$ acelera a oxidação interna do eletrólito e provoca inchamento químico.

* Ao encerrar os testes da semana ou da Sprint, conecte a bateria conforme a Fase B, selecione no menu do carregador o modo **`LiPo STORAGE` — `0.6A` — `7.4V (2S)`** e inicie o ciclo.
* O carregador irá carregar ou descarregar automaticamente a bateria até estabilizar ambas as células em exatos **$3{,}80\text{ V a }3{,}85\text{ V/célula}$ ($7{,}60\text{ V a }7{,}70\text{ V}$ total)**.

---

## 5. Regras de Contingência Homologadas pela Equipe

Caso qualquer anomalia seja detectada antes, durante ou após a recarga, a equipe deve executar imediatamente os protocolos abaixo:

### 5.1 Contingência para Aquecimento Excessivo ($T_{\text{bat}} \ge 45\text{ °C}$)
1. **Ação Imediata:** Pressione **`STOP`** no carregador e desligue a fonte DC da tomada imediatamente.
2. **Isolamento:** Não manipule a bateria sem proteção se estiver muito quente. Mantenha-a dentro da *LiPo Safe Bag* fechada em local ventilado e longe de materiais inflamáveis por no mínimo **60 minutos**.
3. **Diagnóstico Pós-Resfriamento:** Após esfriar totalmente, meça a tensão individual das células ($C_1$ e $C_2$) e a resistência interna (IR). Se houver desbalanceamento $> 0{,}05\text{ V}$ ou deformação, **condene a bateria para uso no robô**.

### 5.2 Contingência para Estufamento de Célula (*Puffing / Swelling*)
O estufamento ocorre pela geração de gases internos ($\text{CO}, \text{CO}_2, \text{H}_2$) resultante de decomposição eletrolítica por sobrecarga, descarga profunda ou curto térmico:
1. **Ação Imediata:** Interrompa imediatamente qualquer carga ou uso no Micromouse.
2. **Proibição Absoluta:** **JAMAIS fure, aperte ou tente comprimir uma célula LiPo estufada** (o contato do lítio metálico interno com o oxigênio e a umidade do ar causa ignição espontânea imediata).
3. **Quarentena:** Isole a bateria em recipiente metálico ou *LiPo Safe Bag* preenchida com areia seca e encaminhe para o protocolo de neutralização e descarte (Seção 5.5).

### 5.3 Contingência para Desbalanceamento Severo ($\Delta V \ge 0{,}10\text{ V}$) ou Subtensão ($< 3{,}00\text{ V/cél}$)
1. Caso o carregador exiba erro **`BATTERY VOL ERR CELL CONNECT`** ou **`LOW VOLTAGE`**, desconecte a bateria e meça os 3 pinos do conector `JST-XHR-3P` com multímetro.
2. Se uma célula estiver abaixo de **$3{,}00\text{ V}$** (ou se a diferença entre $C_1$ e $C_2$ superar **$0{,}10\text{ V}$**), houve precipitação de dendritos de cobre na célula afetada. **Não force a recarga no modo Li-ion/NiMH** — segregue a unidade para descarte.

### 5.4 Contingência para Fumaça, Chiado ou Princípio de Incêndio (*Thermal Runaway*)
1. **Cortar Energia:** Desligue o disjuntor da bancada ou retire o cabo de força da fonte DC da tomada imediatamente.
2. **Contenção Térmica:** Utilize **areia seca**, manta corta-fogo ou **extintor pó químico ABC / $\text{CO}_2$** para abafar as chamas e impedir a propagação para a bancada. *(Atenção: não jogue pequenos volumes de água diretamente sobre célula aberta, e não inale os vapores brancos liberados, pois contêm traços irritantes de $\text{HF}$ / $\text{PF}_5$ — ventile o laboratório e afaste a equipe).*
3. **Rescaldo:** Após a extinção, mantenha o pack submerso em balde com areia em área externa aberta por 24 horas antes da remoção.

### 5.5 Protocolo de Neutralização e Descarte de Baterias Condenadas
1. Descarregue lentamente a bateria através de uma carga resistiva (ex.: lâmpada halógena automotiva de $12\text{ V} / 21\text{ W}$ ou resistor de potência de $47\ \Omega / 10\text{ W}$) ou submerja os terminais em recipiente plástico com **água salgada ($30\text{ g}$ de sal por litro)** em área externa ventilada por 48 horas até que a tensão nos terminais seja **$0{,}00\text{ V}$**.
2. Isole os conectores com fita isolante e entregue em ponto oficial de coleta de resíduos eletroeletrônicos / baterias da UnB.

---

## 6. Checklist Rápido de Operação em Bancada

Antes de apertar `START` no carregador, confira todos os itens:

- [ ] A bateria descansou por $\ge 15\text{ min}$ após o último uso e está fria ao toque ($< 30\text{ °C}$)?
- [ ] O encapsulamento está plano, firme, sem rasgos e **sem estufamento**?
- [ ] A tensão inicial medida é $\ge 6{,}60\text{ V}$ ($\ge 3{,}30\text{ V}$ em cada célula)?
- [ ] Os pinos banana foram conectados no carregador **antes** de plugar o conector XT-30 na bateria?
- [ ] **Ambos** os conectores (**XT-30** de potência e **JST-XHR-3P** de balanceamento na porta 2S) estão firmemente acoplados?
- [ ] O carregador está configurado exatamente em **`LiPo BALANCE` | `0.6A` | `7.4V (2S)`**?
- [ ] A bateria está posicionada dentro da **LiPo Safe Bag** fechada sobre bancada limpa?
- [ ] Há um membro da equipe presente na sala para monitorar o ciclo a cada 15 minutos?

---

## 7. Referências Bibliográficas

1. **GENS ACE / TATTU.** *LiPo Battery Safety Guide and Standard Charging Instructions*. Manual Técnico do Fabricante, 2024. Disponível em: <https://www.gensace.de/lipo-battery-guide>.
2. **SKYRC TECHNOLOGY.** *iMAX B6 / B6 V3 AC/DC Professional Balance Charger/Discharger Instruction Manual*. Versão 3.1, 2022.
3. **UL STANDARDS.** *UL 1642: Standard for Safety for Lithium Batteries*. Underwriters Laboratories, 6. ed., 2020.

---

## Histórico de Versão

| Versão | Data | Descrição | Autor(es) | Revisor(es) |
| :---: | :---: | :--- | :--- | :--- |
| 1.0 | 18/10/2026 | Elaboração e homologação do Procedimento Operacional Padrão (POP) de recarga balanceada e contingência para a bateria LiPo 2S | Paulo Melo | Equipe de Energia |
