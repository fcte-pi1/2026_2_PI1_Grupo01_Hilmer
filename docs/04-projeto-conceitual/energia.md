# Análise de Consumo Energético do Produto

## 1. Identificação dos Subsistemas Elétricos

A arquitetura do Micromouse organiza a distribuição de potência e os níveis lógicos a partir da fonte principal. O subsistema elétrico divide-se em barramentos de potência e barramentos de regulação chaveada. O perfil de carga foi levantado com base nas especificações dos componentes e na folha de dados da equipe. Para o dimensionamento da autonomia contínua, padronizou-se o ciclo de missão unitário em **600 segundos (10 minutos)** de exploração e tomada de tempo, e a missão estendida de homologação em **1.800 segundos (30 minutos)** para cumprir a resolução consecutiva dos 3 labirintos da competição (RNF-04).

* **ESP32-C3-MINI-1 (MCU e Telemetria):** Opera sob linha regulada de 3,3 V derivada do conversor Buck dedicado. Consumo médio nominal de **110 mA (0,11 A)** em processamento de controle e transmissões periódicas de rádio.
* **Motores DC GA12-N20 com Redução (Par de Atuadores M1 e M2):** Tensão nominal de catálogo de 6,0 V. Consumo médio dinâmico em piso de corrida estimado em **0,15 A por motor (0,30 A conjunto)**.
* **Driver Ponte H Dupla (DRV8833 - Lógica de Controle):** Alimentação lógica e pinos de sinal PWM a 3,3 V comutados pelos GPIOs do microcontrolador. Apresenta consumo quiescente de **3 mA (0,003 A)**.
* **Encoders Incrementais de Efeito Hall (2 unidades):** Conectados via interrupções de quadratura aos GPIOs do MCU e alimentados pela linha de 3,3 V. Consumo conjunto estimado em **10 mA (0,01 A)**.
* **Sensor LiDAR ToF 360° (Sensoriamento Primário de Pista):** Opera com linha dedicada de alimentação de 5,0 V (+5V0) e interface serial UART. Apresenta consumo nominal médio de **150 mA (0,15 A)** em rotação contínua e varredura óptica.

---

## 2. Cálculo da Energia Consumida por Componente

O cálculo de energia elétrica em regime de corrente contínua adota a relação fundamental:

$$E = V \cdot I \cdot t$$

Onde:
* $E$ = Energia em joules (J)
* $V$ = Tensão de alimentação do subsistema em volts (V)
* $I$ = Corrente média consumida em ampères (A)
* $t$ = Tempo operacional de missão unitária ($t = 600\text{ s} = 10\text{ min}$)

| Componente / Subsistema | Tensão ($V$) | Corrente Média ($I$) | Tempo ($t$) | Energia Consumida ($E$) |
| :--- | :---: | :---: | :---: | :---: |
| ESP32-C3-MINI-1 | 3,3 V | 0,110 A | 600 s | 217,80 J |
| Motores DC N20 (Par M1 + M2) | 6,0 V | 0,300 A | 600 s | 1.080,00 J |
| Driver DRV8833 (Lógica) | 3,3 V | 0,003 A | 600 s | 5,94 J |
| Encoders Incrementais (Par) | 3,3 V | 0,010 A | 600 s | 19,80 J |
| Sensor LiDAR ToF 360° | 5,0 V | 0,150 A | 600 s | 450,00 J |

---

## 3. Estimativa do Consumo Total de Energia

A demanda energética líquida total calculada para um ciclo unitário de 10 minutos de missão ($E_{\text{total}}$) é obtida pela soma das parcelas energéticas de cada componente:

$$E_{\text{total}} = 217,80 + 1080,00 + 5,94 + 19,80 + 450,00 = \mathbf{1.773,54\text{ J}}$$

Convertendo para Watt-hora (Wh) utilizando a equivalência padrão ($1\text{ Wh} = 3600\text{ J}$):

$$E_{\text{Wh}} = \frac{E_{\text{total}}}{3600} = \frac{1773,54}{3600} \approx \mathbf{0,493\text{ Wh}}$$

---

## 4. Escolha e Dimensionamento da Fonte de Alimentação

### 4.1 Margem de Segurança e Eficiência Energética
Para robôs móveis autônomos de labirinto, adota-se uma margem de segurança de **40%**, combinada com um rendimento médio de **85%** ($\eta = 0{,}85$) dos conversores chaveados Buck da placa. Essa folga de projeto sustenta:
1. **Transientes Dinâmicos:** As partidas, frenagens e rotações rápidas dos motores elevam a corrente momentânea muito acima do patamar nominal de regime permanente.
2. **Perdas Térmicas nos Conversores:** O chaveamento e a resistência parasita das bobinas e diodos internos dos reguladores Step-Down degradam parte da potência útil em calor.
3. **Preservação Química da Célula:** Células à base de lítio sofrem danos de capacidade irreversíveis quando descarregadas abaixo de 20% de sua reserva total (profundidade de descarga limite de 80% DoD).

A energia total demandada na fonte para uma corrida unitária de 10 minutos é:

$$E_{\text{necessária}} = \frac{E_{\text{Wh}}}{\eta \cdot (1 - \text{Margem})} = \frac{0,493}{0,85 \cdot (1 - 0,40)} \approx \mathbf{0,967\text{ Wh}}$$

