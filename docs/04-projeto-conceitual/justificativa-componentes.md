# Justificativa dos Componentes de _Hardware_

!!! warning "Documento da arquitetura v1"
    Este documento descreve a arquitetura v1 do _hardware_ (setembro de 2026) e não foi atualizado. A arquitetura atual está no [Projeto Conceitual de _Hardware_](hardware.md) (Rev 2.0).

_Documento de apresentação — função, motivo de utilização e pontos de validação técnica._

## 1. Objetivo

Este documento apresenta a função de cada componente previsto para o _hardware_ do Micromouse PI1 e justifica sua utilização dentro da arquitetura definida. A organização segue a arquitetura apresentada no documento-base: energia, controle, atuação, sensoriamento e comunicação.

> **Importante:** alguns parâmetros ainda aparecem como **"A DEFINIR / VALIDAR"** no projeto original. Eles são mantidos como pendências neste documento, em vez de serem assumidos como valores já aprovados.

---

## 2. Visão geral da arquitetura

| Bloco | Componentes principais | Função no sistema |
| :--- | :--- | :--- |
| **Energia** | Bateria Li-ion 2S + BMS + chave de potência | Armazenar, proteger e distribuir a energia. |
| **Regulação** | Buck +5 V e Buck +3,3 V | Gerar tensões reguladas para os diferentes circuitos. |
| **Controle** | ESP32-C3-MINI-1 | Executar o controle, gerar PWM e processar os sensores. |
| **Atuação** | DRV8833 + 2 motores | Converter os sinais de controle em acionamento dos motores. |
| **Sensoriamento** | Encoders + LiDAR | Obter informação de movimento e percepção do ambiente. |
| **Referência elétrica** | GND comum | Garantir uma referência comum entre os circuitos. |

---

## 3. Componentes e justificativas

### 3.1 Bateria Li-ion 2S

A bateria é a fonte primária de energia do Micromouse. A arquitetura define uma bateria Li-ion 2S, com **7,4 V nominal** e **8,4 V máximo**.

- **Por que usar:** fornece a energia necessária para o sistema móvel, incluindo os motores e a eletrônica.
- **Por que 2S:** a arquitetura foi concebida em torno de uma alimentação de 7,4 V nominal, com possibilidade de atingir 8,4 V quando totalmente carregada.
- **Função no sistema:** alimenta o barramento `+VBAT_2S`, que segue para o driver dos motores e para os conversores Buck.

### 3.2 BMS

O BMS (_Battery Management System_) é colocado entre a bateria e o restante do sistema.

- **Por que usar:** faz parte do gerenciamento/proteção do conjunto de células da bateria.
- **Função no sistema:** o diagrama indica o fluxo Bateria 2S → BMS → Chave de potência → distribuição.
- **Observação:** o documento-base não especifica o modelo do BMS nem seus parâmetros; portanto, esses dados ainda precisam ser definidos.

### 3.3 Chave de potência

A chave de potência é usada para permitir o acionamento e desligamento geral do Micromouse.

- **Por que usar:** permite interromper a alimentação do sistema de maneira simples e centralizada.
- **Função no sistema:** fica após o BMS e antes da distribuição para os Buck e para o driver.
- **Parâmetro pendente:** a corrente nominal da chave aparece como "a definir".

### 3.4 Buck Converter +5 V

O conversor Buck de +5 V reduz a tensão do barramento da bateria para uma alimentação regulada de 5 V.

- **Por que usar:** a bateria pode chegar a 8,4 V, enquanto determinados módulos/sensores podem exigir uma alimentação regulada diferente da tensão da bateria.
- **Função no sistema:** fornece o barramento `+5V0`, destinado principalmente ao subsistema de sensoriamento indicado no documento.
- **Validação:** a corrente necessária e a compatibilidade/alimentação do LiDAR precisam ser confirmadas pelo _datasheet_.

### 3.5 Buck Converter +3,3 V

