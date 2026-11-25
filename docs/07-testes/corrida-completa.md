# Testes de corrida completa e de ponta a ponta

Resultados dos testes com o robô real nos labirintos 4×4, registrados para as issues
[#185](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/185) (corrida completa no labirinto) e
[#232](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/232) (do ESP32 ao dashboard).

## Como os testes foram feitos

- Robô na célula oficial de largada (canto, com paredes atrás e de lado), virado para a parede da frente, na mesma rede Wi-Fi do computador.
- Largada pelo botão INICIAR do dashboard, sem tocar no robô durante a corrida.
- Acompanhamento ao vivo pelo dashboard (mapa descoberto, trajeto, velocidade, logs) e pelo console remoto (telnet).
- O objetivo é o canto diametralmente oposto à largada, como no enunciado do tema.
- Dois desenhos de labirinto 4×4 foram usados: o primeiro, em que o robô foi calibrado, e um segundo, montado depois, com becos e entradas laterais. O dashboard chama os dois de "Labirinto 1 (4×4)".

## Teste de corrida completa (#185)

| Critério | Resultado | Evidência |
|---|---|---|
| O robô percorre o 4×4 do início ao objetivo sem intervenção | Aprovado. Corridas concluídas nos dois desenhos de labirinto | Histórico do dashboard: tentativas 56, 57, 60, 61 e 64 no primeiro desenho (21,8 s a 22,9 s) e tentativa 68 no segundo (31,2 s), todas com "Desafio cumprido: SIM" |
| Tempo de corrida registrado, com limite de 10 minutos | Aprovado | Coluna "Tempo final" do histórico; o cronômetro do dashboard para em 10:00 |
| Encerramento com status "Não concluída" ao esgotar o tempo | Aprovado | Tentativas 47 (10:00,541) e 66 (10:00,608): "Não concluída" |
| Resultados registrados para o documento de testes | Este documento | |

### Completude do labirinto (mapa descoberto pelo robô)

- O robô monta o mapa durante a corrida (paredes e aberturas de cada célula) e o dashboard o mostra como "descoberto pelo robô", junto com o trajeto.
- No segundo desenho, o mapa se completou até o objetivo depois do ajuste das aberturas laterais: as entradas laterais da 2ª e da 3ª célula do corredor e o beco sem saída do nicho foram mapeados, e o robô seguiu até o objetivo sem reiniciar o mapa.
- O robô reinicia o mapa (aviso `mapa_reiniciado_sem_caminho`, até 3 vezes) quando o mapa fecha sem caminho para o objetivo. Foi o que ocorreu antes do ajuste das aberturas, e nas corridas concluídas isso não foi necessário.

### Problemas encontrados e correções

| Problema | Correção |
|---|---|
| Robô preso em quinas de parede | Desvio de quina (afasta-se de uma parede lateral perto, com o outro lado livre) e ré curta antes do giro |
| Robô parava antes do centro da última célula | Ajuste de chegada pelo ToF frontal, com teto maior na última célula antes da borda |
| Abertura lateral lida como parede (sensores laterais na diagonal batem na quina vizinha) | Perfil das leituras laterais durante o avanço: livre se ler pelo menos 190 mm em 4 amostras seguidas (paredes leram de 105 a 146 mm, aberturas de 200 a 237 mm) |
| Ré que não andava encerrava a corrida | O ciclo segue para perceber e girar |
| Velocidade sempre 0 no dashboard | Velocidade real calculada pelo deslocamento das rodas |

### Limitações

- Taxa de sucesso: as corridas concluídas vieram depois de várias tentativas de ajuste. Ainda não há uma série de repetições seguidas, por labirinto, com o firmware final.
- O primeiro desenho de labirinto não foi repetido com o limite final de abertura lateral (190 mm).
- A "Nota" mostrada no dashboard sai 0 porque as tentativas de teste passaram de 3.

## Teste ponta a ponta: ESP32 até o dashboard (#232)

| Critério | Resultado | Evidência |
|---|---|---|
| Uma corrida completa aparece ao vivo no dashboard e depois no histórico | Aprovado | Telemetria, logs e mapa ao vivo durante as corridas; a corrida passa a constar no histórico com tempo, velocidade média, bateria e status |
| Nenhum dado perdido ou duplicado (números de sequência) | **Pendente de conferência no banco** | No console, a numeração das amostras segue consecutiva (por exemplo 181 a 190 e 387 a 391 na mesma sessão). Falta rodar a consulta abaixo na tabela `telemetria` |
| Resultados registrados para os testes de integração | Este documento | |

Consulta para conferir lacunas e duplicados de uma corrida (trocar `ID_DA_CORRIDA`):

```sql
select count(*) as amostras,
       count(distinct numero_sequencia) as distintas,
       min(numero_sequencia) as primeira,
       max(numero_sequencia) as ultima,
       max(numero_sequencia) - min(numero_sequencia) + 1 - count(distinct numero_sequencia) as lacunas
from telemetria
where corrida_id = ID_DA_CORRIDA;
```

Sem duplicados: `amostras = distintas`. Sem lacunas: `lacunas = 0`.