### 4.2 Capacidade Mínima Teórica Requerida
Adotando topologia recarregável **LiPo 2S** com tensão nominal de $V_{\text{nominal}} = 7{,}4\text{ V}$ (8,4 V em plena carga e corte de subtensão operacional em 6,6 V / 3,3 V por célula):

$$\text{Capacidade Teórica (Ah)} = \frac{E_{\text{necessária}}}{V_{\text{nominal}}} = \frac{0,967\text{ Wh}}{7,4\text{ V}} \approx 0,1306\text{ Ah} \implies \mathbf{130\text{ mAh}}$$

### 4.3 Validação de Autonomia para os 3 Labirintos Consecutivos (RNF-04)
O requisito **RNF-04** exige autonomia ininterrupta para completar as 3 configurações de labirinto da competição (4×4, 8×4 e 12×4 células), cada uma com teto regulamentar de 10 minutos (**RNF-03**), totalizando **30 minutos contínuos ($t_{\text{total}} = 1.800\text{ s} = 0{,}5\text{ h}$)** no pior cenário de estresse.

Calculando a potência média drenada da bateria sob rendimento de 85%:
* Potência útil das cargas lógicas e sensores (+5V0 e +3V3): $P_{\text{lógica}} \approx 1{,}19\text{ W} \implies P_{\text{fonte, lógica}} = \frac{1,19}{0,85} \approx 1{,}40\text{ W}$
* Potência útil dos motores (+VBAT_2S modulado em PWM a 71%): $P_{\text{motores}} \approx 1{,}80\text{ W}$
* **Potência média total drenada da bateria:** $P_{\text{bat\_total}} \approx 3{,}20\text{ W}$

A corrente média contínua consumida do barramento de 7,4 V é:

$$I_{\text{bat\_médio}} = \frac{P_{\text{bat\_total}}}{V_{\text{nominal}}} = \frac{3,20\text{ W}}{7,4\text{ V}} \approx \mathbf{0,432\text{ A}} \ (\mathbf{432\text{ mA}})$$

Portanto, a capacidade consumida durante os 30 minutos ininterruptos de missão é:

$$C_{\text{consumida\_30min}} = I_{\text{bat\_médio}} \cdot 0,5\text{ h} = 432\text{ mA} \cdot 0,5\text{ h} = \mathbf{216\text{ mAh}}$$

Aplicando o fator de segurança química de 80% de profundidade máxima de descarga (DoD):

$$C_{\text{mínima\_30min}} = \frac{216\text{ mAh}}{0,80} = \mathbf{270\text{ mAh}}$$

### 4.4 Homologação do Pack Comercial Selecionado
Com base no dimensionamento de 130 mAh (missão unitária) e na demanda estendida de 270 mAh (3 labirintos), homologa-se formalmente o seguinte pack comercial:

* **Fabricante / Modelo:** **Tattu 650mAh 2S1P 75C 7.4V Lipo Battery Pack with XT30 Plug**
* **Topologia:** Polímero de Lítio (LiPo) 2S1P / 2 Células
* **Tensão:** 7,4 V nominal | 8,4 V em plena carga
* **Capacidade Mínima:** **650 mAh**
* **Taxa de Descarga Contínua (C-Rating):** **75C**
* **Taxa de Descarga de Pico (*Burst*):** **150C**
* **Dimensões Físicas:** **57 mm (Comprimento) × 31 mm (Largura) × 12 mm (Altura)**
* **Massa Líquida:** **43 g (±20 g)**
* **Plugue de Descarga:** **XT-30**
* **Plugue de Carga / Balanceamento:** **JST-XHR-3P**

#### Justificativa Técnica da Homologação:
1. **Margem de Autonomia Real:** Com 650 mAh e respeitando a profundidade máxima de descarga recomendada de 80% (DoD), a bateria disponibiliza **520 mAh úteis**. Sob a corrente média de consumo de 432 mA, a autonomia contínua calculada é de **~72 minutos** ($\frac{520\text{ mAh}}{432\text{ mA}} \times 60 \approx 72\text{ min}$), cobrindo com folga de **+140% de margem** o teto regulamentar de 30 minutos dos 3 labirintos consecutivos (RNF-04).
2. **Robustez Contra Queda de Tensão (*Voltage Sag*):** A taxa contínua de 75C permite entrega ininterrupta de até **48,75 A** ($0,65\text{ A} \times 75$), com picos de até **97,5 A** a 150C. Como os micromotores GA12-N20 demandam picos transitórios de no máximo 2 A a 3 A durante manobras e partidas bruscas, a bateria opera a menos de 6% do seu limite de corrente, garantindo barramento elétrico perfeitamente estável e eliminando qualquer risco de *brownout reset* no microcontrolador ESP32-C3.
3. **Alívio Mecânico e Dinâmica de Pista:** A massa contida de 43 g reduz a inércia rotacional do conjunto móvel, alivia as cargas radiais sobre os mancais e pinhões metálicos da redução dos motores N20 e minimiza o escorregamento dos pneus nas curvas de 90° e 180°, aumentando a precisão da odometria.
4. **Padronização de Conectores e Carga:** O conector de alta corrente XT-30 previne desconexões mecânicas por vibração, enquanto o plugue JST-XHR-3P viabiliza o carregamento com balanceamento individual de células no carregador inteligente de bancada (B6 V3 Smart Charger em modo *LiPo Balance* a 0,6 A / 0,7 A).

