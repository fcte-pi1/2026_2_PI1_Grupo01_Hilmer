# Matriz de riscos

Esta seção reúne os riscos do projeto: o que pode dar errado, o tamanho do estrago se acontecer, a chance de acontecer e o que fazer. Há uma matriz **geral do projeto**, nesta página, e uma **por frente**:

| **Frente** | **Gerente** | **Riscos** | **Maiores notas** |
|------------|-------------|:----------:|-----------------|
| [Software](software.md) | Cláudio Henrique | 12 | Parâmetros provisórios (3) e perda de posição por erro acumulado (12): nota 16; firmware falhar no teste completo (10): nota 15 |
| [Eletrônica](eletronica.md) | Aline de Carvalho Rodrigues | 11 | Conectores dos motores e encoders (3): nota 16; alimentação errada (2): nota 15 |
| [Energia](energia.md) | Paulo Henrique Melo de Souza | 9 | Corte por subtensão não funcionar (3): nota 12 |
| [Estrutura](estrutura.md) | Ana Carolina Nunes | 8 | Peças que não encaixam nos componentes reais (1): nota 12 |

**Avaliação:** 04/11/2026. A matriz é revisada a cada sprint pelo gerente de cada frente: risco resolvido sai da tabela de riscos e continua em "Evolução por sprint", com o desfecho, e risco novo entra com número novo. Depois de mudar uma nota, rode `python docs/scripts/matriz_riscos.py` para atualizar a coluna Nota e o mapa de calor; a tabela "Evolução por sprint" é preenchida à mão em cada fechamento.

## Como ler

Cada risco recebe duas notas de 1 a 5:

| **Nota** | **Probabilidade**: a chance de acontecer | **Impacto**: o tamanho do problema se acontecer |
|:---:|---|---|
| 1 | **Rara**: quase impossível nas condições atuais | **Muito baixo**: quase não atrapalha |
| 2 | **Improvável**: pode acontecer, mas é pouco provável | **Baixo**: problema pontual, resolvido rápido |
| 3 | **Possível**: pode acontecer e precisa de acompanhamento | **Médio**: piora o desempenho e exige ajustes |
| 4 | **Provável**: deve acontecer, pelas pendências ou porque já aconteceu | **Alto**: impede uma função essencial ou exige muito retrabalho |
| 5 | **Muito provável**: quase certo, se nada for feito | **Muito alto**: pode queimar componentes ou parar o robô por completo |

**Nota do risco = probabilidade × impacto**, de 1 a 25. A nota define a cor no mapa de calor e a ordem de prioridade:

| **Nota** | **Faixa** | **O que fazer** |
|:---:|:---:|---|
| 15 a 25 | <span class="nota nota-critica">Crítica</span> | Agir já, nesta sprint, com responsável definido |
| 10 a 14 | <span class="nota nota-alta">Alta</span> | Ter um plano e um responsável, e acompanhar em toda reunião |
| 5 a 9 | <span class="nota nota-moderada">Moderada</span> | Acompanhar e agir se a nota subir |
| 1 a 4 | <span class="nota nota-baixa">Baixa</span> | Só registrar |

**Cores:** verde é a faixa baixa, amarelo a moderada, laranja a alta e vermelho a crítica. Cada célula traz o número da nota, e a lista de prioridade escreve o nome da faixa, então a cor nunca é a única informação.

**O mapa de calor** cruza as duas notas: quanto mais para cima e para a direita, mais grave. Cada número no mapa é um risco da tabela logo abaixo dele, e o número pequeno no canto da célula é a nota daquela posição.

## Matriz geral do projeto

Riscos que envolvem mais de uma frente ou o projeto inteiro.

