# Testes de Energia

>Apresentar com detalhes os testes feitos para conferir se as demandas energéticas do projeto foram atendidas.

| **ID** | **Descrição** | **Data de Início** | **Data de Conclusão** | **Responsável** | **Resultados Esperados** | **Resultados Obtidos** | **Aprovação Interna (S/N)** | **Aprovação do Orientador (S/N)** | **Observações** |
|:------:|---------------|--------------------|-----------------------|-----------------|--------------------------|------------------------|:---------------------------:|:--------------------------------:|-----------------|
| 2.1 | Ensaio de corte por subtensão (BMS 2S / UVLO) sob rampa decrescente na fonte ajustável ($8{,}4\text{ V} \to 6{,}0\text{ V}$) — [#207](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/207) | 17/10/2026 | 17/10/2026 | Paulo Melo | Corte primário UVLO em $6{,}60\text{ V}$ (protegendo *dropout* do LM2596) e corte físico BMS 2S ao atingir $6{,}40\text{ V}$ ($3{,}20\text{ V/célula}$) | Corte UVLO atuou em $6{,}60\text{ V}$ ($V_{\text{ADC}}=1{,}638\text{ V}$, PWM=0%) e corte BMS 2S seccionou `P+` em $6{,}40\text{ V}$ ($t_{\text{delay}}=98\text{ ms}$) | S | — | Detalhado em [4.2.2 Ensaios de subtensão e PPTC](../04-projeto-conceitual/ensaios-protecoes.md) |
| 2.2 | Ensaio de velocidade de desarme (*Time-to-Trip*) do fusível rearmável PPTC (Bourns MF-R185) sob sobrecorrente simulada $> 3{,}5\text{ A}$ — [#207](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/207) | 17/10/2026 | 17/10/2026 | Paulo Melo | Sustentação contínua em $1{,}85\text{ A}$ ($I_{\text{hold}}$) e desarme térmico comprovado acima de $3{,}5\text{ A}$ | Estável sem desarme em $1{,}85\text{ A}$; desarme em $9{,}10\text{ s}$ (@ $3{,}60\text{ A}$), $8{,}70\text{ s}$ (@ $3{,}70\text{ A}$), $3{,}10\text{ s}$ (@ $5{,}00\text{ A}$) e $0{,}60\text{ s}$ (@ $9{,}25\text{ A}$), com $I_{\text{res}}=118\text{ mA}$ | S | — | Detalhado em [4.2.2 Ensaios de subtensão e PPTC](../04-projeto-conceitual/ensaios-protecoes.md) |
| 2.3 | | | | | | | | | |
| 2.4 | | | | | | | | | |
| 2.5 | | | | | | | | | |
| 2.6 | | | | | | | | | |
| 2.7 | | | | | | | | | |
| 2.8 | | | | | | | | | |
| 2.9 | | | | | | | | | |
| 2.10 | | | | | | | | | |
| 2.12 | | | | | | | | | |
| 2.13 | | | | | | | | | |
| 2.15 | | | | | | | | | |
| 2.16 | | | | | | | | | |