---

## 5. Planejamento do Circuito de Alimentação

A distribuição de energia segrega a malha de acionamento eletromecânico dos circuitos lógicos e de sensoriamento:

* **Entrada de Energia, Gerenciamento e Chaveamento:**
  * O polo positivo da bateria LiPo 2S conecta-se ao circuito através de conector de alta corrente **XT30**.
  * Em série com a linha positiva (+VBAT), posiciona-se um **Fusível de Proteção de Ação Rápida (3 A a 5 A)** (formato mini automotivo ou SMD), indispensável para salvaguardar o circuito contra curtos-circuitos acidentais decorrentes da elevada capacidade de descarga da célula LiPo (75C contínuo / 150C pico).
  * Em série após o fusível, uma **Chave de Potência** mecânica (SW1) comanda a ligação geral do circuito antes da derivação dos barramentos de potência e regulação.
* **Barramento Direto dos Motores (+VBAT_2S / Pinos VM):**
  * O terminal `VM` do driver DRV8833 recebe diretamente a tensão não regulada da bateria (+VBAT_2S, entre 7,4 V e 8,4 V).
  * *Validação da Tensão de 8,4 V em Motores Nominais de 6,0 V:* O DRV8833 suporta até 10,8 V em VM, operando com ampla folga de segurança. Para proteger as bobinas dos micromotores N20 sem a inclusão de um regulador de potência de 6 V, adota-se **limitação por software via modulação PWM**: o *duty cycle* máximo enviado pelo ESP32-C3 é travado em **71%** ($6,0\text{ V} / 8,4\text{ V}$), garantindo que a tensão eficaz nos motores não ultrapasse os 6,0 V nominais sob bateria plena.
* **Barramento de Regulação Chaveada (Dois Módulos Buck Independentes):**
  * **Conversor Buck +5V0:** Regulador Step-Down com capacidade mínima de 1 A contínuo. Alimenta de forma isolada a linha de potência do sensor LiDAR ToF 360°.
  * **Conversor Buck +3V3:** Regulador Step-Down com capacidade mínima de 1 A contínuo. Alimenta a linha digital do microcontrolador ESP32-C3-MINI-1, os encoders de quadratura e a polarização lógica do DRV8833. A especificação de 1 A supre com ampla folga os picos transitórios de rádio do chip ESP32 (de até 350 mA).
* **Controle de Ruído e Estabilidade de Referência (GND Comum):**
  * **Filtragem de Transientes:** Instalação de capacitores eletrolíticos Low-ESR de 220 µF a 470 µF soldados nas proximidades do pino VM do DRV8833, absorvendo o rebote indutivo provocado pelas manobras dos motores.
  * **Desacoplamento de Linha:** Capacitores cerâmicos de 100 nF distribuídos nos terminais de cada circuito integrado e diretamente soldados na carcaça dos motores N20.
  * **Topologia de Referência (GND Único em Estrela):** Todos os módulos compartilham o mesmo potencial de terra, mas as trilhas de retorno de alta corrente (motores e chaveamento) são fisicamente separadas do terra de sinal do MCU e dos sensores, conectando-se em um único nó central junto ao conector da bateria. Isso impede que transitórios induzidos por comutação mecânica gerem flutuações e causem *brownout reset* no processador.

---

## 6. Monitoramento via Software Embarcado

Para registrar a curva de descarga da célula e diagnosticar a integridade da bateria em tempo de execução sem danificar os pinos analógicos do ESP32-C3:

* **Atenuação da Tensão de Bateria (Divisor Resistivo R1/R2):**
  * O barramento +VBAT_2S (variando entre 6,0 V descarregada e 8,4 V em carga plena) passa por um divisor resistivo em série formado por resistores de precisão com $R_1 = 100\text{ k}\Omega$ (ligado a +VBAT) e $R_2 = 33\text{ k}\Omega$ (ligado ao GND). O ponto central de medição é conectado diretamente ao canal analógico dedicado **GPIO 2 (ADC1_CH2)** do ESP32-C3.
  * A tensão atenuada entregue ao pino ADC é dada por:

$$V_{\text{ADC}} = V_{\text{BAT}} \cdot \left(\frac{R_2}{R_1 + R_2}\right) = V_{\text{BAT}} \cdot \left(\frac{33}{100 + 33}\right) \approx V_{\text{BAT}} \cdot 0,248$$

  * Sob tensão máxima de 8,4 V, o nível atenuado resultante é de aproximadamente **2,08 V** (ou **2,36 V** caso se adote $R_2 = 39\text{ k}\Omega$). Ambos os patamares operam com ampla margem abaixo do limite absoluto do pino (3,3 V) e dentro da faixa de máxima linearidade e precisão do ADC do ESP32-C3 sob atenuação de 11 dB (0 V a 2,50 V).