<!-- mapa:inicio -->
<div class="mapa-calor" markdown="0">
<table>
<tr><th class="canto" rowspan="2">Probabilidade ↓</th><th colspan="5">Impacto →</th></tr>
<tr><th>Muito baixo<br><small>1</small></th><th>Baixo<br><small>2</small></th><th>Médio<br><small>3</small></th><th>Alto<br><small>4</small></th><th>Muito alto<br><small>5</small></th></tr>
<tr><th>Muito provável<br><small>5</small></th><td class="nota-moderada" title="Nota 5 (Moderada)"><span class="ponto">5</span></td><td class="nota-alta" title="Nota 10 (Alta)"><span class="ponto">10</span></td><td class="nota-critica" title="Nota 15 (Crítica)"><span class="ponto">15</span></td><td class="nota-critica" title="Nota 20 (Crítica)"><span class="ponto">20</span></td><td class="nota-critica" title="Nota 25 (Crítica)"><span class="ponto">25</span></td></tr>
<tr><th>Provável<br><small>4</small></th><td class="nota-baixa" title="Nota 4 (Baixa)"><span class="ponto">4</span></td><td class="nota-moderada" title="Nota 8 (Moderada)"><span class="ponto">8</span></td><td class="nota-alta" title="Nota 12 (Alta)"><span class="ponto">12</span><span class="risco">6</span></td><td class="nota-critica" title="Nota 16 (Crítica)"><span class="ponto">16</span></td><td class="nota-critica" title="Nota 20 (Crítica)"><span class="ponto">20</span><span class="risco">3</span></td></tr>
<tr><th>Possível<br><small>3</small></th><td class="nota-baixa" title="Nota 3 (Baixa)"><span class="ponto">3</span></td><td class="nota-moderada" title="Nota 6 (Moderada)"><span class="ponto">6</span></td><td class="nota-moderada" title="Nota 9 (Moderada)"><span class="ponto">9</span><span class="risco">7</span></td><td class="nota-alta" title="Nota 12 (Alta)"><span class="ponto">12</span><span class="risco">1</span><span class="risco">4</span><span class="risco">8</span></td><td class="nota-critica" title="Nota 15 (Crítica)"><span class="ponto">15</span></td></tr>
<tr><th>Improvável<br><small>2</small></th><td class="nota-baixa" title="Nota 2 (Baixa)"><span class="ponto">2</span></td><td class="nota-baixa" title="Nota 4 (Baixa)"><span class="ponto">4</span><span class="risco">2</span></td><td class="nota-moderada" title="Nota 6 (Moderada)"><span class="ponto">6</span><span class="risco">5</span></td><td class="nota-moderada" title="Nota 8 (Moderada)"><span class="ponto">8</span></td><td class="nota-alta" title="Nota 10 (Alta)"><span class="ponto">10</span></td></tr>
<tr><th>Rara<br><small>1</small></th><td class="nota-baixa" title="Nota 1 (Baixa)"><span class="ponto">1</span></td><td class="nota-baixa" title="Nota 2 (Baixa)"><span class="ponto">2</span></td><td class="nota-baixa" title="Nota 3 (Baixa)"><span class="ponto">3</span></td><td class="nota-baixa" title="Nota 4 (Baixa)"><span class="ponto">4</span></td><td class="nota-moderada" title="Nota 5 (Moderada)"><span class="ponto">5</span></td></tr>
</table>
</div>

**Prioridade (maior nota primeiro):** <span class="nota nota-critica">Crítica</span> 3 (20) · <span class="nota nota-alta">Alta</span> 1 (12), 4 (12), 6 (12), 8 (12) · <span class="nota nota-moderada">Moderada</span> 7 (9), 5 (6) · <span class="nota nota-baixa">Baixa</span> 2 (4)
<!-- mapa:fim -->

## Riscos gerais

