# Testes de Integração

Confiram as regras de avaliação dos testes de integração no [documento do tema do projeto](https://docs.google.com/presentation/d/1hRx0g9mpVGQbTv44gLEoAyf4b4kKM4vnrABTnG8HlUk/edit?usp=sharing).

| **ID** | **Descrição** | **Data de Início** | **Data de Conclusão** | **Sub-equipes** | **Responsável** | **Resultados Esperados** | **Resultados Obtidos** | **Aprovação Interna (S/N)** | **Aprovação do Orientador (S/N)** | **Observações** |
|:------:|---------------|--------------------|-----------------------|----------------|-----------------|--------------------------|------------------------|:---------------------------:|:--------------------------------:|-----------------|
| 5.1 | [Ensaio Integrado em Protoboard (Energia, Hardware, Firmware e Telemetria Supabase)](integracao/ensaio-integrado-protoboard.md) | 29/10/2026 | 29/10/2026 | Energia, Eletrônica e Software | Elias Oliveira e Cláudio Henrique | Circuito completo energizado em protoboard (sem resistores/capacitores discretos externos), saídas dos 3 reguladores Buck estáveis ($3{,}3\text{ V}$, $5{,}0\text{ V}$ e $6{,}0\text{ V}$), operação da ESP32 DevKit V1, driver DRV8833, motores N20, sensores VL53L0X/MPU6050 e envio de telemetria via Wi-Fi para as Edge Functions do Supabase | Circuito 100% energizado e aferido com multímetro; leitura em tempo real dos sensores ToF no firmware; driver acionando o motor esquerdo (motor direito e os dois motores juntos pendentes na #483); conexão Wi-Fi e chamadas HTTPS (`POST /corridas` $\rightarrow$ `201` e `POST /telemetria` $\rightarrow$ `200`) gravando lotes diretamente na tabela `public.telemetria` do Supabase | S | — | Detalhamento completo, plano de ligação, aferição com multímetro, foto e vídeos em [7.5.1 Ensaio Integrado em Protoboard](integracao/ensaio-integrado-protoboard.md) |
| 5.2 | [Telemetria por Wi-Fi: rede da demonstração, rede reserva e reenvio](integracao/telemetria-wifi-rede-reserva.md) | 10/11/2026 | 10/11/2026 | Software | Cláudio Henrique | Rede principal e reserva em 2,4 GHz definidas e testadas; troca automática entre elas; com o Wi-Fi caindo ou sem rede, a telemetria fica no buffer (600 amostras) e chega toda ao banco ao reconectar, sem perda e sem duplicata; com o buffer cheio, as amostras mais antigas são descartadas | Principal: hotspot Android; reserva: iPhone. Troca para a reserva em 11 s e de volta à principal em 12 s; queda real de 39 s com 239 amostras gravadas sem perda e sem duplicata; corrida inteira sem rede (600 amostras) enviada em 6 s; com 650 amostras, as 50 mais antigas descartadas. Contagens conferidas no banco | S | — | Ensaio em bancada numa ESP32-S3 avulsa. Detalhes, varredura das redes do laboratório e conferência no banco em [7.5.2](integracao/telemetria-wifi-rede-reserva.md) |
| 5.3 | [Integração de Energia na PCB Física, Segurança Elétrica e Alimentação do Sistema](energia/integracao-pcb-alimentacao.md) | 17/11/2026 | 19/11/2026 | Energia, Eletrônica e Software | Elias, Cláudio, Bruno, Aline, Ana Carolina e Paulo | PCB física soldada (3× Bucks MP1584EN, MPU6050, DRV8833, soquete removível para ESP32 DevKit V1, chicote XT60 e conectores de 6 pinos) sem curtos entre barramentos; polaridade e chave geral validadas com checklist; LiPo 2S alimentando ESP32 (via `VIN` sem USB), sensores, driver e motores com tensões estáveis | PCB soldada e validada sem curtos entre barramentos; ESP32 DevKit V1 montada em dois barramentos fêmea removíveis; chicote XT60 polarizado e chave geral conferidos com checklist pré-corrida; sistema alimentado pela LiPo 2S sem cabo USB com saídas estáveis em **3,3 V**, **5,1 V** e **6,6 V** (folga positiva para os motores, pequenas variações dentro do esperado). Sem LM2596/BMS/PPTC e INA226 desconectado | S | — | Issues `#212`, `#537` e `#103`. Detalhamento completo, checklist pré-corrida e espaço para evidências em [7.2.2 Integração de Energia na PCB e Sistema](energia/integracao-pcb-alimentacao.md) |
| 5.4 | [Corrida autônoma no labirinto 4×4 com o sistema completo](corrida-completa.md) | 21/11/2026 | 23/11/2026 | Software, Eletrônica, Energia e Estrutura | Cláudio Henrique | O robô sai da célula de largada, descobre as paredes e chega ao canto oposto sem intervenção, dentro de 10 minutos; ao esgotar o tempo, a corrida é encerrada como não concluída | Corridas concluídas nos dois desenhos de labirinto, em 21,8 s a 22,9 s no primeiro e 31,2 s no segundo; corridas que passaram de 10 minutos foram registradas como não concluídas | S | — | Issue `#185`. As conclusões vieram após tentativas de ajuste; ainda não há série de repetições com o firmware final |
| 5.5 | [Ponta a ponta: robô, banco de dados e dashboard](corrida-completa.md#teste-ponta-a-ponta-esp32-ate-o-dashboard-232) | 21/11/2026 | 23/11/2026 | Software | Cláudio Henrique | A largada é dada pelo dashboard; telemetria, logs e mapa aparecem ao vivo durante a corrida; ao final, a corrida fica no histórico com tempo, velocidade média, bateria e situação | Largada pelo botão INICIAR sem tocar no robô; dados ao vivo no painel; corridas gravadas no histórico com os campos esperados | S | — | Issue `#232` |
| 5.6 | [Mapeamento de um labirinto com becos e entradas laterais](corrida-completa.md#completude-do-labirinto-mapa-descoberto-pelo-robo) | 21/11/2026 | 23/11/2026 | Software e Eletrônica | Cláudio Henrique | O mapa é montado durante a corrida, com paredes e aberturas, e exibido no dashboard junto com o trajeto | No segundo desenho, o mapa se completou até o objetivo, com as entradas laterais e o beco sem saída registrados | S | — | Exigiu ajustar o limiar de abertura lateral para 190 mm |
| 5.7 | | | | | | | | | | |
| 5.8 | | | | | | | | | | |
| 5.9 | | | | | | | | | | |
| 5.10 | | | | | | | | | | |
| 5.11 | | | | | | | | | | |
| 5.12 | | | | | | | | | | |
| 5.13 | | | | | | | | | | |
| 5.14 | | | | | | | | | | |
| 5.15 | | | | | | | | | | |
| 5.16 | | | | | | | | | | |

## Parte de software na integração

O teste de integração avalia o sistema completo resolvendo um labirinto, com os dados exibidos no software. O caminho dos dados é:

1. O robô (ESP32 DevKit V1) lê os sensores, decide o movimento e monta o mapa.
2. A telemetria segue por Wi-Fi para as Edge Functions do Supabase, que gravam no banco. Sem rede, as amostras ficam no buffer e são reenviadas ([7.5.2](integracao/telemetria-wifi-rede-reserva.md)).
3. O dashboard recebe os dados em tempo real, guarda o histórico e envia os comandos de largada ao robô.

| Dado ou função exigida no tema | Onde aparece | Verificação |
|---|---|---|
| Tipo do labirinto | Painel ao vivo e histórico | Corridas no robô real (5.5) |
| Trajeto no labirinto | Mapa descoberto pelo robô | Corridas no robô real (5.6) |
| Consumo de bateria | Painel ao vivo e histórico | Campo exibido nas corridas (5.5) |
| Velocidade média | Painel ao vivo e histórico | Corridas no robô real (5.5) |
| Tempo de conclusão | Painel ao vivo e histórico | Corridas no robô real (5.4) |
| Desafio cumprido (S/N) | Histórico | Corridas no robô real (5.4) |
| Consulta por um labirinto ou por todos | Filtro do histórico | Testes automatizados do frontend |
