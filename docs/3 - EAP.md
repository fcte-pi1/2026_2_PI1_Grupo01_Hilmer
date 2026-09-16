# Estrutura Analítica de Produto

| **ID** | **Componente** | **Descrição** | **Dados Técnicos** | **Comentários** |
|:------:|----------------|---------------|--------------------|-----------------|
| 1 | **Sub-sistema: Estrutura** | | | |
| 1.1 | Chassi | | | |
| 1.2 | Suporte | | | |
| 1.3 | Carenagem | | | |
| 1.4 | Atuadores | | | |
| 1.5 | Transmissão | | | |
| 1.6 | Rodas/Hélices | | | |
| 2 | **Sub-sistema: Fonte Energética** | | | |
| 2.1 | Alimentação | | | |
| 2.2 | Eletrônica de Potência | | | |
| 2.3 | Proteções | | | |
| 2.4 | Gerenciamento de Energia | | | |
| 3 | **Sub-sistema: Hardware** | Conjunto de componentes eletrônicos responsáveis pelo processamento, sensoriamento, acionamento, comunicação, alimentação e integração do Micromouse. | | |
| 3.1 | **Processamento** | Microcontrolador responsável por processar os dados provenientes dos sensores e executar as funções de controle do robô. | ESP32 / STM32 (em pesquisa) | Definir o microcontrolador após análise dos requisitos. |
| 3.2 | **Sensores de parede/distância** | Componentes responsáveis por detectar as paredes do labirinto e fornecer informações de distância utilizadas na percepção, localização e navegação do Micromouse. | Tipo, alcance, tensão e quantidade em definição | Definir modelo e quantidade dos sensores. |
| 3.3 | **Motores** | Atuadores responsáveis por gerar o movimento necessário para deslocar as rodas do Micromouse durante a navegação pelo labirinto. | Tensão, corrente, torque e RPM em definição | Selecionar conforme os requisitos de movimentação. |
| 3.4 | **Driver de motores** | Circuito responsável por receber os sinais de controle do microcontrolador e fornecer a potência necessária para o acionamento dos motores. | Tensão, corrente e canais em definição | Deve ser compatível com os motores e com o microcontrolador. |
| 3.5 | **Encoders** | Componentes responsáveis por medir a rotação das rodas, fornecendo informações utilizadas para determinar velocidade, deslocamento e auxiliar na odometria do robô. | Tipo, resolução e tensão em definição | Utilizados no controle dos motores e na localização. |
| 3.6 | **Comunicação** | Sistema responsável pela troca de informações entre os componentes eletrônicos e pela comunicação do Micromouse com sistemas externos de telemetria. | Protocolos em definição | Definir os protocolos e interfaces de comunicação. |
| 3.7 | **Alimentação e distribuição** | Sistema responsável por fornecer energia elétrica ao Micromouse e distribuir a alimentação adequada para os diferentes componentes eletrônicos. | Bateria, tensão e corrente em definição | Definir bateria e arquitetura de distribuição de energia. |
| 3.8 | **Reguladores de tensão** | Componentes responsáveis por converter ou regular a tensão fornecida pela fonte para os níveis de tensão necessários ao funcionamento dos circuitos eletrônicos. | Tensões de entrada e saída em definição | Definir os reguladores de acordo com os componentes selecionados. |
| 3.9 | **Proteções elétricas** | Conjunto de componentes e medidas destinadas a proteger o circuito contra condições elétricas que possam causar danos aos componentes ou interromper o funcionamento do sistema. | Em definição | Definir as proteções necessárias após o dimensionamento da alimentação. |
| 3.10 | **Conectores e cabeamento** | Componentes responsáveis por realizar as conexões elétricas entre os módulos, permitindo uma montagem organizada, segura e confiável do sistema. | Tipos, bitolas e quantidade em definição | Definir conectores e cabeamento conforme a montagem final. |
| 3.11 | **Protótipo e montagem** | Etapa destinada à montagem experimental dos componentes eletrônicos para verificar as conexões e validar o funcionamento do circuito antes da fabricação da PCB. | Protoboard de teste | Utilizar o protótipo para identificar problemas antes da PCB definitiva. |
| 3.12 | **PCB** | Placa de circuito impresso destinada a organizar, interligar e fixar os componentes eletrônicos do Micromouse em uma solução integrada. | Dimensões, número de camadas e layout em definição | Desenvolver após a validação do circuito em protótipo. |
| 3.13 | **Esquemático elétrico** | Representação das conexões elétricas entre alimentação, microcontrolador, sensores, driver, motores e encoders, servindo como referência para montagem e desenvolvimento da PCB. | Em desenvolvimento | Deve contemplar todos os componentes e conexões do sistema. |
| 3.14 | **Integração dos componentes** | Etapa responsável por conectar e integrar os componentes eletrônicos para formar o sistema de Hardware completo do Micromouse. | Em desenvolvimento | Sensores → MCU; Encoders → MCU; MCU → Driver; Driver → Motores; Alimentação → Sistema. |
| 3.15 | **Testes de Hardware** | Conjunto de testes realizados individualmente e de forma integrada para verificar o funcionamento dos componentes e do circuito eletrônico. | Critérios de teste em definição | Inclui testes do MCU, sensores, motores, driver, encoders, alimentação e sistema integrado. |
| 3.16 | **Validação do Hardware** | Etapa final destinada a verificar se o Hardware atende aos requisitos definidos para o projeto e está preparado para integração com o Software. | Critérios de validação em definição | Identificar e corrigir falhas e validar o funcionamento do Hardware. |
| 4 | **Releases de Software** | | | |
| 4.1 | Módulo de Percepção e Mapeamento | | | |
| 4.1.1 | Reconhecimento de ambiente | Identificar os diferentes elementos físicos presentes no labirinto, como paredes e piso. Essa etapa de codificação ficará sob sua responsabilidade direta para garantir a correta percepção do espaço. | | |
| 4.1.2 | Construção do mapa | Construir o mapa quadriculado do labirinto baseado no percurso feito pelo micromouse. Essa tarefa também comporá as suas entregas pessoais no desenvolvimento do projeto. | | |
| 4.1.3 | Armazenamento local | Armazenar o mapa quadriculado do labirinto na memória. | | |
| 4.2 | Módulo de Navegação e Controle Autônomo | | | |
| 4.2.1 | Odometria e Localização | Identificar a posição atual do micromouse no labirinto. | | |
| 4.2.2 | Algoritmo de Resolução | Tomar decisões de forma unicamente autônoma para percorrer o labirinto da posição inicial à final. | | |
| 4.2.3 | Detecção de Sucesso | Identificar quando o robô alcançou a célula de destino no canto oposto ao início. | | |
| 4.2.4 | Recuperação de falhas | Recuperar o sistema de possíveis colisões, retomando a exploração sem perder o mapeamento e a rota já capturada. | | |
| 4.3 | Módulo de Telemetria e Dashboard | | | |
| 4.3.1 | Captura e Transmissão | Exibir dados de telemetria em tempo real. A atualização na interface web deve ocorrer em até 2 segundos após a leitura do sensor. | | |
| 4.3.2 | Estabilização de Conexão | Implementar um buffer para garantir que não haverá perda de dados em caso de interrupções na conexão. | | |
| 4.3.3 | Persistência de Dados e Corridas | Armazenar dados de telemetria e registrar cada tentativa de resolução no banco de dados com um identificador único, permitindo filtragem pelo tipo de labirinto. | | |
| 4.3.4 | Interface Visual (Dashboard) | Renderizar uma interface contendo o mapa bidimensional, trajeto, velocidade média e consumo de bateria. Também deve exibir o percurso feito e os dados de todos os labirintos já percorridos. | | |
| 4.4 | Módulo de Inicialização | | | |
| 4.4.1 | Boot do Sistema | O software deve inicializar e estar pronto para o desafio em até 2 segundos. | | |
| 4.4.2 | Calibração de Hardware | Executar uma rotina de autocalibração dos sensores mecânicos, inerciais e de distância em até 1 segundo antes de iniciar o deslocamento. | | |
