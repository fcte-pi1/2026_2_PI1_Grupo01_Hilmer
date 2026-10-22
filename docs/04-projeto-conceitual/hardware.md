# Projeto Conceitual de _Hardware_

## 1. Visão geral

O _hardware_ do Micromouse integra alimentação, processamento, sensoriamento e acionamento para permitir a navegação autônoma em labirintos. A arquitetura utiliza uma placa **ESP32-C3 Super Mini**, três sensores de distância **VL53L0X**, uma unidade de medição inercial **MPU6050**, um _driver_ de motores **DRV8833** e dois micromotores DC com redução.

A alimentação parte de uma **bateria LiPo 2S**, com 7,4 V nominais e 8,4 V em carga plena. Três conversores **MP1584EN** fornecem barramentos independentes de **3,3 V, 5 V e 6 V**, destinados aos sensores, à placa de controle e ao estágio de potência dos motores, respectivamente.

 O conjunto fornece a base eletrônica para reconhecer paredes, estimar a orientação, controlar o movimento e disponibilizar dados de operação ao sistema de telemetria.

## 2. Diagrama de blocos

O diagrama organiza os subsistemas e identifica as conexões de alimentação e de sinais descritas no relatório técnico e no esquemático.

```mermaid
flowchart TB
    BAT["BT1 — Bateria LiPo 2S<br/>7,4 V nominal / 8,4 V máxima"]
    F["F1 — Fusível de entrada"]
    SW["SW1 — Chave geral"]
    VBAT["Barramento VBAT"]
    B3["U5 — MP1584EN<br/>Saída de 3,3 V"]
    B5["U2 — MP1584EN<br/>Saída de 5 V"]
    B6["U4 — MP1584EN<br/>Saída de 6 V"]
    MCU["ESP32-C3 Super Mini<br/>Controle e firmware"]
    TOF["J3, J4 e J5<br/>Três sensores VL53L0X"]
    IMU["J6 — MPU6050<br/>Acelerômetro e giroscópio"]
    I2C["Barramento I2C<br/>SDA: GPIO6 / SCL: GPIO7"]
    DRV["DRV8833<br/>Ponte H dupla"]
    ML["J1 — Motor esquerdo"]
    MR["J2 — Motor direito"]
    ADC["R1: 100 kΩ / R2: 39 kΩ<br/>Filtro C5: 100 nF"]

    BAT --> F --> SW --> VBAT
    VBAT --> B3
    VBAT --> B5
    VBAT --> B6
    B3 -->|Alimentação| TOF
    B3 -->|Alimentação| IMU
    B5 -->|Pino 5V da placa| MCU
    B6 -->|Potência: 6 V| DRV
    VBAT --> ADC -->|GPIO2 — ADC| MCU
    MCU <-->|Dados e clock| I2C
    I2C <--> TOF
    I2C <--> IMU
    MCU -->|PWM: GPIO0, GPIO1, GPIO10 e GPIO21| DRV
    DRV --> ML
    DRV --> MR
```

**Figura 1.** Visão geral do hardware do Micromouse. Todos os estágios compartilham a referência elétrica **GND**.

## 3. Subsistemas e justificativas de projeto

### 3.1 Alimentação e proteção de entrada

A bateria **BT1**, do tipo LiPo 2S, fornece energia ao robô. Sua linha positiva passa pelo **fusível F1** e pela **chave geral SW1** antes de formar o barramento de distribuição `VBAT`. O fusível protege a entrada contra sobrecorrente, enquanto a chave permite interromper a alimentação do conjunto.

O uso de uma fonte recarregável permite a operação móvel e autônoma. A distribuição em três conversores reduz a tensão da bateria para os níveis utilizados por cada subsistema e permite organizar separadamente as cargas de potência e os circuitos de sensoriamento e controle.

| Estágio | Componente | Saída | Destino | Filtragem indicada no esquemático |
| :--- | :--- | :---: | :--- | :--- |
| U5 | MP1584EN | **3,3 V** | Três VL53L0X e MPU6050 | C8 de 10 µF e C7 de 100 nF. |
| U2 | MP1584EN | **5 V** | Pino `5V` da placa ESP32-C3 | C6 de 10 µF e C2 de 100 µF. |
| U4 | MP1584EN | **6 V** | Alimentação de potência do DRV8833 | C3 de 10 µF e C4 de 100 µF. |

Os capacitores associados aos conversores auxiliam na filtragem e no atendimento às variações de carga. A alimentação dedicada dos motores busca reduzir a influência de suas comutações sobre os sensores e o microcontrolador.

