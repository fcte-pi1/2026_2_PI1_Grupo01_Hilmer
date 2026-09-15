**Requisitos Funcionais (RF)**

| ID | Nome do Requisito | Descrição | Prioridade | Responsáveis | Link Github Projects |
| :---: | :---- | :---- | :---: | :---- | :---- |
| 1 | Navegação pelo labirinto | O micromouse deve ser capaz de percorrer o labirinto da posição inicial à final. | Must have | Paulo Henrique | https://github.com/fcte-pi1/2026\_2\_PI1\_Grupo01\_Hilmer/issues/3 |
| 2 | Reconhecimento de ambiente | O sistema deve ser capaz de identificar os diferentes elementos físicos presentes no labirinto (paredes, piso). | Must have | Daniel Rodrigues | https://github.com/fcte-pi1/2026\_2\_PI1\_Grupo01\_Hilmer/issues/5 |
| 3 | Construção do mapa do labirinto | O sistema deve ser capaz de construir o mapa quadriculado do labirinto baseado no percurso feito pelo micromouse. | Must have | Daniel Rodrigues&nbsp; | https://github.com/fcte-pi1/2026\_2\_PI1\_Grupo01\_Hilmer/issues/6 |
| 4 | Armazenamento do mapa labirinto | O sistema deve ser capaz de armazenar o mapa quadriculado do labirinto. | Must have | Paulo Henrique | https://github.com/fcte-pi1/2026\_2\_PI1\_Grupo01\_Hilmer/issues/7 |
| 5 | Localização do robô | O sistema deve ser capaz de identificar a posição atual do micromouse no labirinto (odometria). | Must have | Ana Carolina | https://github.com/fcte-pi1/2026\_2\_PI1\_Grupo01\_Hilmer/issues/8 |
| 6 | Percurso do robô | O sistema deve ser capaz de exibir o percurso feito pelo micromouse no labirinto. | Should have | Cláudio Henrique | https://github.com/fcte-pi1/2026\_2\_PI1\_Grupo01\_Hilmer/issues/9 |
| 7 | Detecção do objetivo | O sistema deve ser capaz de identificar quando o robô alcançou a célula de destino (canto oposto ao início). | Must have | Eduardo Viana | https://github.com/fcte-pi1/2026\_2\_PI1\_Grupo01\_Hilmer/issues/10 |
| 8 | Telemetria em tempo real | O sistema deve ser capaz de exibir, em tempo real, os dados de telemetria do micromouse. | Must have | Marllon Fausto | https://github.com/fcte-pi1/2026\_2\_PI1\_Grupo01\_Hilmer/issues/11 |
| 9 | Armazenamento em banco de dados | O sistema deve ser capaz de armazenar os dados de telemetria para consulta posterior. | Must have | Cláudio Henrique | https://github.com/fcte-pi1/2026\_2\_PI1\_Grupo01\_Hilmer/issues/12 |
| 10 | Consulta de todos os labirintos | O sistema deve ser capaz de exibir os dados de todos os labirintos percorridos. | Should have | Aline Rodrigues&nbsp; | https://github.com/fcte-pi1/2026\_2\_PI1\_Grupo01\_Hilmer/issues/13 |
| 11 | Sobrepujar condições do solo | O sistema deve ser capaz de superar obstáculos irregulares no solo ao percorrer o labirinto. | Should have | Giovanna Santos, Eduardo Gaspar | https://github.com/fcte-pi1/2026\_2\_PI1\_Grupo01\_Hilmer/issues/14 |
| 12 | Tratamento de colisões | O sistema deve ser capaz de evitar colisões entre o micromouse e as paredes do labirinto | Must have | Giovanna Santos, Eduardo Gaspar | https://github.com/fcte-pi1/2026\_2\_PI1\_Grupo01\_Hilmer/issues/15 |
| 13 | Calibração de Hardware | O sistema deve executar uma rotina de auto calibração dos sensores de distância e inerciais antes de iniciar o deslocamento na pista. | Must have | Elias | https://github.com/fcte-pi1/2026\_2\_PI1\_Grupo01\_Hilmer/issues/16 |
| 14 | Dashboard de Telemetria | O sistema web deve renderizar uma interface visual contendo o mapa bidimensional, trajeto, velocidade média e consumo de bateria. | Must have | Bruno Duarte, Rafael Lima | https://github.com/fcte-pi1/2026\_2\_PI1\_Grupo01\_Hilmer/issues/17 |
| 15 | Gestão de Corridas | O sistema deve registrar cada tentativa de resolução no banco de dados com um identificador único, garantindo a filtragem por tipo de labirinto (4x4, 8x4, 12x4). | Must have | Cláudio Henrique | https://github.com/fcte-pi1/2026\_2\_PI1\_Grupo01\_Hilmer/issues/18 |
| 16 | Estabilização de telemetria | O sistema deve possuir um buffer para garantir que não irá perder os dados caso haja interrupções na conexão. | Must have | Elias | https://github.com/fcte-pi1/2026\_2\_PI1\_Grupo01\_Hilmer/issues/28 |
| 17 | Recuperação de Impacto | O sistema deve se recuperar de possíveis colisões, retornando a exploração do labirinto sem perder o mapeamento dos caminhos e de toda a rota que já capturou | Should have | Elias | https://github.com/fcte-pi1/2026\_2\_PI1\_Grupo01\_Hilmer/issues/29 |

