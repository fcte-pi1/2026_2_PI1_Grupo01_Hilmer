# Matriz de riscos: Software

**Gerente:** Cláudio Henrique. **Horizonte:** entrega da ED2 (23/11/2026). **Avaliação:** 04/11/2026, a partir da lista de riscos da equipe de software.

Como ler o impacto e a probabilidade: veja [Matriz de riscos](README.md#como-ler).

## Matriz

<!-- mapa:inicio -->
<div class="mapa-calor" markdown="0">
<table>
<tr><th class="canto" rowspan="2">Probabilidade ↓</th><th colspan="5">Impacto →</th></tr>
<tr><th>Muito baixo<br><small>1</small></th><th>Baixo<br><small>2</small></th><th>Médio<br><small>3</small></th><th>Alto<br><small>4</small></th><th>Muito alto<br><small>5</small></th></tr>
<tr><th>Muito provável<br><small>5</small></th><td class="nota-moderada" title="Nota 5 (Moderada)"><span class="ponto">5</span></td><td class="nota-alta" title="Nota 10 (Alta)"><span class="ponto">10</span></td><td class="nota-critica" title="Nota 15 (Crítica)"><span class="ponto">15</span></td><td class="nota-critica" title="Nota 20 (Crítica)"><span class="ponto">20</span></td><td class="nota-critica" title="Nota 25 (Crítica)"><span class="ponto">25</span></td></tr>
<tr><th>Provável<br><small>4</small></th><td class="nota-baixa" title="Nota 4 (Baixa)"><span class="ponto">4</span></td><td class="nota-moderada" title="Nota 8 (Moderada)"><span class="ponto">8</span></td><td class="nota-alta" title="Nota 12 (Alta)"><span class="ponto">12</span><span class="risco">6</span><span class="risco">7</span></td><td class="nota-critica" title="Nota 16 (Crítica)"><span class="ponto">16</span><span class="risco">3</span><span class="risco">12</span></td><td class="nota-critica" title="Nota 20 (Crítica)"><span class="ponto">20</span></td></tr>
<tr><th>Possível<br><small>3</small></th><td class="nota-baixa" title="Nota 3 (Baixa)"><span class="ponto">3</span></td><td class="nota-moderada" title="Nota 6 (Moderada)"><span class="ponto">6</span></td><td class="nota-moderada" title="Nota 9 (Moderada)"><span class="ponto">9</span><span class="risco">1</span><span class="risco">2</span></td><td class="nota-alta" title="Nota 12 (Alta)"><span class="ponto">12</span><span class="risco">5</span><span class="risco">8</span></td><td class="nota-critica" title="Nota 15 (Crítica)"><span class="ponto">15</span><span class="risco">10</span></td></tr>
<tr><th>Improvável<br><small>2</small></th><td class="nota-baixa" title="Nota 2 (Baixa)"><span class="ponto">2</span></td><td class="nota-baixa" title="Nota 4 (Baixa)"><span class="ponto">4</span></td><td class="nota-moderada" title="Nota 6 (Moderada)"><span class="ponto">6</span></td><td class="nota-moderada" title="Nota 8 (Moderada)"><span class="ponto">8</span></td><td class="nota-alta" title="Nota 10 (Alta)"><span class="ponto">10</span><span class="risco">4</span></td></tr>
<tr><th>Rara<br><small>1</small></th><td class="nota-baixa" title="Nota 1 (Baixa)"><span class="ponto">1</span></td><td class="nota-baixa" title="Nota 2 (Baixa)"><span class="ponto">2</span><span class="risco">11</span></td><td class="nota-baixa" title="Nota 3 (Baixa)"><span class="ponto">3</span><span class="risco">9</span></td><td class="nota-baixa" title="Nota 4 (Baixa)"><span class="ponto">4</span></td><td class="nota-moderada" title="Nota 5 (Moderada)"><span class="ponto">5</span></td></tr>
</table>
</div>

**Prioridade (maior nota primeiro):** <span class="nota nota-critica">Crítica</span> 3 (16), 12 (16), 10 (15) · <span class="nota nota-alta">Alta</span> 5 (12), 6 (12), 7 (12), 8 (12), 4 (10) · <span class="nota nota-moderada">Moderada</span> 1 (9), 2 (9) · <span class="nota nota-baixa">Baixa</span> 9 (3), 11 (2)
<!-- mapa:fim -->

## Riscos

| **Nº** | **Risco** | **O que acontece se ocorrer** | **Por que essa nota** | **Probabilidade** | **Impacto** | **Nota** |
|:---:|---|---|---|:---:|:---:|:---:|
| 1 | Retrabalho no firmware (troca para a ESP32 DevKit e remoção do código do LiDAR) | Tempo gasto refazendo o que já funcionava, em vez de avançar | A bancada geral já roda na DevKit, mas a migração do firmware do robô ([#489](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/489)) e a remoção do código do LiDAR ([#481](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/481)) estão abertas | Possível (3) | Médio (3) | <span class="nota nota-moderada">9</span> |
| 2 | Atraso na integração com o hardware definitivo (ESP32 DevKit) | Pouco tempo para testar o firmware no robô de verdade antes da ED2 | Os testes de integração com a DevKit foram feitos, mas o teste no robô depende do chassi, que não está pronto | Possível (3) | Médio (3) | <span class="nota nota-moderada">9</span> |
| 3 | Os parâmetros provisórios (limiares, viés, pulsos por volta) não funcionam no robô real | O robô lê paredes errado, anda distâncias erradas e gira errado | Limiares, viés e pulsos por volta ainda são provisórios; um valor errado faz o robô ler paredes e andar errado | Provável (4) | Alto (4) | <span class="nota nota-critica">16</span> |
| 4 | Um teste de firmware danifica o hardware | Motor, driver ou sensor queimado, sem tempo para repor | Limite de duty de 70%, motores freados no boot e testes de 2 s na bancada; nenhum dano até agora, mas uma polaridade invertida na fonte, na bancada, a fez desarmar | Improvável (2) | Muito alto (5) | <span class="nota nota-alta">10</span> |
| 5 | O ESP32 reinicia por queda de tensão | A corrida para no meio e o robô perde o mapa | A queda de tensão com os motores não foi medida; se o ESP32 reiniciar na corrida, o mapa é perdido | Possível (3) | Alto (4) | <span class="nota nota-alta">12</span> |
| 6 | Conhecimento concentrado em poucas pessoas e baixa disponibilidade da equipe | Tarefas param quando alguém falta, e ninguém consegue continuar | A maior parte dos PRs de software é de uma pessoa; as tarefas param quando alguém falta | Provável (4) | Médio (3) | <span class="nota nota-alta">12</span> |
| 7 | Dependência de outras equipes (placa montada, labirinto, bateria) | O software fica esperando para testar | O software depende do chassi, do labirinto físico e da bateria, que ainda não estão prontos | Provável (4) | Médio (3) | <span class="nota nota-alta">12</span> |
| 8 | Falhas de integração aparecem tarde, porque ainda não há teste de sistema | Problemas descobertos perto da ED2, sem tempo para corrigir | O teste ponta a ponta e a corrida completa ainda não foram feitos | Possível (3) | Alto (4) | <span class="nota nota-alta">12</span> |
| 9 | Falta memória ou processamento no ESP32 | O firmware não cabe ou fica lento | O firmware do robô (ambiente atual, Super Mini) usa 74% da Flash e 27% da RAM, e a bancada da DevKit usa 76% e 24%, com folga | Rara (1) | Médio (3) | <span class="nota nota-baixa">3</span> |
| 10 | O firmware não funciona no teste completo | O robô não completa a corrida na ED2 ou na apresentação | A corrida completa no labirinto nunca foi feita; se falhar, o robô não cumpre o objetivo | Possível (3) | Muito alto (5) | <span class="nota nota-critica">15</span> |
| 11 | Limites do plano gratuito do Supabase | O banco recusa dados ou o projeto fica pausado, e o dashboard fica sem telemetria | O envio é em lotes, e o volume de dados é pequeno para o plano gratuito | Rara (1) | Baixo (2) | <span class="nota nota-baixa">2</span> |
| 12 | O robô perde a posição por erro acumulado | O mapa fica errado e o robô se perde no labirinto | Erro acumulado é o problema mais comum no micromouse, e o robô ainda não andou em pista | Provável (4) | Alto (4) | <span class="nota nota-critica">16</span> |

## Possíveis soluções

| **Nº** | **Risco** | **Como evitar** | **Se acontecer** |
|:---:|---|---|---|
| 1 | Retrabalho no firmware | Manter todos os pinos só no `Pinos.h` e os drivers atrás de interfaces, para a troca de placa mexer em poucos arquivos. Migrar em PRs pequenos ([#489](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/489), [#481](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/481)), com os 797 testes nativos e o CI conferindo que nada quebrou. Congelar o escopo do firmware em 16/11, aceitando depois só correção de defeito | Voltar ao último commit que passava nos testes e refazer a mudança em partes menores |
| 2 | Atraso na integração | Testar cada componente assim que ele é montado, com a bancada geral ([#477](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/477)). Seguir desenvolvendo com o simulador de labirinto enquanto o hardware não chega. Ter um dia fixo de bancada com a eletrônica | Priorizar o caminho mínimo da corrida (sensores, motores e encoders) e deixar o resto para depois da ED2 |
| 3 | Parâmetros provisórios | Medir os valores reais na bancada ([#479](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/479), [#490](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/490), [#493](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/493)) e manter os parâmetros num só lugar. Usar a calibração no boot ([#240](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/240)) | Ajustar o parâmetro na bancada, registrar o valor novo e rodar os testes nativos de novo. Se faltar tempo, usar um perfil conservador (velocidade e ganhos baixos): uma volta lenta ainda cumpre o objetivo em 10 minutos |
| 4 | Dano ao hardware | Manter o limite de duty dos motores (70%), os motores freados no boot e os testes de no máximo 2 s na bancada. Fazer o primeiro teste com as rodas fora do chão e com fonte limitada em corrente | Parar os testes daquele componente, avisar a eletrônica e registrar o que causou o dano |
| 5 | Reinício por queda de tensão | Mostrar na serial e na telemetria o motivo do último reinício (a incluir na [#489](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/489)). Acelerar os motores aos poucos, e não de uma vez. Com a energia, validar o *buck* dos motores e os capacitores ([#214](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/214), [#216](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/216)) | Reiniciar a corrida e levar a medição de tensão para a energia investigar a causa |
| 6 | Conhecimento concentrado | Toda tarefa em dupla, com documentação na página da tarefa e revisão de PR feita por outra pessoa, como nas [#487](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/487) a [#493](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/493) | Quem estava em dupla assume a tarefa; a gerência redistribui o restante |
| 7 | Dependência de outras equipes | Tarefas compartilhadas com um responsável de cada time e prazo escrito na issue ([#488](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/488), [#489](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/489), [#490](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/490) e [#492](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/492)). Avisar no grupo assim que algo atrasar | Testar com o simulador ou com a bancada parcial e marcar o teste real para a próxima sprint |
| 8 | Falhas de integração tardias | Rodar os testes de integração do firmware (robô simulado no labirinto) em todo PR. Fazer o teste ponta a ponta ([#232](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/232)) e a corrida completa ([#185](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/185)) antes de 18/11, com folga para corrigir | Registrar a falha como issue, corrigir pela ordem de impacto na corrida e repetir o teste |
| 9 | Memória e processamento | Acompanhar o uso de RAM e Flash a cada build do CI e evitar alocação dinâmica dentro do laço principal. Conferir pilhas e prioridades das tarefas na migração para a DevKit ([#489](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/489)) | Remover código que não é usado (como o do LiDAR) e reduzir os *buffers* |
| 10 | Firmware falhar no teste completo | Fazer um ensaio geral completo antes da ED2 e da APT, marcar no Git a versão estável usada na demonstração e deixar a bancada geral pronta para diagnóstico. Gravar a versão estável em duas placas (uma reserva) e filmar os ensaios como backup | Gravar a versão estável marcada e diagnosticar com a bancada geral. Se o robô não correr, mostrar a bancada com o dashboard ao vivo e os vídeos dos ensaios |
| 11 | Plano gratuito do Supabase | Enviar a telemetria em lotes (já é assim, [#478](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/478)), apagar os dados de teste e acompanhar o uso no painel. Usar o projeto toda semana para ele não ser pausado por inatividade | Exportar os dados, limpar as tabelas de teste e reativar o projeto pelo painel. Se não bastar, criar outro projeto Supabase, reaplicar as migrações e publicar as funções, que estão no repositório |
| 12 | Perda de posição | Corrigir a posição pelas paredes ([#188](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/188)), manter o rumo pelo giroscópio ([#190](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/190)), usar os encoders ([#475](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/475)) e validar 1 célula e o giro de 90° na bancada ([#493](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/493)) | Realinhar o robô numa parede conhecida e seguir com o mapa já explorado |

## Responsável e sinal de alerta

| **Nº** | **Risco** | **Responsável** | **Sinal de alerta** |
|:---:|---|---|---|
| 1 | Retrabalho no firmware | Cláudio | Mais de duas issues de adaptação abertas depois de 11/11, ou PR que altere `Pinos.h` ou drivers depois de 16/11 |
| 2 | Atraso na integração | Cláudio e Aline | Causa do motor direito sem solução até 06/11, esquemático da DevKit fora do prazo ou robô não energizado até 16/11 |
| 3 | Parâmetros provisórios | Rafael e Ana Carolina | Erro de uma célula acima de 3%, giro de 90° fora de ±5° ou robô encostando nas paredes na primeira volta |
| 4 | Dano ao hardware | Cláudio e Elias | Peça aquecendo, fonte desarmando ou motor zumbindo sem girar por mais de 2 s |
| 5 | Reinício por queda de tensão | Elias e Paulo Henrique | Reinício inesperado com os dois motores ligados, ou queda da bateria acima de 1 V na tecla `i` da bancada |
| 6 | Conhecimento concentrado | Cláudio | Issue sem avanço por 3 dias, ou pessoa indisponível por mais de 2 dias nas Sprints 7 e 8 |
| 7 | Dependência de outras equipes | Ana Carolina Fialho e Cláudio | Entrega de outra equipe atrasada mais de 2 dias em relação ao prazo da issue |
| 8 | Falhas de integração tardias | Cláudio | Defeito novo encontrado em teste integrado depois de 16/11 |
| 9 | Memória e processamento | Ana Carolina | Flash acima de 85%, pilha livre abaixo de 512 bytes ou ciclo de navegação acima de 10 ms |
| 10 | Firmware falhar no teste completo | Cláudio e Ana Carolina | Falha no ensaio geral de 22/11 sem correção no mesmo dia |
| 11 | Plano gratuito do Supabase | Marllon | Painel acima de 70% de qualquer limite, ou projeto pausado |
| 12 | Perda de posição | Cláudio e Ana Carolina | Erro de uma célula acima de 3% ou pose inválida registrada no log |

## Evolução por sprint

Nota de cada risco (probabilidade × impacto) ao fim de cada sprint, com uma frase sobre o que mudou. As colunas das sprints seguintes são preenchidas nos fechamentos de 11/11, 18/11 e 23/11. Risco resolvido sai da tabela de riscos e continua aqui, com o desfecho.

| **Nº** | **Risco** | **Sprint 6 (até 04/11)** | **Sprint 7 (até 11/11)** | **Sprint 8 (até 18/11)** | **Fechamento (até 23/11)** |
|:---:|---|:---:|---|---|---|
| 1 | Retrabalho no firmware (troca para a ESP32 DevKit e remoção do código do LiDAR) | 9 | — | — | — |
| 2 | Atraso na integração com o hardware definitivo (ESP32 DevKit) | 9 | — | — | — |
| 3 | Os parâmetros provisórios (limiares, viés, pulsos por volta) não funcionam no robô real | 16 | — | — | — |
| 4 | Um teste de firmware danifica o hardware | 10 | — | — | — |
| 5 | O ESP32 reinicia por queda de tensão | 12 | — | — | — |
| 6 | Conhecimento concentrado em poucas pessoas e baixa disponibilidade da equipe | 12 | — | — | — |
| 7 | Dependência de outras equipes (placa montada, labirinto, bateria) | 12 | — | — | — |
| 8 | Falhas de integração aparecem tarde, porque ainda não há teste de sistema | 12 | — | — | — |
| 9 | Falta memória ou processamento no ESP32 | 3 | — | — | — |
| 10 | O firmware não funciona no teste completo | 15 | — | — | — |
| 11 | Limites do plano gratuito do Supabase | 2 | — | — | — |
| 12 | O robô perde a posição por erro acumulado | 16 | — | — | — |

## Histórico de versões

| **Versão** | **Data** | **Descrição** | **Autor(es)** | **Revisor(es)** |
|:----------:|:--------:|---------------|---------------|-----------------|
| 1.0 | 04/11/2026 | Matriz de riscos do software para a ED2: notas, mapa de calor, motivo de cada nota e possíveis soluções | Equipe de Software (riscos); Ana Carolina Fialho (página, notas e soluções) | Cláudio Henrique |
| 1.1 | 04/11/2026 | Revisão do PR #500: notas e textos dos riscos 1, 4 e 9, novas soluções, responsável e sinal de alerta por risco, e evolução por sprint | Cláudio Henrique | — |