| **Nº** | **Risco** | **O que acontece se ocorrer** | **Frentes** | **Por que essa nota** | **Probabilidade** | **Impacto** | **Nota** |
|:---:|---|---|:---:|---|:---:|:---:|:---:|
| 1 | O robô montado no chassi fica pronto tarde | Sem tempo para os testes em pista e para ajustar o robô antes da apresentação (02/12) | Todas | O chassi final e o labirinto físico ainda não foram fabricados, e a ED2 é em 23/11 | Possível (3) | Alto (4) | <span class="nota nota-alta">12</span> |
| 2 | A troca para a ESP32 DevKit gera retrabalho em várias frentes | Esquemático e documentação ([#488](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/488)), suporte no chassi e leitura da bateria precisam ser refeitos | Eletrônica, estrutura, energia | A bancada geral já foi testada na DevKit; a migração do firmware do robô ([#489](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/489)) e as demais tarefas estão abertas | Improvável (2) | Baixo (2) | <span class="nota nota-baixa">4</span> |
| 3 | O robô não completa o labirinto dentro das regras (10 minutos por labirinto, sem ficar preso) | O objetivo principal do projeto não é cumprido na apresentação | Todas | A ED2 é em 23/11 e o robô ainda não andou em pista | Provável (4) | Muito alto (5) | <span class="nota nota-critica">20</span> |
| 4 | Falha de energia no dia: bateria sem carga ou ESP32 reiniciando com os motores | A corrida para no meio ou nem começa | Energia, eletrônica, software | A queda de tensão com os motores ainda não foi medida | Possível (3) | Alto (4) | <span class="nota nota-alta">12</span> |
| 5 | Saída de integrantes ou conhecimento concentrado em poucas pessoas | Tarefas param e ninguém sabe continuar | Todas | Ninguém saiu até agora, e as tarefas são feitas em dupla | Improvável (2) | Médio (3) | <span class="nota nota-moderada">6</span> |
| 6 | Projects e documentação desatualizados | A avaliação, feita pelo GitHub, não mostra todo o trabalho feito | Todas | Já acontece: em 03/11, havia 38 issues da estrutura abertas com prazo vencido, e a `main` estava 264 commits atrás da `documentacao`; dá para corrigir rápido | Provável (4) | Médio (3) | <span class="nota nota-alta">12</span> |
| 7 | A telemetria falha na demonstração (Wi-Fi da sala ou banco) | O dashboard fica sem dados ao vivo | Software, energia | O Wi-Fi de sala costuma ser instável, mas o robô reenvia as amostras quando a rede volta | Possível (3) | Médio (3) | <span class="nota nota-moderada">9</span> |
| 8 | Um componente queima e não há peça de reposição | O robô fica parado até a compra chegar | Eletrônica, energia | Não há peças reserva registradas, e os testes com os motores ligados ainda vão acontecer | Possível (3) | Alto (4) | <span class="nota nota-alta">12</span> |

## Possíveis soluções

| **Nº** | **Risco** | **Como evitar** | **Se acontecer** |
|:---:|---|---|---|
| 1 | Robô pronto tarde | Montagem, teste integrado e liberação do hardware até 18/11, com responsáveis e sprints no Projects. Bancada conjunta fixa entre eletrônica e software toda semana | Cortar o que não é essencial para a corrida e concentrar todas as frentes em sensores, motores e encoders |
| 2 | Retrabalho da troca de placa | Uma pinagem só (o `Pinos.h`), copiada no esquemático e no `pinagem.csv` ([#488](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/488)). Cada frente com sua tarefa da troca já aberta: firmware ([#489](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/489)), suporte ([#277](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/277)) e bateria ([#492](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/492)) | Priorizar o que impede o robô de andar e deixar o acabamento para depois da ED2 |
| 3 | Robô não completa o labirinto | Testar no labirinto físico assim que ele estiver montado, começando pelo 4×4, e medir o tempo de cada corrida | Usar os dados da telemetria e do mapa para achar onde o robô errou e ajustar os parâmetros |
| 4 | Falha de energia no dia | Bateria reserva carregada, conferência da tensão na véspera e teste dos motores com a placa completa para ver se o ESP32 reinicia | Trocar pela bateria reserva e medir as tensões com o multímetro antes de tentar de novo |
| 5 | Saída de integrantes | Tarefas em dupla, tudo documentado no repositório e revisão de PR por outra pessoa | A gerência geral redistribui as tarefas com os gerentes |
| 6 | Projects desatualizado | Cada gerente fecha as issues feitas e move as atrasadas no fim de cada sprint. Fazer o merge da `documentacao` na `main` com antecedência antes da ED2, porque as duas branches divergiram (cada uma tem commits que a outra não tem) e podem dar conflito | Revisar o Projects na reunião geral e corrigir na hora |
| 7 | Telemetria falha na demo | Definir a rede da demonstração e um plano B, como o hotspot de um celular ([#491](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/491)). O robô guarda e reenvia as amostras quando a rede volta ([#238](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/238)) | Trocar para a rede reserva e mostrar o histórico da corrida depois que os dados chegarem |
| 8 | Componente queimado sem reposição | Ter peças reserva dos itens mais frágeis (driver, reguladores, sensores) e seguir os cuidados de energização das matrizes de eletrônica e energia | Trocar pela peça reserva e registrar a causa para não repetir |

## Evolução por sprint

Nota de cada risco (probabilidade × impacto) ao fim de cada sprint, com uma frase sobre o que mudou. As colunas das sprints seguintes são preenchidas nos fechamentos de 11/11, 18/11 e 23/11. Risco resolvido sai da tabela de riscos e continua aqui, com o desfecho.

| **Nº** | **Risco** | **Sprint 6 (até 04/11)** | **Sprint 7 (até 11/11)** | **Sprint 8 (até 18/11)** | **Fechamento (até 23/11)** |
|:---:|---|:---:|---|---|---|
| 1 | O robô montado no chassi fica pronto tarde | 12 | — | — | — |
| 2 | A troca para a ESP32 DevKit gera retrabalho em várias frentes | 4 | — | — | — |
| 3 | O robô não completa o labirinto dentro das regras (10 minutos por labirinto, sem ficar preso) | 20 | — | — | — |
| 4 | Falha de energia no dia: bateria sem carga ou ESP32 reiniciando com os motores | 12 | — | — | — |
| 5 | Saída de integrantes ou conhecimento concentrado em poucas pessoas | 6 | — | — | — |
| 6 | Projects e documentação desatualizados | 12 | — | — | — |
| 7 | A telemetria falha na demonstração (Wi-Fi da sala ou banco) | 9 | — | — | — |
| 8 | Um componente queima e não há peça de reposição | 12 | — | — | — |

## Histórico de versões

| **Versão** | **Data** | **Descrição** | **Autor(es)** | **Revisor(es)** |
|:----------:|:--------:|---------------|---------------|-----------------|
| 1.0 | 04/11/2026 | Criação da seção: escala de notas de 1 a 5, mapa de calor, matriz geral do projeto com motivo de cada nota, soluções e matrizes por frente | Ana Carolina Fialho | Cláudio Henrique, Aline de Carvalho Rodrigues, Paulo Henrique Melo de Souza e Ana Carolina Nunes |
| 1.1 | 04/11/2026 | Revisão do PR #500: textos dos riscos 2 e 6, legenda das cores e tabela de evolução por sprint | Cláudio Henrique | — |
