# Análise de Consumo Energético do Produto

## 1. Identificação dos Subsistemas Elétricos

A arquitetura do Micromouse organiza a distribuição de potência e os níveis lógicos a partir da fonte principal. O subsistema elétrico divide-se em barramentos de potência e barramentos de regulação chaveada. O perfil de carga foi levantado com base nas especificações dos componentes e na folha de dados da equipe. Para o dimensionamento da autonomia contínua, padronizou-se o ciclo de missão em **600 segundos (10 minutos)** de exploração e tomada de tempo.

* **ESP32-C3-MINI-1 (MCU e Telemetria):** Opera sob linha regulada de 3,3 V derivada do conversor Buck dedicado. Consumo médio nominal de **110 mA (0,11 A)** em processamento de controle e transmissões periódicas de rádio.
* **Motores DC N20 com Redução (Par de Atuadores M1 e M2):** Tensão nominal de catálogo de 6,0 V. Consumo médio dinâmico em piso de corrida estimado em **0,15 A por motor (0,30 A conjunto)**.
* **Driver Ponte H Dupla (DRV8833 - Lógica de Controle):** Alimentação lógica e pinos de sinal PWM a 3,3 V comutados pelos GPIOs 4, 5, 6 e 7 do microcontrolador. Apresenta consumo quiescente de **3 mA (0,003 A)**.
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
* $t$ = Tempo operacional de missão ($t = 600\text{ s}$)

| Componente | Tensão ($V$) | Corrente Média ($I$) | Tempo ($t$) | Energia Consumida ($E$) |
| :--- | :---: | :---: | :---: | :---: |
| ESP32-C3-MINI-1 | 3,3 V | 0,110 A | 600 s | 217,80 J |
| Motores DC N20 (Par M1 + M2) | 6,0 V | 0,300 A | 600 s | 1.080,00 J |
| Driver DRV8833 (Lógica) | 3,3 V | 0,003 A | 600 s | 5,94 J |
| Encoders Incrementais (Par) | 3,3 V | 0,010 A | 600 s | 19,80 J |
| Sensor LiDAR ToF 360° | 5,0 V | 0,150 A | 600 s | 450,00 J |

---

## 3. Estimativa do Consumo Total de Energia

A demanda energética total calculada para um ciclo de 10 minutos de missão ($E_{\text{total}}$) é obtida pela soma das parcelas energéticas de cada componente:

$$E_{\text{total}} = 217,80 + 1080,00 + 5,94 + 19,80 + 450,00$$

$$E_{\text{total}} = \mathbf{1.773,54\text{ J}}$$

---

## 4. Escolha da Fonte de Alimentação

### Conversão para Watt-hora (Wh)
Utilizando a equivalência padrão ($1\text{ Wh} = 3600\text{ J}$):

$$E_{\text{Wh}} = \frac{E_{\text{total}}}{3600} = \frac{1773,54}{3600} \approx \mathbf{0,493\text{ Wh}}$$

### Margem de Segurança e Eficiência Energética
Para robôs móveis autônomos de labirinto, adota-se uma margem de segurança de **40%**, combinada com um rendimento médio de **85%** ($\eta = 0,85$) dos conversores chaveados Buck da placa. Essa folga de projeto sustenta:
1. **Transientes Dinâmicos:** As partidas, frenagens e rotações rápidas dos motores elevam a corrente momentânea muito acima do patamar nominal de regime permanente.
2. **Perdas Térmicas nos Conversores:** O chaveamento e a resistência parasita das bobinas e diodos internos dos módulos Buck degradam parte da potência útil em calor.
3. **Preservação Química da Célula:** Células à base de lítio sofrem danos de capacidade irreversíveis quando descarregadas abaixo de 20% de sua reserva total.

A energia total demandada na fonte é:

$$E_{\text{necessária}} = \frac{E_{\text{Wh}}}{\eta \cdot (1 - \text{Margem})} = \frac{0,493}{0,85 \cdot (1 - 0,40)} \approx \mathbf{0,967\text{ Wh}}$$

### Especificação da Bateria Selecionada
* **Topologia:** Pacote de bateria recarregável Li-ion / LiPo 2S.
* **Faixa de Tensão:** 7,4 V nominal e 8,4 V em patamar de carga plena.
* **Capacidade Mínima Teórica Requerida:**

