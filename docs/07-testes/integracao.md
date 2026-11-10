# Testes de Integração

Confiram as regras de avaliação dos testes de integração no [documento do tema do projeto](https://docs.google.com/presentation/d/1hRx0g9mpVGQbTv44gLEoAyf4b4kKM4vnrABTnG8HlUk/edit?usp=sharing).

| **ID** | **Descrição** | **Data de Início** | **Data de Conclusão** | **Sub-equipes** | **Responsável** | **Resultados Esperados** | **Resultados Obtidos** | **Aprovação Interna (S/N)** | **Aprovação do Orientador (S/N)** | **Observações** |
|:------:|---------------|--------------------|-----------------------|----------------|-----------------|--------------------------|------------------------|:---------------------------:|:--------------------------------:|-----------------|
| 5.1 | [Ensaio Integrado em Protoboard (Energia, Hardware, Firmware e Telemetria Supabase)](integracao/ensaio-integrado-protoboard.md) | 29/10/2026 | 29/10/2026 | Energia, Eletrônica e Software | Elias Oliveira e Cláudio Henrique | Circuito completo energizado em protoboard (sem resistores/capacitores discretos externos), saídas dos 3 reguladores Buck estáveis ($3{,}3\text{ V}$, $5{,}0\text{ V}$ e $6{,}0\text{ V}$), operação da ESP32 DevKit V1, driver DRV8833, motores N20, sensores VL53L0X/MPU6050 e envio de telemetria via Wi-Fi para as Edge Functions do Supabase | Circuito 100% energizado e aferido com multímetro; leitura em tempo real dos sensores ToF no firmware; driver acionando o motor esquerdo (motor direito e os dois motores juntos pendentes na #483); conexão Wi-Fi e chamadas HTTPS (`POST /corridas` $\rightarrow$ `201` e `POST /telemetria` $\rightarrow$ `200`) gravando lotes diretamente na tabela `public.telemetria` do Supabase | S | — | Detalhamento completo, plano de ligação, aferição com multímetro, foto e vídeos em [7.5.1 Ensaio Integrado em Protoboard](integracao/ensaio-integrado-protoboard.md) |
| 5.2 | [Telemetria por Wi-Fi: rede da demonstração, rede reserva e reenvio](integracao/telemetria-wifi-rede-reserva.md) | 10/11/2026 | 10/11/2026 | Software | Cláudio Henrique | Rede principal e reserva em 2,4 GHz definidas e testadas; troca automática entre elas; com o Wi-Fi caindo ou sem rede, a telemetria fica no buffer (600 amostras) e chega toda ao banco ao reconectar, sem perda e sem duplicata; com o buffer cheio, as amostras mais antigas são descartadas | Principal: hotspot Android; reserva: iPhone. Troca para a reserva em 11 s e de volta à principal em 12 s; queda real de 39 s com 239 amostras gravadas sem perda e sem duplicata; corrida inteira sem rede (600 amostras) enviada em 6 s; com 650 amostras, as 50 mais antigas descartadas. Contagens conferidas no banco | S | — | Ensaio em bancada numa ESP32-S3 avulsa. Detalhes, varredura das redes do laboratório e conferência no banco em [7.5.2](integracao/telemetria-wifi-rede-reserva.md) |
| 5.3 | | | | | | | | | | |
| 5.4 | | | | | | | | | | |
| 5.5 | | | | | | | | | | |
| 5.6 | | | | | | | | | | |
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
