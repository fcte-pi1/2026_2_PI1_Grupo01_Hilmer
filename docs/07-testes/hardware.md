# Testes de Hardware

## Testes de _hardware_

> Apresentar com detalhes os testes feitos para conferir se os objetivos do *hardware* foram atendidos, incluindo:
>
> 1. Processamento (Arduino, ESP32 etc):
>    - Definir interface de desenvolvimento;
>    - Rodar exemplo mínimo.
> 2. Sensores (temperatura, pressão, posição, velocidade etc.):
>    - Realizar leitura básica de dados;
>    - Validar/calibrar leituras;
>    - Definir taxas de amostragem e quantidade de dados para transmissão e/ou armazenamento;
>    - Validar taxas definidas e quantidade de dados;
>    - Realizar processamento básico de dados (*thresholding*, filtro média-móvel etc.).
> 3. Armazenamento (memória interna, cartão SD, pendrive etc.):
>    - Armazenar dados básicos;
>    - Armazenar e validar dados de sensores.
> 4. Atuadores (Motores DC, relés, LEDs etc.):
>    - Realizar acionamentos básicos;
>    - Realizar acionamentos com base em dados de sensores;
>    - Realizar processamento básico de dados (PWM etc.).
> 5. Comunicações (UART, I2C, SPI, USB, WiFi, Bluetooth etc.):
>    - Transmissão básica de dados;
>    - Transmissão de dados de sensores dentro da taxa de amostragem.

| **ID** | **Descrição** | **Data de Início** | **Data de Conclusão** | **Responsável** | **Resultados Esperados** | **Resultados Obtidos** | **Aprovação Interna (S/N)** | **Aprovação do Orientador (S/N)** | **Observações** |
|:------:|---------------|--------------------|-----------------------|-----------------|--------------------------|------------------------|:---------------------------:|:--------------------------------:|-----------------|
| 3.1 | Alimentação e Linhas de Tensão | | | | 3,3 V e VBAT estáveis | | | | Verificado em bancada |
| 3.2 | Comunicação LiDAR (UART) | | | | Recepção dos pacotes LD06 | | | | |
| 3.3 | Acionamento dos Motores (DRV8833) | 22/10/2026 | 22/10/2026 | Aline / Equipe | Resposta PWM nos canais A e B | Alimentação direta dos motores a 6 V realizada; integração do acionamento em protoboard com driver alimentado a 5 V não funcionou | | | Resposta PWM não comprovada; corrente com unidade pendente de confirmação. Ver procedimento 3.3. |
| 3.4 | IMU (I2C) | | | | Dados de aceleração e giro | | | | |
| 3.5 | **Encoders de Quadratura (18 cm)** | 16/10/2026 | 17/10/2026 | Cláudio / Equipe | Leitura x4 por interrupção; erro $\le \pm 3\%$ em 180 mm | Roteiro e script serial automatizados; aguardando bancada final | S | | Issue #186, HU-05 |
| 3.6 | | | | | | | | | |

---

## Procedimento de Teste 3.3: Motores e Circuito de Acionamento

**Data dos testes:** 22/10/2026.

### 1. Objetivo
Verificar a alimentação básica dos motores DC e testar a integração do circuito de acionamento montado conforme o esquemático do projeto.

### 2. Instrumentação

* **Fonte de bancada:** alimentação dos motores em 6 V, com indicação de tensão e corrente no visor.
* **Motores DC com redução:** componentes submetidos ao teste de alimentação direta e ao teste de integração.
* **Driver de motores:** alimentado em 5 V no teste de integração.
* **Protoboard e fios de conexão:** montagem do circuito de acionamento conforme o esquemático do projeto.

### 3. Montagem e Procedimento Realizado
1. Conectar os motores diretamente a uma fonte de bancada ajustada para **6 V**.
2. Aplicar a alimentação aos terminais dos motores e observar as indicações de tensão e corrente da fonte.
3. Montar o circuito de acionamento em protoboard conforme o esquemático.
4. Alimentar o driver com **5 V** e realizar o teste de integração para acionamento dos motores.

### 4. Resultados Obtidos

| Ensaio | Condição | Resultado |
|--------|----------|-----------|
| Alimentação direta dos motores | Fonte ajustada para 6 V | Aplicação de alimentação e consumo elétrico registrados. A rotação e o desempenho sob carga não foram quantificados. |
| Corrente dos motores | Leitura no visor da fonte | O relatório por componentes registra uma leitura pontual de 0,022 A (22 mA). Há também uma informação de 22 A, cuja unidade precisa ser confirmada antes de consolidar o resultado. |
| Integração do circuito de acionamento | Montagem em protoboard conforme o esquemático, com driver alimentado a 5 V | A integração não funcionou. A causa da falha não foi identificada. |

### 5. Avaliação
O ensaio com alimentação direta a 6 V documenta uma verificação elétrica preliminar dos motores. O teste de integração apresentou resultado insatisfatório e não comprovou a resposta PWM nos canais A e B exigida no item 3.3. A falha de integração, isoladamente, não identifica qual componente ou ligação causou o problema.

---

## Procedimento de Teste 3.5: Encoders e Deslocamento em Bancada (Issue #186)

### 1. Objetivo
Validar a decodificação em quadratura (x4) via interrupções nos pinos GPIO do ESP32-C3 e aferir a exatidão da conversão de pulsos em milímetros ao longo de um percurso de **18 cm (180 mm)** — correspondente à dimensão de uma célula do labirinto do Micromouse.

### 2. Instrumentação e Configuração
* **Hardware:** Microcontrolador ESP32-C3 com firmware carregado.
* **Sensores:** 2 encoders de efeito Hall acoplados aos micromotores N20.
  * Encoder Esquerdo: `GPIO0` (Canal A) e `GPIO1` (Canal B).
  * Encoder Direito: `GPIO3` (Canal A) e `GPIO10` (Canal B).
* **Guia de Medição:** Régua milimetrada calibrada de 300 mm ou paquímetro digital com batente fixo em 180,0 mm.
* **Ferramenta de Coleta:** Script Python `src/firmware/scripts/calibrar_deslocamento_18cm.py`.

### 3. Critérios de Aceitação
1. **Contagem por Interrupção:** Contagem bidirecional em ambas as rodas sem perda de pulsos e sem transições inválidas reportadas.
2. **Tolerância de Erro:**
   $$\text{Erro Relativo} = \frac{|D_{\text{medido}} - 180{,}0\text{ mm}|}{180{,}0\text{ mm}} \times 100\% \le 3{,}0\%$$
   $$\text{Erro Absoluto Máximo Permissível} = \pm 5{,}4\text{ mm}$$
3. **Calibração:** Determinação da constante de calibração $\mu\text{m/pulso}$ real:
   $$K = \frac{180 \times 1000}{\text{Pulsos}}$$

### 4. Roteiro de Execução
1. Conectar a placa ao computador via USB.
2. Posicionar o micromouse na marca de 0 mm da régua guia.
3. Executar o script de teste:
   ```bash
   python3 src/firmware/scripts/calibrar_deslocamento_18cm.py --porta /dev/ttyUSB0
   ```
4. Pressionar **ENTER** para enviar o comando de zerar (`Z`) e iniciar a amostragem.
5. Deslocar o robô manualmente de forma suave até a marca de 180,0 mm.
6. Finalizar a captura com **CTRL+C**.
7. O script registrará o relatório com os pulsos acumulados, as distâncias calculadas por roda, o erro relativo/absoluto e a nova constante recomendada para `ConfiguracaoEncoder.h`.

