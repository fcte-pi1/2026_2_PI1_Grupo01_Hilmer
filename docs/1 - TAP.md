# **Termo de Abertura do Projeto** 

**Nome do Projeto:** Desenvolvimento de um Micromouse Autônomo com Sistema de Telemetria 

**Data de Início:** 02/09/2026 

**Data de Término:** 09/09/2026 

# **Visão Geral do Projeto** 

# **Descrição do Problema** 

Dado um labirinto composto por múltiplos caminhos, obstáculos e possíveis rotas, buscase desenvolver uma solução robótica capaz de realizar sua exploração e encontrar autonomamente a área de objetivo. Para isso, propõe-se a construção de um robô de pequenas dimensões, denominado Micromouse, capaz de se deslocar pelo labirinto sem a utilização de controle remoto ou intervenção humana durante sua execução. 

O desafio consiste em fazer com que o robô seja capaz de perceber o ambiente, identificar os caminhos disponíveis, tomar decisões de navegação e controlar seu próprio deslocamento até alcançar o objetivo, considerando que a configuração do labirinto é desconhecida previamente pelo sistema. Além disso, o robô deve realizar o percurso sem causar danos à estrutura da pista e respeitando suas restrições físicas e operacionais. 

# **Objetivos** 

Desenvolver um sistema robótico autônomo capaz de navegar e completar diferentes configurações de um labirinto desconhecido, alcançando a área de objetivo sem intervenção humana e sem causar danos à estrutura da pista. Além do sistema robótico, o projeto deverá contemplar um sistema de telemetria capaz de apresentar informações de operação em tempo real e um sistema de armazenamento dos dados obtidos durante os testes e desafios, permitindo posteriormente sua consulta e análise. O sistema será desenvolvido considerando as seguintes configurações de labirinto: 

|**Labirinto**|**Configuração**|**Dimensão**|
|---|---|---|
|1°|4 × 4 células|72 × 72 cm|
|2°|8 × 4 células|144 × 72 cm|
|3°|12 × 4 células|216 × 72 cm|



# **Escopo do Projeto** 

O projeto compreende o desenvolvimento, integração e validação de um sistema de Micromouse autônomo e dos sistemas de suporte necessários para sua operação e avaliação. 

# **Incluído no escopo:** 

- Projeto e fabricação da estrutura mecânica do robô; 

- Desenvolvimento do sistema de locomoção; 

- Seleção e integração dos sensores; 

- Desenvolvimento dos sistemas eletrônicos e de alimentação; 

- Desenvolvimento do software embarcado; 

- Desenvolvimento dos algoritmos de percepção, navegação e tomada de decisão; 

- Desenvolvimento do sistema de controle de movimento; 

- Desenvolvimento da comunicação entre o robô e o sistema de telemetria; 

- Desenvolvimento da interface web de telemetria; 

- Desenvolvimento do sistema de armazenamento e consulta dos dados; 

- Desenvolvimento de uma pista de testes; 

- Integração entre os subsistemas mecânico, eletrônico e computacional; 

- Testes de funcionamento, desempenho e confiabilidade; 

- Documentação técnica do projeto. 

# **Stakeholders** 

- Orientador do projeto: Hilmer Rodrigues Neri 

- Professores (Avaliadores): Diogo C. Garcia, Juliana P. Rodrigues, Lui T. C. Habl, Bruno L. Pereira 

Professores e avaliadores da disciplina de Projeto Integrador 1 (PI1) da FCTE/UnB, responsáveis por avaliar o desempenho técnico e o processo de desenvolvimento do grupo; os próprios estudantes da equipe, que se beneficiam do aprendizado prático e integrado das engenharias Aeroespacial/Automotiva, Eletrônica e Software; e a comunidade acadêmica interessada em competições de robótica que pode usar o projeto como referência de estudo.

# **Recursos do Projeto** 

# **Membros da Equipe** 

