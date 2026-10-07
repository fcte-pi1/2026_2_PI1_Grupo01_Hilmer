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

### Sprint 2 · Classificação parede/livre

| Item | Valor |
| ------ | ------ |
| Tarefa | [#174 Classificação parede/livre](https://github.com/fcte-pi1/2026\_2\_PI1\_Grupo01\_Hilmer/issues/174) |
| História de usuário | HU-02 · Reconhecimento de ambiente ([#36](https://github.com/fcte-pi1/2026\_2\_PI1\_Grupo01\_Hilmer/issues/36)), RF02 |
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

### Casos de Teste

| Caso de teste | O que verifica | HU / tarefa | Resultado |
| :--- | :--- | :--- | :--- |
| `test_classificador_parede_proxima` | Obstáculo próximo (médias < 180 mm) classificado como Parede | HU-02 (#174) | Aprovado |
| `test_classificador_passagem_livre` | Passagem desimpedida (médias > 180 mm) classificada como Livre | HU-02 (#174) | Aprovado |
| `test_classificador_valor_no_limiar` | Limite exato de 180 mm (<= 180 mm Parede, > 180 mm Livre) | HU-02 (#174) | Aprovado |
| `test_classificador_limiar_configuravel` | Alteração e consulta do limiar dinâmico em tempo de execução | HU-02 (#174) | Aprovado |
| `test_classificador_leituras_com_ruido` | Atenuação de picos isolados de ruído via média de amostras | HU-02 (#174) | Aprovado |
| `test_classificador_vetor_vazio` | Vetor de amostras vazio assume Desconhecido por segurança (*fail-safe*) | HU-02 (#174) | Aprovado |

---

### Saída do PlatformIO

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