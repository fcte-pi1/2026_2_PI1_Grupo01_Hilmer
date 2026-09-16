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
| **3** | **SUB-SISTEMA: HARDWARE** | | |
| 3.1 | **Seleção de componentes** | | |
| 3.1.1 | Microcontrolador (ESP32 / STM32) | | |
| 3.1.2 | Sensores de parede/distância | | |
| 3.1.3 | Motores | | |
| 3.1.4 | Driver de motores | | |
| 3.1.5 | Encoders | | |
| 3.1.6 | Conectores e componentes auxiliares | | |
| 3.1.7 | Sistema de alimentação | | |
| 3.1.8 | Reguladores de tensão | | |
| 3.1.9 | Definição das tensões de operação | | |
| 3.1.10 | Definição dos protocolos de comunicação | | |
| 3.2 | **Esquemático elétrico** | | |
| 3.2.1 | Alimentação e distribuição | | |
| 3.2.2 | Microcontrolador | | |
| 3.2.3 | Sensores | | |
| 3.2.4 | Driver de motores | | |
| 3.2.5 | Motores | | |
| 3.2.6 | Encoders | | |
| 3.2.7 | Reguladores de tensão | | |
| 3.2.8 | Proteções elétricas | | |
| 3.2.9 | Conectores e interfaces | | |
| 3.2.10 | Comunicação / telemetria | | |
| 3.3 | **Projeto e montagem do circuito** | | |
| 3.3.1 | Montagem do protótipo | | |
| 3.3.2 | Protoboard de teste | | |
| 3.3.3 | Projeto da PCB | | |
| 3.3.4 | Layout da PCB | | |
| 3.3.5 | Dimensionamento das trilhas | | |
| 3.3.6 | Soldagem | | |
| 3.3.7 | Cabeamento | | |
| 3.3.8 | Conectores | | |
| 3.3.9 | Inspeção da montagem | | |
| 3.4 | **Integração dos componentes** | | |
| 3.4.1 | Sensores → MCU | | |
| 3.4.2 | Encoders → MCU | | |
| 3.4.3 | MCU → Driver | | |
| 3.4.4 | Driver → Motores | | |
| 3.4.5 | Alimentação → Sistema | | |
| 3.4.6 | Comunicação → MCU | | |
| 3.4.7 | Integração da telemetria | | |
| 3.4.8 | Integração elétrica completa | | |
| 3.5 | **Testes de Hardware** | | |
| 3.5.1 | Teste do microcontrolador | | |
| 3.5.2 | Teste dos sensores | | |
| 3.5.3 | Teste dos motores | | |
| 3.5.4 | Teste do driver | | |
| 3.5.5 | Teste dos encoders | | |
| 3.5.6 | Teste de alimentação | | |
| 3.5.7 | Teste dos reguladores | | |
| 3.5.8 | Teste de comunicação | | |
| 3.5.9 | Teste da PCB | | |
| 3.5.10 | Teste integrado | | |
| 3.6 | **Validação do Hardware** | | |
| 3.6.1 | Verificação dos requisitos | | |
| 3.6.2 | Verificação das tensões | | |
| 3.6.3 | Verificação dos sinais | | |
| 3.6.4 | Identificação de falhas | | |
| 3.6.5 | Correção de falhas | | |
| 3.6.6 | Validação do funcionamento | | |
| 3.6.7 | Documentação dos resultados | | |
| 3.6.8 | Aprovação do Hardware para integração com Software | | |
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