$$\text{Capacidade (Ah)} = \frac{E_{\text{necessária}}}{V_{\text{nominal}}} = \frac{0,967\text{ Wh}}{7,4\text{ V}} \approx 0,130\text{ Ah} \implies \mathbf{130\text{ mAh}}$$

* **Modelo Comercial Homologado:** Bateria LiPo 2S de **650 mAh** (modelo **Tattu 650mAh 2S1P 75C / 150C burst** com conector XT-30).
* **Justificativa Técnica:** Essa capacidade garante fornecimento contínuo de pico de até 48,75 A sem colapsar a tensão do barramento (*voltage sag*), assegura peso baixo compatível com o chassi (43 g ±20 g) e estende a autonomia contínua para mais de 65 minutos em regime de corrida, cobrindo com folga de +118% a meta regulamentar de 30 minutos dos 3 labirintos (RNF-04).

### 4.5 Mapeamento Matemático da Curva de Descarga (Issue #139)

O comportamento de descarga de células eletroquímicas à base de Polímero de Lítio (LiPo) não é linear, apresentando três zonas operacionais características: uma queda exponencial pronunciada logo após a plena carga, um extenso platô de tensão relativamente constante e um joelho de descarga profunda com decaimento acentuado de tensão.

Para permitir a simulação do comportamento da bateria no firmware (Issue #215) e o diagnóstico preditivo de carga sem demandar testes destrutivos imediatos de bancada, implementou-se o modelo matemático eletroquímico de **Shepherd modificado** (formalizado por Tremblay e Dessaint):

$$V_{\text{cel}}(Q_{\text{d}}) = E_0 - R \cdot I - K \cdot \left(\frac{Q}{Q - Q_{\text{d}}}\right) \cdot Q_{\text{d}} + A \cdot e^{-B \cdot Q_{\text{d}}}$$

$$V_{\text{pack}}(t) = 2 \cdot V_{\text{cel}}(Q_{\text{d}}(t))$$

Onde:
* $V_{\text{pack}}(t)$: Tensão total nos terminais do pack LiPo 2S (V).
* $E_0$: Tensão constante do patamar eletroquímico por célula ($E_0 = 3{,}740\text{ V}$).
* $R$: Resistência interna série por célula ($R = 15\text{ m}\Omega \implies 30\text{ m}\Omega$ no pack).
* $I$: Corrente média de descarga contínua em regime de missão nominal ($I = 0{,}476\text{ A}$).
* $K$: Coeficiente de polarização na zona de esgotamento ($K = 0{,}085\text{ V}$).
* $Q$: Capacidade nominal total da célula ($Q = 0{,}650\text{ Ah} = 650\text{ mAh}$).
* $Q_{\text{d}}$: Carga acumulada descarregada até o instante $t$ ($Q_{\text{d}} = I \cdot t$).
* $A$: Amplitude da zona exponencial inicial de estabilização pós-carga ($A = 0{,}440\text{ V}$).
* $B$: Constante de capacidade inversa da zona exponencial ($B = 16{,}0\ \text{Ah}^{-1}$).

#### Curva de Descarga Teórica Simulada

O script computacional implementado em `scripts/curva_descarga_lipo.py` modela a resposta dinâmica sob regime de corrida nominal ($476\text{ mA}$) e regime de baixo consumo ($150\text{ mA}$ em *standby* / leitura de sensores), gerando a curva característica apresentada na figura abaixo:

![Curva Teórica de Descarga LiPo 2S](../figs/curva-descarga-lipo-2s.svg)

#### Tabela de Correlação: Estado de Carga (SoC) vs. Tensão de Terminal

A relação entre o Estado de Carga (*State of Charge* - SoC), a tensão sob carga nominal contínua ($I = 476\text{ mA}$) e a energia residual útil do pack Tattu 650 mAh é sumarizada na tabela abaixo:

| Estado de Carga (SoC) | Tensão do Pack 2S ($V_{\text{pack}}$) | Tensão por Célula ($V_{\text{cel}}$) | Capacidade Restante (mAh) | Regime Operacional / Diagnóstico |
| :---: | :---: | :---: | :---: | :--- |
| **100%** | **8,35 V a 8,40 V** | 4,17 V a 4,20 V | 650 mAh | Bateria plena pós-carregamento balanceado |
| **90%**  | 7,78 V | 3,89 V | 585 mAh | Fim da zona de decaimento exponencial inicial |
| **80%**  | 7,57 V | 3,78 V | 520 mAh | Início do platô estável de corrida |
| **70%**  | 7,48 V | 3,74 V | 455 mAh | Platô nominal de regime permanente |
| **60%**  | 7,43 V | 3,72 V | 390 mAh | **Ponto operacional aos 30 min de prova (RNF-04)** |
| **50%**  | 7,39 V | 3,69 V | 325 mAh | Metade da carga útil |
| **40%**  | 7,31 V | 3,66 V | 260 mAh | Operação em corrida contínua segura |
| **30%**  | 7,19 V | 3,59 V | 195 mAh | Alerta amarelo no Dashboard de telemetria |
| **20%**  | 6,92 V | 3,46 V | 130 mAh | Aproximação do joelho de descarga rápida |
| **10%**  | 6,09 V | 3,04 V | 65 mAh | Zona de esgotamento acentuado |
| **0% (Corte UVLO)**| **6,60 V** | **3,30 V** | **~45 mAh (7%)** | **Desarme forçado via software (PWM = 0)** |

#### Integração com a Camada de Software e Firmware

Os coeficientes e a tabela de conversão foram exportados diretamente para o arquivo de cabeçalho C++ `src/firmware/lib/simulacao/TabelaDescargaLiPo.h`. Essa tabela é consumida pela classe de emulação `SensorBateriaMock` (Issue #215), garantindo que:
1. O Dashboard web receba leituras realistas e reproduzíveis de tensão e porcentagem de bateria ao longo do tempo.
2. A máquina de estados de subtensão (*Under-Voltage Lockout* — UVLO) seja testada com antecedência, cortando a tração quando $V_{\text{pack}} \le 6{,}60\text{ V}$.
3. Haja um gabarito teórico formal para validação e cálculo do erro experimental quando a bateria física for submetida ao ensaio de descarga em bancada.

---

## 5. Planejamento do Circuito de Alimentação

A distribuição de energia segrega a malha de acionamento eletromecânico dos circuitos lógicos e de sensoriamento:

* **Entrada de Energia, Gerenciamento e Chaveamento:**
  * O polo positivo da bateria Li-ion 2S passa inicialmente por um módulo **BMS (Battery Management System)**, responsável pelo corte em subtensão, proteção contra curto-circuito e balanceamento de células durante a carga.
  * Em série com a saída do BMS, uma **Chave de Potência** comanda a ligação geral do circuito antes da derivação dos barramentos.
* **Barramento Direto dos Motores (+VBAT_2S / Pinos VM):**
  * O terminal `VM` do driver DRV8833 recebe diretamente a tensão não regulada da bateria (+VBAT_2S, entre 7,4 V e 8,4 V).
  * *Validação da Tensão de 8,4 V em Motores Nominais de 6,0 V:* O DRV8833 suporta até 10,8 V em VM, operando com ampla folga de segurança. Para proteger as bobinas dos micromotores N20 sem a inclusão de um regulador de potência de 6 V, adota-se **limitação por software via modulação PWM**: o *duty cycle* máximo enviado pelo ESP32-C3 é travado em **71%** ($6,0\text{ V} / 8,4\text{ V}$), garantindo que a tensão eficaz nos motores não ultrapasse os 6,0 V nominais sob bateria plena.
* **Barramento de Regulação Chaveada (Dois Módulos Buck Independentes):**
  * **Conversor Buck +5V0:** Regulador Step-Down com capacidade mínima de 1 A contínuo. Alimenta de forma isolada a linha de potência do sensor LiDAR ToF 360°.
  * **Conversor Buck +3V3:** Regulador Step-Down com capacidade mínima de 1 A contínuo. Alimenta a linha digital do microcontrolador ESP32-C3-MINI-1, os encoders de quadratura e a polarização lógica do DRV8833. A especificação de 1 A supre com ampla folga os picos transitórios de rádio do chip ESP32 (de até 350 mA).
* **Controle de Ruído e Estabilidade de Referência (GND Comum):**
  * **Filtragem de Transientes:** Instalação de capacitores eletrolíticos Low-ESR de 220 µF a 470 µF soldados nas proximidades do pino VM do DRV8833, absorvendo o rebote indutivo provocado pelas manobras dos motores.
  * **Desacoplamento de Linha:** Capacitores cerâmicos de 100 nF distribuídos nos terminais de cada circuito integrado e diretamente soldados na carcaça dos motores N20.
  * **Topologia de Referência (GND Único em Estrela):** Todos os módulos compartilham o mesmo potencial de terra, mas as trilhas de retorno de alta corrente (motores e chaveamento) são fisicamente separadas do terra de sinal do MCU e dos sensores, conectando-se em um único nó central junto ao polo negativo do BMS. Isso impede que transitórios induzidos por comutação mecânica gerem flutuações e causem *brownout reset* no processador.

---

## 6. Monitoramento e Condicionamento de Sinal Analógico (Issue #203 e #136)

Para registrar a curva de descarga da célula e diagnosticar a integridade da bateria em tempo de execução sem danificar os pinos analógicos do ESP32-C3 nem sofrer perturbações pelo chaveamento eletromecânico, implementou-se o circuito de atenuação e filtragem analógica passa-baixas anti-aliasing.

### 6.1 Topologia do Circuito de Condicionamento

O circuito conecta-se entre o barramento primário da bateria (+VBAT_2S), o pino analógico dedicado do microcontrolador e a referência comum de terra (GND):

```
       +VBAT_2S (6,60 V a 8,40 V)
             │
            ┌┴┐
            │ │ R1 = 100 kΩ (1% precisão / filme metálico)
            │ │
            └┬┘
             ├─── Nó Analógico (V_ADC) ───► GPIO 2 (ADC1_CH2) do ESP32-C3
             │          │
            ┌┴┐        ───
            │ │ R2     ─── C1 = 100 nF (Cerâmico MLCC 50 V tipo X7R)
            │ │ 33 kΩ   │
            └┬┘         │
             │          │
            ─┴──────────┴─ GND Único do Sistema
```

---

### 6.2 Memorial de Cálculo do Divisor Resistivo ($R_1$ e $R_2$)

1. **Relação de Atenuação Estática ($K_{\text{div}}$):**
   A tensão entregue ao pino analógico em corrente contínua é determinada pela equação do divisor de tensão:

   $$K_{\text{div}} = \frac{R_2}{R_1 + R_2} = \frac{33\text{ k}\Omega}{100\text{ k}\Omega + 33\text{ k}\Omega} = \frac{33}{133} \approx \mathbf{0{,}24812}$$

   $$V_{\text{ADC}} = V_{\text{BAT}} \cdot K_{\text{div}} = V_{\text{BAT}} \cdot 0{,}24812$$

2. **Mapeamento de Tensão nos Níveis Operacionais da Bateria:**
   * **Bateria em Plena Carga ($V_{\text{BAT}} = 8{,}40\text{ V}$):**
     $$V_{\text{ADC,max}} = 8{,}40\text{ V} \cdot 0{,}24812 \approx \mathbf{2{,}084\text{ V}}$$
     *Margem de Proteção:* O valor de $2{,}084\text{ V}$ opera com folga de $0{,}416\text{ V}$ abaixo do teto de máxima linearidade do conversor do ESP32-C3 sob atenuação de 11 dB ($2{,}50\text{ V}$) e com folga de $1{,}216\text{ V}$ em relação ao limite elétrico absoluto do pino ($3{,}30\text{ V}$), eliminando qualquer risco de sobretensão no silício.
   * **Tensão Nominal do Pack ($V_{\text{BAT}} = 7{,}40\text{ V}$):**
     $$V_{\text{ADC,nom}} = 7{,}40\text{ V} \cdot 0{,}24812 \approx \mathbf{1{,}836\text{ V}}$$
   * **Limiar de Corte UVLO ($V_{\text{BAT}} = 6{,}60\text{ V}$):**
     $$V_{\text{ADC,uvlo}} = 6{,}60\text{ V} \cdot 0{,}24812 \approx \mathbf{1{,}638\text{ V}}$$

3. **Função Inversa de Calibração para o Firmware:**
   Para reconstruir no software a tensão real da bateria a partir da amostragem digital do ADC ($V_{\text{ADC}}$):

   $$V_{\text{BAT}} = V_{\text{ADC}} \cdot \left(\frac{R_1 + R_2}{R_2}\right) = V_{\text{ADC}} \cdot \left(\frac{133}{33}\right) = V_{\text{ADC}} \cdot \mathbf{4{,}0303}$$

4. **Corrente de Fuga e Dissipação de Potência:**
   * Corrente drenada no pior caso sob bateria cheia ($8{,}40\text{ V}$):
     $$I_{\text{fuga}} = \frac{8{,}40\text{ V}}{100\text{ k}\Omega + 33\text{ k}\Omega} = \frac{8{,}40\text{ V}}{133\text{ k}\Omega} \approx \mathbf{63{,}16\ \mu\text{A}}$$
     *(Consumo desprezível de 0,063 mA, não descarregando a bateria em repouso).*
   * Potência dissipada máxima nos resistores:
     $$P_{\text{total}} = \frac{(8{,}40\text{ V})^2}{133\text{ k}\Omega} \approx \mathbf{0{,}53\text{ mW}}$$
     $$P_{R1} = (63{,}16\ \mu\text{A})^2 \cdot 100\text{ k}\Omega \approx 0{,}40\text{ mW} \quad \text{e} \quad P_{R2} \approx 0{,}13\text{ mW}$$
     Ambas as dissipações operam a menos de 0,4% da potência nominal de resistores padrão SMD 0805 ou Through-hole de 1/8 W ($125\text{ mW}$), garantindo aquecimento térmico nulo e deriva resistiva desprezível.

---

### 6.3 Memorial de Cálculo do Filtro Anti-Aliasing Passa-Baixas

A comutação PWM da ponte H dos micromotores (operando a $20\text{ kHz}$) e o chaveamento dos reguladores Step-Down (operando a $150\text{ kHz}$) induzem ondulações de alta frequência no barramento elétrico. Se o ADC do microcontrolador amostrar esse sinal sem filtragem, as frequências de chaveamento sofrem falseamento (*aliasing*), gerando oscilações espúrias na leitura que poderiam desarmar indevidamente o sistema de proteção de subtensão.

1. **Impedância Equivalente de Thevenin ($R_{\text{th}}$):**
   A impedância de fonte vista pelo capacitor $C_1$ corresponde à associação em paralelo de $R_1$ e $R_2$:

   $$R_{\text{th}} = R_1 \parallel R_2 = \frac{R_1 \cdot R_2}{R_1 + R_2} = \frac{100\text{ k}\Omega \cdot 33\text{ k}\Omega}{133\text{ k}\Omega} \approx \mathbf{24{,}812\text{ k}\Omega}$$

2. **Frequência de Corte a -3 dB ($f_c$):**
   Adotando capacitor cerâmico multicamadas (MLCC) de $C_1 = 100\text{ nF}$ ($100 \times 10^{-9}\text{ F}$) de alta estabilidade térmica:

   $$f_c = \frac{1}{2\pi \cdot R_{\text{th}} \cdot C_1} = \frac{1}{2\pi \cdot 24.812\ \Omega \cdot 100 \times 10^{-9}\text{ F}} \approx \mathbf{64{,}14\text{ Hz}}$$

3. **Tempo de Resposta Transitória ($t_r$):**
   O tempo de subida (10% a 90%) para o circuito responder a variações em degrau na tensão da bateria é:

   $$t_r = 2{,}2 \cdot R_{\text{th}} \cdot C_1 = 2{,}2 \cdot 24.812\ \Omega \cdot 100 \times 10^{-9}\text{ F} \approx \mathbf{5{,}46\text{ ms}}$$

   Como a curva de descarga química da bateria LiPo ocorre ao longo de dezenas de minutos (taxa de variação $< 1\text{ mV/s}$), o atraso de $5{,}5\text{ ms}$ é desprezível e garante rastreamento instantâneo sem defasagem temporal de telemetria.

---

### 6.4 Comprovação da Rejeição de Ruído e Diagrama de Bode

A função de transferência no domínio da frequência $s = j\omega$ que relaciona a tensão no pino ADC com a tensão da bateria é dada por:

$$H(s) = K_{\text{div}} \cdot \frac{1}{1 + s \cdot R_{\text{th}} \cdot C_1} = 0{,}24812 \cdot \frac{1}{1 + j \left(\frac{f}{64{,}14\text{ Hz}}\right)}$$

A resposta em frequência simulada pelo script `scripts/simulacao_divisor_antialiasing.py` é apresentada no Diagrama de Bode abaixo:

![Diagrama de Bode do Filtro Anti-Aliasing](../figs/bode-filtro-antialiasing.svg)

#### Tabela de Atenuação nas Frequências Críticas

| Ponto de Frequência | Frequência ($f$) | Ganho Relativo (dB) | Fator de Atenuação / Redução | Comportamento do Sinal |
| :--- | :---: | :---: | :---: | :--- |
| **Banda Passante DC** | $0{,}0\text{ Hz}$ | $0{,}00\text{ dB}$ | $1{,}0\times$ | Sinal DC da bateria passa com integridade total |
| **Frequência de Corte ($f_c$)** | $64{,}14\text{ Hz}$ | $-3{,}01\text{ dB}$ | $1{,}41\times$ | Ponto de corte de meia potência |
| **Ondulação de Baixa Frequência** | $120\text{ Hz}$ | $-6{,}53\text{ dB}$ | $2{,}12\times$ | Início da atenuação de primeira ordem |
| **Frequência Intermediária** | $1\text{ kHz}$ | $-23{,}87\text{ dB}$ | $15{,}6\times$ | Rejeição de ruídos de acoplamento |
| **Comutação PWM Motores (DRV8833)** | **$20\text{ kHz}$** | **$-49{,}88\text{ dB}$** | **$311{,}8\times$** | **Ruído atenuado em mais de 310 vezes** |
| **Chaveamento Regulador Buck LM2596**| **$150\text{ kHz}$**| **$-67{,}38\text{ dB}$** | **$2.338{,}5\times$** | **Ruído atenuado em mais de 2.300 vezes** |

---

### 6.5 Rotinas de Aquisição e Proteção no Firmware (Issue #208)

O sinal filtrado entregue ao GPIO 2 é processado pelo firmware através das seguintes etapas sequenciais:
1. **Amostragem com Oversampling:** A tarefa de telemetria realiza rajadas de 10 amostras consecutivas do pino ADC, aplicando média amostral para rejeição de ruído residual de quantização.
2. **Conversão Linear Calibrada:**
   ```cpp
   float tensao_adc = (leitura_raw / 4095.0f) * 2.50f; // Calibração com atenuação de 11 dB
   float tensao_bateria = tensao_adc * 4.0303f;         // Fator inverso do divisor (133k / 33k)
   ```
3. **Máquina de Estados de Subtensão (UVLO):** Se `tensao_bateria <= 6.60f` por mais de 5 amostragens sucessivas (descartando quedas momentâneas de partida), o firmware entra em estado de salvaguarda, forçando o PWM dos motores a zero e emitindo alerta crítico no payload de telemetria (RF08 / HU-08).

---

## 7. Referências Bibliográficas

1. **SHEPHERD, C. M.** *Design of Primary and Secondary Cells - Part 2: An equation describing battery discharge*. Journal of The Electrochemical Society, v. 112, n. 7, p. 657-664, 1965.
2. **TREMBLAY, Olivier; DESSAINT, Louis-A.** *Experimental Validation of a Battery Dynamic Model for EV Applications*. World Electric Vehicle Journal, v. 3, n. 2, p. 289-298, 2009.
3. **TATTU.** *Tattu 650mAh 75C 2S1P Lipo Battery Pack with XT30 Plug Specification Sheet*. Gens Ace & Tattu Battery Co., 2023.
4. **ESPRESSIF SYSTEMS.** *ESP32-C3-MINI-1 Datasheet*. Versão 1.4, 2024. Disponível em: <https://www.espressif.com>.
5. **MOHAN, Ned; UNDELAND, Tore M.; ROBBINS, William P.** *Power Electronics: Converters, Applications, and Design*. 3. ed. Nova York: John Wiley & Sons, 2003.
6. **OPPENHEIM, Alan V.; WILLSKY, Alan S.** *Signals and Systems*. 2. ed. Upper Saddle River: Prentice Hall, 1996.

---

## Histórico de Versão

| Versão | Data | Descrição | Autor(es) | Revisor(es) |
| :---: | :---: | :--- | :--- | :--- |
| 1.0 | 05/10/2026 | Estruturação inicial do consumo de energia e dimensionamento da bateria LiPo 2S | Bruno Duarte e Paulo Melo | Equipe Micromouse |
| 1.1 | 18/10/2026 | Mapeamento matemático da curva de descarga da bateria LiPo 2S (Equação de Shepherd), gráfico vetorial de descarga e geração da tabela SoC para o firmware (#139) | Elias Oliveira | Bruno Duarte (System Architect) |
| 1.2 | 18/10/2026 | Dimensionamento formal do divisor resistivo (100k/33k) e filtro passa-baixas anti-aliasing RC de 64 Hz com comprovação de atenuação a -50 dB do ruído PWM de 20 kHz e Diagrama de Bode (#203) | Elias Oliveira | Bruno Duarte (System Architect) |
