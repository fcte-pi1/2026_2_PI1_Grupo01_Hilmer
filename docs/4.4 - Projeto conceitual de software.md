# Projeto Conceitual de Software

- [Explicações adicionais](https://drive.google.com/file/d/1WWIz6609c7Y7zAQRBWHSbX0t2vEJ2z1A/view?usp=sharing)
- [Noções de UML](https://drive.google.com/file/d/1l1yt2ittHuRKIXVXYT76P1HriR07pK-t/view?usp=sharing)
- [Requisitos](https://engsoftmoderna.info/cap3.html)

> **Itens fundamentais:**
> - **Diagrama Atividades UML:** descrever e explicar o fluxo do comportamento funcional do produto proposto, evidenciando, de forma clara e estruturada, como as atividades são executadas, em que ordem e sob quais condições. Esse diagrama permite compreender o funcionamento dinâmico do sistema, destacando:
>   - os principais atores (usuários ou sistemas externos) envolvidos no processo;
>   - as atividades de negócio realizadas por cada ator ou pelo próprio sistema;
>   - os insumos (entradas) necessários para a execução das atividades;
>   - os resultados (saídas) gerados ao longo do fluxo;
>   - os pontos de decisão, paralelismo e sincronização das atividades;
>   - Entre as notações mais relevantes, destacam-se o estado inicial e final, atividades, nós de decisão e junção, barras de bifurcação, Raias (*swimlanes*) e Fluxos de controle.
> - **_Backlog_ do Produto**
>   - Detalhar os requisitos funcionais (RF) com a técnica de documentação e especificação de história de usuário (HU);
>   - Protótipos de interface gráfica do *software* em alta fidelidade.
>     - **Todas HUs devem conter sua descrição (Eu-Como-Para), critérios de aceitação e protótipos de interface, documentadas no github.**
>   - Exporte as informações do Backlog do Produto no GitHub Projects [em formato CSV](https://docs.github.com/en/issues/planning-and-tracking-with-projects/managing-your-project/exporting-your-projects-data), e [renderize em Markdown](https://www.google.com/search?q=convert+CSV+file+to+Markdown+table) no formato a seguir:

### Requisitos Funcionais

<u>RF-00/Épico-00: Título do RF/Épico</u>

| ID (Link Github Projects) | Título | Prioridade |
|:------| :-- | :--- |
| HU-00 | | |
| HU-01 | | |
| HU-02 | | |

<u>RF-01/Épico-01: Título do RF/Épico</u>

| ID (Link Github Projects) | Título | Prioridade |
|:--------------------------| :-- | :--- |
| HU-03                     | | |
| HU-04                     | | |
| HU-05                     | | |

### Requisitos Não-Funcionais

| ID (Link Github Projects) | Título | Prioridade | Rastreabilidade |
|:--------------------------| :-- | :--- |:----------------|
| RNF-01                    | | | RF-00/HU-00     |
| RNF-02                    | | |                 |
| RNF-03                    | | |                 |

## Protótipo Funcional do Software, Navegável

> **Nota de Integração:** Conforme a diretriz do template de Engenharia de Software, os protótipos de interface estão vinculados diretamente às Histórias de Usuário (HUs) do Backlog do Produto abaixo, acompanhados de descrição (*Eu-Como-Para*), critérios de aceitação e links diretos para os frames de alta fidelidade no Figma.

### 1. Acesso ao Projeto no Figma
* **Ambiente Interativo (Flow 1):** [Executar Protótipo Navegável](https://www.figma.com/proto/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=7-121&p=f&t=jkjBO9W8kCzgRT8u-1&scaling=min-zoom&content-scaling=fixed&page-id=0%3A1&starting-point-node-id=7%3A121)
* **Canvas de Design (Estrutura de Frames):** [Acessar Arquivo de Design](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=0-1&t=axS6kELL1SrmfIfD-1)

---

### 2. Especificação de Interface por História de Usuário (HUs de IHM)

#### HU-01: Monitoramento de Telemetria e Trajeto ao Vivo
* **Requisitos Associados:** RF03, RF05, RF06, RF07, RF08, RF14, RNF03, RNF05
* **Descrição:**  
  * **Eu, como** operador de bancada ou avaliador da competição,  
  * **Quero** visualizar graficamente o labirinto escuro, o percurso em tempo real do micromouse e os painéis de velocidade, bateria e tempo,  
  * **Para que** eu possa supervisionar a execução autônoma do robô e validar o cumprimento do desafio dentro do teto regulamentar de 10 minutos.
* **Critérios de Aceitação:**
  1. A malha do labirinto deve apresentar fundo escuro (`#0F172A`) com a célula de partida em verde suave no canto inferior e a de chegada em vermelho no canto oposto.
  2. O traçado (`Robot Path`) deve ser atualizado ortogonalmente em tempo real conforme a odometria do robô avança.
  3. Devem ser exibidos cards dedicados de telemetria contendo consumo de bateria (%, V, mA), velocidade média/pico e cronômetro atrelado à tentativa atual e ao limite de 10:00 min.
  4. Ao alcançar o objetivo no canto oposto, o badge de status deve transitar dinamicamente para `DESAFIO CUMPRIDO: SIM` (verde).
* **Protótipo da HU:**  
  * [Frame 01_Idle (Aguardando Largada)](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=7-121)
  * [Frame 02_Running (Em Execução)](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=13-211)
  * [Frame 03_Completed (Desafio Concluído)](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=14-337)

---

#### HU-02: Consulta Consolidada e Filtragem do Histórico de Corridas
* **Requisitos Associados:** RF09, RF10, RF15
* **Descrição:**  
  * **Eu, como** membro da equipe ou docente avaliador,  
  * **Quero** acessar o histórico persistido em banco de dados e filtrar as corridas pelo tipo de labirinto (Todos, 4×4, 8×4, 12×4),  
  * **Para que** eu possa auditar a evolução do desempenho, verificar a pontuação por tentativa e comparar os tempos finais consolidados.
* **Critérios de Aceitação:**
  1. A navegação entre a telemetria ao vivo e o histórico deve ocorrer de forma instantânea via abas no cabeçalho global.
  2. A visualização padrão (`Todos os Labirintos`) deve listar tabularmente todas as corridas salvas no banco com Data/Hora, Labirinto, Tentativa/Nota, Tempo Final, Velocidade, Bateria e Status.
  3. O controle segmentado de filtros deve permitir isolar os registros exclusivamente por labirinto (`4×4`, `8×4` ou `12×4`).
* **Protótipo da HU:**  
  * [Frame 04_History_View (Todos os Labirintos)](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=21-411)
  * [Frame 04_History_View_M1 (Filtro Labirinto 1)](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=21-573)
  * [Frame 04_History_View_M2 (Filtro Labirinto 2)](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=21-655)
  * [Frame 04_History_View_M3 (Filtro Labirinto 3)](https://www.figma.com/design/90hJ4r7jT0yDMtCwJ0yzy9/Prot%C3%B3tipo-Telemetria-Web?node-id=21-737)

---

### 3. Matriz Consolidada de Rastreabilidade dos Requisitos

| ID | Requisito Formal | Implementação no Protótipo |
| :---: | :--- | :--- |
| **RF03 / RF06** | Construção do mapa e percurso do robô | Frame `Maze Grid` com malha dimensional ortogonal de fundo (RF03) e vetor dinâmico alaranjado `Robot Path` indicando a rota percorrida e a odometria (RF06). |
| **RF05** | Localização do robô | Marcador circular `Robot` indicando em tempo real a célula discreta ocupada pelo micromouse. |
| **RF07** | Detecção do objetivo | Célula de destino com realce avermelhado no canto diametralmente oposto; aciona o badge de missão cumprida ao ser interceptada. |
| **RF08 / RF14** | Telemetria / Dashboard Web | Painel lateral contendo cartões desacoplados em auto layout para consumo de bateria, velocidade média (com pico) e tempo. |
| **RF09** | Armazenamento em banco de dados | Transição para o estado `03_Completed`, consolidando métricas finais para envio ao repositório relacional. |
| **RF10** | Consulta de todos os labirintos | Modo de visualização global da tabela histórica agregando as corridas de todas as configurações. |
| **RF15** | Gestão de Corridas / Filtros | Barra de controle segmentado (`Todos`, `4×4`, `8×4`, `12×4`) permitindo a filtragem imediata das consultas. |
| **RNF03** | Tempo limite de execução | Cronômetro com marcador explícito do teto de 10:00 minutos regulamentares da bateria de testes. |

---

### 4. Roteiro Operacional de Navegação do Protótipo

1. **Estado Inicial (`01_Idle`):** Clique no botão primário **`INICIAR CORRIDA`** na barra lateral para iniciar a transmissão de pacotes e avançar para `02_Running`.
2. **Execução e Conclusão (`02_Running` $\rightarrow$ `03_Completed`):** A navegação progride dinamicamente via *After delay* com *Smart Animate* até alcançar a célula de chegada oposta, disparando o badge verde de conclusão e o congelamento do cronômetro.
3. **Nova Tentativa:** No frame `03_Completed`, o botão **`RESETAR / NOVA TENTATIVA`** reinicia o ciclo em `01_Idle` para nova passagem de bancada.
4. **Auditoria de Histórico:** No cabeçalho global, clique em **`Histórico de Consultas`** para alternar para a visão analítica (`04_History_View`), navegando entre os filtros de labirinto para inspecionar os dados persistidos.
5. **Filtragem de Dados:** Na tela de histórico, clique nos botões de controle segmentado (`Todos`, `Labirinto 1`, `Labirinto 2`, `Labirinto 3`) para alternar a exibição filtrada dos registros. Para voltar à bancada ao vivo, selecione a aba **`Telemetria ao Vivo`**.

> 
> - **Descrição da arquitetura da solução de _software_ proposta:**
>   - Esta subseção deve contemplar o documento de arquitetura do sistema e deve ser estruturado segundo as visões (4+1) previstas no processo unificado (UP): lógica, de processos; implementação, implantação e dados (substituirá a visão de casos de uso).
>   - Propósito do *software* (qual o seu papel no sistema);
>   - Padrão adotado: MVC, MVP, Microsserviços, Monolítico, etc (Justificar);
>   - Linguagens de programação: Java, Python, C#, JavaScript, etc.
>   - *Frameworks* e bibliotecas: Spring Boot, .NET Core, React, Angular, Django, etc.
>   - Banco de dados: Relacional (PostgreSQL, MySQL, etc) X NãoSQL (MongoDB, etc).
>   - Persistência de dados: Modelo Entidade-Relacionamento (MER) e seu respectivo Diagrama Entidade-Relacionamento (DER), aplicáveis quando a solução utiliza banco de dados relacional; alternativamente, diagrama de estrutura de documentos, empregado nos casos em que a arquitetura adota banco de dados não relacional.
> - **Roteiro de testes funcionais:**
>   - Código do caso de teste;
>   - Nome do caso de teste;
>   - Rastreabilidade: Link do(a) RF/HU associado(a);
>   - Objetivo do caso de teste;
>   - Pré-condições do sistema para o teste ser realizado, quando se aplicar;
>   - Descrição dos procedimentos a serem executados para o teste;
>   - Resultado esperado para o teste ser aprovado (pós-condição após realizado o teste);