### 3.2 Unidade central de controle

A **ESP32-C3 Super Mini** executa o processamento embarcado e coordena os periféricos. O barramento de 5 V alimenta a entrada correspondente da placa, enquanto as interfaces digitais realizam a comunicação I²C e o comando da ponte H.

Sua função no projeto compreende adquirir as distâncias e as informações inerciais, identificar paredes, estimar o ângulo de guinada e calcular os comandos de movimento. O relatório prevê controle PID para alinhamento e navegação e geração de sinais PWM para ajustar a velocidade dos motores.

A escolha reúne processamento, periféricos de comunicação e conectividade em uma placa compacta, adequada à integração no chassi do Micromouse.

### 3.3 Sensores de distância VL53L0X

O sensoriamento de paredes utiliza **três módulos VL53L0X**, representados pelos conectores **J3, J4 e J5**. A distribuição funcional é frontal, esquerda e direita, permitindo reconhecer obstáculos e passagens nas direções relevantes à navegação.

Os sensores utilizam medição por tempo de voo (_Time of Flight_ — ToF), recebem alimentação de **3,3 V** e compartilham o barramento **I²C**. Essa configuração concentra a comunicação em duas linhas: **SDA no GPIO6** e **SCL no GPIO7**.

O gerenciamento de endereços dos três sensores faz parte da inicialização prevista no projeto, por meio do controle de habilitação dos módulos e da atribuição de endereços individuais. A aquisição das três distâncias fornece a referência para detectar paredes e manter o alinhamento nos corredores.

### 3.4 Unidade de medição inercial MPU6050

A **MPU6050**, representada pelo conector **J6**, reúne acelerômetro e giroscópio de seis eixos. Sua função é fornecer informações de aceleração e taxa de rotação para estimar a orientação do robô e apoiar a correção de desvios.

O módulo recebe **3,3 V** e utiliza o mesmo barramento I²C dos sensores de distância. A combinação de medidas inerciais e distâncias permite apoiar tanto o deslocamento em linha reta quanto a execução de curvas de 90°.

### 3.5 Acionamento dos motores

O **DRV8833** é o estágio de potência entre o ESP32-C3 e os dois motores DC com redução. A ponte H dupla permite controlar separadamente o sentido de rotação e a velocidade de cada motor a partir de sinais PWM.

Sua alimentação de potência utiliza o barramento regulado de **6 V**. O esquemático também apresenta o resistor **R4 de 47 kΩ** no circuito de habilitação do módulo. As saídas **OUT1/OUT2** acionam o motor esquerdo, conectado a **J1**, e **OUT3/OUT4** acionam o motor direito, conectado a **J2**.

O controle independente dos motores permite tração diferencial: a combinação de velocidades e sentidos produz avanço, recuo e curvas. O driver fornece a interface de corrente necessária aos motores, preservando a função dos GPIOs como sinais de comando.

### 3.6 Monitoramento da tensão da bateria

O circuito de monitoramento utiliza **R1 de 100 kΩ**, **R2 de 39 kΩ** e **C5 de 100 nF**. O divisor reduz a tensão de `VBAT` para a entrada analógica do ESP32, enquanto o capacitor filtra transientes no nó de leitura.

A relação entre as tensões é:

```text
V_ADC = V_BAT × R2 / (R1 + R2)
V_ADC = V_BAT × 39 / 139
```

Com a bateria em **8,4 V**, a tensão calculada no nó de medição é de aproximadamente **2,357 V**. Esse nó é conectado ao **GPIO2**, conforme o esquemático principal. A leitura permite acompanhar a tensão da fonte e fornecer dados sobre a condição da bateria ao firmware.

## 4. Esquemático elétrico

O esquemático principal apresenta os símbolos elétricos, as referências dos componentes, as conexões de alimentação e sinais e os terminais dos módulos. A figura abaixo foi extraída do PDF disponível na pasta `hw`.

[![Esquemático elétrico do Micromouse](../figs/hardware-esquematico.png)](../../hw/Micromouse.pdf)

**Figura 2.** Esquemático elétrico do Micromouse, com bateria, fusível, chave, três conversores MP1584EN, ESP32-C3, sensores VL53L0X, MPU6050, DRV8833 e motores. O PDF permite ampliar a imagem para consultar os detalhes das conexões.

### 4.1 Conexões principais

