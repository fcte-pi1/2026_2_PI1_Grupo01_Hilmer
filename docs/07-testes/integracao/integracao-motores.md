# Integração dos Motores de Tração (DRV8833, N20 e Operação Autônoma)

Este documento registra a **integração completa dos motores de tração** do Micromouse no protótipo montado sobre o chassi impresso em 3D, alimentado por bateria LiPo 2S embarcada (sem cabo USB). O ensaio resolve a pendência aberta no [ensaio integrado em protoboard (5.1)](ensaio-integrado-protoboard.md) referente à issue [#483](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/483) — na qual apenas o motor esquerdo girava em bancada — e consolida os parâmetros de acionamento e resultados obtidos no robô real registrados na seção de testes da branch `main` ([`corrida-completa.md`](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/blob/main/docs/07-testes/corrida-completa.md)).

| Item | Valor |
| :--- | :--- |
| **Subsistemas Integrados** | Energia (LiPo 2S + reguladores Buck `U2`, `U4` e `U5`), Eletrônica (`ESP32 DevKit V1`, driver ponte H dupla `DRV8833`), Estrutura (chassi 3D com rodas e dois micromotores DC N20) e Software/Firmware (`MotoresDrv8833`, camadas de proteção e controle de movimento) |
| **Período da Execução** | 19/11/2026 (ensaio em vídeo no robô montado) a 23/11/2026 (registro das corridas completas na `main`) |
| **Responsável pelo Registro** | Paulo Melo |
| **Ambiente de Montagem** | Protótipo físico completo do Micromouse com bateria embarcada, em piso de laboratório e nos labirintos de testes 4×4 |
| **Referência de Firmware** | [`MotoresDrv8833`](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/blob/main/src/firmware/lib/drivers_esp32/src/MotoresDrv8833.cpp), [`ConfiguracaoMotores.h`](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/blob/main/src/firmware/lib/atuacao/src/ConfiguracaoMotores.h) e [`main.cpp`](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/blob/main/src/firmware/src/main.cpp) |
| **Resultado Global** | **Aprovado.** Os dois motores N20 operam simultaneamente no robô autônomo em avanço reto, giro diferencial no próprio eixo e frenagem ativa, sem *brownout* nem perda de controle, e sustentam corridas completas no labirinto 4×4 sem intervenção |

---

## 1. Resolução da Pendência (#483) e Configuração Integrada

No ensaio anterior em protoboard ([7.5.1](ensaio-integrado-protoboard.md), de 29/10/2026, e item [3.10 de Hardware](../hardware.md)), o driver `DRV8833` acionou o motor esquerdo, mas o motor direito não girou e os dois motores não rodaram juntos (`#483`). Na integração final do conjunto de tração ao robô real, o funcionamento simultâneo e estável dos dois lados foi alcançado com a configuração abaixo:

### 1.1. Ligação Elétrica e Pinagem do Acionamento

| Sinal / Barramento | Origem | Destino | Configuração / Função |
| :--- | :--- | :--- | :--- |
| **Potência (`VM`)** | Buck `U4` (`+6,0 V` na protoboard / `+6,6 V` na PCB) | `DRV8833` pino `VM` | Alimentação dos dois micromotores N20 a partir da bateria LiPo 2S (na PCB física, ajustado com folga para `6,6 V` conforme frente de Energia) |
| **Habilitação (`nSLEEP`)** | Buck `U5` (`+3,3 V`) | `DRV8833` pino `nSLEEP` (`EEP`) | Mantém as duas pontes H ativas em operação |
| **Lógica / Controle** | Buck `U2` (`+5,0 V` / `+5,1 V` na PCB) | `ESP32 DevKit V1` (`VIN`) | Alimentação isolada do microcontrolador para evitar reinicializações |
| **Motor Esquerdo (`IN1` / `IN2`)** | `GPIO 25` e `GPIO 26` | `DRV8833` `AIN1` / `AIN2` → `OUT1/OUT2` | PWM de avanço, ré e freio do motor esquerdo |
| **Motor Direito (`IN3` / `IN4`)** | `GPIO 27` e `GPIO 14` | `DRV8833` `BIN1` / `BIN2` → `OUT3/OUT4` | PWM do motor direito, com inversão de sentido por software devido à montagem espelhada no chassi |

> **Nota sobre o boot (`GPIO 14`) e esquemático Rev 2.1:** o esquemático Rev 2.1 de Hardware prevê migrar `IN4` de `GPIO 14` para `GPIO 13`, porém o firmware testado no robô real (`Pinos.h`) manteve `GPIO 14`. Como o `GPIO 14` emite um pulso PWM curto durante o *reset* do ESP32, o método `MotoresDrv8833::iniciar()` é executado na primeira linha do `setup()`, colocando os quatro pinos em nível lógico alto (freio) antes de habilitar os canais LEDC.

### 1.2. Parâmetros de PWM, Zona Morta e Proteção no Firmware

Os parâmetros calibrados no robô real ([`ConfiguracaoMotores.h`](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/blob/main/src/firmware/lib/atuacao/src/ConfiguracaoMotores.h) e [`main.cpp`](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/blob/main/src/firmware/src/main.cpp)) garantiram torque de partida suficiente para vencer o atrito das caixas de redução N20 com o peso completo do robô:

| Parâmetro | Valor Integrado | Observação Técnica |
| :--- | :---: | :--- |
| **Frequência do PWM (`LEDC`)** | $20\text{ kHz}$ | Acima da faixa audível e compatível com o `DRV8833` |
| **Resolução do PWM** | $10\text{ bits}$ ($0\text{ a }1023$) | Escala de controle de *duty cycle* |
| **Zona morta configurada** | $60\%$ (`ZONA_MORTA_DUTY_MILESIMOS = 600`) | Piso para qualquer comando acima de zero (na rampa com bateria ligada e rodas fora do chão, o esquerdo partiu em $\approx 52\%$ e o direito em $\approx 56\%$; a $60\%$ ambos giraram entre 650 e 730 pulsos em 300 ms) |
| **Limite máximo em movimento** | $80\%$ (`LIMITE_DUTY_MOTORES_MILESIMOS = 800`) | Faixa útil de operação de $60\%$ a $80\%$ (até $\approx 1450$ pulsos em 300 ms) |
| **Freio ativo** | $100\%$ (`DUTY_TOTAL_MOTORES = 1023`) | As duas entradas de cada ponte H em nível alto |
| **Cadeia de segurança** | `RegistradorPotencia` → `MotoresComTrava` → `MotoresComProtecao` → `MotoresDrv8833` | Trava no *boot* até aprovação do autoteste I²C (`#50`), corte por motor travado após $500\text{ ms}$ sem movimento (`#341`) e freio em falhas de barramento (`#241`) |

---

## 2. Teste Dinâmico no Robô Montado (Vídeo do Ensaio)

O ensaio registrado em vídeo em 19/11/2026 valida o comportamento integrado do sistema de tração diretamente no robô físico montado, operando de forma totalmente autônoma pela bateria LiPo 2S (sem cabos de alimentação ou depuração USB).

### 2.1. Comportamentos Validados no Vídeo

1. **Avanço em linha reta (`00:00–00:02`):** o robô parte do repouso sobre o piso do laboratório, acelera em translação reta com os dois motores atuando em conjunto e executa parada controlada.
2. **Giro diferencial contínuo no próprio eixo (`00:04–00:13`):** após o comando de giro, os dois motores são acionados simultaneamente em sentidos opostos (`M1` e `M2` em contra-rotação), fazendo o chassi girar continuamente em torno do próprio centro sem arraste ou travamento de roda, encerrando com frenagem limpa e sem reinicialização (*brownout*) do ESP32.

<video controls preload="metadata" width="420" src="../../../videos/integracao-motores.mp4"></video>

Vídeo 1. Teste integrado dos motores de tração no protótipo físico autônomo (sem cabo USB): avanço reto inicial seguido de giro diferencial contínuo no próprio eixo e parada controlada. Arquivo: [`docs/videos/integracao-motores.mp4`](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/blob/documentacao/docs/videos/integracao-motores.mp4).

---

## 3. Desempenho Integrado no Labirinto 4×4 (Acréscimo da Branch `main`)

Na branch `main`, a seção de testes documenta em [`docs/07-testes/corrida-completa.md`](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/blob/main/docs/07-testes/corrida-completa.md) a validação de ponta a ponta do robô real operando com esse sistema de tração dentro de dois desenhos do labirinto 4×4 (issues `#185` e `#232`). Os pontos diretamente ligados à atuação dos motores e ao movimento integrado são incorporados abaixo:

### 3.1. Resultados em Corrida Completa (`#185`)

* **Operação autônoma do início ao objetivo:** partindo da célula oficial pelo botão `INICIAR` do *dashboard* e sem intervenção humana durante o percurso, o robô completou corridas em **dois desenhos distintos de labirinto 4×4**:
  * **Primeiro desenho:** tentativas `56`, `57`, `60`, `61` e `64` concluídas entre **$21{,}8\text{ s}$ e $22{,}9\text{ s}$** (`Desafio cumprido: SIM`).
  * **Segundo desenho (com becos e entradas laterais):** tentativa `68` concluída em **$31{,}2\text{ s}$** (`Desafio cumprido: SIM`).

### 3.2. Problemas de Movimentação Encontrados e Correções na Integração

| Problema Observado em Pista | Correção Aplicada na Integração Motores + Controle |
| :--- | :--- |
| **Robô preso em quinas de parede** (cantos batendo e rodas patinando no giro) | Inclusão de desvio de quina (afastamento quando há parede lateral próxima e lado oposto livre) e ré curta antes do giro (`recuoAntesDoGiroMm = 45` com frontal $< 90\text{ mm}$ em `main.cpp`) |
| **Robô parava antes ou empurrava a parede na última célula** | Ajuste de chegada pelo ToF frontal (`distanciaParadaFrontalMm = 105` no robô real e teto maior na última célula antes da borda) |
| **Ré que não avançava encerrava a corrida** | Ajuste na lógica do ciclo para seguir para percepção e giro quando o recuo não produz deslocamento |
| **Velocidade sempre zerada no *dashboard*** | Cálculo da velocidade real transmitida na telemetria a partir do deslocamento das rodas |

### 3.3. Limitações Registradas

* As corridas concluídas no labirinto vieram após rodadas de ajuste de parâmetros; ainda não foi registrada uma série de repetições consecutivas por labirinto com o firmware final.
* O primeiro desenho de labirinto não foi repetido após o ajuste final do limiar lateral (`190 mm`).

---

## 4. Resumo da Avaliação e Conclusão

| Item Avaliado | Critério Esperado | Resultado Obtido | Situação |
| :--- | :--- | :--- | :---: |
| **Acionamento simultâneo dos dois motores N20 (`#483`)** | Ambas as pontes H do `DRV8833` operando juntas na `ESP32 DevKit V1` | Motores esquerdo e direito respondem em conjunto aos comandos PWM ($20\text{ kHz}$, faixa útil de $60\%$ a $80\%$) | **Aprovado** |
| **Avanço reto e giro diferencial** | Translação controlada e rotação no próprio eixo no piso com bateria embarcada | Validados no vídeo em operação autônoma (avanço inicial, giro contínuo no eixo e frenagem ativa sem queda de tensão) | **Aprovado** |
| **Estabilidade energética sob carga dos motores** | Ausência de *brownout* ou travamento do microcontrolador com os dois motores ativos | Circuito operando integralmente pela LiPo 2S (Buck `U2` $5{,}0\text{ V}$/$5{,}1\text{ V}$, `U4` $6{,}0\text{ V}$/$6{,}6\text{ V}$, `U5` $3{,}3\text{ V}$) sem reinícios | **Aprovado** |
| **Integração completa na pista 4×4 (`main`)** | Conclusão autônoma do percurso no labirinto 4×4 sem intervenção manual | Corridas completas concluídas nos dois desenhos de labirinto ($21{,}8\text{ s}$ a $31{,}2\text{ s}$) | **Aprovado** |

---

## Histórico de Versões

| Versão | Data | Descrição | Autor(es) | Revisor(es) |
| :---: | :---: | :--- | :--- | :--- |
| 1.0 | 24/11/2026 | Registro da integração dos motores de tração (`DRV8833` + N20) no robô autônomo: resolução da pendência `#483`, parâmetros de PWM/zona morta, vídeo do ensaio em funcionamento e resultados de movimentação das corridas completas na `main` | Paulo Melo | — |
