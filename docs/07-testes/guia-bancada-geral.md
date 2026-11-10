# Guia da bancada geral

A **bancada geral** é um programa para o ESP32 DevKit V1, a placa do robô, que mostra um menu no terminal do computador. Com a placa ligada por um cabo USB, você aperta uma tecla e o programa lê os sensores, aciona os motores, confere os encoders, a bateria e o botão, e testa a telemetria, usando os mesmos drivers do robô. Serve para testar os componentes sem escrever código e para investigar o que deu errado.

Este guia funciona em **Windows, macOS e Linux** e não exige conhecimento prévio do firmware.

!!! warning "Antes de acionar qualquer motor"
    Leia o [checklist de segurança](#4-checklist-de-seguranca-antes-de-acionar-motores). Na primeira vez, deixe as **rodas fora do chão** e confira a **polaridade da fonte** com o multímetro.

## 1. Para que serve

| Quero... | Use |
|---|---|
| Saber se os sensores e os encoders estão respondendo | Tecla `t` (autoteste) |
| Ver quais dispositivos aparecem no barramento I²C | Tecla `1` |
| Ler as distâncias dos 3 sensores VL53L0X ou o giro e a aceleração do MPU6050 | Teclas `3` a `6` |
| Girar um motor, achar a potência em que ele começa a andar, ver a velocidade ao vivo | Tecla `7` (menu de motores) |
| Conferir se o encoder conta para o lado certo | Tecla `9` e os testes de motor |
| Ligar os dois motores e ver se os sensores aguentam o ruído e a bateria aguenta a corrente | Tecla `i` (teste integrado) |
| Ver a tensão da bateria e se o botão de largada funciona | Teclas `a` e `b` |
| Conferir se a placa conecta no Wi-Fi e grava a telemetria no banco | Tecla `w` |

A bancada **não é o firmware do robô**. Ela não faz a corrida, só testa componentes. Para voltar ao robô, grave o ambiente dele: `pio run -e esp32_devkit -t upload`.

## 2. O que você precisa

| Item | Detalhe |
|---|---|
| Placa | **ESP32 DevKit V1**, com a fiação da bancada integrada (ambiente `bancada_geral_devkit`) |
| Cabo | USB **com dados** (micro-USB ou USB-C, conforme a placa). Cabos só de carga não funcionam: a placa liga, mas o computador não vê a porta |
| Computador | Windows 10/11, macOS ou Linux |
| Software | Git, Python 3 e o PlatformIO Core (instalação abaixo) |
| Para motores | Fonte ou bateria de 5 a 6 V no driver, com limite de corrente (de 0,5 A para cima), e o robô montado ou o motor ligado ao driver |
| Para a telemetria (tecla `w`) | Rede Wi-Fi de **2,4 GHz** (um hotspot de celular serve) e o arquivo `include/credenciais.h` preenchido (seção 5) |

## 3. Instalação (uma vez só)

Os comandos abaixo são para o terminal: **PowerShell** no Windows, **Terminal** no macOS e no Linux.

=== "Windows"

    1. Instale o **Git** (git-scm.com) e o **Python 3** (python.org). No instalador do Python, marque **Add python.exe to PATH**.
    2. Instale o PlatformIO:

        ```powershell
        py -m pip install -U platformio==6.2.0
        ```

    3. Confira (feche e abra o PowerShell antes):

        ```powershell
        pio --version
        ```

        Se aparecer "`pio` não é reconhecido", use `py -m platformio` no lugar de `pio` nos comandos deste guia.

    4. **Driver USB:** a DevKit tem um chip conversor USB-serial (leia o nome no chip da placa: **CP2102/CP210x** ou **CH340/CH341**). O Windows 10 e o 11 costumam instalar sozinhos; se a placa aparecer em *Gerenciador de Dispositivos > Outros dispositivos* ou com triângulo amarelo, instale o driver do fabricante do chip (Silicon Labs para o CP210x, WCH para o CH340). Ela aparece em *Portas (COM e LPT)* como `COMn`.

=== "macOS"

    1. Instale o **Homebrew** (brew.sh), o Git e o Python:

        ```bash
        brew install git python
        ```

    2. Instale o PlatformIO:

        ```bash
        python3 -m pip install -U platformio==6.2.0
        ```

        Se o `pip` recusar com *externally-managed-environment*, use `brew install platformio` ou crie um ambiente virtual (`python3 -m venv ~/.pio-venv` e `source ~/.pio-venv/bin/activate`).

    3. Confira:

        ```bash
        pio --version
        ```

    4. As versões recentes do macOS reconhecem a maioria das DevKit, que aparecem como `/dev/cu.usbserial-XXXX` (ou `/dev/cu.wchusbserial-XXXX`, com o CH340). Se a porta não aparecer, instale o driver do chip (CP210x da Silicon Labs ou CH340 da WCH).

=== "Linux"

    1. Instale o Git e o Python (exemplo para Ubuntu e Debian):

        ```bash
        sudo apt install git python3 python3-pip python3-venv
        ```

    2. Instale o PlatformIO:

        ```bash
        python3 -m pip install --user -U platformio==6.2.0
        ```

        Se o `pip` recusar com *externally-managed-environment*, use um ambiente virtual (`python3 -m venv ~/.pio-venv` e `source ~/.pio-venv/bin/activate`) ou `pipx install platformio`.

    3. **Permissão da porta serial** (sem isso aparece *Permission denied*):

        ```bash
        sudo usermod -a -G dialout $USER
        curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core/develop/platformio/assets/system/99-platformio-udev.rules | sudo tee /etc/udev/rules.d/99-platformio-udev.rules
        sudo udevadm control --reload-rules && sudo udevadm trigger
        ```

        Depois, **saia da sessão e entre de novo** (ou reinicie). No Arch e no Manjaro o grupo é `uucp`, em vez de `dialout`.

    4. Confira com `pio --version`. A DevKit aparece como `/dev/ttyUSB0`. Se a porta abrir e sumir, o ModemManager pode estar interferindo: `sudo systemctl stop ModemManager`.

!!! tip "Prefere o VS Code?"
    Instale a extensão **PlatformIO IDE**, abra a pasta `src/firmware`, escolha o ambiente `bancada_geral_devkit` na barra inferior e use os botões **Upload** e **Monitor**. Os comandos de terminal deste guia fazem a mesma coisa.

## 4. Checklist de segurança antes de acionar motores

Só vale para quem vai usar o menu de motores (tecla `7`). Os sensores e o autoteste não oferecem esse risco.

| # | Confira | Por quê |
|:-:|---|---|
| 1 | **Polaridade da fonte** com o multímetro: o fio que você chama de `+` deve medir tensão positiva | Polaridade invertida queima o driver DRV8833 e faz a fonte desarmar |
| 2 | VM (alimentação do driver) entre **5 e 6 V**, nunca acima de 10,8 V | Acima disso o DRV8833 queima |
| 3 | `GND` da fonte, do driver e da ESP32 ligados juntos | Sem terra comum, o sinal PWM não é entendido |
| 4 | `nSLEEP` do driver em **3,3 V** | Em nível baixo, o driver fica desligado |
| 5 | Encoder alimentado em **3,3 V** (nunca nos 5 V da fonte) | Os canais A e B vão direto aos GPIOs, que aceitam no máximo 3,3 V |
| 6 | **Rodas fora do chão** no primeiro teste | O robô pode sair andando. O pino do `IN4` (GPIO14, motor direito) dá um pulso no reset do ESP32, e a roda direita pode dar um tranco ao ligar ou reiniciar |
| 7 | Fonte com limite de corrente em 0,5 A no primeiro teste | A partida do N20 puxa corrente alta |
| 8 | Resistência entre `VM` e `GND` do driver, **com a fonte desligada**, alta (kΩ). Perto de 0 Ω é curto | Evita ligar a fonte num driver já queimado |

Não alimente a ESP32 pela USB e pelos 5 V da fonte (`VIN`) ao mesmo tempo, a menos que você saiba que a placa tem diodo entre os dois. Para os testes de bancada, deixe a ESP32 na USB e alimente só os sensores e o driver pela bateria, ligando os `GND` entre si (terra comum). Não ligue o 3V3 da ESP32 ao trilho de 3,3 V dos sensores: são dois trilhos separados.

## 5. Primeiro uso, passo a passo

**1. Baixe o código e entre na pasta do firmware**

```bash
git clone https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer.git
cd 2026_2_PI1_Grupo01_Hilmer/src/firmware
```

**2. Conecte a placa e veja a porta**

```bash
pio device list
```

Procure a entrada da placa: algo como `CP2102 USB to UART Bridge` (VID:PID `10C4:EA60`) ou o `USB Serial` do CH340 (`1A86:7523`). A porta é `COMn` no Windows, `/dev/cu.usbserial-XXXX` (ou `/dev/cu.wchusbserial-XXXX`) no macOS e `/dev/ttyUSB0` no Linux. Se não aparecer, vá para [Problemas comuns](#7-problemas-comuns-e-como-resolver).

**3. Grave o programa na placa**

```bash
pio run -e bancada_geral_devkit -t upload
```

A primeira vez baixa a plataforma e o compilador e demora alguns minutos. Termina com `[SUCCESS]`. Se aparecer `Failed to connect`, use o [modo BOOT](#modo-boot-quando-a-gravacao-nao-conecta).

**4. Abra o monitor serial**

```bash
pio device monitor -e bancada_geral_devkit
```

O menu aparece sozinho: abrir o monitor reinicia a placa e o menu sai em cerca de 1 s. Se não aparecer, aperte **Enter**. Se houver mais de uma porta, indique a certa: `pio device monitor -e bancada_geral_devkit --port COM5` (ou `/dev/ttyUSB0`).

**5. Use o menu.** Digite **só a tecla**, sem Enter. Para sair do monitor, `Ctrl+C`.

**6. Rode o autoteste (tecla `t`)** com o robô parado. Ele verifica o I²C, os 3 ToF, o MPU6050 e os encoders e diz o que fazer em cada falha. Exemplo de resposta saudável:

```text
AUTOTESTE (robo parado; os motores nao sao acionados)
1) Barramento I2C
  [OK]    0x30 VL53L0X esquerdo responde
  [OK]    0x31 VL53L0X frontal responde
  [OK]    0x32 VL53L0X direito responde
  [OK]    0x68 MPU6050 responde
  [OK]    nenhum VL53L0X ficou no endereco de fabrica (0x29)
2) Sensores de distancia (ToF)
  [OK]    ToF frente: 182 mm
  [OK]    ToF esquerda: 95 mm
  [OK]    ToF direita: fora de alcance
3) MPU6050
  [OK]    MPU6050 responde (WHO_AM_I 0x68, 50 de 50 leituras)
  [OK]    giro Z parado: 0.3 graus/s (limite 5.0)
  [OK]    aceleracao X=12 mg, Y=-8 mg
4) Encoders (parados)
  [OK]    encoder esquerdo estavel (0 pulsos, 0 transicoes invalidas)
  [OK]    encoder direito estavel (0 pulsos, 0 transicoes invalidas)
5) Bateria e botao
  [OK]    bateria 7.62 V
  [OK]    botao BTN_START solto (nivel alto)
RESULTADO: 15 OK, 0 FALHA(S).
REGISTRO: autoteste | ok=15 falhas=0 | veredito=OK
Nao verificado aqui: motores (tecla 7), o sentido dos encoders (gire a roda a mao com a tecla 9) e o aperto do botao (tecla b).
```

Os valores são um exemplo. A linha `REGISTRO` é a que se cola no registro de testes.

**7. Para testar a telemetria**, copie `include/credenciais.exemplo.h` para `include/credenciais.h` (o Git ignora esse arquivo), preencha o Wi-Fi, a URL do Supabase, a chave `anon` (nunca a `sb_secret`), o `DEVICE_TOKEN` e, na bancada, `CREDENCIAL_SEM_VALIDAR_CERTIFICADO 1`. Grave de novo, tecle `w` e depois `a`. Veja a [tabela da telemetria](#telemetria-tecla-w).

**8. Registre o resultado** (veja [como registrar](#8-como-registrar-um-teste)).

## 6. Teclas do menu

**Menu principal**

| Tecla | O que faz |
|:-:|---|
| `t` | Autoteste dos sensores e dos encoders parados. Não gira motor |
| `1` | Varredura I²C: lista quem responde e compara com o esperado |
| `2` | Reinicia os 3 ToF e o MPU6050 e diz qual não respondeu |
| `3` / `4` | ToF: uma leitura / leitura contínua (passe a mão na frente de cada sensor) |
| `5` / `6` | MPU6050: uma leitura / contínua (gire e incline o robô) |
| `7` | Abre o menu de motores |
| `8` | Mostra a pinagem em uso |
| `9` / `0` | Encoders: contagem ao vivo (gire a roda à mão) / zerar a contagem |
| `i` | **Teste integrado**: liga os dois motores por 5 s e lê ToF, MPU6050 e encoders ao mesmo tempo. Diz se os dois giram, se o PWM derruba algum sensor e quanto a bateria cai. Termina com uma linha `REGISTRO` |
| `a` / `b` | Tensão da bateria pelo divisor do GPIO34 / botão de largada ao vivo (aperte e solte; qualquer tecla para) |
| `w` | Abre o submenu de telemetria (Wi-Fi e banco) |
| `r` | Reinicia a placa por software |
| Enter | Mostra o menu de novo |

**Menu de motores (tecla `7`)**

| Tecla | O que faz |
|:-:|---|
| `1` / `2` | Motor esquerdo para a frente / para trás |
| `3` / `4` | Motor direito para a frente / para trás |
| `5` / `6` | Os dois para a frente / para trás |
| `7` / `8` | Giro no lugar para a direita / para a esquerda |
| `9` / `a` | **Rampa** no motor esquerdo / direito: sobe a potência aos poucos e diz em qual a roda começou a girar |
| `c` | **Modo contínuo** (até 60 s): o motor fica ligado e mostra os pulsos e a velocidade ao vivo. Dentro dele: `e`, `d`, `b` trocam o motor; `r` inverte o sentido; `+` e `-` mudam a potência |
| `+` / `-` | Aumenta / diminui a potência (100 a 1000, começa em 300) |
| `l` / `k` | Aumenta / diminui o **limite de duty** (10% a 100%, começa nos 70% do robô). O multímetro no motor lê duty × VM: com VM de 6 V e 70% o máximo é 4,2 V. Acima de 70% é só para teste |
| `0` | Volta ao menu principal |

Cada teste de motor dura no máximo 2 s e **qualquer tecla freia** os motores. No modo contínuo, o programa não percebe o monitor fechar (a serial é a UART0 da placa), então ele pede a tecla `m` a cada 10 s e, sem ela, freia sozinho. Ao fim de cada teste, a bancada compara o sinal do encoder com o comando e diz se está coerente.

### Telemetria (tecla `w`)

O submenu grava uma corrida e amostras de teste **de verdade** no banco do `credenciais.h`. Use o projeto Supabase de testes ou apague a corrida depois.

| Tecla | O que faz |
|:-:|---|
| `a` | **Fluxo completo**: Wi-Fi, DNS, criar corrida, enviar um lote, reenviar o lote (confere que não duplica) e encerrar. Termina com `RESULTADO` |
| `w` / `d` | Conecta no Wi-Fi (mostra IP e sinal) / resolve o nome do Supabase |
| `c` / `l` / `r` / `e` | Cria a corrida / envia 5 amostras / reenvia o último lote / encerra a corrida |
| `b` | **Teste de reenvio com queda de Wi-Fi:** 60 s de amostras (1 por segundo) pelo mesmo buffer do robô. Desligue o hotspot por cerca de 30 s no meio e religue; no fim, ele confere se nenhuma amostra se perdeu ou duplicou e dá a consulta SQL para conferir no banco |
| `s` / `0` | Status / volta ao menu principal |

## 7. Problemas comuns e como resolver

### A porta não aparece em `pio device list`

| Sistema | O que fazer |
|---|---|
| Todos | Troque o cabo por um **com dados** e teste outra porta USB do computador, sem hub |
| Windows | Veja em *Gerenciador de Dispositivos* se aparece em *Portas (COM)*. Se aparecer em "Outros dispositivos" ou com triângulo amarelo, instale o driver do chip da placa (CP210x ou CH340) |
| macOS | `ls /dev/cu.usb*`. Se não listar, o cabo é só de carga ou falta o driver do chip |
| Linux | `ls /dev/ttyUSB*` e `dmesg \| tail`. Se aparecer e sumir, pare o ModemManager |

### `Permission denied` ao abrir a porta (Linux)

Entre no grupo da porta e saia da sessão para valer:

```bash
sudo usermod -a -G dialout $USER
```

### `could not open port` / `Access is denied` / `Resource busy`

Outro programa está usando a porta: um monitor serial aberto, outra janela do terminal, o VS Code ou o Arduino IDE. Feche-o (no terminal, `Ctrl+C`) e tente de novo. No Windows, desconecte e reconecte o cabo.

### Modo BOOT: quando a gravação não conecta

Sintoma:

```text
Connecting......................................
A fatal error occurred: Failed to connect to ESP32: No serial data received.
```

A placa não entrou no modo de gravação sozinha. Faça isso à mão:

1. Rode `pio run -e bancada_geral_devkit -t upload`.
2. Quando aparecer `Connecting....`, **segure o botão BOOT** da placa e solte quando a gravação começar (aparecem os percentuais).
3. Ao terminar, aperte o botão **EN** (reset) para a placa rodar o programa.
4. Se a gravação nunca começa, troque o cabo por um com dados e confira o driver do chip (seção 3).

### O menu não aparece

- Aperte **Enter**.
- Abra o monitor de novo (`pio device monitor -e bancada_geral_devkit`). Abrir o monitor reinicia a placa, e o menu aparece em cerca de 1 s.
- Aperte `r` para reiniciar a placa por software.
- Confira se gravou o ambiente certo (`bancada_geral_devkit`) e se a gravação terminou em `[SUCCESS]`.

### Aparecem caracteres estranhos ou o terminal não mostra o que você digita

É esperado: a bancada não escreve acentos de propósito e não repete as teclas digitadas. A velocidade é de 115200 baud: o `pio device monitor -e bancada_geral_devkit` já usa esse valor (`monitor_speed` no `platformio.ini`); em outro programa de terminal, escolha 115200.

### A fonte desarma ao ligar o driver

Quase sempre é **polaridade invertida** ou **curto** no driver. Desconecte tudo e confira, com a fonte desligada, o `+` e o `-` com o multímetro e a resistência entre `VM` e `GND` do driver. Perto de 0 Ω: driver danificado. Troque o driver antes de tentar de novo.

### Exemplos de diagnóstico pela própria bancada

**Falta um sensor de distância**

```text
2) Sensores de distancia (ToF)
  [OK]    ToF frente: 182 mm
  [FALHA] ToF esquerda: sem amostra
          dica: sem medida: confira fiacao, XSHUT e alimentacao de 3,3 V; tente a tecla 2 e a varredura (tecla 1)
```

1. Aperte `1` (varredura). Se faltar o `0x30`, o sensor esquerdo não está respondendo no barramento.
2. Confira a fiação de `SDA`, `SCL`, `3,3 V` e `GND` do módulo e o fio do `XSHUT` esquerdo (GPIO16; a tecla `8` mostra a pinagem em uso).
3. Aperte `2` para reiniciar os sensores e rode `t` de novo.

Endereços esperados: esquerdo `0x30`, frontal `0x31`, direito `0x32` e MPU6050 `0x68`. Os pinos de `XSHUT` são GPIO16, 17 e 4.

**Sensor no endereço de fábrica**

```text
  0x29  VL53L0X no endereco de fabrica (nao foi reenderecado)
```

O sensor não recebeu o novo endereço no boot. Aperte `2` e confira o `XSHUT` dele. Se o `XSHUT` estiver solto no ar, o sensor liga junto dos outros e briga pelo endereço.

**MPU6050 não responde**

```text
  MPU6050: FALHOU (sem resposta no 0x68; confira SDA, SCL, energia e AD0)
```

(Mensagem da tecla `2`. No autoteste, o item aparece como `[FALHA]` com a mesma dica.)

Confira `SDA`, `SCL`, `3,3 V`, `GND` e o pino `AD0` no `GND` (endereço `0x68`). Se a varredura (`1`) mostrar `0x69`, o `AD0` está em nível alto.

**Giro parado alto**

```text
  [FALHA] giro Z parado: 12.4 graus/s (limite 5.0)
          dica: o robo precisa estar parado; se estiver, o vies do giroscopio e alto (a calibracao do boot corrige)
```

Deixe o robô imóvel sobre a bancada e rode `t` de novo. Se persistir, o viés do giroscópio está alto; a calibração do boot do robô compensa.

**Encoder sem sinal ou com sinal oposto**

```text
> motor esquerdo para a frente: esquerda 600, direita 0 (duty efetivo 42%). Qualquer tecla freia.
> tempo esgotado, motores freados.
  encoder esquerdo: 0 pulsos (0 mm) -> SEM PULSOS: confira VCC, GND, A e B do encoder e se a roda girou.
```

- **Sem pulsos e a roda girou:** o problema é o encoder. Meça `VCC` (3,3 V) e `GND` na plaquinha e confira os fios `A` e `B` (GPIO32 e 33 no esquerdo e GPIO18 e 19 no direito). Gire a roda à mão com a tecla `9`: a contagem tem que mudar.
- **Sem pulsos e a roda não girou:** veja a rampa abaixo.
- **Sinal oposto ao comando:** se a roda andou para trás, o **motor** está invertido (inverta os dois fios `OUT` do motor ou a flag no código). Se andou para o lado certo, o **encoder** está com `A` e `B` trocados.
- **Muitas transições inválidas:** ruído. Afaste os fios do encoder dos fios do motor e confira o `GND`.

**O motor não gira**

Use a **rampa** (menu de motores, tecla `9` para o esquerdo ou `a` para o direito):

```text
  potencia  100 (duty  7%): 0 pulsos
  potencia  300 (duty 21%): 0 pulsos
  ...
  potencia  500 (duty 35%): 4 pulsos
  potencia  600 (duty 42%): 38 pulsos
> a roda comecou a girar na potencia 500 (duty 35%).
```

Neste exemplo, a potência padrão de 300 não bastava: o N20 tem uma zona morta e precisa de mais tensão para sair do lugar. Se a rampa terminar assim, o problema é de hardware:

```text
> a roda NAO girou em nenhuma potencia. Se o motor nem zumbe: confira VM, GND, nSLEEP e os fios OUT.
  Se zumbe e nao gira: falta corrente na fonte ou o motor esta travado.
  Se gira e o encoder nao conta: o problema esta no encoder (VCC, GND, A, B).
```

Meça o `VM` com o motor acionado (deve ficar entre 5 e 6 V), o `nSLEEP` (cerca de 3,3 V) e a tensão média no `IN1` (GPIO25): com o limite de duty de 70% e VM de 6 V, cerca de 4,2 V na potência 1000. O multímetro lê a média do PWM, então marca duty × VM, e não os 6 V do pulso.

**Telemetria: erros comuns (tecla `w`, depois `a`)**

A bancada mostra o código HTTP, a resposta do servidor e uma dica. Os casos mais comuns:

| O que aparece | Causa provável | O que fazer |
|---|---|---|
| `Wi-Fi nao conectou` | Rede de 5 GHz, senha errada ou rede com login por portal | Use um hotspot de celular em 2,4 GHz e confira `CREDENCIAL_WIFI_SSID` e a senha |
| `sem include/credenciais.h` | O arquivo não existe | Copie o `credenciais.exemplo.h` e preencha (seção 5, passo 7) |
| `HTTP 401` ou `403` | Chave `anon` ou `DEVICE_TOKEN` errados | O token precisa ser igual ao segredo `DEVICE_TOKEN` das Edge Functions. Use a chave `anon` ou `publishable`, **nunca** a `sb_secret` |
| `HTTP 404` com `Requested function was not found` | A Edge Function não foi publicada | `supabase functions deploy corridas` e `telemetria` |
| `HTTP 500` ao enviar o lote | Migração da telemetria não aplicada no banco (falta a função `registrar_telemetria`) | `supabase db push`, ou rode os SQL de `src/backend/supabase/migrations` no SQL Editor, e tente de novo |
| `connection refused` ou tempo esgotado | Sem DNS ou TLS recusado | Tecle `d` para ver o DNS. Na bancada, use `CREDENCIAL_SEM_VALIDAR_CERTIFICADO 1` |

Depois de um fluxo completo bem-sucedido, confira no painel do Supabase (Table Editor) as tabelas `corridas` e `telemetria`. No reenvio (tecla `r`), a resposta correta é `gravadas 0, ja existentes 5`.

## 8. Como registrar um teste

Registre em `docs/07-testes/hardware.md`, uma linha por teste, com o que for relevante:

```text
Data: 29/10/2026
Quem: <nome>
Bancada geral: build de <data e hora mostradas no menu>, branch/commit <hash>
Componente: motor direito + encoder direito
Comando: tecla 7, rampa (a) e teste 3
Resultado: a roda começou a girar na potência 500; encoder +380 pulsos em 2 s, sinal coerente
Observações: VM 5,5 V, fonte limitada a 1 A
```

O menu mostra a linha `Build: <data> <hora>`, que identifica a versão gravada. Copie o autoteste inteiro (com a linha `REGISTRO`) quando for o resultado do dia.

## 9. Como evoluir a bancada

A bancada é de toda a equipe. Ao mudar um driver, um pino ou a arquitetura de um componente, atualize também o menu. Os passos para acrescentar um componente (reuso dos drivers do robô, limites de segurança, item no autoteste) estão no [README do firmware](https://github.com/fcte-pi1/2026_2_PI1_Grupo01_Hilmer/blob/main/src/firmware/README.md#como-acrescentar-um-componente-ao-menu).

Código: `src/firmware/src/bancada/bancada_geral.cpp`. Ambiente do PlatformIO: `bancada_geral_devkit` (ESP32 DevKit V1).