O segundo Buck gera o barramento regulado de +3,3 V para a eletrônica de controle.

- **Por que usar:** separar a tensão da eletrônica da tensão variável da bateria.
- **Função no sistema:** fornece `+3V3` para a placa de controle/ESP32.
- **Parâmetro pendente:** a corrente necessária do conversor deve ser dimensionada a partir do consumo real da placa e dos periféricos.

### 3.6 ESP32-C3-MINI-1

O ESP32-C3-MINI-1 é o microcontrolador da arquitetura apresentada.

- **Por que usar:** centraliza o processamento e o controle do robô.
- **Controle dos motores:** o documento atribui `GPIO4`, `GPIO5`, `GPIO6` e `GPIO7` às quatro entradas de controle do DRV8833, usando PWM.
- **Sensoriamento:** recebe os sinais dos encoders por GPIO e se comunica com o LiDAR por UART.
- **Função geral:** executar a lógica de navegação/controle e coordenar atuação e aquisição de sensores.

### 3.7 DRV8833

O DRV8833 é o estágio de potência/driver entre o microcontrolador e os dois motores.

- **Por que usar:** o ESP32 não deve acionar diretamente os motores; o driver fornece a interface de potência necessária.
- **Topologia:** o documento identifica o DRV8833 como uma ponte H dupla, com quatro entradas de controle e quatro saídas, duas por motor.
- **Controle:** recebe os sinais PWM do ESP32 e controla os motores M1 e M2.
- **Ponto crítico:** o projeto precisa validar a compatibilidade entre o barramento de até 8,4 V, o driver e os motores, que aparecem como 6 V nominais.

### 3.8 Motores DC — M1 e M2

O documento-base especifica dois motores, M1 e M2, apresentados como motores de **6 V nominais**.

- **Por que usar dois motores:** permitem a locomoção diferencial do Micromouse, com controle independente dos lados.
- **Controle:** cada motor é conectado a uma ponte H do DRV8833.
- **Feedback:** os motores são associados a encoders incrementais, permitindo medir o movimento.
- **Validação obrigatória:** corrente e tensão nominal dos motores ainda aparecem como itens a definir/validar, assim como a segurança de operar com o barramento de 8,4 V.

### 3.9 Encoders incrementais em quadratura

Os encoders fornecem realimentação do movimento das rodas/motores.

- **Por que usar:** permitem ao microcontrolador acompanhar deslocamento e sentido de rotação por sinais de quadratura A/B.
- **Conexões indicadas:** encoder esquerdo nos `GPIO0` e `GPIO1`; encoder direito nos `GPIO3` e `GPIO10`.
- **Função:** fornecer feedback para controle de velocidade, distância e movimento do robô.
- **Validação:** o documento solicita verificar a necessidade e os valores dos _pull-ups_ dos encoders.

### 3.10 LiDAR

O LiDAR é o sensor de percepção do ambiente indicado no projeto.

- **Por que usar:** fornece medições de distância para auxiliar a percepção das paredes/obstáculos do labirinto.
- **Comunicação:** a arquitetura prevê comunicação serial UART entre o LiDAR e o microcontrolador.
- **Conexões indicadas:** `GPIO20/RX` e `GPIO21/TX` aparecem associados à UART do LiDAR.
- **Validação:** protocolo UART, alimentação, níveis lógicos e compatibilidade precisam ser confirmados no _datasheet_ do modelo escolhido.

### 3.11 Resistores R1/R2 — condicionamento da UART

O diagrama apresenta R1/R2 de forma condicional no caminho do sinal do LiDAR.

- **Por que podem ser necessários:** podem ser usados para adequar/condicionar níveis de sinal caso os níveis elétricos do LiDAR e do ESP32 não sejam diretamente compatíveis.
- **Importante:** o documento não define os valores de R1/R2.
- **Critério:** somente devem ser dimensionados após verificar os níveis UART TTL do LiDAR e do ESP32 no _datasheet_.

