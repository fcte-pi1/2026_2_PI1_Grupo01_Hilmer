# Matriz de riscos: Eletrônica

**Gerente:** Aline de Carvalho Rodrigues. **Hardware:** Rev. 2.1 (ESP32 DevKit V1). **Avaliação:** 04/11/2026, a partir da matriz da equipe de eletrônica.

Como ler o impacto e a probabilidade: veja [Matriz de riscos](README.md#como-ler).

## Matriz

<!-- mapa:inicio -->
<div class="mapa-calor" markdown="0">
<table>
<tr><th class="canto" rowspan="2">Probabilidade ↓</th><th colspan="5">Impacto →</th></tr>
<tr><th>Muito baixo<br><small>1</small></th><th>Baixo<br><small>2</small></th><th>Médio<br><small>3</small></th><th>Alto<br><small>4</small></th><th>Muito alto<br><small>5</small></th></tr>
<tr><th>Muito provável<br><small>5</small></th><td class="nota-moderada" title="Nota 5 (Moderada)"><span class="ponto">5</span></td><td class="nota-alta" title="Nota 10 (Alta)"><span class="ponto">10</span></td><td class="nota-critica" title="Nota 15 (Crítica)"><span class="ponto">15</span></td><td class="nota-critica" title="Nota 20 (Crítica)"><span class="ponto">20</span></td><td class="nota-critica" title="Nota 25 (Crítica)"><span class="ponto">25</span></td></tr>
<tr><th>Provável<br><small>4</small></th><td class="nota-baixa" title="Nota 4 (Baixa)"><span class="ponto">4</span></td><td class="nota-moderada" title="Nota 8 (Moderada)"><span class="ponto">8</span></td><td class="nota-alta" title="Nota 12 (Alta)"><span class="ponto">12</span><span class="risco">4</span><span class="risco">6</span></td><td class="nota-critica" title="Nota 16 (Crítica)"><span class="ponto">16</span><span class="risco">3</span></td><td class="nota-critica" title="Nota 20 (Crítica)"><span class="ponto">20</span></td></tr>
<tr><th>Possível<br><small>3</small></th><td class="nota-baixa" title="Nota 3 (Baixa)"><span class="ponto">3</span></td><td class="nota-moderada" title="Nota 6 (Moderada)"><span class="ponto">6</span><span class="risco">11</span></td><td class="nota-moderada" title="Nota 9 (Moderada)"><span class="ponto">9</span></td><td class="nota-alta" title="Nota 12 (Alta)"><span class="ponto">12</span><span class="risco">1</span><span class="risco">5</span><span class="risco">7</span><span class="risco">8</span><span class="risco">10</span></td><td class="nota-critica" title="Nota 15 (Crítica)"><span class="ponto">15</span><span class="risco">2</span></td></tr>
<tr><th>Improvável<br><small>2</small></th><td class="nota-baixa" title="Nota 2 (Baixa)"><span class="ponto">2</span></td><td class="nota-baixa" title="Nota 4 (Baixa)"><span class="ponto">4</span><span class="risco">9</span></td><td class="nota-moderada" title="Nota 6 (Moderada)"><span class="ponto">6</span></td><td class="nota-moderada" title="Nota 8 (Moderada)"><span class="ponto">8</span></td><td class="nota-alta" title="Nota 10 (Alta)"><span class="ponto">10</span></td></tr>
<tr><th>Rara<br><small>1</small></th><td class="nota-baixa" title="Nota 1 (Baixa)"><span class="ponto">1</span></td><td class="nota-baixa" title="Nota 2 (Baixa)"><span class="ponto">2</span></td><td class="nota-baixa" title="Nota 3 (Baixa)"><span class="ponto">3</span></td><td class="nota-baixa" title="Nota 4 (Baixa)"><span class="ponto">4</span></td><td class="nota-moderada" title="Nota 5 (Moderada)"><span class="ponto">5</span></td></tr>
</table>
</div>

**Prioridade (maior nota primeiro):** <span class="nota nota-critica">Crítica</span> 3 (16), 2 (15) · <span class="nota nota-alta">Alta</span> 1 (12), 4 (12), 5 (12), 6 (12), 7 (12), 8 (12), 10 (12) · <span class="nota nota-moderada">Moderada</span> 11 (6) · <span class="nota nota-baixa">Baixa</span> 9 (4)
<!-- mapa:fim -->

## Riscos

| **Nº** | **Risco** | **O que acontece se ocorrer** | **Por que essa nota** | **Probabilidade** | **Impacto** | **Nota** |
|:---:|---|---|---|:---:|:---:|:---:|
| 1 | Corrente alta demais ou aquecimento no driver DRV8833 e nos reguladores | O robô para de andar e componentes podem queimar | A corrente de travamento do N20 nunca foi medida, e o driver só foi testado com o motor esquerdo | Possível (3) | Alto (4) | <span class="nota nota-alta">12</span> |
| 2 | Alimentação errada: tensão incorreta, trilhos de 3,3 V unidos sem querer, ou USB e VIN ligados juntos sem isolamento | Pode queimar o ESP32, os sensores ou os reguladores | Na bancada, USB e bateria são ligadas juntas sem isolamento verificado; um erro queima a placa | Possível (3) | Muito alto (5) | <span class="nota nota-critica">15</span> |
| 3 | Mau contato ou ligação errada nos conectores dos motores e encoders | O motor não aciona e o robô perde a medida de quanto andou | Já falhou: o conector do N20 causou a falha do teste 3.3. Em 29/10 o motor direito não girou, e a causa ainda é investigada ([#483](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/483)) | Provável (4) | Alto (4) | <span class="nota nota-critica">16</span> |
| 4 | VL53L0X lendo errado por falta de calibração | O robô vê parede onde não tem (ou o contrário), bate e erra o mapa | A frente lê cerca de 80 mm a mais (teste 3.7), e esquerda e direita nunca foram calibradas | Provável (4) | Médio (3) | <span class="nota nota-alta">12</span> |
| 5 | Sensor tampado ou mal posicionado no chassi | O sensor lê o próprio robô ou os fios, e a navegação erra | No teste 3.8, o sensor da direita leu um valor quase fixo, provavelmente por um fio na frente; o chassi final ainda não foi montado | Possível (3) | Alto (4) | <span class="nota nota-alta">12</span> |
| 6 | Motor gira sozinho ao ligar ou reiniciar a placa | O robô sai andando e bate | O pulso do GPIO14 no boot já é conhecido (está no `Pinos.h`), e a troca para o GPIO13 ainda não foi validada | Provável (4) | Médio (3) | <span class="nota nota-alta">12</span> |
| 7 | Queda de tensão ou ruído quando os motores ligam | O ESP32 reinicia, a comunicação falha e a corrida para no meio | A tensão com os dois motores ligados nunca foi medida ([#214](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/214)) | Possível (3) | Alto (4) | <span class="nota nota-alta">12</span> |
| 8 | Falha no I²C ou dois VL53L0X no mesmo endereço | O robô perde as leituras dos sensores ao mesmo tempo | 0 falhas em 499 amostras, mas com os motores desligados (teste 3.8); com o ruído dos motores ainda não foi testado | Possível (3) | Alto (4) | <span class="nota nota-alta">12</span> |
| 9 | Leitura errada da tensão da bateria pelo ADC | Sem aviso de bateria baixa, o robô desliga de repente | A leitura pode ser conferida com multímetro e corrigida no software; a falha não para o robô | Improvável (2) | Baixo (2) | <span class="nota nota-baixa">4</span> |
| 10 | Atraso na montagem e na validação da placa definitiva | Sobra pouco tempo para integrar com o software e testar no labirinto | Os testes de integração com a DevKit foram feitos, mas a montagem no chassi e a decisão sobre a PCB estão pendentes | Possível (3) | Alto (4) | <span class="nota nota-alta">12</span> |
| 11 | MPU6050 medindo errado por falta de calibração ou por vibração | O robô erra a direção e faz curvas imprecisas | Deriva de 0,07° em 10 s parada (teste 3.8); o acelerômetro lê 4% a mais, o que se corrige com calibração | Possível (3) | Baixo (2) | <span class="nota nota-moderada">6</span> |

## Possíveis soluções

| **Nº** | **Risco** | **Como evitar** |
|:---:|---|---|
| 1 | Corrente alta e aquecimento | Primeiro, descobrir quanta corrente os motores puxam na partida e quando ficam travados, consultando o datasheet ou medindo com o rotor preso. Em seguida, comparar esses valores com o que o DRV8833 e os reguladores aguentam. Fazer os primeiros ensaios com uma fonte com limite de corrente, para que qualquer problema não queime componentes. Durante os testes, acompanhar a temperatura do driver e dos reguladores, tocando ou medindo. Se algum esquentar demais, ajustar o dimensionamento ou acrescentar proteções. Está resolvido quando o robô anda e trava sem passar dos limites dos componentes. |
| 2 | Alimentação errada | Antes de ligar qualquer módulo, ajustar as saídas de 3,3 V, 5 V e 6 V e confirmar cada uma com um multímetro. Só depois de conferir que as tensões estão certas, conectar os módulos. Manter separados o trilho de 3,3 V da DevKit e o dos sensores, sem uni-los. Além disso, alimentar o robô por USB ou por VIN, nunca pelos dois ao mesmo tempo, até que exista um circuito de seleção ou isolamento verificado. Está resolvido quando todas as tensões medidas estão corretas e os trilhos continuam separados. |
| 3 | Conectores | Começar refazendo os conectores que já deram problema. Depois, conferir a ordem física dos terminais e identificar os cabos com etiqueta ou cor, para que ninguém ligue algo no lugar errado. Com a alimentação desligada, testar a continuidade de cada fio. Prender os cabos de forma que um puxão não force o conector (alívio de tração). Por fim, testar o motor sozinho e o encoder sozinho antes de integrar tudo. Está resolvido quando os dois funcionam separadamente e continuam firmes mesmo ao mexer nos cabos. |
| 4 | Calibração dos VL53L0X | Colocar um anteparo, como uma parede ou um papelão, a distâncias conhecidas, medidas com régua, e comparar a leitura de cada um dos três sensores com a distância real. Como cada módulo erra de um jeito, calcular uma correção para cada um, com atenção especial ao sensor frontal, que apresentou desvio nos ensaios. Aplicar as correções no firmware e medir de novo para confirmar que o erro diminuiu. Repetir o teste com o robô montado e com os motores ligados. Está resolvido quando os três sensores acertam as distâncias de teste também na montagem final. |
| 5 | Posição dos sensores | Reposicionar os suportes e arrumar os fios para que nada apareça no campo de visão dos sensores. Fixar cada sensor sempre com a mesma orientação e testar um de cada vez, com anteparos em posições controladas. Dar atenção ao sensor direito, que no teste 3.8 leu um valor quase fixo, provavelmente por um fio na frente. Está resolvido quando cada sensor lê apenas a parede, sem captar o chassi nem os fios. |
| 6 | Motor girando ao ligar | Confirmar no hardware que o pino IN4 foi trocado para o GPIO13 e conferir que o firmware usa essa mesma configuração. O Software atualiza o `Pinos.h` e a bancada geral depois que o [#497](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/497) for mesclado. Em seguida, testar com as rodas suspensas, para que um movimento inesperado não cause colisão: ligar o robô, reiniciar e gravar o firmware. Se houver trancos, revisar a inibição por hardware do driver e o estado das entradas na inicialização. Liberar a tração no chão só depois que as rodas ficarem paradas em todos esses testes. Está resolvido quando as rodas não se mexem sozinhas ao ligar, reiniciar ou gravar. |
| 7 | Queda de tensão e ruído | Medir as tensões de alimentação durante a partida, a frenagem e o acionamento dos dois motores ao mesmo tempo, que são os momentos mais exigentes. Revisar as conexões, o aterramento e o desacoplamento (capacitores próximos dos componentes). Afastar os cabos de potência dos cabos de sinal para reduzir interferência e repetir os testes dos sensores com os motores em carga. Está resolvido quando o ESP32 não reinicia e os sensores continuam estáveis com os motores acionados. |
| 8 | I²C | Como todos os VL53L0X saem de fábrica com o mesmo endereço, mantê-los desligados e ligar um de cada vez pelo pino XSHUT, atribuindo um endereço diferente a cada um em toda partida. Fazer uma varredura do barramento e registrar os endereços encontrados. Revisar as conexões e os resistores de pull-up e preparar o firmware para se recuperar de falhas, por exemplo tentando reinicializar o sensor que parou de responder. Validar tudo com todos os dispositivos e os motores ativos. Está resolvido quando todos aparecem na varredura e respondem com o robô em funcionamento. |
| 9 | Leitura da bateria | Conferir o divisor resistivo, o filtro e a ligação ao GPIO34. Comparar a leitura do ADC com a de um multímetro em várias tensões e ajustar a conversão no firmware para que o valor lido reflita a tensão real da bateria. Depois, testar o aviso de bateria baixa usando os limites definidos pela equipe de energia. Está resolvido quando a tensão lida bate com a do multímetro e o aviso dispara no valor combinado. |
| 10 | Atraso na montagem | Definir um responsável e um prazo para cada etapa de montagem, inspeção e ensaios, e conferir se todos os componentes estão disponíveis. Reservar tempo para retrabalho, porque ele costuma acontecer. Validar o robô por partes (alimentação, motores, sensores e comunicação), registrando o resultado de cada uma, e só depois fazer o teste completo no labirinto. Está resolvido quando todos os subsistemas estão validados e registrados antes desse teste. |
| 11 | Calibração da MPU6050 | Calibrar o acelerômetro e o giroscópio com o robô parado e medir a deriva em ensaios mais longos. Fixar a IMU com firmeza para reduzir as vibrações e conferir a orientação do sensor no chassi. Por fim, comparar as curvas medidas com uma referência conhecida. Está resolvido quando a orientação quase não deriva com o robô parado e as curvas saem próximas do esperado. |

## Evolução por sprint

Nota de cada risco (probabilidade × impacto) ao fim de cada sprint, com uma frase sobre o que mudou. As colunas das sprints seguintes são preenchidas nos fechamentos de 11/11, 18/11 e 23/11. Risco resolvido sai da tabela de riscos e continua aqui, com o desfecho.

| **Nº** | **Risco** | **Sprint 6 (até 04/11)** | **Sprint 7 (até 11/11)** | **Sprint 8 (até 18/11)** | **Fechamento (até 23/11)** |
|:---:|---|:---:|---|---|---|
| 1 | Corrente alta demais ou aquecimento no driver DRV8833 e nos reguladores | 12 | — | — | — |
| 2 | Alimentação errada: tensão incorreta, trilhos de 3,3 V unidos sem querer, ou USB e VIN ligados juntos sem isolamento | 15 | — | — | — |
| 3 | Mau contato ou ligação errada nos conectores dos motores e encoders | 16 | — | — | — |
| 4 | VL53L0X lendo errado por falta de calibração | 12 | — | — | — |
| 5 | Sensor tampado ou mal posicionado no chassi | 12 | — | — | — |
| 6 | Motor gira sozinho ao ligar ou reiniciar a placa | 12 | — | — | — |
| 7 | Queda de tensão ou ruído quando os motores ligam | 12 | — | — | — |
| 8 | Falha no I²C ou dois VL53L0X no mesmo endereço | 12 | — | — | — |
| 9 | Leitura errada da tensão da bateria pelo ADC | 4 | — | — | — |
| 10 | Atraso na montagem e na validação da placa definitiva | 12 | — | — | — |
| 11 | MPU6050 medindo errado por falta de calibração ou por vibração | 6 | — | — | — |

## Histórico de versões

| **Versão** | **Data** | **Descrição** | **Autor(es)** | **Revisor(es)** |
|:----------:|:--------:|---------------|---------------|-----------------|
| 1.0 | 04/11/2026 | Matriz de riscos da eletrônica (riscos 1 a 11): notas, mapa de calor, motivo de cada nota e possíveis soluções | Equipe de Eletrônica (riscos e soluções); Ana Carolina Fialho (página e notas) | Aline de Carvalho Rodrigues |
| 1.1 | 04/11/2026 | Revisão do PR #500: textos dos riscos 3 e 6 e tabela de evolução por sprint | Cláudio Henrique | — |
