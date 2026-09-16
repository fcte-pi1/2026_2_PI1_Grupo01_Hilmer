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
| 3 | **Sub-sistema: Hardware** | | | |
| 3.1 | Processamento | | | |
| 3.2 | Sensor 1 | | | |
| 3.3 | Sensor 2 | | | |
| 3.4 | Controle 1 | | | |
| 3.5 | Controle 2 | | | |
| 3.6 | Comunicação 1 | | | |
| 3.7 | Comunicação 2 | | | |
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
