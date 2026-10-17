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
| `hal` | Interfaces de hardware | Interfaces abstratas (`ILidar` e, nos próximos épicos, encoders e motores) |
| `percepcao` | Percepção | Classificação `parede`/`livre` a partir das distâncias do LiDAR |
| `lidar` | Drivers / Hardware | Driver de leitura do sensor LiDAR 360° via UART (decodificador LD06) e agregação de leituras por setores |
| `mapeamento` | Mapeamento | Mapa do labirinto (4 paredes por célula), atualização a partir das leituras classificadas e armazenamento em memória |
| `navegacao` | Navegação | Localização, decisão de movimento e detecção do objetivo |
| `atuacao` | Atuação | Controle dos motores e correção de trajetória |
| `comunicacao` | Comunicação | Pacotes de telemetria, serialização do mapa em JSON e *buffer* de reenvio |
| `simulacao` | Falsos para teste | `LidarSimulado`, um `ILidar` que devolve as leituras configuradas pelo teste, sem hardware |
| `drivers_esp32` | Implementações do hardware | Drivers que dependem do Arduino (LiDAR por UART, encoders, DRV8833, WiFi) |

## Regras de dependência

- As camadas de lógica (`nucleo`, `percepcao`, `mapeamento`, `navegacao`, `comunicacao`, `simulacao`) **não incluem `Arduino.h`**. Assim elas compilam e são testadas no computador, sem a placa.
- O acesso ao hardware passa pelas interfaces de `hal`. Os drivers reais ficam em `drivers_esp32`, e `src/main.cpp` liga cada driver à sua interface.
- Para testar a lógica sem sensor, use o `LidarSimulado` (`lib/simulacao`): `definirLeituraFixa` repete uma leitura, `enfileirarLeitura` entrega leituras em sequência e `simularFalha(true)` faz o sensor parar de responder.
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

O armazenamento não pode ser copiado, e o acesso `const` impede que a telemetria altere o mapa; as duas regras são conferidas em tempo de compilação.

#### Mapa final da corrida