### 3.12 GND comum

A arquitetura define uma referência única de terra para o sistema.

- **Por que usar:** os sinais de controle, UART e sensores precisam compartilhar uma referência elétrica comum.
- **Função:** conectar a referência de GND dos conversores, ESP32, DRV8833, LiDAR e demais circuitos.
- **Resultado esperado:** reduzir problemas de referência de sinal e garantir que os níveis lógicos sejam interpretados corretamente.

---

## 4. Por que a arquitetura separa as tensões?

A arquitetura não utiliza a bateria diretamente para toda a eletrônica. O barramento da bateria é mantido para o estágio de potência dos motores, enquanto conversores Buck geram tensões reguladas para a eletrônica. Isso permite que cada subsistema receba a tensão adequada, evitando alimentar diretamente o ESP32 com a tensão variável da bateria.

**Fluxo de energia previsto:** Bateria Li-ion 2S → BMS → chave de potência → distribuição → (Buck +5 V / Buck +3,3 V / DRV8833).

---

## 5. Por que o ESP32 não é ligado diretamente aos motores?

O ESP32 atua como controlador e fornece sinais digitais/PWM. O DRV8833 funciona como estágio de acionamento dos motores, recebendo os comandos do microcontrolador e fornecendo as saídas para M1 e M2. Essa separação é representada no diagrama da página 3 do documento-base, com quatro sinais PWM entre o ESP32-C3-MINI-1 e as quatro entradas do DRV8833.

---

## 6. Pontos que ainda precisam ser validados

| Item | O que validar |
| :--- | :--- |
| **Motores** | Tensão nominal, corrente de operação/pico e compatibilidade com o barramento de até 8,4 V. |
| **DRV8833** | Faixa de tensão de `VM` e compatibilidade com a tensão/corrente dos motores. |
| **Buck +5 V** | Corrente máxima necessária e alimentação compatível do LiDAR. |
| **Buck +3,3 V** | Corrente total exigida pelo ESP32 e periféricos. |
| **LiDAR** | Modelo exato, alimentação, protocolo UART, _baud rate_ e níveis lógicos. |
| **R1/R2** | Necessidade e valores, dependendo dos níveis UART. |
| **Encoders** | Níveis lógicos e necessidade/valores dos _pull-ups_. |
| **Chave de potência** | Corrente nominal compatível com o consumo máximo do sistema. |
| **BMS** | Corrente admissível, configuração 2S e proteção adequada ao _pack_ utilizado. |

---

## 7. Resumo para apresentação

| Componente | Justificativa curta para apresentação |
| :--- | :--- |
| **Bateria 2S** | Fonte de energia principal: 7,4 V nominal e 8,4 V máximo. |
| **BMS** | Gerenciamento/proteção do conjunto de células da bateria. |
| **Chave de potência** | Liga/desliga a alimentação geral do robô. |
| **Buck 5 V** | Gera uma alimentação regulada de 5 V para o subsistema que necessitar dessa tensão. |
| **Buck 3,3 V** | Gera a alimentação regulada da eletrônica de controle. |
| **ESP32-C3-MINI-1** | Cérebro do robô: processa sensores e gera os comandos PWM. |
| **DRV8833** | Driver de potência: faz a interface entre o ESP32 e os dois motores. |
| **2 motores** | Executam a movimentação do Micromouse. |
| **Encoders** | Medem o movimento por quadratura A/B e fornecem realimentação ao controle. |
| **LiDAR** | Mede distâncias do ambiente e auxilia a navegação no labirinto. |
| **R1/R2** | Condicionamento opcional da UART, somente se necessário após validação dos níveis. |
| **GND comum** | Mantém uma referência elétrica comum para alimentação e sinais. |

---

> **Fonte:** _Hardware Micromouse PI1_, documento fornecido para o projeto. As justificativas acima seguem a arquitetura e as pendências explicitamente apresentadas no material.
