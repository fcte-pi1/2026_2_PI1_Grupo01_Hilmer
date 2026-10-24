# Firmware

Firmware do micromouse para o ESP32-C3 (Arduino Core, C++17), organizado nas camadas *sense-think-act* definidas na [arquitetura de software](../../docs/04-projeto-conceitual/software.md). O projeto usa [PlatformIO](https://platformio.org/).

A documentação detalhada do código (camadas, tipos, pinagem, interface do LiDAR, tarefas e testes) está em [Firmware: implementação](../../docs/04-projeto-conceitual/firmware.md). Os arquivos `.h` têm comentários de documentação no formato Doxygen (`///`).

## Estrutura

```
src/firmware/
├── platformio.ini     # ambientes esp32c3 (placa), native (testes no computador) e native_cobertura (testes com cobertura, usado no CI)
├── src/main.cpp       # ponto de entrada: cria as tarefas FreeRTOS (navegação e telemetria)
├── lib/               # uma biblioteca por camada
└── test/              # testes unitários e de integração (Unity)
```

| Pasta em `lib/` | Camada | O que entra |
|---|---|---|
| `nucleo` | Base | Tipos compartilhados (`Direcao`, `Lado`, `EstadoParede`, `PosicaoCelula`, `Pose`, `DistanciasLaterais`), constantes e pinagem |
| `hal` | Interfaces de hardware | Interfaces abstratas (`ILidar`, `IEncoder`, `IMotores` e `IImu`) |
| `percepcao` | Percepção | Classificação `parede`/`livre` para os 3 sensores ToF (VL53L0X) com limiares por lado, filtro digital e saturação defensiva (#379) |
| `imu` | Sensoriamento | Giroscópio: conversão da leitura do MPU6050, calibração do viés, ângulo de giro (`IntegradorGiro`) e o ciclo da tarefa da IMU (`AcompanhadorGiro`) |
| `encoders` | Sensoriamento | Decodificação em quadratura (x4) dos canais A e B, conversão de pulsos em milímetros (calibrável) e distância percorrida por cada roda |
| `lidar` | Drivers / Hardware | Driver de leitura do sensor LiDAR 360° via UART (decodificador LD06) e agregação de leituras por setores |
| `mapeamento` | Mapeamento | Mapa do labirinto (4 paredes por célula) e armazenamento em memória |
| `navegacao` | Navegação | Localização (`Odometria`: pose pela odometria diferencial dos encoders), decisão de movimento (`Navegacao`) e detecção do objetivo (`DetectorObjetivo`) |
| `atuacao` | Atuação | Controle dos motores e correção de trajetória |
| `comunicacao` | Comunicação | Pacotes de telemetria (`TelemetriaJson`), serialização JSON do mapa (`MapaJson`) e *buffer* de reenvio |
| `simulacao` | Falsos para teste | `LidarSimulado`, um `ILidar` que devolve as leituras configuradas pelo teste, `LabirintoSimulado`, o labirinto de referência que alimenta o `LidarSimulado`, `EncoderSimulado`, um `IEncoder` com a contagem definida pelo teste, `MotoresSimulados`, um `IMotores` que guarda o último comando, e `ImuSimulado`, um `IImu` com taxa, viés, ruído e falha definidos pelo teste, sem hardware |
| `drivers_esp32` | Implementações do hardware | Drivers que dependem do Arduino (LiDAR por UART, encoders, DRV8833, MPU6050, barramento I²C, WiFi) |

## Regras de dependência

- As camadas de lógica (`nucleo`, `percepcao`, `mapeamento`, `navegacao`, `comunicacao`, `simulacao`) **não incluem `Arduino.h`**. Assim elas compilam e são testadas no computador, sem a placa.
- O acesso ao hardware passa pelas interfaces de `hal`. Os drivers reais ficam em `drivers_esp32`, e `src/main.cpp` liga cada driver à sua interface.
- Para testar a lógica sem sensor, use o `LidarSimulado` (`lib/simulacao`): `definirLeituraFixa` repete uma leitura, `enfileirarLeitura` entrega leituras em sequência, `simularFalha(true)` faz o sensor parar de responder e, com um `LabirintoSimulado`, `posicionar` faz a leitura vir da posição do robô (veja [Labirinto simulado](#labirinto-simulado)).
- Evite alocação dinâmica na lógica: o maior labirinto tem 12×4 células, então estruturas de tamanho fixo (`MAX_LINHAS_LABIRINTO` × `MAX_COLUNAS_LABIRINTO`) bastam.

## Mapa do labirinto

O `Mapa` (`lib/mapeamento`) guarda o estado das quatro paredes de cada célula. O tamanho vem do enum `DimensaoMapa` (`Labirinto4x4`, `Labirinto8x4` e `Labirinto12x4`), escrito como colunas × linhas: `Labirinto12x4` tem 12 colunas e 4 linhas. A matriz tem capacidade fixa para o maior labirinto, sem alocação dinâmica, e o objeto ocupa 195 bytes.

**Eixos:** a partida é a célula (0, 0), no canto inferior esquerdo; Norte aumenta a linha e Leste aumenta a coluna. `vizinha()` (`nucleo/Direcao.h`) calcula a célula ao lado de uma posição.

```cpp
Mapa mapa(DimensaoMapa::Labirinto12x4);
mapa.registrarParede({0, 0}, Direcao::Norte, EstadoParede::Parede);
mapa.obterParede({1, 0}, Direcao::Sul);  // Parede: a célula vizinha foi atualizada junto
```

| Regra | Comportamento |
|---|---|
| Paredes ainda não observadas | `Desconhecido` |
| Perímetro | Já nasce como `Parede` |
| Registro | Grava também o lado oposto da célula vizinha |
| Estado já conhecido | `registrarParede` não sobrescreve: retorna `JaConhecida` (mesmo valor) ou `Conflito` (valor diferente); a política para o conflito fica com quem chama |
| Entrada inválida | `Invalida`: célula fora do mapa ou estado `Desconhecido` |
| Comparação | `==` compara o tamanho e todas as paredes |

### Armazenamento do mapa

O `ArmazenamentoMapa` (`lib/mapeamento`) é o dono do único `Mapa` da corrida. Ele fica em `src/main.cpp`, como variável estática fora das tarefas, para que reiniciar o estado de navegação (por exemplo, após uma colisão) ou perder a conexão não apague o mapa (HU-04).

```cpp
armazenamentoMapa.iniciarCorrida(DimensaoMapa::Labirinto12x4);  // único jeito de zerar o mapa
Mapa& paraNavegacao = armazenamentoMapa.mapa();                  // lê e registra paredes
const Mapa& paraTelemetria = armazenamentoMapa.mapa();           // só lê (pelo acesso const)
```

O armazenamento não pode ser copiado, e o acesso `const` impede que a telemetria altere o mapa; as duas regras são conferidas em tempo de compilação. Medido no ESP32-C3, o objeto ocupa **196 bytes** de RAM estática (`.bss`): 195 do mapa e 1 do indicador de corrida.

## Labirinto simulado

O `LabirintoSimulado` (`lib/simulacao`) é um labirinto com todas as paredes conhecidas. Com ele, o `LidarSimulado` devolve o que o sensor real veria na posição do robô, sem digitar leituras à mão. Serve para testar a classificação de paredes e a atualização do mapa num labirinto inteiro.

**Formato do texto.** O labirinto é escrito em ASCII: cada célula ocupa 4 colunas e 2 linhas de texto, `+` marca os cantos, `---` uma parede horizontal, `|` uma parede vertical e espaço é passagem livre. A primeira linha é a borda Norte e a partida (0, 0) é a célula do canto inferior esquerdo, como no `Mapa`. Cada parede aparece uma única vez, então as duas células vizinhas sempre concordam. Há um exemplo de cada tamanho (4×4, 8×4 e 12×4) em `test/test_labirinto_simulado/LabirintosExemplo.h`.

```cpp
LabirintoSimulado labirinto;
labirinto.carregar(exemplos::LABIRINTO_4X4);   // ResultadoCarga::Carregado
labirinto.temParede({0, 0}, Direcao::Leste);   // true

LidarSimulado lidar;
lidar.usarLabirinto(labirinto);                // o labirinto precisa continuar vivo
lidar.posicionar({{0, 0}, Direcao::Norte});
DistanciasLaterais d;
lidar.lerDistanciasLaterais(d);                // d = {630, 90, 90} (frente, esquerda, direita, em mm)
```

**Distâncias.** O robô fica no centro da célula: a parede da própria célula está a 90 mm e cada célula livre no caminho soma 180 mm. Com o robô virado para Norte em (0, 0) do 4×4 há três células livres à frente, ou seja, 90 + 3 × 180 = 630 mm.

| Operação | O que faz |
|---|---|
| `carregar(texto)` | Lê o labirinto. Se o texto for inválido, devolve o motivo (`TamanhoInvalido`, `CaractereInvalido`, `ParedeIncompleta` ou `PerimetroAberto`) e mantém o labirinto anterior |
| `temParede(celula, direcao)` | `true` se há parede naquele lado; célula fora do labirinto conta como parede |
| `distancias(pose, saida)` | Distâncias à frente, à esquerda e à direita para a pose; `false` sem carga ou com a célula fora do labirinto |
| `LidarSimulado::usarLabirinto` e `posicionar` | Liga o modo labirinto. A falha e a fila continuam com prioridade; sem `posicionar`, vale a leitura fixa; com a pose fora do labirinto, a leitura falha |

## Percepção ToF (VL53L0X) e classificação de paredes

A camada de percepção (`lib/percepcao`, `ClassificadorParede`) processa as leituras de distância dos 3 sensores Time-of-Flight VL53L0X (frontal, esquerdo e direito, I²C) introduzidos na Rev 2.0 em substituição ao LiDAR ([#379](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/379), HU-02).

```cpp
ClassificadorParede classificador; // Inicializa com ConfiguracaoPercepcao padrão
DistanciasLaterais leitura = {54, 250, 52}; // Leitura dos ToF em mm
ClassificadorParede::ResultadoClassificacao paredes = classificador.classificar(leitura);
// paredes.frente: Parede, paredes.esquerda: Livre, paredes.direita: Parede
```

### Geometria dos sensores e dedução dos limiares

No labirinto da competição, a célula mede 180 mm (`TAMANHO_CELULA_MM`) e as paredes têm 12 mm de espessura (`ESPESSURA_PAREDE_MM`), resultando em um corredor livre de 168 mm. Com o robô centralizado, a distância do centro do robô à parede é de **84 mm** (`DISTANCIA_CENTRO_A_PAREDE_MM`).

O chassi possui 120 mm de largura (meia largura: 60 mm) e 100 mm de comprimento (meio comprimento: 50 mm). Se os sensores laterais fossem montados na borda externa do chassi, a distância até a parede seria de apenas $84 - 60 = 24\text{ mm}$, valor que cai abaixo do limite operacional confiável do VL53L0X (~30 mm). Portanto, a montagem física exige sensores recuados em relação à borda.

Para um sensor com offset $d_{\text{offset}}$ em relação ao centro do robô na direção da medição:
- **Distância nominal com parede imediata:** $d_{\text{parede}} = 84 - d_{\text{offset}}$
- **Distância nominal com passagem livre (parede na célula seguinte):** $d_{\text{livre}} = d_{\text{parede}} + 180\text{ mm}$
- **Limiar de decisão ideal (ponto médio):** $\text{limiar} = d_{\text{parede}} + 90\text{ mm}$

Adotando provisoriamente um offset de montagem de $30\text{ mm}$ (sensores recuados 30 mm para o interior do chassi):
- $d_{\text{parede}} = 84 - 30 = 54\text{ mm}$ (com boa margem acima dos 30 mm mínimos do VL53L0X).
- $d_{\text{livre}} = 54 + 180 = 234\text{ mm}$ (ou fora de alcance em corredores abertos).
- $\text{Limiar ideal} = 54 + 90 = 144\text{ mm}$, adotado provisoriamente como **140 mm**.

O valor de 140 mm também é 100% compatível com o ambiente simulado (`LabirintoSimulado`), onde paredes imediatas medem 90 mm ($\le 140\text{ mm} \to \text{Parede}$) e passagens livres medem $\ge 270\text{ mm}$ ($> 140\text{ mm} \to \text{Livre}$).

### Valores provisórios de configuração (`ConfiguracaoPercepcao.h`)

| Parâmetro | Constante | Valor provisório | Origem / Justificativa |
|---|---|---|---|
| Limiar frontal | `limiarFrenteMm` | **140 mm** | Ponto de decisão para offset frontal de 30 mm (centro da célula: 54 mm vs 234 mm) |
| Limiar esquerdo | `limiarEsquerdaMm` | **140 mm** | Ponto de decisão para offset lateral de 30 mm (robô centralizado: 54 mm vs 234 mm) |
| Limiar direito | `limiarDireitaMm` | **140 mm** | Ponto de decisão para offset lateral de 30 mm (robô centralizado: 54 mm vs 234 mm) |
| Distância mínima | `distanciaMinimaMm` | **30 mm** | Limite operacional inferior do VL53L0X (datasheet ST DocID029104) |
| Distância máxima | `distanciaMaximaMm` | **1200 mm** | Alcance máximo confiável em ambiente interno sem alvos difusos de alta refletância |
| Código fora de alcance | `DISTANCIA_OUT_OF_RANGE_TOF_MM` | **8190 mm** | Código retornado pelo firmware/hardware VL53L0X na ausência de eco de retorno |
| Tipo de filtro | `tipoFiltro` | **`Mediana`** | Mediana sobre janela de até 16 amostras, imune a outliers pontuais |

> [!WARNING]
> **Valores provisórios:** os limiares e offsets acima foram calculados teoricamente a partir das dimensões do labirinto e do esquemático Rev 2.0. A calibração final e o ajuste dos offsets dependem da medição na bancada com o hardware real montado ([#175](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/175)).

### Tratamento defensivo de limites e filtro de ruído

- **"Fora de alcance" ($\ge 8190\text{ mm}$ ou $> 1200\text{ mm}$):** interpretado como lado livre. A função `tratarDistancia()` satura a medida em 1200 mm, garantindo que o valor fique estritamente acima do limiar ($\to \text{Livre}$).
- **"Abaixo do mínimo" ($< 30\text{ mm}$):** interpretado como parede muito próxima (ou robô encostado na parede). A função satura a medida em 30 mm, garantindo que o valor fique estritamente abaixo do limiar ($\to \text{Parede}$).
- **Filtro digital:** o método `classificar(amostras, quantidade)` filtra um lote de amostras antes da decisão. O filtro padrão por **mediana** descarta outliers pontuais (como um falso fora de alcance de 8190 mm durante a passagem por uma parede ou uma queda espúria momentânea para 0 mm num corredor aberto), mantendo a integridade do mapa.

## Controle de movimento

O `ControleMovimento` (`lib/atuacao`) leva o robô em linha reta, centralizado no corredor, e para no centro da célula antes de uma parede frontal ([#191](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/191), HU-12). Chame `atualizar` uma vez por leitura nova do LiDAR (cerca de 100 ms):

```cpp
ControleMovimento controle;
EstadoMovimento estado = controle.atualizar(lidar, motores, 100);  // ILidar e IMotores
if (estado == EstadoMovimento::ParadoNaParede) {
  // decidir o próximo passo e, antes de andar de novo, chamar controle.reiniciar()
}
```

| Regra | Comportamento |
|---|---|
| Centralização | PD sobre o deslocamento lateral: `(esquerda - direita) / 2` com parede dos dois lados, ou a diferença para 84 mm com uma só; sem parede ao lado, segue reto |
| Correção | Tirada da roda esquerda e somada à direita (positiva vira para a esquerda), limitada a 250 e a 60% da potência base, para nenhuma roda girar para trás |
| Parada frontal | Cruzeiro (400) até 264 mm da parede, rampa até a mínima (150) e parada a 84 mm, o centro da célula |
| Parado | `ParadoNaParede` freia e fica travado até `reiniciar()` |
| Sem leitura do LiDAR | Segue reto na potência mínima; depois de 500 ms sem leitura, freia (`ParadoSemLeitura`) |

Os ganhos e as potências estão em `ConfiguracaoControle.h` e são **provisórios**: foram ajustados numa simulação do corredor (`test_controle_movimento`) e precisam ser confirmados na bancada. A geometria vem do tema do PI1 (célula de 180 mm com parede de 12 mm) e do chassi de 100 × 120 mm, com o LiDAR no centro: sobram 24 mm de cada lado com o robô centralizado.

O `MotoresDrv8833` (`lib/drivers_esp32`) aplica a potência com PWM de 20 kHz e 10 bits nos GPIO 4 a 7, no modo *fast decay*; `parar()` liga IN1 e IN2 (freio). O motor direito fica invertido no `main.cpp`, pela montagem espelhada.

## Detecção do objetivo e conclusão do desafio

O `DetectorObjetivo` (`lib/navegacao`) identifica o instante em que o robô chega à célula de destino (canto diametralmente oposto à partida) a partir da pose estimada pela odometria ([#189](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/189), HU-07 / RF-07):

```cpp
DetectorObjetivo detector(DimensaoMapa::Labirinto4x4); // Objetivo em (3, 3)
// A cada ciclo da tarefa de navegação:
if (detector.processarChegada(pose, motores, &armazenamentoMapa)) {
  // Motores freados imediatamente e mapa final congelado
}
```

| Critério de aceite | Solução implementada |
|---|---|
| Chegada detectada na malha | `DetectorObjetivo::calcularObjetivo` determina a célula oposta (`linhas - 1, colunas - 1`) para 4x4 (`(3, 3)`), 8x4 (`(3, 7)`), 12x4 (`(3, 11)`) ou dimensões customizadas, comparando com a célula atual estimada pela odometria |
| Motores parados imediatamente | Ao detectar a chegada, aciona `motores.parar()` (freio elétrico instantâneo via DRV8833) e congela o estado de navegação |
| Congelamento do mapa final | `ArmazenamentoMapa::congelarMapa()` salva a imagem imutável das paredes exploradas para telemetria e persistência (HU-04) |
| Evento pontual na telemetria | `DetectorObjetivo::consumirEventoChegada()` sinaliza e consome a chegada pontual, fazendo `TelemetriaJson` incluir `"evento": "objetivo_alcancado"` em um único pacote serializado após a chegada, voltando a `null` nos pacotes subsequentes |

> [!NOTE]
> **Dimensão do labirinto em `main.cpp`:** a inicialização com `Labirinto4x4` no `main.cpp` (tanto em `detectorObjetivo` quanto em `armazenamentoMapa.iniciarCorrida`) é **provisória** para bancada e testes iniciais. Em labirintos 8x4 ou 12x4 a chegada só é detectada se o tamanho correto for configurado. A seleção definitiva do tamanho será recebida dinamicamente no início da corrida.
>
> **Sincronização entre tarefas:** `processarChegada`, `consumirEventoChegada` e `resetar` acessam o estado de detecção e odometria sob a seção crítica `travaOdometria`, garantindo thread-safety entre `tarefaNavegacao` (10 ms) e `tarefaTelemetria` (100 ms).
>
> **Arquitetura dos detectores:** atualmente coexistem duas instâncias (`Navegacao::detector_` para planejamento BFS/Flood Fill e `detectorObjetivo` em `main.cpp` para o laço do firmware). Ambas serão unificadas e a API de construtores será simplificada quando `Navegacao` for incorporada diretamente ao laço da tarefa FreeRTOS.

### Telemetria e compatibilidade com o banco de dados

O serializador `TelemetriaJson` (`lib/comunicacao`) gera payloads compatíveis com a tabela `telemetria` do Supabase e diagnósticos de bancada:

| Campo JSON | Tabela `telemetria` (Supabase) | Descrição |
|---|---|---|
| `numero_sequencia` | `numero_sequencia` (INT) | Sequencial monotônico incremental |
| `posicao_linha` | `posicao_linha` (INT) | Coordenada atual da linha na malha |
| `posicao_coluna` | `posicao_coluna` (INT) | Coordenada atual da coluna na malha |
| `orientacao` | `orientacao` (VARCHAR(2)) | Letra da direção: "N", "S", "L", "O" |
| `velocidade` | `velocidade` (FLOAT) | Velocidade linear instantânea (m/s) |
| `tensao_bateria` | `tensao_bateria` (FLOAT) | Tensão instantânea da bateria (V) |
| `evento` | `evento` (VARCHAR(30)) | Evento pontual (`"objetivo_alcancado"`, `"colisao"` ou `null`) |

Campos diagnósticos adicionais (`esq_pulsos`, `dir_pulsos`, `esq_mm`, `dir_mm`, `medio_mm`, `pose_valida`) são úteis para calibração de bancada e scripts de teste (como `calibrar_deslocamento_18cm.py`), e podem ser descartados pelo gateway antes da inserção no banco de dados.

## Instalação

Instale o PlatformIO por uma das opções:

- Extensão **PlatformIO IDE** no VS Code (abra a pasta `src/firmware`).
- Linha de comando: `pip install platformio`.

## Compilar, gravar e testar

Execute os comandos dentro de `src/firmware`:

| Objetivo | Comando |
|---|---|
| Compilar para o ESP32-C3 | `pio run -e esp32c3` |
| Gravar na placa | `pio run -e esp32c3 -t upload` |
| Abrir o monitor serial | `pio device monitor` |
| Rodar os testes no computador | `pio test -e native` |
| Rodar um teste específico | `pio test -e native -f test_nucleo` |

## Pinagem e log serial (Rev 2.0)

A placa é a **ESP32-C3 Super Mini** (esquemático Rev 2.0 de 21/10/2026). Todos os 15 GPIOs do ESP32-C3 estão em uso:

| Função | GPIO |
|---|---|
| I²C: SDA / SCL (3 VL53L0X e MPU6050) | 9 / 8 |
| XSHUT dos VL53L0X: esquerdo / frontal / direito | 0 / 1 / 2 |
| Motor esquerdo no DRV8833: IN1 / IN2 | 10 / 3 |
| Motor direito no DRV8833: IN3 / IN4 | 20 / 21 |
| Encoder esquerdo: A / B (reservado, ainda não instalado) | 4 / 5 |
| Encoder direito: A / B (reservado, ainda não instalado) | 6 / 7 |
| USB nativa: D− / D+ (log e gravação) | 18 / 19 |

O **log serial sai pela USB nativa** (`ARDUINO_USB_MODE=1` e `ARDUINO_USB_CDC_ON_BOOT=1` no `platformio.ini`), porque GPIO20/21, a UART0, são as entradas do driver dos motores. O `main.cpp` não compila sem essas opções. Ao ligar, a ROM do ESP32-C3 ainda escreve no GPIO21 durante o reset: a montagem precisa manter o driver desabilitado no boot (validação da eletrônica na bancada). A porta serial aparece no computador como um dispositivo USB (`pio device monitor` a encontra sozinho); sem um computador conectado, o log é descartado.

## IMU (MPU6050) e barramento I²C

O giroscópio do MPU6050 dá o **ângulo de giro** (yaw) do robô, a principal fonte de orientação enquanto não há encoders. A lógica (`lib/imu`) é testada no computador com o `ImuSimulado`; o driver (`ImuMpu6050`, em `drivers_esp32`) só lê os registradores.

| Peça | Onde | O que faz |
|---|---|---|
| `IImu` | `lib/hal` | `lerTaxaGiroZ(taxa)`: taxa de giro em décimos de °/s, positiva para a esquerda |
| `ImuMpu6050` | `lib/drivers_esp32` | Reinicia o sensor, confere o `WHO_AM_I`, liga o filtro de 42 Hz, configura a faixa e lê os dois bytes do eixo Z (registradores 0x47 e 0x48) |
| `BarramentoI2c` | `lib/drivers_esp32` | O `Wire` com um mutex do FreeRTOS: toda transação I²C passa por `BarramentoI2c::Uso` |
| `AcompanhadorGiro` | `lib/imu` | Um ciclo da tarefa da IMU: calibra o viés na largada (recomeça sozinha se o robô se mexer), integra o ângulo e invalida o ângulo depois de 5 falhas seguidas |
| `tarefaImu` | `src/main.cpp` | Lê a IMU a cada 10 ms com `vTaskDelayUntil` (intervalo fixo), com prioridade 3, e publica a `LeituraGiro`, que as outras tarefas leem por `lerGiro()` |

**Barramento compartilhado.** Os 3 VL53L0X (lidos pela navegação) e o MPU6050 (lido pela tarefa da IMU) usam o mesmo SDA/SCL. Uma transação no meio de outra mistura os bytes dos dois sensores, então **todo acesso ao `Wire` precisa estar dentro de um `BarramentoI2c::Uso`**, inclusive o do driver dos ToF. A proteção é um mutex, e não uma seção crítica (`portENTER_CRITICAL`): a seção crítica desliga as interrupções, de que o driver I²C depende. Se o barramento não vier em `ESPERA_MAXIMA_I2C_MS` (8 ms), a leitura falha e é contada, em vez de atrasar a tarefa.

```cpp
micromouse::BarramentoI2c::Uso uso(barramentoI2c);
if (uso.conseguiu()) {
  valida = tof.lerDistanciasLaterais(distancias);   // o barramento fica livre ao sair do bloco
}
```

**Na largada.** Nos primeiros 2 s (200 leituras), o robô precisa ficar parado: a média vira o viés. A mensagem `{"status":"giroscopio_calibrado"}` sai no log quando a calibração termina. Ao iniciar, o log mostra `imu_iniciada` com o `who_am_i` (0x68 no MPU6050) ou `falha_imu` se o sensor não respondeu.

**Valores provisórios**, para conferir na bancada:

| Parâmetro | Arquivo | Valor |
|---|---|---|
| Faixa do giroscópio | `ConfiguracaoImu.h` | ±500 °/s |
| Período de leitura | `ConfiguracaoImu.h` | 10 ms (100 leituras por segundo, igual à taxa configurada no sensor) |
| Amostras da calibração, variação máxima e zona morta | `ConfiguracaoImu.h` | 200, 5 °/s e 0,3 °/s |
| Sensor de cabeça para baixo | `ConfiguracaoImu.h` (`INVERTER_GIRO_Z`) | `false`: girando o robô para a esquerda com a mão, a taxa tem que ser positiva |
| Falhas seguidas que invalidam o ângulo | `ConfiguracaoImu.h` | 5 (50 ms) |
| Frequência do I²C e espera pelo barramento | `Configuracao.h` | 100 kHz e 8 ms |

## Padrão de testes

| Nível | Onde roda | Ferramenta | Quando usar |
|---|---|---|---|
| Unitário | Computador (`native`) | Unity | Lógica pura: tipos, classificação, mapa, serialização |
| Integração | Computador (`native`) | Unity, com falsos de `simulacao` no lugar do hardware | Percepção → mapa em labirinto simulado |
| Bancada | Placa com o hardware real | Monitor serial e checklist do teste | LiDAR real, encoders, motores |

Para criar um teste:

1. Crie a pasta `test/test_<modulo>/` (o prefixo `test_` é obrigatório).
2. Adicione um arquivo `.cpp` com `setUp`, `tearDown`, as funções `test_*` e um `main` com `RUN_TEST`. Use `test/test_nucleo/test_direcao.cpp` como modelo.
3. Rode `pio test -e native -f test_<modulo>`.

Nomeie as funções de teste pelo comportamento esperado (`test_classifica_como_parede_quando_distancia_abaixo_do_limiar`) e mantenha cada teste verificando uma única regra.

## Convenções

**Idioma do código:** nomes de pastas, arquivos, tipos, funções, variáveis, constantes e testes em português, sem acentos (identificadores só com ASCII), e comentários em português. Mantêm-se em inglês apenas as APIs e os termos consagrados: `setup`/`loop`, `Serial`, `xTaskCreate`, `LiDAR`, `UART`, `PWM`, `buffer`, `ILidar` (prefixo `I` de interface).

| Elemento | Estilo | Exemplo |
|---|---|---|
| Tipos e enums | `PascalCase` | `Direcao`, `EstadoParede` |
| Valores de enum | `PascalCase` | `Direcao::Norte` |
| Funções e variáveis | `camelCase` | `girarDireita`, `frenteMm` |
| Constantes | `MAIÚSCULAS_COM_SUBLINHADO`, sem acento | `MAX_LINHAS_LABIRINTO` |
| Unidade no nome | sufixo | `distanciaMm`, `periodoMs` |
| Testes | `test_<comportamento>` | `test_oposta_inverte_a_direcao` |

**Estrutura:**

- Código dentro do namespace `micromouse`.
- Um arquivo `.h` por tipo ou conceito, com `#pragma once`.
- Cada tarefa do épico adiciona seus arquivos na pasta da própria camada e seus testes em `test/test_<modulo>/`.
- A pinagem está em `lib/nucleo/src/Pinos.h` e segue o esquemático Rev 2.0 (ver [hardware](../../docs/04-projeto-conceitual/hardware.md)). Ela é o único lugar com número de GPIO; o teste `test_pinos` reprova repetição e GPIO inexistente.

## CI

Todo PR para a `main` (e todo push nela) roda o workflow **CI Firmware** (`.github/workflows/ci-firmware.yml`). A `main` é protegida: o PR só pode ser integrado com os dois checks verdes.

| Check | O que faz | Reprova quando |
|---|---|---|
| **Testes nativos** | Roda `pio test -e native_cobertura` (os testes do `native` com medição de cobertura) e gera o relatório com o `gcovr` | Algum teste falha ou a cobertura das linhas de `lib/` fica abaixo de 80% |
| **Build ESP32-C3** | Roda `pio run -e esp32c3` | O firmware não compila para a placa |

A cobertura mede só o código compilado nos testes nativos (`lib/`, sem `drivers_esp32`); `src/main.cpp` fica de fora. Código novo em `lib/` sem teste derruba a cobertura e reprova o PR.

### Como ler o resultado

1. No PR, abra a aba **Checks** e escolha **CI Firmware**.
2. Clique no job com ✗ e expanda o passo que falhou:
   - **Rodar os testes com cobertura:** procure a linha `FAIL`. Ela mostra o arquivo, a linha e a mensagem do Unity, por exemplo `Expected 2 Was 1`.
   - **Exigir cobertura mínima de 80%:** a cobertura ficou abaixo da meta. Veja na tabela do resumo quais arquivos têm linhas sem teste.
   - **`pio run -e esp32c3`:** erro de compilação. A mensagem do compilador indica o arquivo e a linha.
3. A tabela de cobertura aparece no **Summary** do workflow. O relatório HTML linha a linha fica no artefato **cobertura-firmware** (no fim da página do Summary, disponível por 14 dias): baixe, descompacte e abra o `index.html`.

### Rodar localmente antes de abrir o PR

Dentro de `src/firmware`:

| Objetivo | Comando |
|---|---|
| Mesmos testes do CI, com cobertura | `pio test -e native_cobertura` |
| Ver a cobertura (precisa de `pip install gcovr`) | `gcovr --root . --filter 'lib/' --object-directory .pio/build/native_cobertura --txt` |
| Conferir a meta de 80% | `gcovr --root . --filter 'lib/' --object-directory .pio/build/native_cobertura --fail-under-line 80` |
| Mesmo build do CI | `pio run -e esp32c3` |

O resultado esperado hoje é **10 testes aprovados** e cobertura de 100%. A pasta `cobertura/` é ignorada pelo Git.
