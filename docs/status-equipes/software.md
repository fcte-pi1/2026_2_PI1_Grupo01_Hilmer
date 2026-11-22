# Status de Software

**Gerente:** Cláudio Henrique. **Integrantes:** Ana Carolina Fialho (também gerente geral do projeto), Eduardo Viana, Marllon Fausto e Rafael Lima.

**Última atualização:** 23/10/2026 (Sprint 5). **Atualizado por:** Ana Carolina Fialho (gerente geral), a partir das *issues* e dos PRs do GitHub.

**O que a equipe entrega:** o firmware do ESP32 DevKit V1 (percepção, mapa, navegação, controle dos motores), o backend no Supabase (banco e Edge Function) e o dashboard web. Detalhes em [Software](../04-projeto-conceitual/software.md) e [Firmware](../04-projeto-conceitual/firmware.md).

!!! warning "Mudança de hardware (Rev 2.0, 21/10/2026)"
    A eletrônica trocou o LiDAR por **3 sensores VL53L0X** (I²C), acrescentou a **IMU MPU6050** e passou para o motor **N20**, com encoders. Desde 10/11/2026 a placa do robô é o **ESP32 DevKit V1** da bancada integrada, com os encoders em GPIO32/33 e GPIO18/19 (veja a [pinagem](../04-projeto-conceitual/firmware.md#pinagem)). O software abriu as tarefas [#377](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/377) a [#382](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/382) para se adaptar; a pinagem nova já está no `Pinos.h` ([#377](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/377), PR [#390](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/390)).

## Onde está

| **Épico** | **Marco** | **Situação** |
|-----------|:---------:|--------------|
| 01 - Percepção e Mapeamento | ED1 | Mapa completo (estrutura, atualização, armazenamento, JSON e mapa final). Falta a percepção da Rev 2.0: driver dos VL53L0X ([#344](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/344), PR [#391](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/391) com alterações pedidas), limiares para ToF ([#379](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/379)) e bancada dos sensores ([#175](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/175)) |
| 02 - Navegação e Controle Autônomo | ED1 | Resolução do labirinto, odometria, centralização, detecção do objetivo e tratamento de colisões (HU-12) prontos. Em andamento: robustez da navegação ([#357](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/357), PR [#400](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/400)), motor travado ([#341](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/341), PR [#396](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/396) em rascunho), IMU ([#378](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/378)) e deslocamento sem encoder ([#380](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/380)) |
| 03 - Telemetria e Dashboard | ED1/ED2 | Banco, registro de corridas, frontend e histórico com dados reais prontos (HU-10 fechada). Em andamento: gravação da telemetria no banco ([#229](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/229), PR [#398](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/398) em rascunho). Faltam o envio do ESP32 ([#236](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/236)) e o dashboard ao vivo ([#231](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/231)) |
| 04 - Inicialização | ED2 | Não iniciado |

!!! note "Sobre o cronograma"
    A tabela de software em [Cronograma](../05-cronograma.md) pode não refletir as linhas já entregues. Para o progresso real, vale o estado das *issues* e dos PRs abaixo.

## Entregue (mesclado em `main`)

- **Base:** projeto base do firmware ([#172](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/172)), simulador de labirinto ([#224](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/224)), CI do firmware com testes nativos, cobertura mínima de 80% e build da placa ([#223](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/223)) e CI do backend com testes e cobertura ([#345](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/345)). Os dois CIs são obrigatórios na proteção da `main` (o do backend desde 23/10).
- **Rev 2.0:** pinagem nova no `Pinos.h` ([#377](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/377)), hoje a do ESP32 DevKit V1, com o log pela UART0.
- **Percepção (versão LiDAR):** driver de leitura do LiDAR ([#173](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/173)) e classificação parede/livre ([#174](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/174)). Com a troca de sensor, a percepção é refeita nas tarefas [#344](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/344) e [#379](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/379).
- **Mapa:** estrutura de dados ([#176](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/176)), atualização a cada célula ([#177](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/177)), armazenamento em memória ([#179](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/179)), mapa final serializado ([#180](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/180)) e formato JSON ([#178](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/178)).
- **Navegação:** algoritmo de resolução ([#183](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/183)), leitura dos encoders ([#186](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/186)), odometria ([#187](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/187)), centralização no corredor e parada frontal ([#191](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/191)) e detecção da célula objetivo com evento na telemetria ([#189](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/189)).
- **Backend:** Supabase com esquema do banco ([#228](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/228)) e registro e encerramento de corridas por Edge Function ([#233](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/233)).
- **Frontend:** estrutura React + Vite e layout do dashboard ([#230](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/230)) e tela de histórico com dados reais ([#235](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/235)).

## Em revisão

| **PR** | **Assunto** | **Tipo** |
|:------:|-------------|:--------:|
| [#400](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/400) | Robustez da navegação: validação da pose e passo bloqueado ([#357](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/357)) | Código |
| [#391](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/391) | Driver dos 3 VL53L0X: I²C, XSHUT e endereços ([#344](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/344)); alterações pedidas | Código |
| [#396](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/396) | Motor travado sem encoder ([#341](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/341)); rascunho | Código |
| [#398](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/398) | Telemetria em lote, mapa final e consolidação da corrida no banco ([#229](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/229)); rascunho | Código |
| [#373](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/373) | Documentação do algoritmo de resolução ([#183](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/183)) | Documentação |
| [#392](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/392) | Documentação do driver dos VL53L0X ([#344](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/344)) | Documentação |
| [#401](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/pull/401) | Documentação da robustez da navegação ([#357](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/357)) | Documentação |
| — | Testes do frontend (Vitest e Testing Library) e CI ([#394](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/394)) | Testes / CI |

## Em aberto, por prioridade para a ED1 (26/10)

- **Testes da Rev 2.0:** execução e registro dos testes isolados de software ([#381](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/381), prazo 25/10) e documentação da Rev 2.0 ([#382](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/382)).
- **Sensores e movimento:** driver dos VL53L0X ([#344](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/344)), limiares para ToF ([#379](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/379)), IMU ([#378](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/378)), deslocamento sem encoder ([#380](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/380)), motores N20 ([#341](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/341)) e execução dos movimentos entre células ([#184](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/184)).
- **Navegação:** correção do erro acumulado pelas paredes ([#188](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/188)) e detecção de colisão e recuperação sem perda do mapa ([#192](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/192)).
- **Telemetria:** envio do ESP32 ao Supabase ([#236](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/236)), buffer e reenvio ([#238](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/238)), dashboard ao vivo ([#231](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/231)) e mapa e percurso no dashboard ([#239](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/239)).
- **Frontend:** testes com Vitest e Testing Library e CI do frontend ([#394](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/394)), implementados na branch feat e documentados, destravando as demais tarefas do dashboard (#231, #234, #239 e #242).

**Depois da ED1:** corrida completa ([#185](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/185)), latência ([#237](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/237)), filtro do histórico ([#234](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/234)), teste ponta a ponta ([#232](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/232)), calibração ([#240](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/240)), falha de I²C ([#241](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/241)), estado "Aguardando Largada" ([#242](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/242)) e controle de rumo pelo giroscópio ([#190](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/190), APT).

## Precisa ou aguarda de outras equipes

| **De** | **O quê** | **Dependência** |
|--------|-----------|:---------------:|
| Eletrônica | Hardware Rev 2.0 montado para a bancada (3 VL53L0X, MPU6050, driver e motores N20) | D1 |
| Eletrônica | Divisor da bateria (100 kΩ / 33 kΩ) montado no GPIO34 do ESP32 DevKit V1, o pino do ADC da bateria no `Pinos.h` | D2, D7 |
| Eletrônica | Redução, corrente de travamento e tensão do motor N20 (valores provisórios em `ConfiguracaoProtecaoMotor.h`) | D1 |
| Estrutura | Posição dos 3 VL53L0X no chassi, base da calibração dos limiares ([#379](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/379)) | D9 |
| Estrutura | Diâmetro da roda e bitola definitivos (a odometria usa valores provisórios) | D3 |
| Estrutura | Labirinto de testes físico | D4 |
| Energia | Tabela de estado de carga e leitura do ADC para a telemetria de bateria | D7 |

Veja a tabela completa em [Status das equipes](README.md#dependencias-entre-equipes).

## Riscos conhecidos

- A ED1 é em 26/10 e a percepção com os VL53L0X ([#344](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/344), [#379](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/379)) e a IMU ([#378](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/378)) seguem abertas; os testes de software com o hardware real dependem delas e do hardware montado.
- Não consta nos documentos nenhum teste do firmware com o hardware Rev 2.0 concluído; os testes registrados rodam no computador. A [#381](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/381) prevê que, sem hardware a tempo, a validação em bancada passe para a ED2.
- Até os encoders dos N20 serem integrados ao firmware, o deslocamento é estimado pelo comando dos motores e corrigido pelos ToF ([#380](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/380)); com os encoders já disponíveis, vale rever com a equipe o escopo da #380 e da detecção de motor travado (#341), que foram pensadas para o robô sem encoder.
- A infraestrutura de testes automatizados e o CI do frontend foram configurados com sucesso ([#394](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/394)), mitigando o risco de regressões no dashboard.

## Fontes

[Épicos e tarefas de software](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues?q=is%3Aissue+label%3Asoftware) · [Cronograma de software](../05-cronograma.md#software-epicos-sprints-e-historias-de-usuario) · [Testes de software](../07-testes/software.md) · [Projeto conceitual de hardware](../04-projeto-conceitual/hardware.md)

## Histórico de versões

| **Versão** | **Data** | **Descrição** | **Autor(es)** | **Revisor(es)** |
|:----------:|:--------:|---------------|---------------|-----------------|
| 1.0 | 19/10/2026 | Criação da página e primeiro levantamento a partir das *issues*, dos PRs e dos documentos do projeto | Cláudio Henrique | Paulo Henrique |
| 1.1 | 23/10/2026 | Atualização para a Rev 2.0 do hardware: entregas, PRs em revisão, prioridades da ED1 (#377 a #382 e #394), dependências e riscos | Ana Carolina Fialho | — |
| 1.2 | 23/10/2026 | Atualiza status da infraestrutura de testes do frontend e CI (#394) | Rafael Lima | Eduardo Viana |