* **Rotinas de Aquisição e Proteção:**
  * O firmware realiza leituras periódicas do canal ADC com filtro digital de média móvel para rejeitar flutuações induzidas pelo chaveamento PWM do driver.
  * O software implementa proteção de subtensão lógica (*Under-Voltage Lockout* - UVLO): se a leitura atestar tensão global inferior a **6,6 V** (equivalente a **3,3 V por célula**) em amostragens sucessivas, os sinais de controle PWM para o DRV8833 são zerados, cortando a tração dos motores e preservando as células LiPo contra descarga profunda.
  * *Compatibilidade com o Regulador Buck de 5 V:* O limiar de corte em 6,6 V garante também que o conversor Step-Down LM2596 (responsável pela linha do LiDAR) opere sempre acima de sua tensão mínima de *dropout* ($V_{\text{in}} \ge 5{,}0\text{ V} + 1{,}5\text{ V} = 6{,}5\text{ V}$), impedindo flutuações e reinicializações no sensor óptico sem sacrificar autonomia útil (a LiPo já entregou mais de 96% de sua carga em 3,3 V/célula).
  * A telemetria empacota o tempo contínuo de atividade e a leitura instantânea de tensão, registrando os dados de consumo em tempo real para contraste com o modelo de energia projetado (atendendo ao requisito **RF-08 / HU-08**).

---

## 7. Dimensionamento e Análise dos Reguladores Step-Down (LM2596 e MP1584EN)

Esta seção formaliza o projeto, os cálculos de componentes e a validação térmica dos dois circuitos reguladores de tensão chaveados (conversores Step-Down Buck) que compõem o subsistema de conversão de energia do Micromouse, atendendo aos critérios de aceitação da **Issue #199**.

### 7.1 Requisitos Operacionais e Metodologia de Projeto

A alimentação primária provém do barramento de bateria LiPo 2S, operando na faixa dinâmica de **6,6 V a 8,4 V** ($V_{\text{nom}} = 7{,}4\text{ V}$):
* **Tensão Máxima de Entrada ($V_{\text{in,max}}$):** $8{,}4\text{ V}$ (células plenamente carregadas a 4,20 V cada).
* **Tensão Nominal de Entrada ($V_{\text{in,nom}}$):** $7{,}4\text{ V}$ (patamar médio da curva de descarga).
* **Tensão Mínima de Entrada ($V_{\text{in,min}}$):** $6{,}6\text{ V}$ (limiar de corte seguro por software via UVLO a 3,30 V/célula).
* **Requisito de Capacidade de Corrente Contínua:** Ambos os barramentos são dimensionados para fornecer até **1,0 A contínuo** em regime permanente com estabilidade de tensão e margem térmica comprovada, superando a demanda de pico e nominal dos dispositivos conectados.

---

### 7.2 Dimensionamento do Regulador LM2596-ADJ (Barramento +5V0 — Sensor LiDAR ToF)

O regulador **LM2596** (Texas Instruments) é um circuito integrado conversor Buck monolítico que opera com frequência de chaveamento fixa de **150 kHz**. É adotado na versão ajustável (**LM2596-ADJ**) ou módulo calibrado para suprir a linha dedicada de **5,0 V** do sensor óptico LiDAR ToF 360°.

#### 7.2.1 Razão Cíclica (*Duty Cycle*)
A razão cíclica $D$ relaciona as tensões de saída e entrada sob rendimento estimado de $\eta \approx 82\%$ ($0{,}82$):

$$D = \frac{V_{\text{out}}}{V_{\text{in}} \cdot \eta}$$

* Para bateria em plena carga ($V_{\text{in}} = 8{,}4\text{ V}$):
  $$D_{\text{min}} = \frac{5{,}0}{8{,}4 \cdot 0{,}82} \approx 0{,}725 \implies \mathbf{72{,}5\%}$$
* Para bateria no corte UVLO ($V_{\text{in}} = 6{,}6\text{ V}$):
  $$D_{\text{max}} = \frac{5{,}0}{6{,}6 \cdot 0{,}82} \approx 0{,}923 \implies \mathbf{92{,}3\%}$$

#### 7.2.2 Malha de Realimentação e Ajuste de Tensão (Feedback)
O CI mantém a saída regulada por meio de uma tensão de referência interna estável de $V_{\text{ref}} = 1{,}23\text{ V}$ no pino de Feedback (`FB`). A tensão de saída $V_{\text{out}}$ é definida pelo divisor resistivo formado por $R_1$ (conectado entre FB e GND) e $R_2$ (conectado entre $V_{\text{out}}$ e FB):

$$V_{\text{out}} = V_{\text{ref}} \cdot \left(1 + \frac{R_2}{R_1}\right) = 1{,}23\text{ V} \cdot \left(1 + \frac{R_2}{R_1}\right)$$

Fixando $R_1 = 1{,}0\text{ k}\Omega$ (valor recomendado pelo fabricante para garantir corrente de polarização estável de ~1,23 mA):

$$\frac{R_2}{R_1} = \frac{V_{\text{out}}}{V_{\text{ref}}} - 1 = \frac{5{,}0}{1{,}23} - 1 \approx 3{,}065 \implies R_2 = 1{,}0\text{ k}\Omega \cdot 3{,}065 \approx \mathbf{3{,}06\text{ k}\Omega}$$

