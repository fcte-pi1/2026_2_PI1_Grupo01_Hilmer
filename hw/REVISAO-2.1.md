# Hardware Rev 2.1 — ESP32 DevKit V1

Data: **03/11/2026**.

A Rev 2.1 substitui a ESP32-C3 Super Mini por uma **ESP32 DevKit V1 com módulo ESP32-WROOM**, sem PSRAM ocupando GPIO16/17. A referência recebida é `Pinagens_Micromouse_PI1.pdf`, tabela de fiação da bancada integrada fornecida em 03/11/2026. Esse PDF descreve a bancada antes da troca de IN4: usa GPIO14. A nova revisão usa GPIO13 e depende de retrabalho físico.

## Decisão: IN4 em GPIO13

O responsável solicitou nesta revisão a troca do GPIO14 devido ao risco relatado de pulso/tranco ao reiniciar. Selecionado **GPIO13**, livre na distribuição recebida, com capacidade de saída/PWM e sem função de strapping. GPIO13 também é MTCK: JTAG nesse terminal fica indisponível durante o uso do motor. Referências: [GPIOs do ESP32](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/gpio.html) e [datasheet ESP32](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf).

A forma de onda de GPIO14 não foi medida nesta atualização; não se afirma um pulso PWM com frequência ou duração definida. A mudança evita continuar usando o terminal relatado como problemático, mas **não comprova ausência de trancos no novo terminal**. R4 mantém nSLEEP habilitado; não é proteção de partida.

Com a alimentação desligada, mover **DRV8833 IN4/BIN2: GPIO14 → GPIO13**. Deixar GPIO14 desconectado. Gravar o ambiente `esp32devkit` somente após o retrabalho. Conferir motor direito com rodas suspensas em energização, reset por EN, reinício por software e gravação; medir IN3/IN4 e as saídas com osciloscópio. Se houver movimento inesperado, manter a potência dos motores desligada e revisar a inibição por hardware antes da liberação.

## Alimentação

- U2 fornece **5,0 V ao VIN / 5V** da DevKit; seu LDO gera o 3V3 interno.
- U5 fornece **+3V3_SENS**, para ToF, IMU, encoders e pull-up R4 de nSLEEP. **Não interligar +3V3_SENS ao terminal 3V3 da DevKit.** GND é comum.
- R4 de 47 kΩ vai para +3V3_SENS; nSLEEP não recebe 6 V. U4 alimenta apenas a potência VM do driver e os motores por suas saídas.
- Ao usar USB com VBUS, desconectar U2 do VIN da DevKit. Uso simultâneo exige isolamento/seleção de fontes verificado no exemplar adquirido. A recomendação da [Espressif para DevKitC](https://docs.espressif.com/projects/esp-idf/en/v5.0.4/esp32/hw-reference/esp32/get-started-devkitc.html#power-supply-options) é usar fontes mutuamente exclusivas; o circuito da DevKit V1 específica deve ser conferido.

## Aviso ao Software — preparado em 03/11/2026

**Especificação de Hardware: IN4 muda de GPIO14 para GPIO13.** Os demais 14 sinais seguem a tabela recebida. A entrega foi restringida a Hardware: `src/firmware/lib/nucleo/src/Pinos.h` e `platformio.ini` permanecem na versão original C3. A equipe de Software deve criar/sincronizar o bloco `PLACA_ESP32_DEVKIT` com os 15 sinais do CSV `diagramas/pinagem.csv`, configurar a placa DevKit e validar a compilação. GPIO18/19 pertencem ao encoder direito; a interface prevista para log é UART0 pelo conversor USB–serial.

Esta nota contém o aviso preparado para encaminhamento; **não há confirmação de envio externo nem de recebimento pelo Software**. Encaminhar o link deste arquivo no canal da equipe no mesmo dia da aplicação da alteração. A conferência com `Pinos.h` permanece pendente até a atualização pelo Software.

## Artefatos e verificação

`Micromouse.kicad_sch` e `Micromouse.pdf` foram redesenhados como interface elétrica de módulos por redes nomeadas, sem footprints. Os identificadores IOxx em U3 são GPIOs lógicos, não números de pads. J1/J2 agora incluem motor e encoder em seis vias; os conectores ToF/IMU exigem conferência da ordem física. Os passivos e valores foram preservados; SW2 foi acrescentado para START. Q1, D_Z1 e R_G1 continuam somente em `protecao_entrada.kicad_sch`, conforme observações na BOM.

A manutenção dos artefatos é manual: editar o esquema no KiCad, manter a tabela de `hardware.md` e o CSV `diagramas/pinagem.csv` consistentes, exportar o PDF e atualizar a figura da documentação. A conferência com o firmware deve comparar os 15 sinais, linha a linha, com o bloco DevKit quando ele for atualizado pelo Software.

Os scripts auxiliares de geração e verificação foram removidos a pedido do responsável. O KiCad, o PDF, a figura, a BOM e o CSV já produzidos foram preservados. As verificações de conectividade registradas abaixo correspondem às verificações realizadas antes dessa remoção; não há geração ou verificação automática disponível nesta entrega.

## Apresentação gráfica — 03/11/2026

A folha foi reorganizada em **A3 horizontal**, com moldura, zonas, carimbo e quatro blocos: alimentação/proteção, acionamento/encoders, controle/START/bateria e sensores I²C. Bateria, PPTC, chaves, resistores e capacitores passam a usar símbolos elétricos; módulos usam terminais, fios, rótulos de rede e marcações de não conexão. A DevKit tem uma representação gráfica simplificada, sem compromisso com a posição física dos terminais.

A alteração mantém a **revisão elétrica 2.1**: todos os terminais foram comparados com o esquema anterior e preservaram suas redes, incluindo IN4 em GPIO13, ADC em GPIO34 e 3V3 da DevKit sem conexão ao trilho de U5. Os 25 componentes e as 27 redes seguem conferidos com a BOM. O arquivo KiCad permanece editável; a figura publicada em `docs/figs/hardware-esquematico.png` foi atualizada e inspecionada.

A verificação estrutural e de conectividade não substitui ERC: abertura/ERC no KiCad e os ensaios físicos permanecem pendentes. A imagem fornecida pelo usuário orientou apenas a organização visual; as ligações e os componentes vêm dos arquivos desta revisão do repositório.

## Estado de entrega — escopo apenas Hardware

- [x] Esquemático, PDF, BOM e figura da Rev 2.1 preparados em `hw/` e `docs/figs/`.
- [x] Tabela e esquema conferidos com os 15 sinais do CSV de Hardware.
- [x] Decisão GPIO14 → GPIO13 registrada.
- [x] Trilhos 3V3 separados, VIN por U2 e restrição de USB documentados.
- [ ] Software avisado no mesmo dia e `Pinos.h` sincronizado linha a linha.
- [ ] Troca física de IN4, abertura/ERC no KiCad e ensaios de reset/partida realizados.
- [ ] Artefatos publicados no repositório remoto.

A pinagem está definida para Hardware; a integração com o firmware e a bancada permanece pendente. As páginas de Energia, Software, orçamento, a CI e as configurações da raiz foram mantidas fora desta alteração.
