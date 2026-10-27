# Justificativa dos Componentes de _Hardware_

_Documento de apresentação — função, motivo de utilização e pontos de validação técnica._

## 1. Objetivo

Este documento apresenta a função de cada componente previsto para o _hardware_ do Micromouse PI1 e justifica sua utilização dentro da arquitetura definida. A organização segue a arquitetura apresentada no documento-base: energia, controle, atuação, sensoriamento e comunicação.

> **Importante:** alguns parâmetros ainda aparecem como **"A DEFINIR / VALIDAR"** no projeto original. Eles são mantidos como pendências neste documento, em vez de serem assumidos como valores já aprovados.

---

## 2. Visão geral da arquitetura

| Bloco | Componentes principais | Função no sistema |
| :--- | :--- | :--- |
| **Energia** | Bateria LiPo 2S + fusível + chave de potência | Armazenar, proteger e distribuir a energia. |
| **Regulação** | Conversores MP1584EN de +5 V, +3,3 V e +6 V | Gerar tensões reguladas para os diferentes circuitos. |
| **Controle** | ESP32-C3 Super Mini | Executar o controle, gerar PWM e processar os sensores. |
| **Atuação** | DRV8833 + 2 motores | Converter os sinais de controle em acionamento dos motores. |
| **Sensoriamento** | Três VL53L0X + MPU6050 + encoders previstos | Medir distâncias, aceleração, taxa de rotação e movimento das rodas. |
| **Referência elétrica** | GND comum | Garantir uma referência comum entre os circuitos. |

---

## 3. Componentes e justificativas

### 3.1 Bateria LiPo 2S

A bateria é a fonte primária de energia do Micromouse. A arquitetura define uma bateria LiPo 2S, com **7,4 V nominal** e **8,4 V máximo**.

- **Por que usar:** fornece a energia necessária para o sistema móvel, incluindo os motores e a eletrônica.
- **Por que 2S:** a arquitetura foi concebida em torno de uma alimentação de 7,4 V nominal, com possibilidade de atingir 8,4 V quando totalmente carregada.
- **Função no sistema:** alimenta o barramento `VBAT`, após o fusível e a chave de potência, que segue para os três conversores Buck.

### 3.2 Fusível F1

O fusível F1 é colocado na linha positiva da bateria, antes da chave de potência.

- **Por que usar:** protege a entrada de alimentação contra sobrecorrente.
- **Função no sistema:** integra o fluxo Bateria 2S → Fusível F1 → Chave de potência → distribuição.
- **Validação:** verificar a compatibilidade da proteção com as correntes de operação e de partida dos motores.

### 3.3 Chave de potência

A chave de potência é usada para permitir o acionamento e desligamento geral do Micromouse.

- **Por que usar:** permite interromper a alimentação do sistema de maneira simples e centralizada.
- **Função no sistema:** fica após o fusível F1 e antes da distribuição para os três conversores Buck.
- **Parâmetro pendente:** a corrente nominal da chave aparece como "a definir".

### 3.4 Buck Converter +5 V

O conversor Buck MP1584EN de +5 V (U2) reduz a tensão do barramento da bateria para uma alimentação regulada de 5 V.

- **Por que usar:** a bateria pode chegar a 8,4 V, enquanto determinados módulos/sensores podem exigir uma alimentação regulada diferente da tensão da bateria.
- **Função no sistema:** fornece 5 V ao pino `5V` da placa ESP32-C3 Super Mini.
- **Validação:** confirmar a tensão de saída e a corrente necessária para a placa de controle.

### 3.5 Buck Converter +3,3 V

O conversor Buck MP1584EN de +3,3 V (U5) gera a alimentação regulada dos sensores.

- **Por que usar:** separar a tensão da eletrônica da tensão variável da bateria.
- **Função no sistema:** fornece 3,3 V aos três sensores VL53L0X e à MPU6050.
- **Parâmetro pendente:** a corrente necessária do conversor deve ser dimensionada a partir do consumo real dos sensores.

### 3.6 Buck Converter +6 V

O conversor Buck MP1584EN de +6 V (U4) gera a alimentação regulada do estágio de potência dos motores.

- **Por que usar:** adequa a tensão variável da bateria à alimentação do acionamento dos motores.
- **Função no sistema:** fornece 6 V ao estágio de potência do DRV8833.
- **Validação:** confirmar a tensão de saída e a capacidade de corrente durante a partida e a operação dos motores.

