# Testes de Software
## Testes de _software_

> Apresentar com detalhes os testes feitos para conferir se os objetivos do *software* foram atendidos.
>
> - Listar as histórias de usuários e seus respectivos protótipos  implementadas.
> - Associar as histórias de usuários e os casos de testes funcionais implementados.
> - Enviar o print da execução dos suítes de testes automatizados em nível:
>   - Unitário e integração, com cobertura de código-fonte de pelo menos 80%;
>   - Sistema, do tipo funcional (end-to-end E2E).
>
> **Observações:**
>
> 1. Em relação ao *backlog* do produto, será considerado como referência aquele descrito no primeiro ponto de controle, contemplando as correções e *feedbacks* da-os professora(e)s. O ideal seria a real atualização entre o planejado X realizado.
> 2. Os testes mencionados nesta subseção dizem respeito aos testes de *software*. O Roteiro de testes funcionais, entregue no primeiro ponto de controle deve ser corrigido a partir do apontamento da-os professora(e)s. Além disso, ele deve guiar a execução dos testes E2E automatizados. O conjunto de casos de testes funcionais documentados devem estar coerentes com a automação via interface gráfica de entrada de dados.
>
> Exemplos de ferramentas de teste funcionais E2E:
>
> - Selenium
> - Cypress
> - Playwright
>
> Exemplos de ferramentas de testes em nível unitário e integração:
>
> - Jest
> - unittest
> - pytest
> - tox
> - gTest
> - Catch2
## Registro de execução dos testes

### Sprint 2 · Projeto base do firmware

