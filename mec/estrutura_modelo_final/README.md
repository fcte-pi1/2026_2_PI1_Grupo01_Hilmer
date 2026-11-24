# Micromouse — modelo final da estrutura

Modelo final do robô: base e 2º andar ligados por 4 espaçadores de 55 mm, bateria LiPo 2S presa sob o 2º andar e PCB de 100 × 100 mm sobre 4 suportes com presilhas. Substitui o modelo v4 (`micromouse_v3_entregaveis.zip`). Os desenhos técnicos (MM-01 a MM-07) estão em [`../estrutura_entregaveis.pdf`](../estrutura_entregaveis.pdf) e, folha por folha, em [`desenhos/`](desenhos/).

## Pastas

| Pasta | Conteúdo |
| :--- | :--- |
| [`step/`](step/) | `micromouse_montagem_final.step` (robô montado) e um STEP por peça |
| [`stl/`](stl/) | Peças para impressão 3D em PETG |
| [`desenhos/`](desenhos/) | Folhas MM-01 a MM-07 em PNG |
| [`imagens/`](imagens/) | Vistas, cortes e verificação do giro na célula |

## O que mudou em relação à v4

- **Espaçadores de 55 mm** (antes eram de 35 mm). O 2º andar, a bateria e a PCB subiram 20 mm: bateria em Z 40–60, 2º andar em Z 60–65 e PCB em Z 73–74,6 (medidas a partir da face de baixo da base). O ponto mais alto fica a 81 mm do chão.
- **Fixação da PCB.** São 4 suportes de 10 × 10 mm, parafusados por baixo do 2º andar com M3, e 4 presilhas rígidas presas por cima com M2. A PCB não é furada e nenhuma peça precisa flexionar.
- **Rodas.** O eixo fica na altura real do motor, 12 mm acima da face de baixo da base, e as rodas ficam 5 mm mais para fora, como no robô montado.
- **Esferas.** Ficam embutidas na base: a placa entra num rebaixo de 1,46 mm, as garras passam por um recorte vazado e cada esfera é presa com 2 parafusos M3 × 8 por cima. Rodas e esferas tocam o chão 5,0 mm abaixo da base, com uma arruela de 0,5 mm sob a placa da esfera.

## Peças para imprimir (PETG)

| Arquivo | Qtd. |
| :--- | :---: |
| `stl/base.stl` | 1 |
| `stl/segundo_andar.stl` | 1 |
| `stl/suporte_pcb_traseiro_D.stl` e `stl/suporte_pcb_traseiro_E.stl` | 1 de cada |
| `stl/suporte_pcb_dianteiro_2x.stl` | 2 |
| `stl/presilha_pcb_traseira_D.stl` e `stl/presilha_pcb_traseira_E.stl` | 1 de cada |
| `stl/presilha_pcb_dianteira_2x.stl` | 2 |
| `stl/espacador_55mm.stl` (ou espaçador comercial M3 × 55, ou 30 + 25 mm rosqueados) | 4 |

## Parafusos e peças compradas

| Item | Qtd. | Uso |
| :--- | :---: | :--- |
| Parafuso M3 × 8 | 8 | 4 nas esferas e 4 nos espaçadores, pela base |
| Parafuso M3 × 10 | 8 | 4 nos espaçadores, pelo 2º andar, e 4 nos suportes da PCB, por baixo do 2º andar |
| Parafuso M2 × 8 | 4 | Presilhas da PCB |
| Arruela M3 DIN 433 (Ø6 × 0,5 mm) | 4 ou mais | Ajuste de altura das esferas |
| Abraçadeira de nylon | 2 | Bateria |

## Ajuste da altura das esferas

1. Monte as esferas com 1 arruela em cada parafuso.
2. Ponha o robô numa mesa reta e confira se rodas e esferas tocam juntas.
3. Se a esfera estiver levantando a roda, acrescente uma arruela.
4. Se a esfera ficar no ar, tire a arruela.

## Conferir antes de montar

- **Furo da placa metálica da esfera:** no modelo ele tem Ø2,5 mm. Se o real for igual, abra com broca de 3,2 mm para o M3 passar.
- **Esfera:** a do modelo tem 3/8" (9,53 mm), acima do limite de 9 mm.
- **Ordem de montagem:** parafuse as esferas antes de montar o 2º andar, porque a bateria fica por cima delas.