### 3.7 ESP32-C3 Super Mini

A ESP32-C3 Super Mini é a placa de controle da arquitetura apresentada.

- **Por que usar:** centraliza o processamento e o controle do robô.
- **Controle dos motores:** fornece quatro sinais de controle ao DRV8833, usando PWM.
- **Sensoriamento:** comunica-se com os três sensores VL53L0X e com a MPU6050 pelo barramento I²C.
- **Encoders:** prevê a leitura dos sinais A/B por GPIO para realimentação do movimento.
- **Função geral:** executar a lógica de navegação/controle e coordenar atuação e aquisição de sensores.

### 3.8 DRV8833

O DRV8833 é o estágio de potência/driver entre o microcontrolador e os dois motores.

- **Por que usar:** o ESP32 não deve acionar diretamente os motores; o driver fornece a interface de potência necessária.
- **Topologia:** o documento identifica o DRV8833 como uma ponte H dupla, com quatro entradas de controle e quatro saídas, duas por motor.
- **Controle:** recebe os sinais PWM do ESP32 e controla os motores M1 e M2.
- **Alimentação:** recebe 6 V regulados do conversor Buck U4 para o acionamento dos motores.

### 3.9 Motores DC — M1 e M2

O documento-base especifica dois micromotores DC com redução, conectados a J1 e J2 e acionados pelo estágio de potência de **6 V**.

- **Por que usar dois motores:** permitem a locomoção diferencial do Micromouse, com controle independente dos lados.
- **Controle:** cada motor é conectado a uma ponte H do DRV8833.
- **Feedback previsto:** encoders incrementais permitem medir o movimento dos motores para realimentar o controle.
- **Validação obrigatória:** confirmar a tensão nominal e as correntes de operação e de partida dos motores, assim como sua compatibilidade com o driver.

### 3.10 Encoders incrementais em quadratura

Os encoders fornecem realimentação do movimento das rodas/motores e estão previstos para integração ao projeto.

- **Por que usar:** permitem ao microcontrolador acompanhar deslocamento e sentido de rotação por sinais de quadratura A/B.
- **Conexões previstas:** encoder esquerdo nos `GPIO4` e `GPIO5`; encoder direito nos `GPIO6` e `GPIO7`, conforme as reservas do projeto conceitual de hardware (Rev 2.0). No protótipo em protoboard de 23/10/2026, que usa o I²C nos GPIO6/7, o encoder esquerdo foi ligado nos `GPIO3` e `GPIO4`, com o direito reservado nos `GPIO5` e `GPIO20` (ver [testes de hardware](../07-testes/hardware.md)).
- **Função no sistema:** fornecer feedback para controle de velocidade, distância e movimento do robô.
- **Validação:** verificar os níveis lógicos e a necessidade/valores dos _pull-ups_; os encoders ainda não estão instalados na Rev 2.0.

### 3.11 Sensores de distância VL53L0X

Os três sensores VL53L0X medem a distância até as paredes nas direções frontal, esquerda e direita.

- **Por que usar:** fornecem medições por tempo de voo (ToF) para detectar paredes e auxiliar o alinhamento no labirinto.
- **Comunicação:** compartilham o barramento I²C com a MPU6050.
- **Função no sistema:** recebem 3,3 V do conversor U5 e são representados pelos conectores J3, J4 e J5.
- **Validação:** verificar a inicialização pelos pinos XSHUT e a atribuição de endereços I²C distintos para evitar conflitos entre os três sensores.

### 3.12 Unidade de medição inercial MPU6050

A MPU6050 reúne acelerômetro e giroscópio de seis eixos para acompanhar o movimento do robô.

- **Por que usar:** fornece aceleração e taxa de rotação para estimar a orientação e auxiliar a correção de desvios e as curvas de 90°.
- **Comunicação:** utiliza o mesmo barramento I²C dos sensores VL53L0X.
- **Função no sistema:** recebe 3,3 V do conversor U5 e é representada pelo conector J6.
- **Validação:** verificar o endereço I²C, a calibração e a resposta das medidas durante o movimento.

### 3.13 Resistores R1/R2 e capacitor C5 — monitoramento da bateria

Os resistores R1/R2 formam um divisor de tensão da bateria, associado ao capacitor C5 de 100 nF.

