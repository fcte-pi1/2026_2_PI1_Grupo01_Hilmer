# Telemetria por Wi-Fi: rede da demonstração, rede reserva e reenvio

Este documento registra o ensaio em bancada da telemetria com Wi-Fi real: a escolha da rede da demonstração e da rede reserva ([#491](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/491)), a troca automática entre as duas, o reenvio com queda real do Wi-Fi e o comportamento do buffer sem rede e cheio ([#478](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/issues/478)). As contagens de cada corrida foram conferidas no banco do Supabase de testes.

| Item | Valor |
| :--- | :--- |
| **Subsistemas** | Software (firmware de telemetria e backend Supabase) |
| **Data da execução** | 10/11/2026 |
| **Responsável** | Cláudio Henrique |
| **Placa** | ESP32-S3 avulsa, com o firmware de bancada `bancada_geral` (submenu de telemetria, tecla `w`), sem sensores ligados |
| **Rede principal** | Hotspot de um celular Android, SSID `Teste_Esp`, 2,4 GHz |
| **Rede reserva** | Acesso Pessoal de um iPhone, SSID `iPhone`, com **Maximizar Compatibilidade** ligado (2,4 GHz) |
| **Resultado global** | **Aprovado.** A placa troca sozinha para a reserva quando a principal some, volta para a principal quando a reserva cai, guarda a telemetria no buffer sem rede e envia tudo ao reconectar, sem perder nem duplicar amostras; com o buffer cheio, descarta as mais antigas |

---

## 1. Por que hotspot de celular

As redes do campus não servem para o robô. O ESP32 não faz login em portal cativo (página de login no navegador) nem autenticação WPA2 corporativa (usuário e senha, como na `eduroam`), e só enxerga redes de 2,4 GHz. Na varredura feita no laboratório, a placa viu **41 redes de 2,4 GHz**, a maioria nos canais 1, 6 e 11 (`UNB Wireless`, `eduroam` e redes de laboratórios). Por isso a telemetria usa o hotspot de um celular, com um segundo celular de reserva.

**Decisão.** A rede principal da demonstração é o hotspot Android (`Teste_Esp`) e a reserva é o iPhone. O iPhone fica como reserva porque o iOS deixa de anunciar o Acesso Pessoal quando não há cliente conectado, a menos que a tela do Acesso Pessoal esteja aberta. A troca de rede no dia segue o roteiro da seção "Rede da demonstração e plano B" do README do firmware (`src/firmware/README.md`).

## 2. Como a placa escolhe a rede

O firmware tenta a principal primeiro. Uma tentativa que não conecta em 10 s passa para a reserva na hora; só depois de as duas falharem seguidas a placa espera 1 s, 2 s, 4 s... até 30 s e recomeça pela principal. Quando a conexão cai, ela tenta primeiro a mesma rede e, se ela não voltar em 10 s, a outra. Conectada na reserva, ela fica nela mesmo que a principal volte, para não derrubar uma conexão boa. Sem nenhuma rede, as amostras ficam no buffer (600 amostras, 10 minutos a 1 por segundo) e são enviadas em ordem quando a rede volta.

## 3. Procedimento e resultados

Cada teste usa uma tecla do submenu de telemetria da bancada geral (descrição completa no [guia da bancada geral](../guia-bancada-geral.md)). Os testes de reenvio usam o mesmo `ControladorEnvio`, a mesma `ReconexaoWifi` e o mesmo cliente HTTPS do robô.

| # | Teste (tecla) | Procedimento | Resultado obtido | Situação |
| :---: | :--- | :--- | :--- | :---: |
| 1 | Fluxo completo (`a`) | Wi-Fi, DNS, criar corrida, enviar lote, reenviar o lote e encerrar, na rede principal | 5 etapas OK (corrida 12): criar a corrida em 2,1 s, lote de 5 amostras em 1,0 s, reenvio com `gravadas 0, ja existentes 5`, encerramento em 0,6 s | Aprovado |
| 2 | Varredura (`v`) | Lista as redes visíveis e confere os SSIDs do `credenciais.h` | Mostrou o hotspot como `iPhone`, e não `Iphone` como estava configurado: o SSID diferencia maiúsculas. Corrigido o `credenciais.h`, a reserva conectou | Aprovado |
| 3 | Troca para a reserva (`w`) | Principal desligada, reserva ligada | 10 s tentando a principal; conectou na reserva 1 s depois (sinal de -22 dBm) | Aprovado |
| 4 | Volta para a principal (`m`) | Conectada na reserva: liga a principal, depois desliga a reserva | Com a principal de volta, a placa ficou na reserva (197 s). Com a reserva desligada, ficou 12 s sem conexão e conectou na principal (-64 dBm) | Aprovado |
| 5 | Reenvio com queda real (`b`) | Os dois hotspots desligados por cerca de 30 s com a corrida em andamento | Corrida 16: 39 s sem conexão, até 43 amostras no buffer; ao voltar na principal, o buffer esvaziou em cerca de 3 s. 239 geradas, 238 confirmadas, 1 já existente (lote reenviado depois de uma resposta perdida), nenhuma perdida | Aprovado |
| 6 | Corrida inteira sem rede (`n`) | Corrida criada, Wi-Fi desligado pela bancada, 600 amostras no buffer, Wi-Fi religado | Corrida 17: 600 amostras no buffer, nenhuma descartada; todas enviadas em 6 s depois de religar, e a corrida encerrada | Aprovado |
| 7 | Buffer cheio (`x`) | O mesmo, com 650 amostras | Corrida 18: o buffer descartou as 50 mais antigas (`perdidas no buffer 50`) e enviou as 600 restantes em 9 s | Aprovado |

**Sinal medido na bancada (tecla `m`).** Na principal, de -79 a -53 dBm (média de -64 dBm), com o celular a alguns metros da placa; na reserva, de -31 a -13 dBm, com o celular ao lado da placa. Esses valores valem para esta placa e esta posição dos celulares: a antena e o rádio da ESP32-S3 não são os da placa do robô.

## 4. Conferência no banco

As contagens de cada corrida foram lidas da tabela `telemetria` do Supabase de testes, pela API REST com a chave `anon` (só leitura). Em todas, o número de linhas é igual ao de sequências distintas (nenhuma duplicata) e as sequências vão do mínimo ao máximo sem buracos.

| Corrida | Teste | Linhas | `numero_sequencia` distintos | Mínimo | Máximo | Esperado |
| :---: | :--- | :---: | :---: | :---: | :---: | :---: |
| 13 | `b`, sem queda | 60 | 60 | 1 | 60 | 60, 60, 1, 60 |
| 14 | `b`, sem queda | 90 | 90 | 1 | 90 | 90, 90, 1, 90 |
| 15 | `b`, sem queda | 300 | 300 | 1 | 300 | 300, 300, 1, 300 |
| 16 | `b`, queda de 39 s | 239 | 239 | 1 | 239 | 239, 239, 1, 239 |
| 17 | `n`, sem rede | 600 | 600 | 1 | 600 | 600, 600, 1, 600 |
| 18 | `x`, buffer cheio | 600 | 600 | 51 | 650 | 600, 600, 51, 650 |

As corridas 13, 14 e 15 são tentativas do teste 5 em que os hotspots não foram desligados a tempo. Elas valem como corridas sem perda, mas não como teste de queda.

Linhas `REGISTRO` impressas pela bancada nos testes aprovados:

```text
REGISTRO: reenvio | corrida=16 geradas=239 confirmadas=238 existentes=1 descartadas=0 perdidas=0 reenvios=2 quedas=1 trocas=3 rede_final=principal | veredito=OK
REGISTRO: sem_rede | corrida=17 geradas=600 confirmadas=600 existentes=0 descartadas=0 perdidas=0 esperado_perdidas=0 quedas=1 rede_final=principal envio_s=6 | veredito=OK
REGISTRO: sem_rede | corrida=18 geradas=650 confirmadas=600 existentes=0 descartadas=0 perdidas=50 esperado_perdidas=50 quedas=1 rede_final=principal envio_s=9 | veredito=OK
```

## 5. Observações

1. **Primeira chamada depois de ligar.** Na primeira execução do fluxo completo, a criação da corrida passou do tempo limite de 5 s (`read Timeout`); repetida logo depois, levou 2,1 s. A Edge Function estava parada e demorou a responder na primeira chamada. No robô, a criação da corrida é tentada de novo com espera crescente, então isso só atrasa o início do envio.
2. **Primeira chamada depois de reconectar.** Nos testes 6 e 7, o primeiro envio depois de religar o Wi-Fi falhou com erro de TLS (`-76`), porque o cliente HTTPS tentou usar a conexão aberta antes da queda; o reenvio seguinte passou. A falha conta em `falhas` e não perde amostras.
3. **Laço da bancada durante a queda.** No teste 5, a chamada HTTPS em andamento quando a rede caiu (DNS falhando e tempo limite de 5 s) segurou o laço da bancada por cerca de 13 s, e a geração de amostras atrasou e depois compensou. No robô isso não acontece: o envio roda numa tarefa separada da coleta.
4. **Correção na bancada.** Durante o ensaio, o teste `b` aprovava a queda pelas quedas da sessão inteira (inclusive de testes anteriores). Ele passou a contar só as quedas do próprio teste, medir o tempo sem conexão e esperar a queda e a volta do Wi-Fi para terminar.
5. **Escopo.** Este ensaio foi feito em bancada, com uma ESP32-S3 avulsa. Não mede o sinal na pista com o robô em movimento.
