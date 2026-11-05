# Matriz de riscos: Estrutura

**Gerente:** Ana Carolina Nunes. **Avaliação:** 04/11/2026, a partir do levantamento da equipe de estrutura.

Como ler o impacto e a probabilidade: veja [Matriz de riscos](README.md#como-ler).

## Matriz

<!-- mapa:inicio -->
<div class="mapa-calor" markdown="0">
<table>
<tr><th class="canto" rowspan="2">Probabilidade ↓</th><th colspan="5">Impacto →</th></tr>
<tr><th>Muito baixo<br><small>1</small></th><th>Baixo<br><small>2</small></th><th>Médio<br><small>3</small></th><th>Alto<br><small>4</small></th><th>Muito alto<br><small>5</small></th></tr>
<tr><th>Muito provável<br><small>5</small></th><td class="nota-moderada" title="Nota 5 (Moderada)"><span class="ponto">5</span></td><td class="nota-alta" title="Nota 10 (Alta)"><span class="ponto">10</span></td><td class="nota-critica" title="Nota 15 (Crítica)"><span class="ponto">15</span></td><td class="nota-critica" title="Nota 20 (Crítica)"><span class="ponto">20</span></td><td class="nota-critica" title="Nota 25 (Crítica)"><span class="ponto">25</span></td></tr>
<tr><th>Provável<br><small>4</small></th><td class="nota-baixa" title="Nota 4 (Baixa)"><span class="ponto">4</span></td><td class="nota-moderada" title="Nota 8 (Moderada)"><span class="ponto">8</span><span class="risco">6</span></td><td class="nota-alta" title="Nota 12 (Alta)"><span class="ponto">12</span><span class="risco">1</span></td><td class="nota-critica" title="Nota 16 (Crítica)"><span class="ponto">16</span></td><td class="nota-critica" title="Nota 20 (Crítica)"><span class="ponto">20</span></td></tr>
<tr><th>Possível<br><small>3</small></th><td class="nota-baixa" title="Nota 3 (Baixa)"><span class="ponto">3</span></td><td class="nota-moderada" title="Nota 6 (Moderada)"><span class="ponto">6</span><span class="risco">7</span></td><td class="nota-moderada" title="Nota 9 (Moderada)"><span class="ponto">9</span><span class="risco">3</span></td><td class="nota-alta" title="Nota 12 (Alta)"><span class="ponto">12</span></td><td class="nota-critica" title="Nota 15 (Crítica)"><span class="ponto">15</span></td></tr>
<tr><th>Improvável<br><small>2</small></th><td class="nota-baixa" title="Nota 2 (Baixa)"><span class="ponto">2</span></td><td class="nota-baixa" title="Nota 4 (Baixa)"><span class="ponto">4</span><span class="risco">2</span><span class="risco">4</span><span class="risco">8</span></td><td class="nota-moderada" title="Nota 6 (Moderada)"><span class="ponto">6</span></td><td class="nota-moderada" title="Nota 8 (Moderada)"><span class="ponto">8</span></td><td class="nota-alta" title="Nota 10 (Alta)"><span class="ponto">10</span><span class="risco">5</span></td></tr>
<tr><th>Rara<br><small>1</small></th><td class="nota-baixa" title="Nota 1 (Baixa)"><span class="ponto">1</span></td><td class="nota-baixa" title="Nota 2 (Baixa)"><span class="ponto">2</span></td><td class="nota-baixa" title="Nota 3 (Baixa)"><span class="ponto">3</span></td><td class="nota-baixa" title="Nota 4 (Baixa)"><span class="ponto">4</span></td><td class="nota-moderada" title="Nota 5 (Moderada)"><span class="ponto">5</span></td></tr>
</table>
</div>

**Prioridade (maior nota primeiro):** <span class="nota nota-alta">Alta</span> 1 (12), 5 (10) · <span class="nota nota-moderada">Moderada</span> 3 (9), 6 (8), 7 (6) · <span class="nota nota-baixa">Baixa</span> 2 (4), 4 (4), 8 (4)
<!-- mapa:fim -->

## Riscos

| **Nº** | **Risco** | **O que acontece se ocorrer** | **Por que essa nota** | **Probabilidade** | **Impacto** | **Nota** |
|:---:|---|---|---|:---:|:---:|:---:|
| 1 | As peças não encaixam nos componentes reais (medidas, folgas ou alinhamento diferentes do CAD) | A montagem trava e os componentes ficam mal presos | A troca para a DevKit, que é maior, obriga a refazer o suporte da placa | Provável (4) | Médio (3) | <span class="nota nota-alta">12</span> |
| 2 | As bases da estrutura ficam desalinhadas | O robô fica instável, a montagem fica difícil e surgem esforços que podem quebrar peças | Na maioria dos casos, um ajuste na montagem resolve | Improvável (2) | Baixo (2) | <span class="nota nota-baixa">4</span> |
| 3 | A estrutura é frágil e deforma ou quebra | Peças quebram na montagem, no transporte ou numa batida durante a corrida | A estrutura ainda não passou por teste físico, e batidas durante a corrida são esperadas | Possível (3) | Médio (3) | <span class="nota nota-moderada">9</span> |
| 4 | As paredes removíveis do labirinto não encaixam ou travam (rasgos ou cavilhas) | Fica difícil montar e trocar a configuração do labirinto | Os encaixes foram validados em CAD (testes 1.1 a 1.6); um ajuste de folga resolve | Improvável (2) | Baixo (2) | <span class="nota nota-baixa">4</span> |
| 5 | O robô não passa pelos corredores do labirinto | O robô fica preso e não completa a corrida | O robô tem no máximo 16,5 cm e o corredor, cerca de 168 mm: é improvável, mas impede a corrida | Improvável (2) | Muito alto (5) | <span class="nota nota-alta">10</span> |
| 6 | É preciso imprimir de novo a estrutura do robô | Gasto extra de material e atraso no cronograma | Reimprimir é comum na impressão 3D; custa tempo e material, mas não impede a corrida | Provável (4) | Baixo (2) | <span class="nota nota-moderada">8</span> |
| 7 | É preciso fabricar de novo partes do labirinto | Atraso nos testes em pista de software e energia | O labirinto físico ainda não foi fabricado; refazer uma parede custa pouco | Possível (3) | Baixo (2) | <span class="nota nota-moderada">6</span> |
| 8 | Mudanças no CAD sem registro de versão | Peças fabricadas a partir de um arquivo antigo, diferente do validado | Os arquivos ficam no repositório, com o histórico do Git | Improvável (2) | Baixo (2) | <span class="nota nota-baixa">4</span> |

## Possíveis soluções

| **Nº** | **Risco** | **Como evitar** | **Se acontecer** |
|:---:|---|---|---|
| 1 | Peças não encaixam | Medir os componentes reais antes de fabricar, conferir os encaixes no CAD com as tolerâncias de impressão, deixar folga para o que precisa sair e testar a montagem antes da impressão final | Ajustar o CAD e imprimir de novo só a peça com problema |
| 2 | Bases desalinhadas | Conferir o alinhamento dos pontos de fixação, usar pinos ou guias de posicionamento e testar a montagem completa | Revisar os furos de fixação; se não der para ajustar na montagem, imprimir as bases de novo |
| 3 | Estrutura frágil | Reforçar as regiões que recebem mais esforço (fixações e suportes), escolher bem a orientação e os parâmetros de impressão, distribuir o peso dos componentes e fazer teste físico de resistência | Reforçar a geometria da região que falhou e imprimir uma nova versão |
| 4 | Paredes do labirinto travam | Conferir as folgas dos encaixes, testar cada tipo de parede antes de montar o labirinto inteiro e verificar se há atrito | Ajustar as medidas dos encaixes e reimprimir as peças com interferência |
| 5 | Robô não passa | Medir a largura e o comprimento máximos do robô com rodas e saliências, conferir a largura real dos corredores montados e testar a passagem em vários pontos, inclusive nas curvas | Reduzir o que estiver para fora no robô ou corrigir as paredes fora de medida, sem sair dos requisitos (16,5 cm) |
| 6 | Reimprimir a estrutura | Validar o modelo 3D antes de imprimir, conferir as medidas nos arquivos de fabricação e imprimir primeiro as peças críticas para testar o encaixe | Corrigir o CAD, registrar a mudança e reimprimir começando pelas peças que falharam |
| 7 | Refabricar o labirinto | Conferir as medidas do modelo, o alinhamento das paredes e o encaixe das partes antes de fabricar tudo | Ajustar o modelo, reimprimir as peças com problema e medir de novo depois de montar |
| 8 | CAD sem versão | Versionar os arquivos de CAD e os desenhos no repositório, registrar cada mudança e o motivo, e ligar a mudança à tarefa no GitHub Projects | Identificar a versão válida de cada peça e conferir os arquivos usados na fabricação |

**Observação da equipe:** os riscos que impedem o robô de passar no labirinto (5), que afetam a resistência da estrutura (3) e o encaixe dos componentes (1) merecem atenção primeiro, porque impedem a corrida.

## Evolução por sprint

Nota de cada risco (probabilidade × impacto) ao fim de cada sprint, com uma frase sobre o que mudou. As colunas das sprints seguintes são preenchidas nos fechamentos de 11/11, 18/11 e 23/11. Risco resolvido sai da tabela de riscos e continua aqui, com o desfecho.

| **Nº** | **Risco** | **Sprint 6 (até 04/11)** | **Sprint 7 (até 11/11)** | **Sprint 8 (até 18/11)** | **Fechamento (até 23/11)** |
|:---:|---|:---:|---|---|---|
| 1 | As peças não encaixam nos componentes reais (medidas, folgas ou alinhamento diferentes do CAD) | 12 | — | — | — |
| 2 | As bases da estrutura ficam desalinhadas | 4 | — | — | — |
| 3 | A estrutura é frágil e deforma ou quebra | 9 | — | — | — |
| 4 | As paredes removíveis do labirinto não encaixam ou travam (rasgos ou cavilhas) | 4 | — | — | — |
| 5 | O robô não passa pelos corredores do labirinto | 10 | — | — | — |
| 6 | É preciso imprimir de novo a estrutura do robô | 8 | — | — | — |
| 7 | É preciso fabricar de novo partes do labirinto | 6 | — | — | — |
| 8 | Mudanças no CAD sem registro de versão | 4 | — | — | — |

## Histórico de versões

| **Versão** | **Data** | **Descrição** | **Autor(es)** | **Revisor(es)** |
|:----------:|:--------:|---------------|---------------|-----------------|
| 1.0 | 04/11/2026 | Matriz de riscos da estrutura: notas, mapa de calor, motivo de cada nota e possíveis soluções | Equipe de Estrutura (riscos e soluções); Ana Carolina Fialho (página e notas) | Ana Carolina Nunes |
| 1.1 | 04/11/2026 | Revisão do PR #500: tabela de evolução por sprint | Cláudio Henrique | — |
