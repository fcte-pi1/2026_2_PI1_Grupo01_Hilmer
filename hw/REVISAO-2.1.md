# Hardware Rev 2.1 — ESP32 DevKit V1

Data: **03/11/2026**.

A Rev 2.1 substitui a ESP32-C3 Super Mini por uma **ESP32 DevKit V1 com módulo ESP32-WROOM**, sem PSRAM ocupando GPIO16/17. A referência recebida é `Pinagens_Micromouse_PI1.pdf`, tabela de fiação da bancada integrada fornecida em 03/11/2026. Esse PDF descreve a bancada antes da troca de IN4: usa GPIO14. A nova revisão usa GPIO13 e depende de retrabalho físico.

## Decisão: IN4 em GPIO13

O responsável solicitou nesta revisão a troca do GPIO14 devido ao risco relatado de pulso/tranco ao reiniciar. Selecionado **GPIO13**, livre na distribuição recebida, com capacidade de saída/PWM e sem função de strapping. GPIO13 também é MTCK: JTAG nesse terminal fica indisponível durante o uso do motor. Referências: [GPIOs do ESP32](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/gpio.html) e [datasheet ESP32](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf).

A forma de onda de GPIO14 não foi medida nesta atualização; não se afirma um pulso PWM com frequência ou duração definida. A mudança evita continuar usando o terminal relatado como problemático, mas **não comprova ausência de trancos no novo terminal**. R4 mantém nSLEEP habilitado; não é proteção de partida.

