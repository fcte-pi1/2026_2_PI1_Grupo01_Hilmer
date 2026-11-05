# Matriz de riscos: Energia

**Gerente:** Paulo Henrique Melo de Souza. **Avaliação:** 04/11/2026, a partir do rascunho da equipe de energia.

Como ler o impacto e a probabilidade: veja [Matriz de riscos](README.md#como-ler).

## Matriz

<!-- mapa:inicio -->
<div class="mapa-calor" markdown="0">
<table>
<tr><th class="canto" rowspan="2">Probabilidade ↓</th><th colspan="5">Impacto →</th></tr>
<tr><th>Muito baixo<br><small>1</small></th><th>Baixo<br><small>2</small></th><th>Médio<br><small>3</small></th><th>Alto<br><small>4</small></th><th>Muito alto<br><small>5</small></th></tr>
<tr><th>Muito provável<br><small>5</small></th><td class="nota-moderada" title="Nota 5 (Moderada)"><span class="ponto">5</span></td><td class="nota-alta" title="Nota 10 (Alta)"><span class="ponto">10</span></td><td class="nota-critica" title="Nota 15 (Crítica)"><span class="ponto">15</span></td><td class="nota-critica" title="Nota 20 (Crítica)"><span class="ponto">20</span></td><td class="nota-critica" title="Nota 25 (Crítica)"><span class="ponto">25</span></td></tr>
<tr><th>Provável<br><small>4</small></th><td class="nota-baixa" title="Nota 4 (Baixa)"><span class="ponto">4</span></td><td class="nota-moderada" title="Nota 8 (Moderada)"><span class="ponto">8</span></td><td class="nota-alta" title="Nota 12 (Alta)"><span class="ponto">12</span></td><td class="nota-critica" title="Nota 16 (Crítica)"><span class="ponto">16</span></td><td class="nota-critica" title="Nota 20 (Crítica)"><span class="ponto">20</span></td></tr>
<tr><th>Possível<br><small>3</small></th><td class="nota-baixa" title="Nota 3 (Baixa)"><span class="ponto">3</span></td><td class="nota-moderada" title="Nota 6 (Moderada)"><span class="ponto">6</span><span class="risco">8</span></td><td class="nota-moderada" title="Nota 9 (Moderada)"><span class="ponto">9</span><span class="risco">5</span></td><td class="nota-alta" title="Nota 12 (Alta)"><span class="ponto">12</span><span class="risco">3</span></td><td class="nota-critica" title="Nota 15 (Crítica)"><span class="ponto">15</span></td></tr>
<tr><th>Improvável<br><small>2</small></th><td class="nota-baixa" title="Nota 2 (Baixa)"><span class="ponto">2</span></td><td class="nota-baixa" title="Nota 4 (Baixa)"><span class="ponto">4</span></td><td class="nota-moderada" title="Nota 6 (Moderada)"><span class="ponto">6</span><span class="risco">9</span></td><td class="nota-moderada" title="Nota 8 (Moderada)"><span class="ponto">8</span><span class="risco">7</span></td><td class="nota-alta" title="Nota 10 (Alta)"><span class="ponto">10</span><span class="risco">4</span></td></tr>
<tr><th>Rara<br><small>1</small></th><td class="nota-baixa" title="Nota 1 (Baixa)"><span class="ponto">1</span></td><td class="nota-baixa" title="Nota 2 (Baixa)"><span class="ponto">2</span><span class="risco">6</span></td><td class="nota-baixa" title="Nota 3 (Baixa)"><span class="ponto">3</span></td><td class="nota-baixa" title="Nota 4 (Baixa)"><span class="ponto">4</span></td><td class="nota-moderada" title="Nota 5 (Moderada)"><span class="ponto">5</span><span class="risco">1</span><span class="risco">2</span></td></tr>
</table>
</div>

**Prioridade (maior nota primeiro):** <span class="nota nota-alta">Alta</span> 3 (12), 4 (10) · <span class="nota nota-moderada">Moderada</span> 5 (9), 7 (8), 8 (6), 9 (6), 1 (5), 2 (5) · <span class="nota nota-baixa">Baixa</span> 6 (2)
<!-- mapa:fim -->

## Riscos

| **Nº** | **Risco** | **O que acontece se ocorrer** | **Por que essa nota** | **Probabilidade** | **Impacto** | **Nota** |
|:---:|---|---|---|:---:|:---:|:---:|
| 1 | Curto ou polaridade invertida ao conectar a bateria | Pode queimar a placa inteira ou causar fogo | Há proteção contra inversão e fusível PPTC ([#200](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/200)); o dano seria grave, mas é muito improvável | Rara (1) | Muito alto (5) | <span class="nota nota-moderada">5</span> |
| 2 | Bateria LiPo inchar, pegar fogo ou explodir | Perda da bateria, risco de queimadura e dano ao robô | A bateria é carregada com balanceamento, seguindo o POP; com esses cuidados, inchar é raro | Rara (1) | Muito alto (5) | <span class="nota nota-moderada">5</span> |
| 3 | Corte por subtensão (BMS e UVLO de 6,4 V) não funcionar | A bateria descarrega demais e estraga, ou o robô desliga sem aviso | O corte por subtensão e o BMS só foram ensaiados em simulação; o teste real ainda não foi feito | Possível (3) | Alto (4) | <span class="nota nota-alta">12</span> |
| 4 | Tensão errada na saída dos reguladores *buck* (3,3 V, 5 V ou 6 V) | Queima o ESP32, os sensores ou os motores | Os *bucks* foram testados em bancada, mas um ajuste errado ou um trimpot solto queima os módulos | Improvável (2) | Muito alto (5) | <span class="nota nota-alta">10</span> |
| 5 | Fio soldado no lugar errado na placa | A placa precisa ser refeita, e algum componente pode queimar | A soldagem da placa ainda vai acontecer; o erro tem conserto, mas custa tempo | Possível (3) | Médio (3) | <span class="nota nota-moderada">9</span> |
| 6 | Carregador quebrar | Sem como carregar a bateria até conseguir outro | O carregador funcionou em todos os ensaios, e dá para pedir outro emprestado | Rara (1) | Baixo (2) | <span class="nota nota-baixa">2</span> |
| 7 | Bateria sem carga suficiente no dia da inspeção | O robô não completa os labirintos na apresentação | O ensaio de autonomia passou; com bateria reserva e o POP, é pouco provável faltar carga | Improvável (2) | Alto (4) | <span class="nota nota-moderada">8</span> |
| 8 | Sem telemetria do consumo da bateria no dashboard | A equipe não acompanha a carga durante a corrida | A leitura no firmware está em andamento; sem ela, ainda dá para medir com multímetro | Possível (3) | Baixo (2) | <span class="nota nota-moderada">6</span> |
| 9 | Integrante da frente sair da disciplina | O conhecimento da frente se perde e as tarefas atrasam | Nenhum integrante saiu até agora, e a documentação da frente está no repositório | Improvável (2) | Médio (3) | <span class="nota nota-moderada">6</span> |

## Possíveis soluções

| **Nº** | **Risco** | **Como evitar** | **Se acontecer** |
|:---:|---|---|---|
| 1 | Curto ou polaridade invertida | Usar conector que só encaixa num sentido e conferir a polaridade com o multímetro antes de ligar. A proteção contra inversão e o fusível PPTC já foram dimensionados ([#200](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/200)). Fazer a primeira energização sem os módulos | Desconectar a bateria, inspecionar a placa e testar cada módulo antes de religar |
| 2 | Bateria inchar ou pegar fogo | Guardar e carregar dentro de LiPo Safe Bag, só carregar com balanceamento ([POP de recarga](../04-projeto-conceitual/energia/pop-recarga-bateria.md)) e proteger a bateria no chassi | Ao menor sinal de inchaço, parar de usar e descartar a bateria |
| 3 | Corte por subtensão falhar | Testar o corte real em bancada ([#218](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/218)) e instalar um alarme de LiPo (*buzzer*) no cabo de balanceamento, como segunda proteção | Recarregar com balanceamento e medir cada célula; se alguma ficar abaixo do limite, trocar a bateria |
| 4 | Tensão errada nos *bucks* | Ajustar e medir cada saída com o multímetro antes de ligar os módulos, fixar o ajuste (cola no trimpot) e medir de novo depois de transportar o robô | Desligar na hora, conferir os módulos que estavam ligados e reajustar o *buck* |
| 5 | Fio soldado errado | Conferir com o esquemático, testar continuidade e curto antes de energizar e pedir que uma segunda pessoa confira | Dessoldar, corrigir a ligação e testar os componentes daquele trecho |
| 6 | Carregador quebrar | Combinar antes um empréstimo de carregador com outra equipe da disciplina | Usar o carregador emprestado até consertar ou comprar outro |
| 7 | Bateria sem carga no dia | Ter uma bateria reserva carregada e, na véspera, mandar no grupo uma foto da tensão total medida | Trocar pela bateria reserva |
| 8 | Sem telemetria da bateria | Ler a tensão no GPIO34 pelo firmware e enviar na telemetria ([#492](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/492), [#215](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/215)) | Mostrar a tensão no monitor serial como plano B e medir com o multímetro entre as corridas |
| 9 | Integrante sair | Manter esquemáticos, cálculos e procedimentos documentados no repositório e fazer as tarefas críticas sempre em dupla | Redistribuir as tarefas entre a frente, com apoio da gerência geral |

## Histórico de versões

| **Versão** | **Data** | **Descrição** | **Autor(es)** | **Revisor(es)** |
|:----------:|:--------:|---------------|---------------|-----------------|
| 1.0 | 04/11/2026 | Matriz de riscos da energia: notas, mapa de calor, motivo de cada nota e possíveis soluções | Equipe de Energia (riscos e soluções); Ana Carolina Fialho (página e notas) | Paulo Henrique Melo de Souza |
