# Termo de Abertura do Projeto

**Nome do Projeto:** Desenvolvimento de um Micromouse Autônomo com Sistema de Telemetria  
**Data de Início:** 07/09/2026  
**Data de Término:** 04/12/2026

---

## 1. Visão Geral do Projeto

### 1.1 Descrição do Problema
Dado um labirinto composto por múltiplos caminhos, obstáculos e possíveis rotas, busca-se desenvolver uma solução robótica capaz de realizar sua exploração e encontrar autonomamente a área de objetivo. Para isso, propõe-se a construção de um robô de pequenas dimensões, denominado Micromouse, capaz de se deslocar pelo labirinto sem a utilização de controle remoto ou intervenção humana durante sua execução.

O desafio consiste em fazer com que o robô seja capaz de perceber o ambiente, identificar os caminhos disponíveis, tomar decisões de navegação e controlar seu próprio deslocamento até alcançar o objetivo, considerando que a configuração do labirinto é desconhecida previamente pelo sistema. Além disso, o robô deve realizar o percurso sem causar danos à estrutura da pista e respeitando suas restrições físicas e operacionais.

### 1.2 Objetivos
Desenvolver um sistema robótico autônomo capaz de navegar e completar diferentes configurações de um labirinto desconhecido, alcançando a área de objetivo sem intervenção humana e sem causar danos à estrutura da pista. 

Além do sistema robótico, o projeto deverá contemplar um sistema de telemetria capaz de apresentar informações de operação em tempo real e um sistema de armazenamento dos dados obtidos durante os testes e desafios, permitindo posteriormente sua consulta e análise.

O sistema será desenvolvido considerando as seguintes configurações de labirinto:

| Labirinto | Configuração | Dimensão |
| :--- | :--- | :--- |
| **1°** | 4 × 4 células | 72 × 72 cm |
| **2°** | 8 × 4 células | 144 × 72 cm |
| **3°** | 12 × 4 células | 216 × 72 cm |

---

## 2. Escopo do Projeto

O projeto compreende o desenvolvimento, integração e validação de um sistema de Micromouse autônomo e dos sistemas de suporte necessários para sua operação e avaliação.

### 2.1 Incluído no Escopo
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

---

## 3. Stakeholders

- **Professores e avaliadores da disciplina de Projeto Integrador 1 (PI1) da FCTE/UnB:** Responsáveis por avaliar o desempenho técnico e o processo de desenvolvimento do grupo;
- **Comunidade acadêmica:** Interessados em competições de robótica que podem usar o projeto como referência de estudo.

---

## 4. Recursos do Projeto

### 4.1 Membros da Equipe

| Nome | Matrícula | Curso | E-mail | Função Principal |
| :--- | :--- | :--- | :--- | :--- |
| **Ágata Dias Vieira** | 241012089 | Eng. Aeroespacial | 241012089@aluno.unb.br | Estrutura |
| **Aline de Carvalho Rodrigues** | 180096923 | Eng. Eletrônica | 180096923@aluno.unb.br | Gerente de Eletrônica |
| **Ana Carolina Fialho** | 221031102 | Eng. de Software | 221031102@aluno.unb.br | Gerente de Software |
| **Ana Carolina Nunes** | 221007770 | Eng. Aeroespacial | 221007770@aluno.unb.br | Gerente de Estrutura |
| **Bruno Bernardes Duarte** | 242034483 | Eng. de Software | 242034483@aluno.unb.br | Gerente Geral |
| **Cláudio Henrique** | 221007958 | Eng. de Software | 221007958@aluno.unb.br | Software |
| **Daniel Rodrigues Nascimento** | 231037665 | Eng. de Software | 231037665@aluno.unb.br | Software |
| **Eduardo Gaspar** | 251020191 | Eng. de Software | 251020191@aluno.unb.br | Software |
| **Eduardo Viana Ribeiro da Silva** | 251036440 | Eng. de Software | 251036440@aluno.unb.br | Software |
| **Eliabe Alves** | 251020208 | Eng. de Software | 251020208@aluno.unb.br | Software |
| **Elias Faria de Oliveira** | 221007706 | Eng. de Software | 221007706@aluno.unb.br | Software |
| **Giovanna da Costa** | 251021303 | Eng. de Software | 251021303@aluno.unb.br | Software |
| **João Artur de Andrades** | 231038690 | Eng. Aeroespacial | 231038690@aluno.unb.br | Estrutura |
| **José Felipe Duarte Guedes de Oliveira** | 221008211 | Eng. de Software | 221008211@aluno.unb.br | Software |
| **Letícia Lima dos Santos** | 200022393 | Eng. Aeroespacial | 200022393@aluno.unb.br | Estrutura |
| **Marllon Fausto Cardoso** | 222025914 | Eng. de Software | 222025914@aluno.unb.br | Software |
| **Paulo Henrique Melo de Souza** | 221022417 | Eng. de Software | 221022417@aluno.unb.br | Gerente de Energia |
| **Rafael Lima Sant'Ana** | 251035659 | Eng. de Software | 251035659@aluno.unb.br | Software |

**Orientador:** Prof. Dr. Hilmer Rodrigues Neri (Turma 05)

> **Nota sobre funções:** Ao decorrer do projeto, é comum que algumas funções possam ser remanejadas em detrimento de outras, sendo importante ressaltar que temos funções primárias e secundárias, de acordo com as habilidades de cada membro.

---

### 4.2 Orçamento Estimado

| Categoria | Itens | Faixa de Preço (R$) |
| :--- | :--- | :--- |
| **Estrutura / Mecânica** | Filamento 3D, rodas, parafusos/porcas, suportes de motor | R$ 120,00 |
| **Energia** | Bateria Li-Po/Li-Ion, carregador, regulador de tensão, conectores | R$ 80,00 |
| **Hardware / Controle** | Microcontrolador (ESP32), driver de motor, micromotores c/ encoder, IMU, protoboard/cabos, PCB (opcional) | R$ 430,00 |
| **Sensores** | Sensores de distância (x3 a x5) | R$ 150,00 |
| **Software / Telemetria** | Hospedagem web (free tier), banco de dados | R$ 0,00 |
| **Ferramentas / Pista de testes** | Solda, fita isolante, material da pista 4x4 simplificada | R$ 0,00 |
| **TOTAL (sem contingência)** | — | **R$ 780,00** |
| **Total com contingência (15–20%)** | Peças extras, retrabalho de PCB, imprevistos | **R$ 924,00** |

---

### 4.3 Esforço Estimado (Horas)

O esforço estimado será contabilizado considerando a quantidade de horas dedicadas por cada integrante da equipe às atividades relacionadas ao projeto. Serão 90h por aluno, contabilizando 60h dentro do horário de aula, mais cerca de 30h fora da sala de aula. Considerando atividades de pesquisa, planejamento, definição de requisitos, desenvolvimento, fabricação, programação, integração, testes e documentação.
Considerando os 18 integrantes da equipe, o esforço total estimado para o projeto é de aproximadamente 1.620 horas.