Com a alimentação desligada, mover **DRV8833 IN4/BIN2: GPIO14 → GPIO13**. Deixar GPIO14 desconectado. Após atualizar `MOTOR_DIR_IN4` para GPIO13 no bloco `PLACA_ESP32_DEVKIT` de `Pinos.h` e realizar o retrabalho, compilar e gravar o ambiente de bancada `bancada_geral_devkit` (`pio run -e bancada_geral_devkit -t upload`, em `src/firmware`). O ambiente do robô para a DevKit será tratado na [#489](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/489). Conferir motor direito com rodas suspensas em energização, reset por EN, reinício por software e gravação; medir IN3/IN4 e as saídas com osciloscópio. Se houver movimento inesperado, manter a potência dos motores desligada e revisar a inibição por hardware antes da liberação.

## Alimentação

- U2 fornece **5,0 V ao VIN / 5V** da DevKit; seu LDO gera o 3V3 interno.
- U5 fornece **+3V3_SENS**, para ToF, IMU, encoders e pull-up R4 de nSLEEP. **Não interligar +3V3_SENS ao terminal 3V3 da DevKit.** GND é comum.
- R4 de 47 kΩ vai para +3V3_SENS; nSLEEP não recebe 6 V. U4 alimenta apenas a potência VM do driver e os motores por suas saídas.
- Ao usar USB com VBUS, desconectar U2 do VIN da DevKit. Uso simultâneo exige isolamento/seleção de fontes verificado no exemplar adquirido. A recomendação da [Espressif para DevKitC](https://docs.espressif.com/projects/esp-idf/en/v5.0.4/esp32/hw-reference/esp32/get-started-devkitc.html#power-supply-options) é usar fontes mutuamente exclusivas; o circuito da DevKit V1 específica deve ser conferido.

## Sincronização com Software — atualizado em 09/11/2026

**Especificação de Hardware: IN4 muda de GPIO14 para GPIO13.** A `main` já contém o bloco `PLACA_ESP32_DEVKIT` em `src/firmware/lib/nucleo/src/Pinos.h` e o ambiente `bancada_geral_devkit` em `src/firmware/platformio.ini`, integrados pela [#485](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/485). A conferência dos 15 sinais do CSV `diagramas/pinagem.csv` com esse bloco encontrou uma única divergência de GPIO: `MOTOR_DIR_IN4` ainda está em GPIO14 e deve mudar para GPIO13. Os outros 14 sinais coincidem. Não é necessário criar o bloco nem o ambiente de bancada; a migração do firmware completo do robô é a [#489](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/489). GPIO18/19 pertencem ao encoder direito; o log usa UART0 pelo conversor USB–serial.

Esta nota contém o aviso preparado para encaminhamento; **não há confirmação de envio externo nem de recebimento pelo Software**. Encaminhar o link deste arquivo no canal da equipe no mesmo dia da aplicação da alteração. Após a troca de `MOTOR_DIR_IN4`, repetir a conferência dos 15 sinais e validar a compilação de `bancada_geral_devkit`.

## Artefatos e verificação

`Micromouse.kicad_sch` e `Micromouse.pdf` foram redesenhados como interface elétrica de módulos por redes nomeadas, sem footprints. Os identificadores IOxx em U3 são GPIOs lógicos, não números de pads. J1/J2 agora incluem motor e encoder em seis vias; os conectores ToF/IMU exigem conferência da ordem física. Os passivos e valores foram preservados; SW2 foi acrescentado para START. Q1, D_Z1 e R_G1 continuam somente em `protecao_entrada.kicad_sch`, conforme observações na BOM.

A manutenção dos artefatos é manual: editar o esquema no KiCad, manter a tabela de `hardware.md` e o CSV `diagramas/pinagem.csv` consistentes, exportar o PDF e atualizar a figura da documentação. A tabela e o CSV usam os nomes reais das constantes do firmware; as redes `BTN_START` e `BAT_ADC` do esquema correspondem a `BOTAO_START` e `BATERIA_ADC`. Repetir a conferência dos 15 sinais, linha a linha, com o bloco DevKit existente após corrigir IN4.

### Pendência com Energia — bulk de VM

O esquema, o PDF e a figura da Rev 2.1 representam **C4 de 100 µF** no barramento de 6 V/VM. A [#216 de Energia](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/216) especifica **bulk de 470 µF Low-ESR**; portanto, o C4 atual não corresponde a esse requisito. A adequação de C4 para 470 µF Low-ESR deve ser refletida conjuntamente no KiCad, PDF, figura e BOM, com verificação do componente e ensaio de ruído pela Energia. O requisito de 470 µF está registrado na BOM como adequação pendente; não há confirmação de substituição física nem de eficácia medida.

Os scripts auxiliares de geração e verificação foram removidos a pedido do responsável. O KiCad, o PDF, a figura, a BOM e o CSV já produzidos foram preservados. As verificações de conectividade registradas abaixo correspondem às verificações realizadas antes dessa remoção; não há geração ou verificação automática disponível nesta entrega.

## Apresentação gráfica — 03/11/2026

A folha foi reorganizada em **A3 horizontal**, com moldura, zonas, carimbo e quatro blocos: alimentação/proteção, acionamento/encoders, controle/START/bateria e sensores I²C. Bateria, PPTC, chaves, resistores e capacitores passam a usar símbolos elétricos; módulos usam terminais, fios, rótulos de rede e marcações de não conexão. A DevKit tem uma representação gráfica simplificada, sem compromisso com a posição física dos terminais.

A alteração mantém a **revisão elétrica 2.1**: todos os terminais foram comparados com o esquema anterior e preservaram suas redes, incluindo IN4 em GPIO13, ADC em GPIO34 e 3V3 da DevKit sem conexão ao trilho de U5. Os 25 componentes e as 27 redes seguem conferidos com a BOM. O arquivo KiCad permanece editável; a figura publicada em `docs/figs/hardware-esquematico.png` foi atualizada e inspecionada.

A verificação estrutural e de conectividade não substitui ERC: abertura/ERC no KiCad e os ensaios físicos permanecem pendentes. A imagem fornecida pelo usuário orientou apenas a organização visual; as ligações e os componentes vêm dos arquivos desta revisão do repositório.

## Estado de entrega — escopo apenas Hardware

- [x] Esquemático, PDF, BOM e figura da Rev 2.1 preparados em `hw/` e `docs/figs/`.
- [x] Tabela e esquema conferidos com os 15 sinais do CSV de Hardware.
- [x] CSV e tabela comparados com o bloco DevKit da `main` (#485): 14 GPIOs coincidem; IN4 requer GPIO14 → GPIO13.
- [x] Decisão GPIO14 → GPIO13 registrada.
- [x] Trilhos 3V3 separados, VIN por U2 e restrição de USB documentados.
- [ ] Software avisado no mesmo dia e `Pinos.h` sincronizado linha a linha.
- [ ] Troca física de IN4, abertura/ERC no KiCad e ensaios de reset/partida realizados.
- [ ] C4 adequado ao bulk de 470 µF Low-ESR da #216, artefatos sincronizados e eficácia validada com Energia.
- [ ] Artefatos publicados no repositório remoto.

A pinagem está definida para Hardware; a integração com o firmware e a bancada permanece pendente. Nesta correção documental, o requisito de bulk também foi registrado em Energia. O firmware, o orçamento, a CI e as configurações da raiz não foram alterados.
