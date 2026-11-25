# Montagem e Fixação Mecânica dos Suportes no Chassi

## 1. Objetivo
Realizar a montagem e a fixação mecânica definitiva de todos os suportes (PCB, bateria e sensores) na estrutura principal do chassi, garantindo alinhamento e estabilidade antes da integração eletrônica.

## 2. Escopo

### Incluído
- Montagem dos suportes da PCB, da bateria e dos sensores na estrutura principal do chassi.
- Fixação mecânica definitiva (parafusos, porcas, travas ou adesivos estruturais, conforme projeto).
- Verificação de alinhamento, esquadro e ausência de folgas.

### Fora do escopo
- Instalação final dos componentes eletrônicos (PCB, bateria, sensores).
- Cabeamento, conexões elétricas e testes funcionais eletrônicos.

## 3. Requisitos

### 3.1 Requisitos de montagem
| Requisito | Observação |
| :--- | :--- |
| O suporte da PCB deve ser montado na posição definida no desenho mecânico. | Referência: MM-01, MM-05 e MM-06 em [`estrutura_entregaveis.pdf`](estrutura_entregaveis.pdf) |
| O suporte da bateria deve ser montado na posição definida, com acesso para troca/retirada. | Considerar o centro de massa |
| Os suportes dos sensores devem ser montados com a orientação e a distância especificadas. | Tolerância: a preencher na inspeção final ([#546](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/546), até 01/12/2026) |
| Todos os suportes devem ser fixados diretamente à estrutura principal do chassi. | Sem fixações provisórias |

### 3.2 Requisitos de fixação
| Requisito | Observação |
| :--- | :--- |
| Utilizar elementos de fixação conforme especificação (tipo, bitola, material). | M3 × 8 (esferas e espaçadores na base), M3 × 10 (espaçadores no 2º andar e suportes da PCB), M2 × 8 (presilhas da PCB). Lista completa em [`estrutura_modelo_final/README.md`](estrutura_modelo_final/README.md) |
| Aplicar torque de aperto especificado, quando aplicável. | Torque: a preencher na inspeção final ([#546](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/546), até 01/12/2026) |
| Utilizar trava química ou arruela de pressão/trava onde houver vibração. | Conforme projeto |
| Nenhum suporte pode apresentar movimento perceptível após a fixação. | Verificar manualmente |

### 3.3 Requisitos de alinhamento e estabilidade
| Requisito | Observação |
| :--- | :--- |
| Os suportes devem estar alinhados entre si e com os eixos de referência do chassi. | Tolerância: `± 2mm / °` |
| Os suportes devem estar nivelados e em esquadro com a estrutura. | Usar esquadro/paquímetro |
| O conjunto deve estar livre de folgas, ruídos e flexões ao manuseio. | Teste de balanço leve |
| Furos e pontos de fixação de PCB e sensores devem coincidir com os componentes reais. | Teste de encaixe a seco |

## 4. Critérios de pronto
- Chassi impresso.
- Suportes devidamente montados e fixados no chassi.
- Fixação mecânica e alinhamento realizados, garantindo ausência de folgas.
- Chassi pronto para a instalação final dos componentes eletrônicos e sensores.

## 5. Procedimentos realizados
1. Conferir o chassi e os suportes (integridade, dimensões e quantidade de peças).
2. Realizar pré-montagem a seco para validar posições e encaixes.
3. Posicionar e fixar os suportes na ordem definida (estrutura -> bateria -> PCB -> sensores).
4. Aplicar torque e travas conforme especificação.
5. Inspecionar alinhamento, esquadro e nivelamento.
6. Realizar teste de folga e estabilidade.
7. Registrar resultados e liberar o chassi para a integração eletrônica.

## 6. Verificação e inspeção
| Item verificado | Método | Critério de aceitação | Resultado |
| :--- | :--- | :--- | :--- |
| Posição dos suportes | Medição com paquímetro/gabarito | Dentro da tolerância | `[x] OK` / `[ ] NOK` |
| Alinhamento e esquadro | Esquadro / régua | Sem desvio visível ou medido | `[x] OK` / `[ ] NOK` |
| Aperto dos fixadores | Torquímetro / verificação manual | Torque especificado | `[x] OK` / `[ ] NOK` |
| Folgas e estabilidade | Teste manual de balanço | Sem movimento perceptível | `[x] OK` / `[ ] NOK` |
| Encaixe a seco dos componentes | Teste com PCB, bateria e sensores | Encaixe sem interferência | `[x] OK` / `[ ] NOK`|

## 7. Riscos e pontos de atenção
- Aperto excessivo pode danificar roscas ou deformar suportes.
- Desalinhamento pode comprometer a leitura dos sensores.
- Má distribuição de massa (bateria) pode afetar a estabilidade do conjunto.
- Fixações soltas por vibração se não houver trava adequada.
- A roda de apoio pode ficar bamba.

## 8. Entregáveis
- Chassi com todos os suportes montados e fixados.
- Checklist de inspeção preenchido (seção 6).
- Registro fotográfico da montagem final.
- Lista de eventuais não conformidades e ajustes realizados.

## 9. Fabricação/impressão definitiva dos suportes
Fabricar ou imprimir de forma definitiva os suportes dos componentes (PCB, bateria e sensores), com base nos modelos CAD validados e nos desenhos técnicos finais, utilizando as especificações do material definitivo.

### 9.1 Requisitos de fabricação
| Requisito | Observação |
| :--- | :--- |
| Fabricar/imprimir os suportes da PCB, da bateria e dos sensores a partir dos modelos CAD validados. | Modelo final: [`estrutura_modelo_final/`](estrutura_modelo_final/) |
| Seguir os desenhos técnicos finais (dimensões, furos, tolerâncias). | Desenhos MM-01 a MM-07 em [`estrutura_entregaveis.pdf`](estrutura_entregaveis.pdf) |
| Utilizar o material definitivo conforme especificação. | Material: `PETG` |
| Definir e registrar os parâmetros de fabricação/impressão. | Preenchimento, altura de camada, orientação e temperatura: a preencher na inspeção final ([#546](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/546), até 01/12/2026) |
| Identificar cada suporte fabricado (componente e lote). | Facilita a rastreabilidade |

### 9.2 Critérios de pronto
- [x] Suportes finais totalmente fabricados/impressos.
- [x] Inspeção dimensional e visual concluída, conferindo com o desenho técnico final.
- [x] Componentes testados quanto ao encaixe e fixação firme nos suportes fabricados.

### 9.3 Verificação e inspeção
| Item verificado | Método | Critério de aceitação | Resultado |
| :--- | :--- | :--- | :--- |
| Dimensões dos suportes | Paquímetro / gabarito | Conforme desenho técnico final | `[x] OK` / `[ ] NOK` |
| Aspecto visual | Inspeção visual | Sem rebarbas, empenamentos, trincas ou falhas de camada | `[x] OK` / `[ ] NOK` |
| Encaixe dos componentes | Teste com PCB, bateria e sensores | Encaixe sem interferência | `[x] OK` / `[ ] NOK` |
| Fixação dos componentes | Teste manual | Componentes firmes, sem folga | `[x] OK` / `[ ] NOK` |

<img width="664" height="631" alt="image" src="https://github.com/user-attachments/assets/18854bfe-c744-4f98-9dab-b91af7bd3730" />
<img width="664" height="689" alt="image" src="https://github.com/user-attachments/assets/ba354cf4-19b7-4ec6-a2d5-1a57496b1681" />
<img width="666" height="746" alt="image" src="https://github.com/user-attachments/assets/b21ee1f6-ee9c-43d4-9959-d67f542b1f4c" />