* **Implementação Física:** Em placas com resistores fixos de precisão (tolerância de 1%), emprega-se $R_2 = 3{,}0\text{ k}\Omega$ em série com $68\ \Omega$ (ou valor comercial E96 de $3{,}09\text{ k}\Omega$, resultando em $V_{\text{out}} = 5{,}03\text{ V}$). Em módulos comerciais step-down, o ajuste é efetuado através de trimpot multivoltas de $10\text{ k}\Omega$, aferido em bancada sob carga para $5{,}00\text{ V} \pm 0{,}02\text{ V}$.

#### 7.2.3 Indutor de Filtragem de Potência ($L$)
O valor mínimo de indutância para operação contínua (CCM - *Continuous Conduction Mode*) sob $f_{\text{sw}} = 150\text{ kHz}$ e ondulação de corrente ($\Delta I_L$) arbitrada em 30% da corrente máxima de projeto ($I_{\text{out,max}} = 1{,}0\text{ A} \implies \Delta I_L = 0{,}30\text{ A}$):

$$L_{\text{mín}} = \frac{(V_{\text{in,max}} - V_{\text{out}}) \cdot D_{\text{min}}}{\Delta I_L \cdot f_{\text{sw}}} = \frac{(8{,}4 - 5{,}0) \cdot 0{,}725}{0{,}30 \cdot 150.000} = \frac{2{,}465}{45.000} \approx 54{,}8\ \mu\text{H}$$

Adota-se indutor comercial blindado de **$47\ \mu\text{H}$ a $68\ \mu\text{H}$**, com corrente nominal contínua $I_{\text{rms}} \ge 1{,}5\text{ A}$ e corrente de saturação $I_{\text{sat}} \ge 2{,}0\text{ A}$ (ex: indutores da série CDRH104R ou toroidais blindados).

#### 7.2.4 Diodo de Recirculação (*Catch Diode*)
Durante o tempo em que a chave interna desliga, a corrente do indutor circula pelo diodo externo ligado ao terra. Exige-se diodo **Schottky** de alta velocidade com tempo de recuperação reversa quase nulo e baixa queda direta ($V_F \le 0{,}45\text{ V}$):
* **Componente Selecionado:** **SS34** (SMD) ou **1N5822** (Through-hole).
* **Parâmetros:** Tensão reversa máxima $V_R = 40\text{ V}$ (muito superior a 8,4 V) e corrente média direta $I_F = 3{,}0\text{ A}$ (suporta com 200% de margem o regime de 1,0 A).

#### 7.2.5 Capacitores de Entrada e Saída
* **Capacitor de Entrada ($C_{\text{in}}$):** Absorve a corrente pulsada de entrada. Adota-se capacitor eletrolítico de alumínio Low-ESR de **$220\ \mu\text{F} \ / \ 16\text{ V}$** em paralelo com um cerâmico multicamadas (MLCC) de **$100\text{ nF} \ / \ 50\text{ V}$**, garantindo imunidade a transientes de comutação.
* **Capacitor de Saída ($C_{\text{out}}$):** Determina a ondulação de tensão (*ripple*) e a estabilidade da malha. Para manter a ondulação $\Delta V_{\text{out}} \le 30\text{ mV}$:
  $$C_{\text{out}} \ge \frac{\Delta I_L}{8 \cdot f_{\text{sw}} \cdot \Delta V_{\text{out}}} = \frac{0{,}30}{8 \cdot 150.000 \cdot 0{,}030} \approx 8{,}3\ \mu\text{F} \implies \text{Limitado pela ESR}$$
  Como a ondulação em capacitores eletrolíticos é dominada pela resistência equivalente série ($\text{ESR} \le \Delta V_{\text{out}} / \Delta I_L = 0{,}030 / 0{,}30 = 0{,}10\ \Omega$), padroniza-se **$330\ \mu\text{F} \ / \ 10\text{ V}$** Low-ESR em paralelo com cerâmico de **$100\text{ nF}$**.

#### 7.2.6 Compatibilidade Crítica de *Dropout Voltage*
O transistor de comutação interno do LM2596 é bipolar NPN Darlington, apresentando queda de saturação de $V_{\text{drop}} \approx 1{,}2\text{ V}$ a $1{,}5\text{ V}$. Portanto, a tensão de entrada mínima para que a saída de 5,0 V não entre em queda de regulação é:

$$V_{\text{in,mín\_reg}} = V_{\text{out}} + V_{\text{drop}} = 5{,}0\text{ V} + 1{,}5\text{ V} = \mathbf{6{,}5\text{ V}}$$

Como a proteção de subtensão (UVLO) no firmware do robô atua desligando o sistema em **6,60 V** (3,30 V/célula), a bateria **nunca atinge o patamar inferior a 6,5 V**, garantindo que o regulador de 5 V opere 100% do tempo em regime de regulação linear e estável.

#### 7.2.7 Análise de Dissipação Térmica e Temperatura de Junção
A potência dissipada em calor pelo módulo no pior caso operacional (1,0 A contínuo sob rendimento conservador de $\eta = 82\%$):

$$P_{\text{d,estresse}} = P_{\text{out}} \cdot \left(\frac{1}{\eta} - 1\right) = (5{,}0\text{ V} \cdot 1{,}0\text{ A}) \cdot \left(\frac{1}{0{,}82} - 1\right) \approx 5{,}0 \cdot 0{,}2195 \approx \mathbf{1{,}10\text{ W}}$$

