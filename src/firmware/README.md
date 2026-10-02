# Firmware

Firmware do micromouse para o ESP32-C3 (Arduino Core, C++17), organizado nas camadas *sense-think-act* definidas na [arquitetura de software](../../docs/04-projeto-conceitual/software.md). O projeto usa [PlatformIO](https://platformio.org/).

## Estrutura

```
src/firmware/
├── platformio.ini     # ambientes esp32c3 (placa) e native (testes no computador)
├── src/main.cpp       # ponto de entrada: cria as tarefas FreeRTOS (navegação e telemetria)
├── lib/               # uma biblioteca por camada
└── test/              # testes unitários e de integração (Unity)
```

| Pasta em `lib/` | Camada | O que entra |
|---|---|---|
| `nucleo` | Base | Tipos compartilhados (`Direcao`, `Lado`, `EstadoParede`, `PosicaoCelula`, `Pose`, `DistanciasLaterais`), constantes e pinagem |
| `hal` | Interfaces de hardware | Interfaces abstratas (`ILidar` e, nos próximos épicos, encoders e motores) |
| `percepcao` | Percepção | Classificação `parede`/`livre` a partir das distâncias do LiDAR |
| `mapeamento` | Mapeamento | Mapa do labirinto (4 paredes por célula) e armazenamento em memória |
| `navegacao` | Navegação | Localização, decisão de movimento e detecção do objetivo |
| `atuacao` | Atuação | Controle dos motores e correção de trajetória |
| `comunicacao` | Comunicação | Pacotes de telemetria, serialização JSON e *buffer* de reenvio |
| `simulacao` | Falsos para teste | `LidarSimulado`, um `ILidar` que devolve as leituras configuradas pelo teste, sem hardware |
| `drivers_esp32` | Implementações do hardware | Drivers que dependem do Arduino (LiDAR por UART, encoders, DRV8833, WiFi) |

## Regras de dependência

- As camadas de lógica (`nucleo`, `percepcao`, `mapeamento`, `navegacao`, `comunicacao`, `simulacao`) **não incluem `Arduino.h`**. Assim elas compilam e são testadas no computador, sem a placa.
- O acesso ao hardware passa pelas interfaces de `hal`. Os drivers reais ficam em `drivers_esp32`, e `src/main.cpp` liga cada driver à sua interface.
- Para testar a lógica sem sensor, use o `LidarSimulado` (`lib/simulacao`): `definirLeituraFixa` repete uma leitura, `enfileirarLeitura` entrega leituras em sequência e `simularFalha(true)` faz o sensor parar de responder.
- Evite alocação dinâmica na lógica: o maior labirinto tem 12×4 células, então estruturas de tamanho fixo (`MAX_LINHAS_LABIRINTO` × `MAX_COLUNAS_LABIRINTO`) bastam.

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
