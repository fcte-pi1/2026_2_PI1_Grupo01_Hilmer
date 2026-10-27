# Registros do teste de integração dos sensores I²C (26/10/2026)

Registro completo e análise em [`docs/07-testes/hardware/integracao-sensores-i2c.md`](../../../../docs/07-testes/hardware/integracao-sensores-i2c.md). Firmware em [`hw/testes/firmware/integracao-sensores-i2c`](../../firmware/integracao-sensores-i2c).

| Arquivo | Conteúdo |
|---|---|
| `serial_bruto.txt` | Saída serial copiada do terminal, sem edição (999 linhas). Cada amostra aparece quebrada em duas linhas, sem a vírgula entre `gz` e `frente`: uma com as 10 colunas do MPU6050 e outra com as 3 distâncias. A primeira linha é o fim de uma amostra cortada |
| `serial.csv` | As 499 amostras completas de `serial_bruto.txt`, cada linha do MPU6050 juntada à linha de distâncias seguinte. Distâncias **cruas** (sem desconto) |
| `painel_1hz.csv` | Valores do painel do visualizador transcritos do vídeo da tela, 1 por segundo (0 a 22 s). As distâncias do painel **já têm 68 mm descontados** pelo visualizador; `livre` = nada a menos de 2 m |
| `quadros/tela_tNNs.jpg` | Quadros da gravação da tela (painel), a cada 2 s |

Os vídeos completos estão em `docs/videos/`:

| Vídeo | Conteúdo |
|---|---|
| `integracao-i2c-bancada.mp4` | Celular filmando a protoboard girada à mão e a tela do computador (22,6 s, 464×832) |
| `integracao-i2c-tela.mp4` | Gravação da tela do visualizador no mesmo ensaio (22,6 s). Reduzida de 2560×1664 (43 MB, `.mov`) para 960×540 (15 MB) com o `avconvert` do macOS; os números do painel continuam legíveis |

Os gráficos são gerados com:

```bash
python docs/scripts/grafico_integracao_i2c.py hw/testes/registros/integracao-2026-10-26 docs/figs
```