| Origem | Destino | Função |
| :--- | :--- | :--- |
| BT1, terminal positivo | F1 → SW1 → VBAT | Entrada protegida por fusível e comando geral. |
| VBAT | Entradas dos conversores U2, U4 e U5 | Distribuição de energia. |
| U2, saída de 5 V | Pino `5V` da placa ESP32-C3 | Alimentação da unidade de controle. |
| U5, saída de 3,3 V | J3, J4, J5 e J6 | Alimentação dos sensores de distância e da IMU. |
| U4, saída de 6 V | `VCC` de potência do módulo DRV8833 | Alimentação do acionamento. |
| GPIO6 e GPIO7 | SDA e SCL dos sensores | Comunicação I²C compartilhada. |
| GPIO0 e GPIO1 | IN1 e IN2 do DRV8833 | Comandos do motor esquerdo. |
| GPIO10 e GPIO21 | IN3 e IN4 do DRV8833 | Comandos do motor direito. |
| DRV8833, OUT1/OUT2 | J1, motor esquerdo | Saída de potência. |
| DRV8833, OUT3/OUT4 | J2, motor direito | Saída de potência. |
| VBAT → R1/R2 e C5 | GPIO2 | Medição analógica da bateria. |
| Terras dos módulos | GND comum | Referência elétrica do conjunto. |

### 4.2 Pinagem do ESP32-C3

A tabela identifica os **GPIOs lógicos**, os sinais e suas funções no circuito. A identificação física dos terminais deve acompanhar a placa empregada na montagem.

| GPIO / terminal | Periférico | Sinal | Função |
| :---: | :--- | :--- | :--- |
| **GPIO0** | DRV8833 | IN1 — motor esquerdo | Saída de controle PWM. |
| **GPIO1** | DRV8833 | IN2 — motor esquerdo | Saída de controle PWM. |
| **GPIO10** | DRV8833 | IN3 — motor direito | Saída de controle PWM. |
| **GPIO21 / TX** | DRV8833 | IN4 — motor direito | Saída de controle PWM. |
| **GPIO6** | VL53L0X e MPU6050 | SDA | Dados do barramento I²C. |
| **GPIO7** | VL53L0X e MPU6050 | SCL | Clock do barramento I²C. |
| **GPIO2** | Divisor R1/R2 | ADC de bateria | Entrada analógica de tensão. |
| **5V** | Conversor U2 | Alimentação da placa | Entrada de 5 V regulados. |
| **GND** | Todos os subsistemas | Terra | Referência comum. |

## 5. Integração e sequência de montagem

A montagem organiza a bateria, os conversores, a placa de controle e o driver no chassi, com os motores nas laterais. Os sensores de distância devem manter seus campos de medição desobstruídos nas direções frontal e laterais, e a IMU deve permanecer fixada à estrutura para acompanhar o movimento do robô.

O relatório técnico estabelece a seguinte sequência de montagem e verificação:

1. Inspecionar as ligações e verificar a ausência de curtos entre VBAT, os barramentos regulados e GND.
2. Ajustar os conversores individualmente para **3,3 V, 5 V e 6 V**, antes de conectar as cargas.
3. Verificar o fusível e a interrupção da alimentação pela chave geral.
4. Conectar a placa ESP32-C3 e verificar alimentação e gravação de firmware básico via USB.
5. Executar uma varredura I²C nos GPIO6 e GPIO7 e verificar a identificação dos sensores.
6. Testar os motores com as rodas suspensas, aplicando PWM progressivo e verificando o sentido de rotação.
7. Integrar a aquisição de distâncias, as medidas inerciais e o controle de movimento.

## 6. Componentes

| Componente / conjunto | Quantidade | Referência no circuito |
| :--- | :---: | :--- |
| Bateria LiPo 2S | 1 | BT1. |
| Fusível de entrada | 1 | F1. |
| Chave geral | 1 | SW1. |
| Conversor MP1584EN de 5 V | 1 | U2. |
| Conversor MP1584EN de 3,3 V | 1 | U5. |
| Conversor MP1584EN de 6 V | 1 | U4. |
| Placa ESP32-C3 | 1 | U3. |
| Sensor de distância VL53L0X | 3 | J3, J4 e J5. |
| IMU MPU6050 | 1 | J6. |
| Módulo DRV8833 | 1 | `U4-DRV8833_Module1`. |
| Motor DC com redução | 2 | J1 e J2. |
| Resistores de monitoramento | 2 | R1: 100 kΩ; R2: 39 kΩ. |
| Resistor de habilitação do driver | 1 | R4: 47 kΩ. |
| Capacitores de filtragem | 7 | C2 e C4: 100 µF; C3, C6 e C8: 10 µF; C5 e C7: 100 nF. |