Ao alcançar a célula objetivo, a navegação chama `congelarMapa()`. O armazenamento copia o mapa da corrida para um segundo `Mapa`, o **mapa final**, que a partir daí só é lido: a navegação pode continuar registrando paredes no mapa da corrida (por exemplo, no caminho de volta) sem mudar o resultado ([#180](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/180), HU-04).

```cpp
if (chegouAoObjetivo) {
  armazenamentoMapa.congelarMapa();  // true na primeira vez; false se já estava congelado
}

static char json[TAMANHO_MAX_JSON_MAPA + 1];
size_t tamanho = MapaJson::serializarFinal(armazenamentoMapa, json, sizeof(json));  // 0 se ainda não congelou
```

| Regra | Comportamento |
|---|---|
| Congelar antes de `iniciarCorrida()` | Recusado: `congelarMapa()` devolve `false` |
| Congelar de novo | Recusado: devolve `false`, e o mapa final continua o da primeira vez |
| `mapaFinal()` | `nullptr` enquanto não congelou; depois, o mapa final, só para leitura |
| Escritas depois de congelar | Mudam só o mapa da corrida (`mapa()`); o mapa final e o JSON dele não mudam |
| Nova corrida | `iniciarCorrida()` destrava o congelamento, e `mapaFinal()` volta a ser `nullptr` |
| `serializarFinal()` | O mesmo JSON de `serializar()` (formato da [#178](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/178)), mas do mapa final; devolve `0` e deixa a string vazia se o mapa não foi congelado ou se o buffer não comporta o JSON |

**Memória.** Com o mapa final, o objeto passou de 196 para **392 bytes** (`sizeof` no computador): dois mapas de 195 bytes e os dois indicadores, de corrida iniciada e de mapa congelado. O limite do teste `test_armazenamento_ocupa_pouca_memoria` subiu de 256 para 512 bytes. Antes do mapa final, a medida no ESP32-C3 era de 196 bytes de RAM estática (`.bss`), igual ao `sizeof`.

## Atualização do mapa

O `AtualizadorMapa` (`lib/mapeamento`) integra a percepção ao mapa: a partir da `Pose` do robô e da classificação das leituras (`ClassificadorParede`), converte as direções relativas (`Frente`, `Esquerda`, `Direita`) para as direções absolutas do mapa (`Norte`, `Leste`, `Sul`, `Oeste`) e atualiza as paredes no `Mapa`.

```cpp
Mapa mapa;
Pose pose = {{0, 0}, Direcao::Norte};
ClassificadorParede classificador;

// Sobrecarga de conveniência: recebe amostras do LiDAR e classifica antes de atualizar
DistanciasLaterais amostras[3] = { {80, 400, 420}, {85, 390, 410}, {90, 410, 400} };
ResultadoAtualizacao res = AtualizadorMapa::atualizar(mapa, pose, classificador, amostras, 3);
```

| Regra | Comportamento |
|---|---|
| Conversão de eixos | Converte `Frente`, `Esquerda` e `Direita` da `Pose` para as direções cardeais absolutas |
| `Desconhecido` | Leituras desconhecidas não alteram o mapa e retornam `Invalida` |
| Conflitos | Se uma leitura contradiz o mapa, `temConflito` é marcado como `true`, retorna `Conflito` e o valor antigo do mapa é preservado; a política de navegação fica com quem chama (#188) |
| Pose inválida | Coordenadas fora dos limites do mapa retornam `Invalida` para todas as direções |

## Serialização do mapa em JSON

O `MapaJson` (`lib/comunicacao`) converte o `Mapa` em JSON (ArduinoJson) para o envio na telemetria. Não depende do Arduino, então é testado no ambiente `native`.

```cpp
char buffer[TAMANHO_MAX_JSON_MAPA + 1];
size_t tamanho = MapaJson::serializar(mapa, buffer, sizeof(buffer));
// {"dimensao":"4x4","linhas":4,"colunas":4,"celulas":[{"linha":0,"coluna":0,
//  "parede_norte":null,"parede_sul":true,"parede_leste":null,"parede_oeste":true}, ...]}
```

| Campo | Valor |
|---|---|
| `dimensao` | `"4x4"`, `"8x4"` ou `"12x4"` (colunas × linhas, como `labirintos.tipo`) |
| `linhas`, `colunas` | Tamanho do labirinto |
| `linha`, `coluna` | Posição da célula; a partida é (0, 0) |
| `parede_norte`, `parede_sul`, `parede_leste`, `parede_oeste` | `true` se há parede, `false` se está livre e `null` se ainda é desconhecida |

Os nomes dos campos das células são os das colunas da tabela `mapa_celulas` do banco, então a lista pode ser inserida nela, acrescentando só o `corrida_id`.

| Regra | Comportamento |
|---|---|
| Saída | Compacta, sem espaços nem quebras de linha, terminada em `\0` |
| Buffer | Do chamador, de tamanho fixo; a ArduinoJson só usa memória dinâmica durante a conversão, liberada ao final |
| `TAMANHO_MAX_JSON_MAPA` (5.100 bytes) | Pior caso: um 12×4 com as 48 células conhecidas e todas as paredes internas `Livre` (`false` ocupa mais caracteres que `true` ou `null`; o perímetro continua `true`). Maior que a pilha das tarefas (4.096 bytes), por isso precisa ser `static` ou global |
| Capacidade insuficiente, ou `saida` nulo/capacidade zero | Devolve `0` e deixa `saida` com string vazia, sem escrever além da capacidade informada |

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
- A pinagem está em `lib/nucleo/src/Pinos.h` e segue os [diagramas de hardware](../../docs/04-projeto-conceitual/hardware-diagramas.md).

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
