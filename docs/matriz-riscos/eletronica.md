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
| 3 | Mau contato ou ligação errada nos conectores dos motores e encoders | O motor não aciona e o robô perde a medida de quanto andou | Já falhou: o conector do N20 causou a falha do teste 3.3, e o motor direito não girou em 29/10 | Provável (4) | Alto (4) | <span class="nota nota-critica">16</span> |
| 4 | VL53L0X lendo errado por falta de calibração | O robô vê parede onde não tem (ou o contrário), bate e erra o mapa | A frente lê cerca de 80 mm a mais (teste 3.7), e esquerda e direita nunca foram calibradas | Provável (4) | Médio (3) | <span class="nota nota-alta">12</span> |
| 5 | Sensor tampado ou mal posicionado no chassi | O sensor lê o próprio robô ou os fios, e a navegação erra | No teste 3.8, o sensor da direita leu um valor quase fixo, provavelmente por um fio na frente; o chassi final ainda não foi montado | Possível (3) | Alto (4) | <span class="nota nota-alta">12</span> |
| 6 | Motor gira sozinho ao ligar ou reiniciar a placa | O robô sai andando e bate | O pulso do GPIO14 no boot já é conhecido (está no `Pinos.h`), e a troca para o GPIO13 ainda não foi validada | Provável (4) | Médio (3) | <span class="nota nota-alta">12</span> |
| 7 | Queda de tensão ou ruído quando os motores ligam | O ESP32 reinicia, a comunicação falha e a corrida para no meio | A tensão com os dois motores ligados nunca foi medida ([#214](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/214)) | Possível (3) | Alto (4) | <span class="nota nota-alta">12</span> |
| 8 | Falha no I²C ou dois VL53L0X no mesmo endereço | O robô perde as leituras dos sensores ao mesmo tempo | 0 falhas em 499 amostras, mas com os motores desligados (teste 3.8); com o ruído dos motores ainda não foi testado | Possível (3) | Alto (4) | <span class="nota nota-alta">12</span> |
| 9 | Leitura errada da tensão da bateria pelo ADC | Sem aviso de bateria baixa, o robô desliga de repente | A leitura pode ser conferida com multímetro e corrigida no software; a falha não para o robô | Improvável (2) | Baixo (2) | <span class="nota nota-baixa">4</span> |
| 10 | Atraso na montagem e na validação da placa definitiva | Sobra pouco tempo para integrar com o software e testar no labirinto | Os testes de integração com a DevKit foram feitos, mas a montagem no chassi e a decisão sobre a PCB estão pendentes | Possível (3) | Alto (4) | <span class="nota nota-alta">12</span> |
| 11 | MPU6050 medindo errado por falta de calibração ou por vibração | O robô erra a direção e faz curvas imprecisas | Deriva de 0,07° em 10 s parada (teste 3.8); o acelerômetro lê 4% a mais, o que se corrige com calibração | Possível (3) | Baixo (2) | <span class="nota nota-moderada">6</span> |

## Possíveis soluções

| **Nº** | **Risco** | **Como evitar** | **Se acontecer** |
|:---:|---|---|---|
| 1 | Corrente alta e aquecimento | Medir a corrente de partida e de travamento dos motores e comparar com o limite do driver e dos reguladores. Testar com fonte limitada em corrente e acompanhar a temperatura | Desligar na hora, trocar o componente danificado e revisar o dimensionamento e as proteções |
| 2 | Alimentação errada | Ajustar e medir 3,3 V, 5 V e 6 V antes de ligar os módulos. Manter separados o 3,3 V da DevKit e o dos sensores. Usar só USB ou só VIN até ter um circuito de seleção | Desligar tudo, medir cada trilho e testar os módulos um por um antes de religar |
| 3 | Conectores | Refazer os conectores com defeito, conferir a ordem dos terminais e identificar os cabos. Testar continuidade com tudo desligado e prender os fios para não puxarem o conector | Testar motor e encoder separados para achar o ponto da falha e refazer o conector |
| 4 | Calibração dos VL53L0X | Calibrar cada um dos 3 sensores com régua e anteparo em distâncias conhecidas e aplicar a correção de cada um. Repetir na montagem final e com os motores ligados | Usar a calibração no boot do firmware (#240) e refazer a medição com régua |
| 5 | Posição dos sensores | Afastar suportes e fios do campo de visão e fixar cada sensor sempre na mesma orientação. Testar cada um com anteparo, principalmente o da direita, que ainda não foi demonstrado | Reposicionar o sensor ou o fio que está na frente e repetir o teste |
| 6 | Motor girando ao ligar | Confirmar o IN4 no GPIO13 na placa e no firmware. Testar ligar, reiniciar e gravar com as rodas fora do chão | Revisar o bloqueio do driver por hardware e o estado dos pinos no boot antes de liberar os motores |
| 7 | Queda de tensão e ruído | Medir as tensões na partida, na frenagem e com os dois motores ligados. Revisar aterramento e capacitores e separar os cabos de potência dos de sinal | Repetir o teste dos sensores com os motores ligados e reforçar o desacoplamento |
| 8 | I²C | Ligar os VL53L0X um por vez pelo XSHUT e dar um endereço para cada um em toda partida. Registrar a varredura do barramento e conferir os pull-ups | Usar a recuperação de falha do firmware (#241) e revisar fios e soldas do barramento |
| 9 | Leitura da bateria | Conferir o divisor, o filtro e a ligação no GPIO34. Comparar a leitura do ADC com o multímetro em várias tensões e calibrar | Medir a bateria com multímetro antes de cada corrida até a leitura ser corrigida |
| 10 | Atraso na montagem | Ter responsável e prazo para montagem, inspeção e ensaios (já no Projects). Conferir as peças disponíveis e deixar tempo para retrabalho | Validar por partes e priorizar o que o software precisa primeiro: motores, encoders e sensores |
| 11 | Calibração da MPU6050 | Calibrar acelerômetro e giroscópio com o robô parado, medir a deriva em ensaios longos e prender bem a IMU | Recalibrar e comparar as curvas com uma referência conhecida (giro de 90° medido) |

## Histórico de versões

| **Versão** | **Data** | **Descrição** | **Autor(es)** | **Revisor(es)** |
|:----------:|:--------:|---------------|---------------|-----------------|
| 1.0 | 04/11/2026 | Matriz de riscos da eletrônica (riscos 1 a 11): notas, mapa de calor, motivo de cada nota e possíveis soluções | Equipe de Eletrônica (riscos e soluções); Ana Carolina Fialho (página e notas) | Aline de Carvalho Rodrigues |
