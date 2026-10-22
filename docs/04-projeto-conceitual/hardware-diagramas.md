# Diagramas de _Hardware_ — Micromouse PI1

Esta página apresenta o **diagrama de blocos** e o **esquemático elétrico** do Micromouse, com os subsistemas, as conexões de alimentação e sinais e a pinagem do controlador.

O sistema organiza-se em quatro domínios: **energia**, com bateria, fusível, chave e conversores; **controle**, com o ESP32-C3; **sensoriamento**, com três VL53L0X e a IMU MPU6050; e **atuação**, com o DRV8833 e dois motores DC com redução. A descrição dos subsistemas e as justificativas das escolhas estão no [projeto conceitual de hardware](hardware.md).

## 1. Diagrama de blocos

O [diagrama de blocos](hardware.md#2-diagrama-de-blocos) apresenta os barramentos de alimentação, o I²C em GPIO9/8, os três XSHUT e o acionamento dos motores. A definição Mermaid fica na página principal para manter uma única versão do diagrama.

## 2. Esquemático elétrico

[![Esquemático elétrico do Micromouse](../figs/hardware-esquematico.png)](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/blob/documentacao/hw/Micromouse.pdf)

O PDF e o arquivo editável `hw/Micromouse.kicad_sch` descrevem as conexões. O divisor 100 kΩ / 33 kΩ termina em **BAT_ADC_TEST**, sem ligação ao GPIO2, reservado ao XSHUT direito.