- **Por que usar:** reduzem a tensão da bateria para permitir seu monitoramento; C5 filtra ruídos no ponto de medição.
- **Função no sistema:** disponibilizar uma tensão proporcional a `VBAT` para acompanhar a tensão da bateria.
- **Validação:** conferir os valores de R1/R2 no esquemático vigente, a tensão resultante e a conexão à entrada ADC, ainda pendente na Rev 2.0.

### 3.14 GND comum

A arquitetura define uma referência única de terra para o sistema.

- **Por que usar:** os sinais de controle, I²C e sensores precisam compartilhar uma referência elétrica comum.
- **Função:** conectar a referência de GND dos conversores, ESP32, DRV8833, VL53L0X, MPU6050 e demais circuitos.
- **Resultado esperado:** reduzir problemas de referência de sinal e garantir que os níveis lógicos sejam interpretados corretamente.

---

## 4. Por que a arquitetura separa as tensões?

A arquitetura utiliza três conversores Buck para gerar 3,3 V para os sensores, 5 V para a placa ESP32-C3 Super Mini e 6 V para o estágio de potência dos motores. Isso permite que cada subsistema receba a tensão adequada e reduz a influência das variações de carga dos motores sobre os sensores e o controle.

**Fluxo de energia previsto:** Bateria LiPo 2S → Fusível F1 → chave de potência → distribuição → (Buck +5 V / Buck +3,3 V / Buck +6 V).

---

## 5. Por que o ESP32 não é ligado diretamente aos motores?

O ESP32 atua como controlador e fornece sinais digitais/PWM. O DRV8833 funciona como estágio de acionamento dos motores, recebendo os comandos do microcontrolador e fornecendo as saídas para os motores esquerdo e direito. Essa separação utiliza quatro sinais de controle entre a ESP32-C3 Super Mini e as quatro entradas do DRV8833.

---

## 6. Pontos que ainda precisam ser validados

| Item | O que validar |
| :--- | :--- |
| **Motores** | Tensão nominal, corrente de operação/pico e compatibilidade com o barramento regulado de 6 V. |
| **DRV8833** | Faixa de tensão de `VM` e compatibilidade com a tensão/corrente dos motores. |
| **Buck +5 V** | Corrente total exigida pela placa ESP32-C3 Super Mini. |
| **Buck +3,3 V** | Corrente total exigida pelos três VL53L0X e pela MPU6050. |
| **Buck +6 V** | Capacidade de corrente para o driver e os dois motores. |
| **VL53L0X** | Inicialização por XSHUT, endereços I²C distintos e leituras de distância. |
| **MPU6050** | Endereço I²C, calibração e resposta do acelerômetro e giroscópio. |
| **Encoders** | Integração dos sinais A/B, níveis lógicos e necessidade/valores dos _pull-ups_. |
| **R1/R2 e C5** | Valores no esquemático vigente, tensão de medição e conexão ao ADC. |
| **Chave de potência** | Corrente nominal compatível com o consumo máximo do sistema. |
| **Fusível F1** | Compatibilidade com o consumo do sistema e a corrente de partida dos motores. |

---

## 7. Resumo para apresentação

| Componente | Justificativa curta para apresentação |
| :--- | :--- |
| **Bateria LiPo 2S** | Fonte de energia principal: 7,4 V nominal e 8,4 V máximo. |
| **Fusível F1** | Proteção contra sobrecorrente na entrada da bateria. |
| **Chave de potência** | Liga/desliga a alimentação geral do robô. |
| **Buck 5 V** | Alimenta o pino 5V da placa ESP32-C3 Super Mini. |
| **Buck 3,3 V** | Alimenta os três VL53L0X e a MPU6050. |
| **Buck 6 V** | Alimenta o estágio de potência do DRV8833. |
| **ESP32-C3 Super Mini** | Cérebro do robô: processa sensores e gera os comandos PWM. |
| **DRV8833** | Driver de potência: faz a interface entre o ESP32 e os dois motores. |
| **2 motores** | Executam a movimentação do Micromouse. |
| **Encoders** | Previstos para medir o movimento por quadratura A/B e fornecer realimentação ao controle. |
| **Três VL53L0X** | Medem as distâncias frontal e laterais para detectar paredes e auxiliar o alinhamento. |
| **MPU6050** | Mede aceleração e taxa de rotação para apoiar a orientação e as curvas. |
| **R1/R2 e C5** | Reduzem e filtram a tensão da bateria para monitoramento. |
| **GND comum** | Mantém uma referência elétrica comum para alimentação e sinais. |

---