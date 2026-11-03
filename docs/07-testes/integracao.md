# Testes de Integração

Confiram as regras de avaliação dos testes de integração no [documento do tema do projeto](https://docs.google.com/presentation/d/1hRx0g9mpVGQbTv44gLEoAyf4b4kKM4vnrABTnG8HlUk/edit?usp=sharing).

| **ID** | **Descrição** | **Data de Início** | **Data de Conclusão** | **Sub-equipes** | **Responsável** | **Resultados Esperados** | **Resultados Obtidos** | **Aprovação Interna (S/N)** | **Aprovação do Orientador (S/N)** | **Observações** |
|:------:|---------------|--------------------|-----------------------|----------------|-----------------|--------------------------|------------------------|:---------------------------:|:--------------------------------:|-----------------|
| 5.1 | [Ensaio Integrado em Protoboard (Energia, Hardware, Firmware e Telemetria Supabase)](integracao/ensaio-integrado-protoboard.md) | 03/11/2026 | 03/11/2026 | Energia, Eletrônica e Software | Elias Oliveira e Cláudio Henrique | Circuito completo energizado em protoboard (sem resistores/capacitores discretos externos), saídas dos 3 reguladores Buck estáveis ($3{,}3\text{ V}$, $5{,}0\text{ V}$ e $6{,}0\text{ V}$), operação da ESP32 DevKit V1, driver DRV8833, motores N20, sensores VL53L0X/MPU6050 e envio de telemetria via Wi-Fi para as Edge Functions do Supabase | Circuito 100% energizado e aferido com multímetro; leitura em tempo real dos sensores ToF no firmware; acionamento do driver e motores; conexão Wi-Fi e chamadas HTTPS (`POST /corridas` $\rightarrow$ `201` e `POST /telemetria` $\rightarrow$ `200`) gravando lotes diretamente na tabela `public.telemetria` do Supabase | S | — | Detalhamento completo, plano de ligação, aferição com multímetro, foto e vídeos em [7.5.1 Ensaio Integrado em Protoboard](integracao/ensaio-integrado-protoboard.md) |
| 5.2 | | | | | | | | | | |
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