&nbsp;

## **Requisitos Não-Funcionais (RNF)**

| ID | Nome do Requisito | Descrição | Prioridade | Responsáveis | Link Github Projects |
| :---: | :---- | :---- | :---: | :---- | :---- |
| 1 | Dimensão máxima | O robô não deve exceder 16,5 cm de comprimento ou largura, sem restrição de altura. | Must have | Ágata, João Artur | https://github.com/fcte-pi1/2026\_2\_PI1\_Grupo01\_Hilmer/issues/19 |
| 2 | Modos de locomoção proibidos | O sistema não deve percorrer o labirinto utilizando voo, salto, escalada ou propulsão por combustão/foguete. | Must have | Aline Rodrigues&nbsp; | https://github.com/fcte-pi1/2026\_2\_PI1\_Grupo01\_Hilmer/issues/20 |
| 3 | Tempo limite de execução | O robô deve concluir cada labirinto em até 10 minutos. | Must have | Ana Carolina | https://github.com/fcte-pi1/2026\_2\_PI1\_Grupo01\_Hilmer/issues/21 |
| 4 | Autonomia de bateria | O micromouse deve possuir bateria suficiente para completar os 3 labirintos consecutivamente. | Should have | Paulo Henrique | https://github.com/fcte-pi1/2026\_2\_PI1\_Grupo01\_Hilmer/issues/22 |
| 5 | Latência da telemetria | O sistema web deve atualizar os dados de telemetria exibidos em até 2 segundos após a leitura do sensor correspondente. | Should have | Eduardo Viana | https://github.com/fcte-pi1/2026\_2\_PI1\_Grupo01\_Hilmer/issues/23 |
| 6 | Resistência Mecânica | A estrutura do micromouse deve suportar os diferentes esforços e colisões sem prejudicar seu funcionamento. |  | Ana Carolina Nunes, Letícia | https://github.com/fcte-pi1/2026\_2\_PI1\_Grupo01\_Hilmer/issues/24 |
| 7 | Autonomia do sistema | O micromouse deve tomar decisões e agir de forma unicamente autônoma para percorrer o labirinto do início ao fim. | Must have | Bruno Duarte | https://github.com/fcte-pi1/2026\_2\_PI1\_Grupo01\_Hilmer/issues/25 |
| 8 | Inicialização de Hardware | O sistema dever capaz de se auto-calibrar seus componentes mecânicos em até 1 segundo. | Must have | Elias | https://github.com/fcte-pi1/2026\_2\_PI1\_Grupo01\_Hilmer/issues/26 |
| 9 | Inicialização de software | O sistema deve inicializar e estar pronto para iniciar o desafio em até 2 seg | Should Have | Rafael Lima | https://github.com/fcte-pi1/2026\_2\_PI1\_Grupo01\_Hilmer/issues/27 |

&nbsp;