|**Nome**|**Matrícula**|**Curso**|**E-mail**|**Funções**|
|---|---|---|---|---|
|Ágata Dias<br>Vieira|241012089|Engenharia<br>Aeroespacial|241012089@<br>aluno.unb.br|Estrutura|
|Aline<br>de<br>Carvalho<br>Rodrigues|180096923|Engenharia<br>Eletrônica|180096923@<br>aluno.unb.br|Eletrônica|
|Ana Carolina<br>Fialho|221031102|Engenharia<br>de Software|221031102@<br>aluno.unb.br|Software|
|Ana Carolina<br>Nunes|22100770|Engenharia<br>Aeroespacial|221007770@<br>aluno.unb.br|Estrutura|
|Cláudio|221007958|Engenharia|221007958@<br>aluno.unb.br|Software|



|**Nome**|**Matrícula**|**Curso**|**E-mail**|**Funções**|
|---|---|---|---|---|
|Henrique||de Software|||
|Eduardo<br>Gaspar|251020191|Engenharia<br>de Software|251020191@<br>aluno.unb.br|Software|
|Eliabe Alves|251020208|Engenharia<br>de Software|251020208@<br>aluno.unb.br|Software|
|Elias Farias<br>de Oliveira|221007706|Engenharia<br>de Software|221007706@<br>aluno.unb.br|Software|
|Giovanna da<br>Costa|251021303|Engenharia<br>de Software|251021303@<br>aluno.unb.br|Software|
|Letícia Lima<br>Dos Santos|200022393|Engenharia<br>Aeroespacial||Estrutura|
|Marllon<br>Fausto<br>Cardoso|222025914|Engenharia<br>de Software|222025914@<br>aluno.unb.br|Software|
|Paulo<br>Henrique<br>Melo<br>de<br>Souza|221022417|Engenharia<br>de Software|221022417@<br>aluno.unb.br|Software|



|**Nome**|**Matrícula**|**Curso**|**E-mail**|**Funções**|
|---|---|---|---|---|
|Eduardo<br>Viana RIbeiro<br>da Silva|251036440|Engenharia<br>de Software|251036440@<br>aluno.unb.br|Software|



**Nota sobre funções:** Ao decorrer do projeto, é comum que algumas funções possam ser remanejadas em detrimento de outras, sendo importante ressaltar que temos funções primárias e secundárias, de acordo com habilidades. **Orientador:** Prof. Hilmer Rodrigues Neri (Turma 05) 

# **Orçamento estimado (R$)** 

|**Categoria**|**Itens**|**Faixa (R$)**|
|---|---|---|
|Estrutura/Mecânica|Filamento 3D, rodas,<br>parafusos/porcas,<br>suportes de motor|R$120,00|
|Energia|Bateria<br>Li-Po/Li-Ion,<br>carregador, regulador de<br>tensão, conectores|R$80,00|
|Hardware/Controle|Microcontrolador(ESP32),<br>driver<br>de<br>motor,<br>micromotores c/ encoder,<br>IMU, protoboard/cabos,<br>PCB (opcional)|R$430,00|



|**Categoria**|**Itens**|**Faixa (R$)**|
|---|---|---|
|Sensores|Sensores de distância x3-<br>5|R$150,00|
|Software/Telemetria|Hospedagem web (free<br>tier), banco de dados|R$0,00|
|Ferramentas/Pista<br>de<br>testes|Solda, fita isolante,<br>material da pista 4x4<br>simplificada|R$0,00|
|**TOTAL**<br>**(sem**<br>**contingência)**||**R$780**|
|**Total com contingência**<br>**(15-20%)**|Peças extras, retrabalho<br>de PCB, imprevistos|R$924|



# **Esforço estimado (horas)** 

O esforço estimado será contabilizado considerando a quantidade de horas dedicadas por cada integrante da equipe às atividades relacionadas ao projeto. Serão 90h por aluno, contabilizando 60h dentro do horário de aula, mais cerca de 30h fora da sala de aula. Considerando atividades de pesquisa, planejamento, definição de requisitos, desenvolvimento, fabricação, programação, integração, testes e documentação.

Considerando os 13 integrantes da equipe, o esforço total estimado para o projeto é de aproximadamente 1.170 horas.