Em operação real de pista, o sensor LiDAR ToF 360° demanda apenas $150\text{ mA}$ ($0{,}15\text{ A}$):

$$P_{\text{d,nominal}} = (5{,}0\text{ V} \cdot 0{,}15\text{ A}) \cdot \left(\frac{1}{0{,}85} - 1\right) \approx 0{,}75 \cdot 0{,}176 \approx \mathbf{0{,}13\text{ W}}$$

Para o encapsulamento **TO-263 (SMD)** montado com plano de cobre de dissipação na PCB ou em módulo comercial padrão, a resistência térmica típica junção-ambiente é de $R_{\theta JA} \approx 35\text{ °C/W}$ a $40\text{ °C/W}$. Adotando $R_{\theta JA} = 35\text{ °C/W}$ e temperatura ambiente máxima de $T_a = 25\text{ °C}$:

* **Elevação Térmica sob Carga Nominal (150 mA):**
  $$\Delta T_{\text{nom}} = P_{\text{d,nominal}} \cdot R_{\theta JA} = 0{,}13\text{ W} \cdot 35\text{ °C/W} \approx 4{,}6\text{ °C} \implies T_j \approx \mathbf{29{,}6\text{ °C}}$$
* **Elevação Térmica sob Limite Contínuo de Projeto (1,0 A):**
  $$\Delta T_{\text{1A}} = P_{\text{d,estresse}} \cdot R_{\theta JA} = 1{,}10\text{ W} \cdot 35\text{ °C/W} \approx 38{,}5\text{ °C} \implies T_j \approx 25 + 38{,}5 = \mathbf{63{,}5\text{ °C}}$$

Como a temperatura máxima admissível de junção do silício é de $T_{j,\text{max}} = 125\text{ °C}$, a temperatura calculada de $63{,}5\text{ °C}$ atesta **ampla estabilidade contínua**, sem necessidade de dissipador térmico adicional.

---

### 7.3 Dimensionamento do Regulador MP1584EN (Barramento +3V3 — MCU, Encoders e Lógica)

O regulador **MP1584EN** (Monolithic Power Systems - MPS) é um conversor Step-Down monolítico que emprega chave interna de efeito de campo MOSFET de potência de alta eficiência ($R_{\text{DS(on)}} \approx 180\text{ m}\Omega$). Ele opera em alta frequência programável (de 100 kHz a 1,5 MHz, padronizado tipicamente em **1,0 MHz a 1,5 MHz**), permitindo componentes magnéticos de tamanho reduzido e resposta transiente ultra-rápida. É responsável pela linha de **3,3 V** que alimenta o núcleo do processador ESP32-C3-MINI-1, os encoders de efeito Hall e os pinos de controle do DRV8833.

#### 7.3.1 Razão Cíclica (*Duty Cycle*)
Sob rendimento de $\eta \approx 88\%$ ($0{,}88$):

* Para bateria em $8{,}4\text{ V}$:
  $$D_{\text{min}} = \frac{3{,}3}{8{,}4 \cdot 0{,}88} \approx 0{,}446 \implies \mathbf{44{,}6\%}$$
* Para bateria em $6{,}6\text{ V}$:
  $$D_{\text{max}} = \frac{3{,}3}{6{,}6 \cdot 0{,}88} \approx 0{,}568 \implies \mathbf{56{,}8\%}$$

Devido à chave MOSFET High-Side, o MP1584EN não sofre das elevadas perdas por queda de saturação encontradas no LM2596, apresentando tensão de *dropout* muito baixa ($< 0{,}4\text{ V}$), operando com folga total mesmo que a bateria caísse a níveis extremos.

#### 7.3.2 Malha de Realimentação e Ajuste de Tensão (Feedback)
A tensão de referência interna do MP1584 é de $V_{\text{ref}} = 0{,}80\text{ V} \pm 2\%$. A saída $V_{\text{out}}$ é ajustada pelo divisor formado por $R_1$ (superior) e $R_2$ (inferior ligado ao GND):

$$V_{\text{out}} = V_{\text{ref}} \cdot \left(1 + \frac{R_1}{R_2}\right) = 0{,}80\text{ V} \cdot \left(1 + \frac{R_1}{R_2}\right)$$

Fixando $R_2 = 10\text{ k}\Omega$ (valor ótimo para imunidade a ruídos na entrada FB):

$$\frac{R_1}{R_2} = \frac{V_{\text{out}}}{V_{\text{ref}}} - 1 = \frac{3{,}3}{0{,}80} - 1 = 4{,}125 - 1 = 3{,}125 \implies R_1 = 10\text{ k}\Omega \cdot 3{,}125 = \mathbf{31{,}25\text{ k}\Omega}$$

* **Implementação Física:** Utiliza-se um resistor de precisão da série E96 de **$31{,}6\text{ k}\Omega \ (1\%)$** (resultando em $V_{\text{out}} = 3{,}328\text{ V}$, erro relativo de apenas +0,8%, perfeitamente compatível com o patamar de 3,0 V a 3,6 V do ESP32-C3) ou uma associação série de $30\text{ k}\Omega + 1{,}2\text{ k}\Omega = 31{,}2\text{ k}\Omega$ ($V_{\text{out}} = 3{,}296\text{ V}$). Em micromódulos comerciais, o ajuste é efetuado por trimpot SMD pré-calibrado em bancada para $3{,}30\text{ V}$.