| Item | Valor |
|---|---|
| Tarefa | [#172 Projeto base do firmware](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/172) |
| História de usuário | HU-02 · Reconhecimento de ambiente ([#36](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/36)), RF02 |
| Código testado | `src/firmware` (branch `feat/firmware-base`, PR [#194](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/194)) |
| Data da execução | 04/10/2026 |
| Executado por | Ana Carolina Fialho |
| Documentação do código | [Firmware: implementação](../04-projeto-conceitual/firmware.md) |

**Ambiente de execução**

| Componente | Versão |
|---|---|
| Sistema operacional | macOS (Apple Silicon) |
| PlatformIO Core | 6.2.0 |
| Plataforma de testes no computador | `native` 1.2.1 |
| Framework de testes | Unity 2.6.1 |
| Plataforma da placa | Espressif 32 7.1.3 (ESP32-C3-DevKitM-1, 160 MHz, 320 KB de RAM, 4 MB de Flash) |
| Compilador da placa | `toolchain-riscv32-esp` 8.4.0+2021r2-patch5 |
| Framework da placa | `framework-arduinoespressif32` 4.20017 |

#### Testes unitários (ambiente `native`)

Comando, executado em `src/firmware`:

```bash
pio test -e native
```

| Suíte | Casos | Resultado | Duração |
|---|:-:|:-:|:-:|
| `test_simulacao` | 6 | **6 aprovados** | 0,97 s |
| `test_nucleo` | 4 | **4 aprovados** | 0,78 s |
| **Total** | **10** | **10 aprovados, 0 falhas** | **1,75 s** |

Os tempos são da segunda execução. Na primeira, a suíte `test_simulacao` levou 8,99 s porque o PlatformIO baixou a plataforma `native`, o SCons e o Unity.

| Caso de teste | O que verifica | HU / tarefa | Resultado |
|---|---|---|:-:|
| `test_girar_direita_percorre_as_quatro_direcoes` | Norte → Leste → Sul → Oeste → Norte | HU-03 (base para #176 e #177) | Aprovado |
| `test_girar_esquerda_desfaz_girar_direita` | O giro à esquerda é o inverso do giro à direita | HU-03 (base para #176 e #177) | Aprovado |
| `test_oposta_inverte_a_direcao` | Norte ↔ Sul, Leste ↔ Oeste | HU-03 (base para #176 e #177) | Aprovado |
| `test_direcao_absoluta_converte_lado_do_robo_em_direcao_do_labirinto` | Frente, esquerda e direita convertidas em direção do labirinto | HU-02 e HU-03 (#177) | Aprovado |
| `test_sem_leitura_configurada_a_leitura_falha` | Sem leitura configurada, o LiDAR simulado falha | HU-02 (#172, #174) | Aprovado |
| `test_leitura_fixa_se_repete_a_cada_chamada` | A leitura fixa se repete a cada chamada | HU-02 (#172, #174) | Aprovado |
| `test_fila_entrega_as_leituras_em_ordem_e_depois_volta_para_a_fixa` | Leituras enfileiradas saem em ordem e depois volta a leitura fixa | HU-02 (#172, #174) | Aprovado |
| `test_falha_simulada_retorna_false_e_nao_consome_a_fila` | Falha do sensor não perde leituras da fila | HU-02 (#172) e HU-13 | Aprovado |
| `test_fila_cheia_recusa_novas_leituras` | A fila recusa leituras além da capacidade (16) | HU-02 (#172) | Aprovado |
| `test_pode_ser_usado_pela_interface_ILidar` | O simulado funciona por meio da interface `ILidar` | HU-02 (#172, #173) | Aprovado |

Saída do PlatformIO (resumo):

```text
Processing test_simulacao in native environment
test/test_simulacao/test_lidar_simulado.cpp:85: test_sem_leitura_configurada_a_leitura_falha    [PASSED]
test/test_simulacao/test_lidar_simulado.cpp:86: test_leitura_fixa_se_repete_a_cada_chamada      [PASSED]
test/test_simulacao/test_lidar_simulado.cpp:87: test_fila_entrega_as_leituras_em_ordem_e_depois_volta_para_a_fixa    [PASSED]
test/test_simulacao/test_lidar_simulado.cpp:88: test_falha_simulada_retorna_false_e_nao_consome_a_fila  [PASSED]
test/test_simulacao/test_lidar_simulado.cpp:89: test_fila_cheia_recusa_novas_leituras   [PASSED]
test/test_simulacao/test_lidar_simulado.cpp:90: test_pode_ser_usado_pela_interface_ILidar       [PASSED]
---------------------------------- native:test_simulacao [PASSED] Took 0.97 seconds ----------------------------------

Processing test_nucleo in native environment
test/test_nucleo/test_direcao.cpp:45: test_girar_direita_percorre_as_quatro_direcoes    [PASSED]
test/test_nucleo/test_direcao.cpp:46: test_girar_esquerda_desfaz_girar_direita  [PASSED]
test/test_nucleo/test_direcao.cpp:47: test_oposta_inverte_a_direcao     [PASSED]
test/test_nucleo/test_direcao.cpp:48: test_direcao_absoluta_converte_lado_do_robo_em_direcao_do_labirinto       [PASSED]
----------------------------------- native:test_nucleo [PASSED] Took 0.78 seconds -----------------------------------

Environment    Test            Status    Duration
-------------  --------------  --------  ------------
native         test_simulacao  PASSED    00:00:00.971
native         test_nucleo     PASSED    00:00:00.776
==================================== 10 test cases: 10 succeeded in 00:00:01.747 ====================================
```

A execução filtrada (`pio test -e native -f test_simulacao`) também aprovou os 6 casos da suíte, em 0,89 s.

#### Compilação para a placa (ambiente `esp32c3`)

Comando, executado em `src/firmware`:

```bash
pio run -e esp32c3
```

| Verificação | Resultado |
|---|---|
| Compilação e geração do `firmware.bin` | **Sucesso** |
| Uso de RAM | 4,2% (13 748 de 327 680 bytes) |
| Uso de Flash | 18,9% (247 704 de 1 310 720 bytes) |
| Duração | 143,9 s na primeira execução, incluindo o download da plataforma, do compilador e do Arduino Core |

```text
RAM:   [          ]   4.2% (used 13748 bytes from 327680 bytes)
Flash: [==        ]  18.9% (used 247704 bytes from 1310720 bytes)
Building .pio/build/esp32c3/firmware.bin
Successfully created esp32c3 image.
=========================================== [SUCCESS] Took 143.89 seconds ===========================================
```

#### Verificação de detecção de falhas (teste negativo)

Para confirmar que os testes detectam erros, e não apenas passam, um caso foi alterado de propósito para esperar um resultado errado e depois restaurado.

| Etapa | Ação | Resultado |
|---|---|---|
| 1 | Na linha 21 de `test/test_nucleo/test_direcao.cpp`, o valor esperado de `girarDireita(Norte)` foi trocado de `Leste` para `Sul` | Teste propositalmente incorreto |
| 2 | `pio test -e native -f test_nucleo` | **Falha detectada** na linha 21: `Expected 2 Was 1` |
| 3 | Linha restaurada para `Leste` | Código idêntico ao da execução registrada acima (10 de 10 aprovados) |

```text
test/test_nucleo/test_direcao.cpp:21: test_girar_direita_percorre_as_quatro_direcoes: Expected 2 Was 1  [FAILED]
test/test_nucleo/test_direcao.cpp:46: test_girar_esquerda_desfaz_girar_direita  [PASSED]
test/test_nucleo/test_direcao.cpp:47: test_oposta_inverte_a_direcao     [PASSED]
test/test_nucleo/test_direcao.cpp:48: test_direcao_absoluta_converte_lado_do_robo_em_direcao_do_labirinto       [PASSED]
Program received signal SIGHUP (Hangup: 1)
----------------------------------- native:test_nucleo [ERRORED] Took 1.37 seconds -----------------------------------
================================ 5 test cases: 1 failed, 3 succeeded in 00:00:01.370 ================================
```

**Interpretação da saída:**

- `:21:` indica a linha da verificação que falhou, exatamente a que foi alterada.
- `Expected 2 Was 1`: o Unity compara as direções pelo valor numérico (Norte = 0, Leste = 1, Sul = 2, Oeste = 3). O teste esperava `Sul` (2) e a função devolveu `Leste` (1), que é o resultado correto de um giro à direita a partir do Norte.
- Ao encontrar a primeira verificação que falha, o Unity encerra aquele caso e segue para os próximos; por isso os outros 3 casos continuam aprovados.
- `Program received signal SIGHUP (Hangup: 1)` e o status `ERRORED` não indicam um travamento: o `main` do teste retorna `UNITY_END()`, que é o número de falhas (1). O PlatformIO trata esse código de saída diferente de zero como erro do programa e o conta como mais um item no resumo, daí os "5 test cases" (4 casos + o erro de execução).

#### Pendências desta etapa

- **Cobertura de código:** ainda não medida. O relatório de cobertura (meta de 80%) entra com o CI do firmware ([#223](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/223)).
- **Teste na placa:** o firmware foi compilado, mas ainda não gravado no ESP32-C3. O teste de bancada com o LiDAR real está previsto para 13 e 14/10 ([#175](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/175)), depois de validado o uso dos pinos GPIO20/21, que coincidem com a UART0 do `Serial`.

### Sprint 2 · Classificação parede/livre

| Item | Valor |
| ------ | ------ |
| Tarefa | [#174 Classificação parede/livre](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/174) |
| História de usuário | HU-02 · Reconhecimento de ambiente ([#36](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/36)), RF02 |
| Código testado | `src/firmware` (branch `feat/174-classificador-parede-livre`) |
| Data da execução | 05/10/2026 |
| Executado por | Rafael Lima |
| Documentação do código | [Firmware: implementação](../04-projeto-conceitual/firmware.md) |

**Ambiente de execução**

| Componente | Versão |
| ------ | ------ |
| Sistema operacional | Computador Host (Linux / macOS / Windows) |
| PlatformIO Core | 6.2.0 |
| Plataforma de testes no computador | native 1.2.1 |
| Framework de testes | Unity 2.6.1 |

#### Testes unitários (ambiente native)

Comando executado em src/firmware:

```bash
pio test -e native
```

| Suíte | Casos | Resultado | Duração |
| :--- | :--- | :--- | :--- |
| `test_percepcao` | 6 | **6 aprovados** | 0,80 s |

---

#### Casos de teste

| Caso de teste | O que verifica | HU / tarefa | Resultado |
| :--- | :--- | :--- | :--- |
| `test_classificador_parede_proxima` | Obstáculo próximo (médias < 180 mm) classificado como Parede | HU-02 (#174) | Aprovado |
| `test_classificador_passagem_livre` | Passagem desimpedida (médias > 180 mm) classificada como Livre | HU-02 (#174) | Aprovado |
| `test_classificador_valor_no_limiar` | Limite exato de 180 mm (<= 180 mm Parede, > 180 mm Livre) | HU-02 (#174) | Aprovado |
| `test_classificador_limiar_configuravel` | Alteração e consulta do limiar dinâmico em tempo de execução | HU-02 (#174) | Aprovado |
| `test_classificador_leituras_com_ruido` | Atenuação de picos isolados de ruído via média de amostras | HU-02 (#174) | Aprovado |
| `test_classificador_vetor_vazio` | Vetor de amostras vazio assume Desconhecido por segurança (*fail-safe*) | HU-02 (#174) | Aprovado |

---

#### Saída do PlatformIO

```text
Processing test_percepcao in native environment
test/test_percepcao/test_classificador.cpp: test_classificador_parede_proxima [PASSED]
test/test_percepcao/test_classificador.cpp: test_classificador_passagem_livre [PASSED]
test/test_percepcao/test_classificador.cpp: test_classificador_valor_no_limiar [PASSED]
test/test_percepcao/test_classificador.cpp: test_classificador_limiar_configuravel [PASSED]
test/test_percepcao/test_classificador.cpp: test_classificador_leituras_com_ruido [PASSED]
test/test_percepcao/test_classificador.cpp: test_classificador_vetor_vazio [PASSED]
----------------------------------
native:test_percepcao [PASSED] Took 0.80 seconds
----------------------------------
=================================== 6 test cases: 6 succeeded in 00:00:00.800 ===================================
```

#### Suíte completa com cobertura (o mesmo comando do CI)

```bash
pio test -e native_cobertura
gcovr --root . --filter 'lib/' --object-directory .pio/build/native_cobertura --fail-under-line 80
```

| Verificação | Resultado |
|---|---|
| Testes existentes (núcleo, simulação, percepção, mapa e LiDAR) | **72 aprovados, 0 falhas** |
| Testes novos do simulador | **19 aprovados, 0 falhas** |
| Cobertura das linhas de `lib/` | **98%** (556 de 562), acima da meta de 80% |
| Cobertura de `LabirintoSimulado.cpp` | 99% (115 de 116); a linha fora é o `return` depois do `switch` de `temParede`, que nunca é alcançado e existe para o compilador |

```text
=========== 91 test cases: 91 succeeded in 00:00:04.161 ===========
TOTAL                                        562      556    98%
```

#### Compilação para a placa

```bash
pio run -e esp32c3
```

| Verificação | Resultado |
|---|---|
| Compilação | **Sucesso** |
| Uso de RAM | 4,2% (13 748 de 327 680 bytes) |
| Uso de Flash | 18,9% (247 640 de 1 310 720 bytes) |

O uso de memória não mudou, porque o simulador só é usado nos testes do computador e não entra no firmware da placa.

#### Validação dos labirintos de exemplo

Os três labirintos de `test/test_labirinto_simulado/LabirintosExemplo.h` têm o perímetro fechado e todas as células alcançáveis a partir de (0, 0). Essas duas propriedades são conferidas pelos testes a cada execução (`test_recusa_perimetro_aberto` e `test_todas_as_celulas_dos_exemplos_sao_alcancaveis_da_partida`):

| Labirinto | Formato | Perímetro | Células alcançáveis a partir de (0, 0) |
|---|:-:|:-:|:-:|
| 4×4 | Válido | Fechado | 16 de 16 |
| 8×4 | Válido | Fechado | 32 de 32 (a primeira versão tinha 16 de 32 e foi redesenhada) |
| 12×4 | Válido | Fechado | 48 de 48 |

A tabela de leituras esperadas do 4×4, escrita à mão antes da implementação, bateu com o que o `LabirintoSimulado` calcula nas cinco poses.

---

### Sprint 2 · Armazenamento do mapa em memória

| Item | Valor |
| ------ | ------ |
| Tarefa | [#179 Armazenamento do mapa em memória](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/179) |
| História de usuário | HU-04 · Armazenamento local ([#38](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/38)), RF04 |
| Código testado | `src/firmware` (branch `feat/179-armazenamento-mapa-memoria`) |
| Data da execução | 06/10/2026 |
| Executado por | Marllon Fausto |
| Documentação do código | [README do firmware](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/blob/main/src/firmware/README.md#armazenamento-do-mapa) |

**Ambiente de execução**

| Componente | Versão |
| ------ | ------ |
| Sistema operacional | Linux 7.0 |
| PlatformIO Core | 6.2.0 |
| Plataforma de testes no computador | native 1.2.1 |
| Framework de testes | Unity 2.6.1 |
| Plataforma da placa | espressif32 7.1.3 (ESP32-C3, toolchain riscv32-esp 8.4.0) |

#### Testes unitários (ambiente native)

Comando executado em `src/firmware`:

```bash
pio test -e native -f test_armazenamento_mapa
```

| Suíte | Casos | Resultado | Duração |
| :--- | :--- | :--- | :--- |
| `test_armazenamento_mapa` | 9 | **9 aprovados** | 0,24 s |

A suíte completa (`pio test -e native`) também foi executada: 81 casos, 81 aprovados.

O reinício do estado de navegação é simulado por uma `NavegacaoFalsa`, que recebe o mapa emprestado e guarda a própria posição; destruir e recriar esse objeto equivale a reiniciar a navegação. Além dos casos abaixo, o arquivo de teste confere em tempo de compilação (`static_assert`) que o `ArmazenamentoMapa` não pode ser copiado nem atribuído e que o acesso `const` ao mapa é somente leitura.

#### Casos de teste

| Caso de teste | O que verifica | Critério da #179 | Resultado |
| :--- | :--- | :--- | :--- |
| `test_comeca_sem_corrida_iniciada` | Antes de `iniciarCorrida()` não há corrida e o mapa é 4×4 vazio; depois, a corrida está iniciada | — | Aprovado |
| `test_corrida_12x4_comporta_todas_as_paredes` | Pelo armazenamento, o labirinto 12×4 aceita todas as paredes internas e as devolve pelos dois lados | 1 | Aprovado |
| `test_mapa_preservado_apos_reiniciar_navegacao` | Após destruir e recriar a navegação, o mapa é idêntico (`==`) à cópia feita antes do reinício | 2 | Aprovado |
| `test_mapa_preservado_apos_varios_reinicios` | Em 10 ciclos de registrar e reiniciar, nenhuma parede de ciclos anteriores se perde | 2 | Aprovado |
| `test_navegacao_recriada_enxerga_paredes_anteriores` | A navegação nova encontra as paredes registradas pela anterior | 2 | Aprovado |
| `test_mapa_e_sempre_o_mesmo_objeto` | `mapa()` devolve sempre o mesmo objeto, inclusive após uma nova corrida, então uma referência guardada nunca fica inválida | 2 | Aprovado |
| `test_nova_corrida_zera_o_mapa` | Uma nova corrida volta o mapa ao estado inicial (perímetro `Parede`, interior `Desconhecido`) | 2 | Aprovado |
| `test_nova_corrida_pode_trocar_o_tamanho` | Uma nova corrida pode trocar o labirinto de 4×4 para 12×4 | 1 | Aprovado |
| `test_armazenamento_ocupa_pouca_memoria` | `sizeof(ArmazenamentoMapa)` é de no máximo 256 bytes | 3 | Aprovado |

Para confirmar que os testes detectam falhas, a classe foi alterada de propósito e restaurada em seguida: com `iniciarCorrida()` sem zerar o mapa, 5 casos falharam; permitindo a cópia do armazenamento, o arquivo de teste deixou de compilar com a mensagem do `static_assert`.

#### Uso de memória (critério 3)

Medido compilando o firmware para a placa (`pio run -e esp32c3`), sem e com o `ArmazenamentoMapa` em `src/main.cpp`:

| Medida | Sem o mapa | Com o mapa | Diferença |
| :--- | ---: | ---: | ---: |
| RAM estática (`.data` + `.bss`) | 13.748 bytes | 13.940 bytes | +192 bytes |
| Flash | 247.562 bytes | 247.788 bytes | +226 bytes |

- O objeto `armazenamentoMapa` ocupa **196 bytes** na seção `.bss` (`riscv32-esp-elf-nm -S`: `000000c4 b armazenamentoMapa`): 195 bytes do `Mapa` (48 células × 4 paredes × 1 byte, mais dimensão, linhas e colunas) e 1 byte do indicador de corrida. O valor coincide com o `sizeof` no computador.
- A RAM estática cresce 4 bytes menos que o tamanho do objeto porque parte dele ocupou o enchimento de alinhamento que já existia na `.bss`.
- O mapa usa cerca de 0,06% dos 320 KB de RAM do ESP32-C3; o firmware inteiro usa 4,3%.
- O aumento de Flash é o código que constrói o mapa na partida (preenchimento do perímetro).
- As pilhas das tarefas do FreeRTOS (2 × 4.096 bytes) são reservadas durante a execução e não entram nessa conta. O mapa não fica nelas, e é isso que o preserva quando uma tarefa é reiniciada.

#### Saída do PlatformIO

```text
Processing test_armazenamento_mapa in native environment
test/test_armazenamento_mapa/test_armazenamento_mapa.cpp:231: test_comeca_sem_corrida_iniciada [PASSED]
test/test_armazenamento_mapa/test_armazenamento_mapa.cpp:232: test_corrida_12x4_comporta_todas_as_paredes [PASSED]
test/test_armazenamento_mapa/test_armazenamento_mapa.cpp:233: test_mapa_preservado_apos_reiniciar_navegacao [PASSED]
test/test_armazenamento_mapa/test_armazenamento_mapa.cpp:234: test_mapa_preservado_apos_varios_reinicios [PASSED]
test/test_armazenamento_mapa/test_armazenamento_mapa.cpp:235: test_navegacao_recriada_enxerga_paredes_anteriores [PASSED]
test/test_armazenamento_mapa/test_armazenamento_mapa.cpp:236: test_mapa_e_sempre_o_mesmo_objeto [PASSED]
test/test_armazenamento_mapa/test_armazenamento_mapa.cpp:237: test_nova_corrida_zera_o_mapa [PASSED]
test/test_armazenamento_mapa/test_armazenamento_mapa.cpp:238: test_nova_corrida_pode_trocar_o_tamanho [PASSED]
test/test_armazenamento_mapa/test_armazenamento_mapa.cpp:239: test_armazenamento_ocupa_pouca_memoria [PASSED]
---------- native:test_armazenamento_mapa [PASSED] Took 0.24 seconds ----------
================== 9 test cases: 9 succeeded in 00:00:00.244 ==================
```

```text
RAM:   [          ]   4.3% (used 13940 bytes from 327680 bytes)
Flash: [==        ]  18.9% (used 247788 bytes from 1310720 bytes)
```

---

### Sprint 2 · Simulador de labirinto

| Item | Valor |
|---|---|
| Tarefa | [#224 Simulador de labirinto (LabirintoSimulado)](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/224) |
| História de usuário | HU-02 · Reconhecimento de ambiente ([#36](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/36)), RF02 |
| Código testado | `src/firmware` (branch `feat/labirinto-simulado`) |
| Data da execução | 08/10/2026 |
| Executado por | Cláudio Henrique |
| Documentação do código | [Labirinto simulado](../04-projeto-conceitual/firmware.md#labirinto-simulado-labirintosimulado) |

Esta etapa implementou o `LabirintoSimulado` (carga do texto, validação, consulta de paredes e distâncias) e o modo labirinto do `LidarSimulado`. Os 15 testes ignorados da etapa anterior foram implementados e outros quatro foram acrescentados.

**Ambiente de execução:** o mesmo da Sprint 2 (macOS, PlatformIO Core 6.2.0, `native` 1.2.1, Unity 2.6.1, Espressif 32 7.1.3), com `gcovr` 8.6 para a cobertura.

#### Testes do simulador (`test_labirinto_simulado`)

```bash
pio test -e native -f test_labirinto_simulado
```

| Grupo | Testes | O que verificam |
|---|:-:|---|
| Carga e validação | 7 | Os exemplos 4×4, 8×4 e 12×4 carregam com as dimensões certas, com ou sem `\n` final; tamanho, caractere, parede incompleta e perímetro aberto são recusados com o motivo certo; texto inválido mantém o labirinto anterior |
| Consulta de paredes | 4 | Células vizinhas concordam nos três exemplos; todas as células são alcançáveis a partir de (0, 0); os quatro lados de uma célula; fora do labirinto conta como parede |
| Distâncias | 6 | Corredor, curva, beco sem saída, a tabela de cinco leituras do 4×4, corredores longos do 8×4 e do 12×4 e falha sem carga ou fora do labirinto |
| `LidarSimulado` no modo labirinto | 2 | A leitura acompanha a pose; falha, fila e leitura fixa mantêm a ordem de prioridade |

```text
=================== 19 test cases: 19 succeeded in 00:00:00.694 ============
```

---

### Sprint 3 · Atualização do mapa

| Item | Valor |
| ------ | ------ |
| Tarefa | [#177 Atualização do mapa a cada célula](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/177) |
| História de usuário | HU-03 · Construir o mapa quadriculado durante o percurso ([#37](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/37)), RF03 |
| Código testado | `src/firmware` (branch `feat/177-atualizacao-mapa`, PR [#318](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/318)) |
| Data da execução | 13/10/2026 |
| Executado por | Rafael Lima e Cláudio Henrique |
| Documentação do código | [Firmware: implementação](../04-projeto-conceitual/firmware.md) |

**Ambiente de execução**

| Componente | Versão |
| ------ | ------ |
| Sistema operacional | Computador Host (Linux / macOS / Windows) |
| PlatformIO Core | 6.2.0 |
| Plataforma de testes no computador | native 1.2.1 |
| Framework de testes | Unity 2.6.1 |

#### Testes unitários e de integração (ambiente native)

Comandos executados em `src/firmware`:

```bash
pio test -e native -f test_atualizador_mapa
pio test -e native -f test_integracao_mapa
```

| Suíte | Casos | Resultado | Duração |
| :--- | :--- | :--- | :--- |
| `test_atualizador_mapa` | 10 | **10 aprovados** | 0,44 s |
| `test_integracao_mapa` | 5 | **5 aprovados** | 0,44 s |

---

#### Casos de teste

| Caso de teste | O que verifica | HU / tarefa | Resultado |
| :--- | :--- | :--- | :--- |
| `test_registro_nova_parede` | Registra parede interior desconhecida retornando `Registrada` | HU-03 (#177) | Aprovado |
| `test_registro_parede_ja_conhecida` | Leitura idêntica em parede já existente retorna `JaConhecida` | HU-03 (#177) | Aprovado |
| `test_sem_conflito_ao_percorrer_labirinto` | Percorre células mantendo consistência e `temConflito` falso | HU-03 (#177) | Aprovado |
| `test_parede_desconhecida_nao_registrada` | Classificação `Desconhecido` não altera o mapa e retorna `Invalida` | HU-03 (#177) | Aprovado |
| `test_parede_compartilhada_sem_conflito` | Paredes registradas de células vizinhas coincidem por espelhamento | HU-03 (#177) | Aprovado |
| `test_mapa_final_identico_ao_labirinto_4x4` | Exploração completa das células gera mapa idêntico ao labirinto | HU-03 (#177) | Aprovado |
| `test_conflito_preserva_mapa_e_sinaliza_tem_conflito` | Leitura contraditória marca `temConflito = true` e preserva valor antigo | HU-03 (#177) | Aprovado |
| `test_conversao_direcoes_nas_quatro_orientacoes` | Conversão correta frente/esquerda/direita $\rightarrow$ N/L/S/O nas 4 poses | HU-03 (#177) | Aprovado |
| `test_pose_fora_do_mapa_retorna_invalida` | Pose fora dos limites dimensionais do mapa retorna `Invalida` | HU-03 (#177) | Aprovado |
| `test_sobrecarga_multiplas_amostras_com_classificador` | Sobrecarga com múltiplas amostras filtra ruído e atualiza o mapa | HU-03 (#177) | Aprovado |
| `test_mapa_final_identico_ao_labirinto_4x4` (integração) | Robô simulado explora labirinto 4×4 e mapa fica idêntico | HU-03 (#177) | Aprovado |
| `test_mapa_final_identico_ao_labirinto_8x4` | Robô simulado explora labirinto 8×4 e mapa fica idêntico | HU-03 (#177) | Aprovado |
| `test_mapa_final_identico_ao_labirinto_12x4` | Robô simulado explora labirinto 12×4 e mapa fica idêntico | HU-03 (#177) | Aprovado |
| `test_mapa_nao_e_identico_antes_de_explorar` | Mapa inicial difere do labirinto antes da exploração | HU-03 (#177) | Aprovado |
| `test_uma_celula_registra_so_as_tres_paredes_que_o_sensor_ve` | Registra apenas as paredes observadas na célula atual | HU-03 (#177) | Aprovado |

#### Suíte completa com cobertura (o mesmo comando do CI)

```bash
pio test -e native_cobertura
gcovr --root . --filter 'lib/' --object-directory .pio/build/native_cobertura --fail-under-line 80
```

| Verificação | Resultado |
|---|---|
| Suíte completa (com a `main` mesclada) | **115 aprovados, 0 falhas** |
| Cobertura das linhas de `lib/` | **99%** (592 de 598), acima da meta de 80% |
| Cobertura de `AtualizadorMapa.cpp` | 100% (22 de 22) |

---

#### Saída do PlatformIO

```text
Processing test_atualizador_mapa in native environment
test/test_atualizador_mapa/test_atualizador_mapa.cpp: test_registro_nova_parede                          [PASSED]
test/test_atualizador_mapa/test_atualizador_mapa.cpp: test_registro_parede_ja_conhecida                  [PASSED]
test/test_atualizador_mapa/test_atualizador_mapa.cpp: test_sem_conflito_ao_percorrer_labirinto           [PASSED]
test/test_atualizador_mapa/test_atualizador_mapa.cpp: test_parede_desconhecida_nao_registrada            [PASSED]
test/test_atualizador_mapa/test_atualizador_mapa.cpp: test_parede_compartilhada_sem_conflito            [PASSED]
test/test_atualizador_mapa/test_atualizador_mapa.cpp: test_mapa_final_identico_ao_labirinto_4x4          [PASSED]
test/test_atualizador_mapa/test_atualizador_mapa.cpp: test_conflito_preserva_mapa_e_sinaliza_tem_conflito  [PASSED]
test/test_atualizador_mapa/test_atualizador_mapa.cpp: test_conversao_direcoes_nas_quatro_orientacoes     [PASSED]
test/test_atualizador_mapa/test_atualizador_mapa.cpp: test_pose_fora_do_mapa_retorna_invalida           [PASSED]
test/test_atualizador_mapa/test_atualizador_mapa.cpp: test_sobrecarga_multiplas_amostras_com_classificador [PASSED]
----------------------------------
native:test_atualizador_mapa [PASSED] Took 0.44 seconds
----------------------------------
Processing test_integracao_mapa in native environment
test/test_integracao_mapa/test_integracao_mapa.cpp: test_mapa_final_identico_ao_labirinto_4x4                         [PASSED]
test/test_integracao_mapa/test_integracao_mapa.cpp: test_mapa_final_identico_ao_labirinto_8x4                         [PASSED]
test/test_integracao_mapa/test_integracao_mapa.cpp: test_mapa_final_identico_ao_labirinto_12x4                        [PASSED]
test/test_integracao_mapa/test_integracao_mapa.cpp: test_mapa_nao_e_identico_antes_de_explorar                       [PASSED]
test/test_integracao_mapa/test_integracao_mapa.cpp: test_uma_celula_registra_so_as_tres_paredes_que_o_sensor_ve      [PASSED]
----------------------------------
native:test_integracao_mapa [PASSED] Took 0.44 seconds
----------------------------------
=================================== 15 test cases: 15 succeeded ===================================
```

---

### Sprint 3 · Formato JSON do mapa

| Item | Valor |
| ------ | ------ |
| Tarefa | [#178 Formato JSON do mapa](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/178) |
| História de usuário | HU-03 · Construir o mapa quadriculado durante o percurso ([#37](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/37)), RF03 |
| Código testado | `src/firmware` (branch `feat/178-json-mapa`, PR [#322](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/322)) |
| Data da execução | 15/10/2026 |
| Executado por | Marllon Fausto |
| Documentação do código | [Firmware: implementação](../04-projeto-conceitual/firmware.md) |

**Ambiente de execução**

| Componente | Versão |
| ------ | ------ |
| Sistema operacional | Linux (Ubuntu 22.04.5, kernel 6.8) |
| PlatformIO Core | 6.2.0 |
| Plataforma de testes no computador | native 1.2.1 |
| Framework de testes | Unity 2.6.1 |
| ArduinoJson | 7.4.2 |

#### Testes unitários (ambiente native)

Comando executado em `src/firmware`:

```bash
pio test -e native -f test_json_mapa
```

| Suíte | Casos | Resultado | Duração |
| :--- | :--- | :--- | :--- |
| `test_json_mapa` | 22 | **22 aprovados** | 4,0 s |

A suíte completa com cobertura (`pio test -e native_cobertura`) também foi executada: **103 casos, 103 aprovados**, com `lib/comunicacao/src/MapaJson.cpp` em **100%** de cobertura de linhas (37 de 37) e `lib/` no total em 99%.

---

#### Casos de teste

| Caso de teste | O que verifica | HU / tarefa | Resultado |
| :--- | :--- | :--- | :--- |
| `test_mapa_vazio_4x4_tem_so_o_perimetro_conhecido` | Mapa novo: só o perímetro sai como `true`, o resto como `null` | HU-03 (#178) | Aprovado |
| `test_cabecalho_traz_dimensao_linhas_e_colunas` | Início do JSON do 12×4 traz `dimensao`, `linhas` e `colunas` corretos | HU-03 (#178) | Aprovado |
| `test_parede_livre_e_parede_viram_false_e_true` | `EstadoParede::Parede` vira `true` e `Livre` vira `false` | HU-03 (#178) | Aprovado |
| `test_parede_registrada_aparece_nas_duas_celulas_vizinhas` | A mesma parede aparece espelhada nas duas células que ela separa | HU-03 (#178) | Aprovado |
| `test_json_tem_exatamente_os_campos_combinados_para_a_telemetria` | Objeto raiz com 4 campos e cada célula com 6, sem campo a mais ou a menos | HU-03 (#178) | Aprovado |
| `test_json_sai_compacto_sem_espacos_nem_quebras_de_linha` | Saída sem espaço, tab ou quebra de linha | HU-03 (#178) | Aprovado |
| `test_dimensao_combina_com_as_linhas_e_as_colunas_nos_tres_tamanhos` | `"4x4"/4/4`, `"8x4"/4/8` e `"12x4"/4/12` nos três tamanhos da competição | HU-03 (#178) | Aprovado |
| `test_serializa_mapa_4x4_completo` | Mapa 4×4 com todas as paredes internas: ida e volta bate célula a célula | HU-03 (#178) | Aprovado |
| `test_serializa_mapa_8x4` | Mapa 8×4 parcialmente preenchido: ida e volta bate célula a célula | HU-03 (#178) | Aprovado |
| `test_serializa_mapa_12x4_completo` | Mapa 12×4 com todas as paredes internas, dentro de `TAMANHO_MAX_JSON_MAPA` | HU-03 (#178) | Aprovado |
| `test_serializa_mapa_12x4_parcial` | Mapa 12×4 com poucas paredes conhecidas, como no meio de uma corrida | HU-03 (#178) | Aprovado |
| `test_mapa_12x4_completo_e_reconstruido_a_partir_do_json` | O JSON do 12×4 completo remonta um `Mapa` idêntico (`==`) ao original | HU-03 (#178) | Aprovado |
| `test_mapa_parcial_reconstruido_mantem_as_paredes_desconhecidas` | `null` volta como `Desconhecido`, não como `Livre`, na reconstrução | HU-03 (#178) | Aprovado |
| `test_serializar_nao_altera_o_mapa` | `serializar` não muda o `Mapa` original | HU-03 (#178), HU-04 | Aprovado |
| `test_serializacoes_repetidas_do_mesmo_mapa_geram_o_mesmo_json` | Duas serializações do mesmo mapa produzem o mesmo texto | HU-03 (#178) | Aprovado |
| `test_pior_caso_cabe_em_tamanho_max_json_mapa` | 12×4 com todas as paredes `Livre` (o pior caso) cabe em `TAMANHO_MAX_JSON_MAPA`, com folga menor que 10% | HU-03 (#178) | Aprovado |
| `test_buffer_pequeno_devolve_zero_e_deixa_string_vazia` | Buffer pequeno demais devolve `0` e deixa string vazia | HU-03 (#178) | Aprovado |
| `test_buffer_com_o_tamanho_exato_serve_e_um_a_menos_nao` | Capacidade exata (JSON + `\0`) funciona; um byte a menos falha | HU-03 (#178) | Aprovado |
| `test_buffer_nulo_ou_sem_capacidade_devolve_zero` | Buffer nulo ou capacidade zero devolvem `0` | HU-03 (#178) | Aprovado |
| `test_nao_escreve_depois_do_fim_da_capacidade_informada` | Nada é escrito além da capacidade informada, no sucesso e na falha (checado por sentinela) | HU-03 (#178) | Aprovado |
| `test_capacidade_de_um_byte_devolve_zero_e_string_vazia` | Capacidade de 1 byte devolve `0` e grava só o `\0` | HU-03 (#178) | Aprovado |
| `test_dimensao_fora_do_enum_sai_como_texto_vazio` | Valor fora do enum `DimensaoMapa` sai com `dimensao` vazio, sem travar (*fail-safe*) | HU-03 (#178) | Aprovado |

---

#### Saída do PlatformIO

```text
Processing test_json_mapa in native environment
test/test_json_mapa/test_json_mapa.cpp:419: test_mapa_vazio_4x4_tem_so_o_perimetro_conhecido	[PASSED]
test/test_json_mapa/test_json_mapa.cpp:420: test_cabecalho_traz_dimensao_linhas_e_colunas	[PASSED]
test/test_json_mapa/test_json_mapa.cpp:421: test_parede_livre_e_parede_viram_false_e_true	[PASSED]
test/test_json_mapa/test_json_mapa.cpp:422: test_parede_registrada_aparece_nas_duas_celulas_vizinhas	[PASSED]
test/test_json_mapa/test_json_mapa.cpp:423: test_json_tem_exatamente_os_campos_combinados_para_a_telemetria	[PASSED]
test/test_json_mapa/test_json_mapa.cpp:424: test_json_sai_compacto_sem_espacos_nem_quebras_de_linha	[PASSED]
test/test_json_mapa/test_json_mapa.cpp:425: test_dimensao_combina_com_as_linhas_e_as_colunas_nos_tres_tamanhos	[PASSED]
test/test_json_mapa/test_json_mapa.cpp:427: test_serializa_mapa_4x4_completo	[PASSED]
test/test_json_mapa/test_json_mapa.cpp:428: test_serializa_mapa_8x4	[PASSED]
test/test_json_mapa/test_json_mapa.cpp:429: test_serializa_mapa_12x4_completo	[PASSED]
test/test_json_mapa/test_json_mapa.cpp:430: test_serializa_mapa_12x4_parcial	[PASSED]
test/test_json_mapa/test_json_mapa.cpp:432: test_mapa_12x4_completo_e_reconstruido_a_partir_do_json	[PASSED]
test/test_json_mapa/test_json_mapa.cpp:433: test_mapa_parcial_reconstruido_mantem_as_paredes_desconhecidas	[PASSED]
test/test_json_mapa/test_json_mapa.cpp:434: test_serializar_nao_altera_o_mapa	[PASSED]
test/test_json_mapa/test_json_mapa.cpp:435: test_serializacoes_repetidas_do_mesmo_mapa_geram_o_mesmo_json	[PASSED]
test/test_json_mapa/test_json_mapa.cpp:437: test_pior_caso_cabe_em_tamanho_max_json_mapa	[PASSED]
test/test_json_mapa/test_json_mapa.cpp:438: test_buffer_pequeno_devolve_zero_e_deixa_string_vazia	[PASSED]
test/test_json_mapa/test_json_mapa.cpp:439: test_buffer_com_o_tamanho_exato_serve_e_um_a_menos_nao	[PASSED]
test/test_json_mapa/test_json_mapa.cpp:440: test_buffer_nulo_ou_sem_capacidade_devolve_zero	[PASSED]
test/test_json_mapa/test_json_mapa.cpp:441: test_nao_escreve_depois_do_fim_da_capacidade_informada	[PASSED]
test/test_json_mapa/test_json_mapa.cpp:442: test_capacidade_de_um_byte_devolve_zero_e_string_vazia	[PASSED]
test/test_json_mapa/test_json_mapa.cpp:444: test_dimensao_fora_do_enum_sai_como_texto_vazio	[PASSED]
-------------- native:test_json_mapa [PASSED] Took 4.00 seconds --------------
=================================== 22 test cases: 22 succeeded in 00:00:03.999 ===================================
```

#### Validação do formato com as outras equipes

O formato foi confirmado com a equipe do dashboard e do banco antes da integração (critério 3 da #178):

| Equipe | Resposta | Efeito |
|---|---|---|
| Dashboard ([#239](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/239)) | O JSON atende; `null` será tratado como área não explorada | Formato mantido como está |
| Banco ([#228](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/228)) | Não há requisito de `NOT NULL`; a tabela `mapa_celulas` passou a aceitar `null` nas colunas `parede_*` (BOOLEAN) no PR [#328](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/328) | O firmware pode enviar `null` para parede desconhecida |

#### Pendências desta etapa

- Testar o envio de fato da lista de células à tabela `mapa_celulas` (via PostgREST, com o `corrida_id` acrescentado), que entra com a telemetria (Épico 03).

---

### Sprint 3 · Mapa final serializado

| Item | Valor |
| ------ | ------ |
| Tarefa | [#180 Mapa final serializado](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/180) |
| História de usuário | HU-04 · Armazenamento local ([#38](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/38)), RF04 |
| Código testado | `src/firmware` (branch `feat/180-mapa-final-serializado`, PR [#330](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/330), na `main` desde 16/10/2026) |
| Data da execução | 16/10/2026 |
| Executado por | Eduardo Viana |
| Documentação do código | [Firmware: mapa final da corrida](../04-projeto-conceitual/firmware.md#mapeamento-mapa-final-da-corrida) |

**Ambiente de execução**

| Componente | Versão |
| ------ | ------ |
| Sistema operacional | Linux (Ubuntu 24.04.5 LTS, kernel 7.0) |
| PlatformIO Core | 6.2.0 |
| Plataforma de testes no computador | native 1.2.1 |
| Framework de testes | Unity 2.6.1 |
| ArduinoJson | 7.4.2 |

#### O que mudou no código

| Arquivo | Mudança |
| :--- | :--- |
| `lib/mapeamento/src/ArmazenamentoMapa.h/.cpp` | `congelarMapa()`, `mapaCongelado()` e `mapaFinal()`; o mapa final é uma cópia (`mapaFinal_`) feita no congelamento, e `iniciarCorrida()` destrava o congelamento |
| `lib/comunicacao/src/MapaJson.h/.cpp` | `serializarFinal()`, que serializa o mapa final no formato da [#178](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/178) e devolve `0` se o mapa não foi congelado |
| `test/test_mapa_final/test_mapa_final.cpp` | Suíte nova, um caso por critério de aceite e um de isolamento |
| `test/test_armazenamento_mapa/test_armazenamento_mapa.cpp` | Caso novo `test_congelamento_do_mapa`; o limite de memória subiu de 256 para 512 bytes, porque o objeto agora guarda dois mapas (392 bytes) |

#### Testes unitários e de integração (ambiente native)

Comando executado em `src/firmware`:

```bash
pio test -e native -f test_mapa_final -f test_armazenamento_mapa
```

| Suíte | Casos | Resultado | Duração |
| :--- | :--- | :--- | :--- |
| `test_mapa_final` | 4 | **4 aprovados** | 13,14 s |
| `test_armazenamento_mapa` | 10 | **10 aprovados** | 1,29 s |

A suíte completa com cobertura (`pio test -e native_cobertura`, o mesmo comando do CI) também foi executada: **142 casos, 142 aprovados**, e o `gcovr` mediu **99%** de cobertura das linhas de `lib/` (557 de 561), acima da meta de 80%.

A duração de `test_mapa_final` inclui a compilação na primeira execução, quando o PlatformIO também baixou a plataforma `native`, o Unity e o ArduinoJson.

---

#### Casos de teste

| Caso de teste | O que verifica | Critério da #180 | Resultado |
| :--- | :--- | :--- | :--- |
| `test_congelamento_ao_sinalizar_objetivo` | Antes da corrida não há mapa final; ao sinalizar o objetivo, `congelarMapa()` congela uma única vez (a segunda chamada devolve `false`) e o mapa final guarda as paredes descobertas | 1 | Aprovado |
| `test_serializacao_mapa_final_formato_5_3_3` | `serializarFinal` devolve `0` e string vazia antes do congelamento; depois, gera JSON válido com `dimensao`, `linhas`, `colunas` e as 16 células do 4×4, com os campos `parede_*`; com buffer de 32 bytes devolve `0` | 2 | Aprovado |
| `test_mapa_final_testado_no_labirinto_simulado` | Com o `LabirintoSimulado` 4×4 e o `LidarSimulado`, o robô vai da partida (0, 0) ao objetivo (3, 3) atualizando o mapa sem conflito, congela ao chegar e o JSON do mapa final traz as paredes da partida e do objetivo | 3 | Aprovado |
| `test_mapa_final_permanece_inalterado_apos_novos_registros` | Uma parede registrada depois do congelamento aparece no mapa da corrida e no JSON de `serializar()`, mas o mapa final e o JSON de `serializarFinal()` continuam idênticos | 1 | Aprovado |
| `test_congelamento_do_mapa` (`test_armazenamento_mapa`) | Ciclo de vida completo: recusa sem corrida, recusa do segundo congelamento, isolamento da cópia e destrave com `iniciarCorrida()` | 1 | Aprovado |
| `test_armazenamento_ocupa_pouca_memoria` (`test_armazenamento_mapa`) | `sizeof(ArmazenamentoMapa)` é de no máximo 512 bytes (392 com os dois mapas) | — | Aprovado |

---

#### Saída do PlatformIO

```text
Processing test_mapa_final in native environment
--------------------------------------------------------------------------------
Building...
Testing...
test/test_mapa_final/test_mapa_final.cpp:264: test_congelamento_ao_sinalizar_objetivo	[PASSED]
test/test_mapa_final/test_mapa_final.cpp:265: test_serializacao_mapa_final_formato_5_3_3	[PASSED]
test/test_mapa_final/test_mapa_final.cpp:266: test_mapa_final_testado_no_labirinto_simulado[PASSED]
test/test_mapa_final/test_mapa_final.cpp:267: test_mapa_final_permanece_inalterado_apos_novos_registros	[PASSED]
-------------- native:test_mapa_final [PASSED] Took 13.14 seconds --------------

Processing test_armazenamento_mapa in native environment
--------------------------------------------------------------------------------
Building...
Testing...
test/test_armazenamento_mapa/test_armazenamento_mapa.cpp:281: test_comeca_sem_corrida_iniciada	[PASSED]
test/test_armazenamento_mapa/test_armazenamento_mapa.cpp:282: test_corrida_12x4_comporta_todas_as_paredes	[PASSED]
test/test_armazenamento_mapa/test_armazenamento_mapa.cpp:283: test_mapa_preservado_apos_reiniciar_navegacao	[PASSED]
test/test_armazenamento_mapa/test_armazenamento_mapa.cpp:284: test_mapa_preservado_apos_varios_reinicios	[PASSED]
test/test_armazenamento_mapa/test_armazenamento_mapa.cpp:285: test_navegacao_recriada_enxerga_paredes_anteriores	[PASSED]
test/test_armazenamento_mapa/test_armazenamento_mapa.cpp:286: test_mapa_e_sempre_o_mesmo_objeto	[PASSED]
test/test_armazenamento_mapa/test_armazenamento_mapa.cpp:287: test_nova_corrida_zera_o_mapa[PASSED]
test/test_armazenamento_mapa/test_armazenamento_mapa.cpp:288: test_nova_corrida_pode_trocar_o_tamanho	[PASSED]
test/test_armazenamento_mapa/test_armazenamento_mapa.cpp:289: test_armazenamento_ocupa_pouca_memoria	[PASSED]
test/test_armazenamento_mapa/test_armazenamento_mapa.cpp:290: test_congelamento_do_mapa	[PASSED]
---------- native:test_armazenamento_mapa [PASSED] Took 1.29 seconds ----------

=================================== SUMMARY ===================================
Environment    Test                     Status    Duration
-------------  -----------------------  --------  ------------
native         test_mapa_final          PASSED    00:00:13.143
native         test_armazenamento_mapa  PASSED    00:00:01.287
================= 14 test cases: 14 succeeded in 00:00:14.430 =================
```

#### Pendências desta etapa

- A navegação (Épico 02) ainda precisa chamar `congelarMapa()` quando detectar a célula objetivo; por enquanto o objetivo é sinalizado pelos próprios testes.
- O envio do JSON do mapa final ao servidor, vinculado à corrida criada pela Edge Function `corridas` ([#233](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/233)), entra com a telemetria (Épico 03).
- Medir no ESP32-C3 (`pio run -e esp32c3`) a RAM estática do `ArmazenamentoMapa` com o mapa final; no computador, o `sizeof` é de 392 bytes.
