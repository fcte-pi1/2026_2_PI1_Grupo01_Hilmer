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
| 3.1 | **Seleção de Componentes** | | | |
| 3.1.1 | Microcontrolador (MCU) | Processar os dados dos sensores e executar o controle do robô. | | |
| 3.1.2 | Sensores de parede/distância | Detectar paredes e fornecer informações para localização e navegação. | | |
| 3.1.3 | Motores | Movimentar as rodas do robô. | | |
| 3.1.4 | Driver de motores | Fornecer a interface de potência para acionamento dos motores. | | |
| 3.1.5 | Encoders | Medir a rotação das rodas e auxiliar no controle de velocidade e deslocamento. | | |
| 3.1.6 | Conectores e componentes auxiliares | Realizar conexões elétricas de forma organizada e confiável. | | |
| 3.2 | **Esquemático Elétrico** | | | |
| 3.2.1 | Alimentação e distribuição | Mapear o circuito para alimentar o sistema e distribuir a energia entre os componentes. | | |
| 3.2.2 | Microcontrolador | Projetar as conexões de pinos, alimentação e interfaces da MCU. | | |
| 3.2.3 | Sensores | Projetar o esquemático das linhas de sinal e alimentação dos sensores de detecção de parede. | | |
| 3.2.4 | Driver de motores e Motores | Projetar as linhas lógicas de controle e potência entre MCU, driver e motores. | | |
| 3.2.5 | Encoders | Projetar as conexões dos sinais de leitura das rodas com as entradas da MCU. | | |
| 3.3 | **Projeto e Montagem do Circuito** | | | |
| 3.3.1 | Montagem do protótipo | Realizar montagem experimental em bancada para validação preliminar do circuito. | | |
| 3.3.2 | Projeto da PCB | Organizar e interligar os circuitos e componentes eletrônicos. | | |
| 3.3.3 | Soldagem | Fixar e conectar fisicamente os componentes à PCB fabricada. | | |
| 3.3.4 | Cabeamento e conectores | Realizar conexões elétricas de forma organizada e confiável. | | |
| 3.4 | **Integração dos Componentes** | | | |
| 3.4.1 | Sensores → MCU | Integrar as linhas de sinal dos sensores às entradas da MCU. | | |
| 3.4.2 | Encoders → MCU | Integrar as saídas de quadratura dos encoders às entradas da MCU. | | |
| 3.4.3 | MCU → Driver | Conectar as saídas de controle da MCU às entradas lógicas do driver. | | |
| 3.4.4 | Driver → Motores | Conectar as saídas de potência do driver aos terminais dos motores. | | |
| 3.4.5 | Alimentação → Sistema | Conectar e distribuir as linhas de alimentação para todos os módulos. | | |
| 3.5 | **Testes de Hardware** | | | |
| 3.5.1 | Teste do microcontrolador | Verificar o funcionamento de clock, gravação de firmware e processamento da MCU. | | |
| 3.5.2 | Teste dos sensores | Verificar a resposta de detecção e a precisão da leitura de distâncias. | | |
| 3.5.3 | Teste dos motores | Verificar a movimentação e rotação dos motores em vazio e com carga. | | |
| 3.5.4 | Teste do driver | Verificar o envio de sinais de potência e chaveamento dos canais. | | |
| 3.5.5 | Teste dos encoders | Verificar a leitura correta de rotação, pulsos e sentido de giro. | | |
| 3.5.6 | Teste de alimentação | Medir estabilidade, tensões reguladas e capacidade de corrente. | | |
| 3.5.7 | Teste integrado | Verificar a operação simultânea de todos os módulos eletrônicos interligados. | | |
| 3.6 | **Validação do Hardware** | | | |
| 3.6.1 | Verificação dos requisitos | Avaliar o atendimento a todas as especificações técnicas estipuladas. | | |
| 3.6.2 | Identificação e correção de falhas | Isolar anomalias operacionais e implementar correções no circuito. | | |
| 3.6.3 | Validação do funcionamento | Homologar o funcionamento geral do conjunto eletrônico montado. | | |
| 3.6.4 | Aprovação para integração com o software | Liberar formalmente a plataforma física para receber o firmware e algoritmos de controle. | | |
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