#### 7.3.3 Indutor de Filtragem de Alta Frequência ($L$)
Operando na frequência de $f_{\text{sw}} = 1{,}0\text{ MHz}$ ($10^6\text{ Hz}$) e estipulando ondulação $\Delta I_L = 0{,}30\text{ A}$ (30% de 1,0 A):

$$L_{\text{mín}} = \frac{(V_{\text{in,max}} - V_{\text{out}}) \cdot D_{\text{min}}}{\Delta I_L \cdot f_{\text{sw}}} = \frac{(8{,}4 - 3{,}3) \cdot 0{,}446}{0{,}30 \cdot 1.000.000} = \frac{2{,}2746}{300.000} \approx 7{,}58\ \mu\text{H}$$

Adota-se indutor SMD blindado de potência de **$4{,}7\ \mu\text{H}$ a $10\ \mu\text{H}$** (ex: perfil SMD 0630 ou 0530), com corrente de saturação $I_{\text{sat}} \ge 2{,}0\text{ A}$ e baixa resistência ôhmica DC ($\text{DCR} \le 50\text{ m}\Omega$).

#### 7.3.4 Diodo de Recirculação Schottky
* **Componente Selecionado:** **SS24** ou **B230A** (SMD).
* **Parâmetros:** Tensão reversa máxima $V_R = 40\text{ V}$, corrente contínua direta $I_F = 2{,}0\text{ A}$ e queda de condução $V_F \approx 0{,}40\text{ V}$, comutando com baixíssima perda de potência na faixa de megahertz.

#### 7.3.5 Capacitores de Filtragem Cerâmicos (MLCC)
Devido à frequência de comutação elevada de 1 MHz a 1,5 MHz, o MP1584 dispensa capacitores eletrolíticos volumosos, permitindo filtragem 100% cerâmica de baixa indutância parasita:
* **Capacitor de Entrada ($C_{\text{in}}$):** Dois capacitores cerâmicos MLCC de **$10\ \mu\text{F} \ / \ 16\text{ V}$ tipo X7R** em paralelo com cerâmico de **$100\text{ nF}$**.
* **Capacitor de Saída ($C_{\text{out}}$):** Dois capacitores cerâmicos MLCC de **$22\ \mu\text{F} \ / \ 10\text{ V}$ tipo X7R** em paralelo com cerâmico de **$100\text{ nF}$**, garantindo ondulação na linha digital inferior a $15\text{ mV}$, essencial para a integridade do rádio e do ADC do processador.

#### 7.3.6 Análise de Dissipação Térmica e Temperatura de Junção
Em regime nominal de corrida, a linha de 3,3 V alimenta o microcontrolador ESP32-C3 (~110 mA), encoders Hall (10 mA) e portas do DRV8833 (3 mA), totalizando $I_{\text{out,nom}} \approx 123\text{ mA} = 0{,}123\text{ A}$:

$$P_{\text{d,nominal}} = (3{,}3\text{ V} \cdot 0{,}123\text{ A}) \cdot \left(\frac{1}{0{,}88} - 1\right) \approx 0{,}406 \cdot 0{,}136 \approx \mathbf{0{,}055\text{ W}}$$

Sob o limite de projeto da Issue (1,0 A contínuo):

$$P_{\text{d,estresse}} = (3{,}3\text{ V} \cdot 1{,}0\text{ A}) \cdot \left(\frac{1}{0{,}88} - 1\right) \approx 3{,}3 \cdot 0{,}136 \approx \mathbf{0{,}449\text{ W}}$$

O circuito integrado MP1584EN possui encapsulamento compacto **SOIC-8E (Exposed Pad)** com ilha metálica inferior soldada à placa de circuito impresso. Com um plano de dissipação de cobre na camada interna/inferior ligado por 4 a 6 vias térmicas, sua resistência térmica atinge $R_{\theta JA} \approx 60\text{ °C/W}$.

* **Elevação Térmica sob Carga Nominal (123 mA):**
  $$\Delta T_{\text{nom}} = 0{,}055\text{ W} \cdot 60\text{ °C/W} \approx 3{,}3\text{ °C} \implies T_j \approx \mathbf{28{,}3\text{ °C}}$$
* **Elevação Térmica sob Limite Contínuo de Projeto (1,0 A):**
  $$\Delta T_{\text{1A}} = 0{,}449\text{ W} \cdot 60\text{ °C/W} \approx 26{,}9\text{ °C} \implies T_j \approx 25 + 26{,}9 = \mathbf{51{,}9\text{ °C}}$$

A temperatura de junção de $51{,}9\text{ °C}$ fica muito aquém da temperatura de desligamento térmico (*Thermal Shutdown*) do MP1584EN ($160\text{ °C}$), assegurando estabilidade em regime contínuo.

---

### 7.4 Tabela de consolidação

A tabela abaixo consolida os parâmetros operacionais, componentes dimensionados e a validação formal frente aos critérios estipulados na issue:

