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
=======
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
