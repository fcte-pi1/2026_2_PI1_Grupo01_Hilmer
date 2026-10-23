# 4. Projeto conceitual

Esta seção reúne o projeto conceitual do produto, organizado por área:

- [4.1 Estruturas](estruturas.md) — projeto conceitual da estrutura do produto.
- [4.2 Energia](energia.md) — análise de consumo energético e arquitetura de potência.
  - [4.2.1 Proteções de entrada](energia/protecoes-entrada.md) — dimensionamento da proteção contra inversão de polaridade (P-MOSFET) e sobrecorrente (PPTC).
  - [4.2.2 Reguladores Step-Down (Buck)](energia/reguladores-buck.md) — dimensionamento dos conversores chaveados de 6 V, 5 V e 3,3 V.
  - [4.2.3 Curva de Descarga e SoC da LiPo](energia/curva-descarga-lipo.md) — modelo de Shepherd modificado e correlação de SoC.
  - [4.2.4 Monitoramento de Tensão](energia/monitoramento-tensao.md) — divisor resistivo e filtro anti-aliasing para o ADC.
  - [4.2.5 Ensaios de subtensão e PPTC](energia/ensaios-protecoes.md) — relatório de ensaio e simulação do corte por subtensão e desarme do fusível PPTC.
  - [4.2.6 POP de recarga balanceada](energia/pop-recarga-bateria.md) — procedimento operacional padrão e regras de segurança para recarga da bateria LiPo 2S.
- [4.3 Hardware](hardware.md) — projeto conceitual de _hardware_.
- [4.4 Software](software.md) — projeto conceitual de _software_.
