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