| Critério de Pronto / Parâmetro | Linha +5V0 (Sensor LiDAR ToF) | Linha +3V3 (MCU, Encoders, Driver) | Status de Homologação |
| :--- | :--- | :--- | :---: |
| **CI Regulador Adotado** | **LM2596-ADJ** | **MP1584EN** | Conforme |
| **Topologia / Chaveamento** | Buck bipolar (150 kHz) | Buck MOSFET ($R_{\text{DS(on)}}=180\text{ m}\Omega$, 1,0 MHz) | Conforme |
| **Tensão de Saída ($V_{\text{out}}$)** | $5{,}00\text{ V}$ | $3{,}30\text{ V}$ | Conforme |
| **Tensão de Referência ($V_{\text{ref}}$)**| $1{,}23\text{ V}$ | $0{,}80\text{ V}$ | Conforme |
| **Divisor de Feedback Calculado** | $R_1 = 1{,}0\text{ k}\Omega$, $R_2 = 3{,}06\text{ k}\Omega$ (ou trimpot) | $R_2 = 10\text{ k}\Omega$, $R_1 = 31{,}25\text{ k}\Omega$ ($31{,}6\text{ k}\Omega$ E96) | Conforme |
| **Indutância de Projeto ($L$)** | $47\ \mu\text{H}$ a $68\ \mu\text{H}$ ($I_{\text{sat}} \ge 2\text{ A}$) | $4{,}7\ \mu\text{H}$ a $10\ \mu\text{H}$ ($I_{\text{sat}} \ge 2\text{ A}$) | Conforme |
| **Diodo Schottky de Recirculação** | SS34 / 1N5822 ($40\text{ V} \ / \ 3\text{ A}$) | SS24 / B230A ($40\text{ V} \ / \ 2\text{ A}$) | Conforme |
| **Capacitores de Entrada / Saída**| $C_{\text{in}} = 220\ \mu\text{F}$, $C_{\text{out}} = 330\ \mu\text{F}$ | $C_{\text{in}} = 20\ \mu\text{F}$ MLCC, $C_{\text{out}} = 44\ \mu\text{F}$ MLCC | Conforme |
| **Capacidade Contínua Requerida** | **1,0 A contínuo** | **1,0 A contínuo** | **Atendido (100% de margem)** |
| **Consumo em Regime Nominal** | $150\text{ mA}$ ($0{,}75\text{ W}$) | $123\text{ mA}$ nom. / $350\text{ mA}$ pico ($0{,}41\text{ W}$) | Suportado |
| **Potência Dissipada em 1 A ($P_d$)**| $1{,}10\text{ W}$ | $0{,}45\text{ W}$ | Conforme |
| **Temperatura Junção ($T_j$) sob 1 A**| **$63{,}5\text{ °C}$** ($T_{\text{max}} = 125\text{ °C}$) | **$51{,}9\text{ °C}$** ($T_{\text{max}} = 160\text{ °C}$) | **Estável / Seguro** |
| **Margem Térmica Residual** | **$+61{,}5\text{ °C}$** de margem de segurança | **$+108{,}1\text{ °C}$** de margem de segurança | **Aprovado sem dissipador** |

---

### 7.5 Diagramas Esquemáticos Funcionais dos Módulos Buck

Os esquemáticos a seguir ilustram as interconexões dos nós de potência, malhas de chaveamento e nós de realimentação dimensionados para os dois conversores:


## 8. Referências Bibliográficas

1. **TEXAS INSTRUMENTS.** *LM2596 SIMPLE SWITCHER® Power Converter 150-kHz 3-A Step-Down Voltage Regulator*. Datasheet SNVS124D, Rev. Maio 2020. Disponível em: <https://www.ti.com/lit/ds/symlink/lm2596.pdf>.
2. **MONOLITHIC POWER SYSTEMS (MPS).** *MP1584: 3A, 1.5MHz, 28V Step-Down Converter*. Datasheet Rev. 1.0, 2011. Disponível em: <https://www.monolithicpower.com>.
3. **ESPRESSIF SYSTEMS.** *ESP32-C3-MINI-1 Datasheet*. Versão 1.4, 2024. Disponível em: <https://www.espressif.com>.
4. **TEXAS INSTRUMENTS.** *DRV8833 Dual H-Bridge Motor Driver*. Datasheet SLVSAR1E, Rev. Julho 2015. Disponível em: <https://www.ti.com/lit/ds/symlink/drv8833.pdf>.
5. **GIBILISCO, Stan.** *Beginner's Guide to Reading Schematics*. 3. ed. Nova York: McGraw-Hill Education, 2014.
6. **MOHAN, Ned; UNDELAND, Tore M.; ROBBINS, William P.** *Power Electronics: Converters, Applications, and Design*. 3. ed. Nova York: John Wiley & Sons, 2003.

---

## Histórico de Versão

| Versão | Data | Descrição | Autor(es) | Revisor(es) |
| :---: | :---: | :--- | :--- | :--- |
| 1.0 | 05/10/2026 | Estruturação inicial do consumo de energia e dimensionamento da bateria LiPo 2S | Bruno Duarte e Paulo Melo | Equipe Micromouse |
| 1.1 | 10/10/2026 | Atualização das especificações dinâmicas da bateria LiPo e modelo Tattu 650mAh | Paulo Melo | Bruno Duarte |
| 1.2 | 12/10/2026 | Dimensionamento dos reguladores Step-Down LM2596 e MP1584EN, cálculo de componentes passivos, análise de dissipação térmica | Elias Oliveira | Equipe de Hardware |

