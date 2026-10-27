# Firmware do teste de integração dos sensores I²C

Firmware gravado no ESP32-C3 Super Mini no teste de integração de 26/10/2026: os três VL53L0X e o MPU6050 no mesmo barramento I²C, no protótipo em protoboard. Registro completo em [`docs/07-testes/hardware/integracao-sensores-i2c.md`](../../../../docs/07-testes/hardware/integracao-sensores-i2c.md).

**Não é o firmware do robô** (`src/firmware/`). É um programa de bancada, guardado aqui para que o teste possa ser repetido com o mesmo código.

## Origem

| Arquivo | Origem |
|---|---|
| `src/main.cpp` | `src/main.cpp` do repositório [brun0duarte/micromouse](https://github.com/brun0duarte/micromouse), alterado localmente para o teste. É exatamente o código que estava gravado no ESP32-C3 durante o ensaio |
| `lib/robo/i2c.h`, `lib/robo/i2c.cpp`, `lib/robo/pinos.h` | Mesmo repositório, commit `baa44c9`, sem alteração |
| `platformio.ini` | Mesma plataforma do firmware do projeto (`espressif32@7.1.3`) e as bibliotecas do repositório micromouse (`electroniccats/MPU6050@^1.4.5`, `pololu/VL53L0X@^1.3.1`) |

## Ligações (protoboard)

Todos os sensores em 3V3 e GND da placa, no mesmo I²C. Pinagem completa em [`hw/diagramas/pinagem.csv`](../../../diagramas/pinagem.csv).

| Sinal | GPIO |
|---|---|
| SDA / SCL | 6 / 7 |
| XSHUT do VL53L0X esquerdo | 9 |
| XSHUT do VL53L0X direito | 8 |
| XSHUT do VL53L0X da frente | solto |

| Dispositivo | Endereço atribuído pelo firmware |
|---|---|
| VL53L0X frente | 0x30 |
| VL53L0X esquerda | 0x31 |
| VL53L0X direita | 0x29 |
| MPU6050 | 0x68 |

## Compilar e gravar

```bash
cd hw/testes/firmware/integracao-sensores-i2c
pio run                      # compila
pio run -t upload            # grava (ESP32-C3 na USB)
pio device monitor -b 115200 # mostra a saída
```

Compilado em 26/10/2026 com PlatformIO 6.2: RAM 4,4% (14.360 bytes) e Flash 21,4% (280.382 bytes), sem avisos.

Para ver o painel 3D, use o `visualizador/servidor.py` do repositório micromouse e abra `http://localhost:8000`.

## Saída serial

Uma linha a cada 20 ms (50 por segundo), com 13 colunas:

```
w,x,y,z,ax,ay,az,gx,gy,gz,frente,esquerda,direita
```

| Colunas | Conteúdo | Unidade |
|---|---|---|
| `w,x,y,z` | Quaternion da orientação, calculado pelo DMP do MPU6050 | — |
| `ax,ay,az` | Acelerômetro | g |
| `gx,gy,gz` | Giroscópio | graus/s |
| `frente,esquerda,direita` | Distância **crua** de cada VL53L0X (sem desconto) | mm; `-1` = nada a menos de 2 m; `-2` = sensor não respondeu |

## Diferenças entre o comentário do cabeçalho e o código

O código foi guardado exatamente como foi gravado. Quem for reutilizá-lo deve saber que:

1. **O cabeçalho descreve 17 colunas** (com `motorEsq,motorDir,encEsq,velEsq`) **e comandos `M,esq,dir` para os motores**, mas o código envia só as 13 colunas acima, e o corpo de `lerComandos()` está comentado. **Este firmware não aciona os motores nem lê o encoder**: no painel, os campos "esq", "dir" e "enc" ficam com "—".
2. **Se o MPU6050 não iniciar**, a mensagem diz "seguindo sem ele", mas o `while(1);` logo em seguida trava o programa.
3. `lerDistancias()` é chamada duas vezes em cada `loop()`. Não afeta o resultado: a função só lê um sensor quando ele tem medida nova.
4. Só o giroscópio é calibrado na partida (`CalibrateGyro(6)`, com o sensor parado). O acelerômetro não é calibrado; o comentário no código explica que o `CalibrateAccel` da biblioteca 1.4.5 satura o eixo Z.
