#!/usr/bin/env python3
"""Menu de bancada do deslocamento sem encoder (#380), da execução dos movimentos (#184) e da
correção pelas paredes (#188).

Grava o firmware de bancada (ambiente `bancada_deslocamento`), aciona o robô e mostra o log da
estimativa ao vivo, para a equipe conferir na régua se o que o robô acha que andou bate com o que
ele andou de verdade: tabela de velocidade, avanço de uma célula, correção pelo ToF num corredor,
confiança da pose e giro de 90°. As etapas 7 e 8 executam os comandos do robô ("avançar uma célula",
"girar 90°" e "girar 180°", #184) e registram o erro de deslocamento medido na régua e no
transferidor. As etapas 9 e 10 (#188) conferem o desvio lateral medido pelas paredes, o
realinhamento ao centro do corredor e, depois de um percurso, se a célula estimada coincide com a
real. Cada movimento é salvo em CSV, e o roteiro gera um relatório em Markdown para
registrar em `docs/07-testes/software.md`.

Uso (a partir de `src/firmware`):
    ./scripts/bancada_deslocamento.py              # escolhe a porta no menu
    ./scripts/bancada_deslocamento.py --porta /dev/ttyACM0

ATENÇÃO: as etapas acionam os motores. Robô no chão, com espaço livre, e qualquer tecla (Ctrl+C)
freia na hora. Requer `pyserial` (pip install pyserial) e, para gravar, o PlatformIO (`pio`).
"""

import argparse
import csv
import json
import os
import shutil
import subprocess
import sys
import time
from datetime import datetime

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("ERRO: Pacote 'pyserial' não encontrado. Instale com: pip install pyserial")
    sys.exit(1)

PASTA_FIRMWARE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BAUDRATE = 115200
CELULA_MM = 180.0

# Critérios provisórios: ajuste aqui se a equipe combinar outros valores.
ERRO_TABELA_PERC = 10.0  # velocidade real x tabela
ERRO_UMA_CELULA_MM = 18.0  # 10% de uma célula, sem correção
ERRO_CORREDOR_MM = 15.0  # com correção pelo ToF
GIRO_TOLERANCIA_GRAUS = 20.0
# Execução dos movimentos (#184): o avanço termina no centro da célula e o giro, no rumo alvo.
ERRO_EXECUCAO_MM = 10.0  # parada depois de n células, contra a régua
# Erro lateral abaixo da folga do giro no lugar: chassi de 120 × 100 mm num corredor de 168 mm deixa
# a quina a 78,1 mm do centro e a parede a 84 mm (5,9 mm de folga).
ERRO_LATERAL_MM = 5.0
GIRO_EXECUTADO_TOLERANCIA_GRAUS = 3.0
# Correção pelas paredes (#188).
ERRO_DESVIO_MEDIDO_MM = 2.0  # desvio lateral medido pelos ToF contra a régua
ERRO_REALINHAMENTO_MM = 2.0  # desvio que sobra depois do realinhamento
PERCURSO_PADRAO = "AAADAEA"
GIROS_EXECUTADOS = (("GIRO90D", "90° para a direita", 90.0), ("GIRO90E", "90° para a esquerda", 90.0),
                    ("GIRO180", "180° (pela esquerda)", 180.0))
# Giro de 90° com bitola de 100 mm: cada roda anda π/2 × 50 mm (ConfiguracaoOdometria: 625 pulsos).
ARCO_POR_RODA_90_GRAUS_MM = 78.5

VERDE = "\033[32m" if sys.stdout.isatty() else ""
VERMELHO = "\033[31m" if sys.stdout.isatty() else ""
AMARELO = "\033[33m" if sys.stdout.isatty() else ""
NORMAL = "\033[0m" if sys.stdout.isatty() else ""

CAMPOS_CSV = ["tipo", "t_ms", "pot_esq", "pot_dir", "pulsos_esq", "pulsos_dir", "norte_mm",
              "leste_mm", "linha", "coluna", "orientacao", "pose_valida", "sem_correcao_mm",
              "tof_frente_mm", "tof_esq_mm", "tof_dir_mm", "tof_valida", "resultado", "delta_mm"]


class Bancada:
    """Conexão com o firmware de bancada, logs e resultados."""

    def __init__(self, porta):
        self.porta = porta
        self.serial = None
        self.info = None
        self.resultados = []  # (etapa, situação, detalhe)
        self.anotacoes = {}
        self.velocidades_medidas = {}  # potência -> mm/s medidos
        self.pasta_logs = os.path.abspath(f"bancada_deslocamento_{datetime.now():%Y%m%d_%H%M%S}")
        self.movimentos = 0

    # ---- conexão -----------------------------------------------------------------------------

    def conectar(self):
        self.fechar()
        try:
            self.serial = serial.Serial(self.porta, BAUDRATE, timeout=0.2)
        except serial.SerialException as erro:
            print(f"{VERMELHO}Não foi possível abrir {self.porta}: {erro}{NORMAL}")
            return False
        time.sleep(1.0)
        self.serial.reset_input_buffer()
        info = self.comando("INFO", "info", espera_s=3)
        if info is None or info.get("programa") != "bancada_deslocamento":
            print(f"{VERMELHO}A placa não respondeu como firmware de bancada do deslocamento. "
                  f"Grave-o pela opção 1 do menu.{NORMAL}")
            return False
        self.info = info
        print(f"{VERDE}Conectado em {self.porta}.{NORMAL}")
        if info.get("encoders_instalados"):
            print(f"{AMARELO}ENCODERS_INSTALADOS = true: esta bancada é do modo estimado.{NORMAL}")
        if not info.get("tof_ok"):
            print(f"{AMARELO}Algum ToF não iniciou: as correções não vão acontecer.{NORMAL}")
        return True

    def fechar(self):
        if self.serial is not None:
            self.serial.close()
            self.serial = None

    def comando(self, texto, tipo, espera_s=5.0, ao_receber=None):
        """Envia um comando e devolve a primeira resposta JSON com o `tipo` pedido.

        `ao_receber` recebe as outras linhas JSON que chegam enquanto espera (log do movimento).
        """
        if self.serial is None:
            print(f"{VERMELHO}Sem conexão. Use a opção 2 do menu.{NORMAL}")
            return None
        self.serial.reset_input_buffer()
        self.serial.write((texto + "\n").encode())
        limite = time.time() + espera_s
        while time.time() < limite:
            linha = self.serial.readline().decode("utf-8", errors="ignore").strip()
            if not linha.startswith("{"):
                continue
            try:
                dados = json.loads(linha)
            except json.JSONDecodeError:
                continue
            if dados.get("tipo") == tipo:
                return dados
            if dados.get("tipo") == "erro":
                print(f"{VERMELHO}Placa: {dados.get('mensagem')}{NORMAL}")
                return None
            if ao_receber is not None:
                ao_receber(dados)
        print(f"{VERMELHO}Sem resposta '{tipo}' ao comando {texto} em {espera_s:.0f} s.{NORMAL}")
        return None

    # ---- movimentos e logs -------------------------------------------------------------------

    def velocidade(self, potencia):
        resposta = self.comando(f"VEL {potencia}", "vel")
        return resposta["mm_por_s"] if resposta else 0

    def mover(self, tipo, potencia, duracao_ms, correcao):
        """Executa ANDAR/GIRAR mostrando o log ao vivo. Devolve (fim, estado final, linhas)."""
        self.comando(f"CORRECAO {'ON' if correcao else 'OFF'}", "correcao_ligada")
        linhas = []

        def mostrar(dados):
            linhas.append(dados)
            if dados.get("tipo") == "log":
                valida = f"{VERDE}sim{NORMAL}" if dados["pose_valida"] else f"{VERMELHO}não{NORMAL}"
                print(f"  {dados['t_ms']:5d} ms | norte {dados['norte_mm']:7.1f} mm | leste "
                      f"{dados['leste_mm']:6.1f} | ({dados['linha']},{dados['coluna']}) "
                      f"{dados['orientacao']} | pose válida {valida} | sem correção "
                      f"{dados['sem_correcao_mm']:4d} mm | ToF {dados['tof_frente_mm']}/"
                      f"{dados['tof_esq_mm']}/{dados['tof_dir_mm']}")
            elif dados.get("tipo") == "correcao":
                cor = AMARELO if dados["resultado"] == "descartada" else VERDE
                print(f"  {cor}>>> correção {dados['resultado']}: {dados['delta_mm']:+.1f} mm "
                      f"(norte agora {dados['norte_mm']:.1f} mm){NORMAL}")

        print(f"\n{AMARELO}Motores ligados por {duracao_ms} ms. Ctrl+C freia na hora.{NORMAL}")
        fim = None
        try:
            fim = self.comando(f"{tipo} {potencia} {duracao_ms}", "fim_movimento",
                               espera_s=duracao_ms / 1000 + 5, ao_receber=mostrar)
        except KeyboardInterrupt:
            self.serial.write(b"\n")  # qualquer byte freia
            print(f"\n{AMARELO}Interrompido: motores freados.{NORMAL}")
            estado = self.comando("LER", "estado")  # descarta o resto e lê o estado
            self.salvar_csv(tipo, potencia, duracao_ms, correcao, linhas)
            return None, estado, linhas
        estado = self.comando("LER", "estado")
        if fim is not None:
            linhas.append(fim)
            print(f"  Fim: avanço estimado {fim['avanco_norte_mm']:.1f} mm (norte), "
                  f"{fim['correcoes']} correção(ões), {fim['descartadas']} descartada(s), soma das "
                  f"correções {fim['soma_correcoes_mm']:+.1f} mm.")
        self.salvar_csv(tipo, potencia, duracao_ms, correcao, linhas)
        return fim, estado, linhas

    def executar(self, texto, tipo_fim, espera_s):
        """Executa um comando do `ExecutorMovimentos` (CELULA, GIRO*) com o log ao vivo.

        Devolve (linhas `fim_execucao`, resposta `tipo_fim` ou None, todas as linhas).
        """
        linhas = []
        fins = []

        def mostrar(dados):
            linhas.append(dados)
            if dados.get("tipo") == "log":
                print(f"  {dados['t_ms']:5d} ms | norte {dados['norte_mm']:7.1f} mm | leste "
                      f"{dados['leste_mm']:6.1f} | ({dados['linha']},{dados['coluna']}) "
                      f"{dados['orientacao']} | pot {dados['pot_esq']}/{dados['pot_dir']} | ToF "
                      f"{dados['tof_frente_mm']}/{dados['tof_esq_mm']}/{dados['tof_dir_mm']}")
            elif dados.get("tipo") == "fim_execucao":
                fins.append(dados)
                cor = VERDE if dados["estado"] == "concluido" else VERMELHO
                print(f"  {cor}>>> {dados['movimento']}: {dados['estado']} ({dados['falha']}) em "
                      f"{dados['tempo_ms']} ms, faltou {dados['restante_mm']} mm, giro medido "
                      f"{dados['giro_medido_graus']:+.1f}°, erro de rumo {dados['erro_rumo_graus']:+.1f}°{NORMAL}")

        print(f"\n{AMARELO}Motores ligados ({texto}). Ctrl+C freia na hora.{NORMAL}")
        fim = None
        try:
            fim = self.comando(texto, tipo_fim, espera_s=espera_s, ao_receber=mostrar)
        except KeyboardInterrupt:
            self.serial.write(b"\n")  # qualquer byte freia
            print(f"\n{AMARELO}Interrompido: motores freados.{NORMAL}")
            self.comando("LER", "estado")
        if fim is not None:
            linhas.append(fim)
            if fim.get("tipo") == "fim_execucao" and fim not in fins:
                fins.append(fim)
        self.salvar_csv(None, 0, 0, True, linhas, nome="execucao_" + texto.lower().replace(" ", "_"))
        return fins, fim, linhas

    def salvar_csv(self, tipo, potencia, duracao_ms, correcao, linhas, nome=None):
        os.makedirs(self.pasta_logs, exist_ok=True)
        self.movimentos += 1
        nome = f"{self.movimentos:02d}_{nome}.csv" if nome else (
            f"{self.movimentos:02d}_{tipo.lower()}_p{potencia}_{duracao_ms}ms_"
            f"{'com' if correcao else 'sem'}_tof.csv")
        caminho = os.path.join(self.pasta_logs, nome)
        with open(caminho, "w", newline="", encoding="utf-8") as arquivo:
            escritor = csv.DictWriter(arquivo, fieldnames=CAMPOS_CSV, extrasaction="ignore")
            escritor.writeheader()
            for dados in linhas:
                if dados.get("tipo") in ("log", "correcao"):
                    escritor.writerow(dados)
        print(f"  Log salvo em {caminho}")

    # ---- registro ----------------------------------------------------------------------------

    def registrar(self, etapa, situacao, detalhe):
        self.resultados.append((etapa, situacao, detalhe))
        cor = {"APROVADO": VERDE, "REPROVADO": VERMELHO}.get(situacao, AMARELO)
        print(f"  [{cor}{situacao}{NORMAL}] {etapa}: {detalhe}")

    def aprovar_se(self, etapa, condicao, detalhe):
        self.registrar(etapa, "APROVADO" if condicao else "REPROVADO", detalhe)
        return condicao


# ---- utilitários de tela ---------------------------------------------------------------------


def pausar(mensagem):
    input(f"\n>>> {mensagem}\n    Pressione ENTER para continuar...")


def perguntar_numero(mensagem, padrao=None):
    while True:
        sufixo = f" [{padrao}]" if padrao is not None else ""
        texto = input(f"{mensagem}{sufixo}: ").strip().replace(",", ".")
        if not texto and padrao is not None:
            return padrao
        try:
            return float(texto)
        except ValueError:
            print("Valor inválido.")


def titulo(texto):
    print(f"\n=== {texto} " + "=" * max(0, 60 - len(texto)))


def duracao_para(distancia_mm, velocidade_mm_s):
    return int(round(distancia_mm / velocidade_mm_s * 1000)) if velocidade_mm_s > 0 else 0


# ---- etapas ----------------------------------------------------------------------------------


def gravar_firmware(bancada):
    titulo("Gravar o firmware de bancada do deslocamento")
    pio = shutil.which("pio") or os.path.expanduser("~/.platformio/penv/bin/pio")
    if not os.path.exists(pio) and shutil.which("pio") is None:
        print(f"{VERMELHO}PlatformIO (pio) não encontrado. Instale com: pipx install platformio{NORMAL}")
        return
    bancada.fechar()
    comando = [pio, "run", "-e", "bancada_deslocamento", "-t", "upload", "--upload-port", bancada.porta]
    print("Executando:", " ".join(comando))
    if subprocess.run(comando, cwd=PASTA_FIRMWARE).returncode != 0:
        print(f"{VERMELHO}A gravação falhou. Se a placa não entrar em modo de gravação, segure o "
              f"botão BOOT, aperte RESET e tente de novo.{NORMAL}")
        return
    print("Aguardando a placa reiniciar...")
    time.sleep(3)
    bancada.conectar()


def verificar_estado_inicial(bancada):
    titulo("1. Estado inicial (sem mover)")
    info = bancada.comando("INFO", "info")
    if info is None:
        return
    bancada.aprovar_se("Modo estimado", not info["encoders_instalados"],
                       f"ENCODERS_INSTALADOS = {str(info['encoders_instalados']).lower()}")
    bancada.aprovar_se("ToF iniciados", info["tof_ok"],
                       "os 3 sensores responderam" if info["tof_ok"] else "algum sensor falhou (#344)")
    estado = bancada.comando("ZERAR", "estado")
    if estado is None:
        return
    bancada.aprovar_se("Pose inicial", estado["linha"] == 0 and estado["coluna"] == 0 and
                       estado["orientacao"] == "N" and estado["pose_valida"] and
                       estado["sem_correcao_mm"] == 0,
                       f"({estado['linha']},{estado['coluna']}) {estado['orientacao']}, pose válida "
                       f"{estado['pose_valida']}, sem correção {estado['sem_correcao_mm']} mm")
    tabela = ", ".join(f"{p} → {v} mm/s" for p, v in info["tabela"])
    print(f"  Tabela em uso: {tabela}")


def calibrar_tabela(bancada):
    titulo("2. Tabela de velocidade (sem correção)")
    print("Cada ponto da tabela é testado: o robô anda 2 s em linha reta com aquela potência, e você")
    print("mede na régua quanto ele andou (em linha reta, do ponto de partida até o ponto final).")
    print(f"Critério provisório: velocidade real até ±{ERRO_TABELA_PERC:.0f}% da tabela.")
    if bancada.info is None and not bancada.conectar():
        return
    tabela = bancada.info["tabela"]

    menor = tabela[0][0]
    pausar(f"Zona morta: o robô vai receber potência {menor - 20} (abaixo do primeiro ponto). "
           "Marque a posição dele no chão.")
    bancada.comando("ZERAR", "estado")
    bancada.mover("ANDAR", menor - 20, 1500, correcao=False)
    moveu = input("O robô se moveu? (s/N): ").strip().lower() == "s"
    if moveu:
        bancada.registrar("Zona morta", "OBSERVADO",
                          f"o robô andou com {menor - 20}: a zona morta é menor que {menor}; "
                          "baixe o primeiro ponto da tabela")
    else:
        bancada.registrar("Zona morta", "APROVADO", f"parado com {menor - 20}, como a tabela supõe")

    for potencia, velocidade_tabela in tabela:
        pausar(f"Potência {potencia} (tabela: {velocidade_tabela} mm/s, ~{velocidade_tabela * 2} mm "
               "em 2 s). Ponha o robô na marca 0 da régua, alinhado.")
        bancada.comando("ZERAR", "estado")
        fim, _, _ = bancada.mover("ANDAR", potencia, 2000, correcao=False)
        if fim is None:
            continue
        medido = perguntar_numero("Distância medida na régua, em mm")
        real = medido / (fim["duracao_ms"] / 1000.0)
        bancada.velocidades_medidas[potencia] = real
        erro = (real - velocidade_tabela) / velocidade_tabela * 100 if velocidade_tabela else 0
        bancada.aprovar_se(f"Tabela, potência {potencia}", abs(erro) <= ERRO_TABELA_PERC,
                           f"real {real:.0f} mm/s, tabela {velocidade_tabela} mm/s ({erro:+.0f}%); "
                           f"estimado {fim['avanco_norte_mm']:.0f} mm, medido {medido:.0f} mm")
    sugerir_tabela(bancada)


def sugerir_tabela(bancada):
    if not bancada.velocidades_medidas:
        return
    print("\nTabela medida, para colar em lib/deslocamento/src/ConfiguracaoDeslocamento.h:")
    print("constexpr PontoVelocidade TABELA_VELOCIDADE[] = {")
    for potencia in sorted(bancada.velocidades_medidas):
        print(f"    {{{potencia}, {round(bancada.velocidades_medidas[potencia])}}},")
    print("};")


def verificar_uma_celula(bancada):
    titulo("3. Avanço de uma célula (sem correção)")
    potencia = int(perguntar_numero("Potência", 400))
    velocidade = bancada.velocidade(potencia)
    duracao = duracao_para(CELULA_MM, velocidade)
    if duracao == 0:
        print(f"{VERMELHO}A tabela dá velocidade zero para {potencia}.{NORMAL}")
        return
    pausar(f"O robô vai andar ~180 mm ({duracao} ms a {velocidade} mm/s pela tabela). "
           "Ponha o robô na marca 0 da régua, alinhado.")
    bancada.comando("ZERAR", "estado")
    fim, estado, _ = bancada.mover("ANDAR", potencia, duracao, correcao=False)
    if fim is None or estado is None:
        return
    medido = perguntar_numero("Distância medida na régua, em mm")
    erro = fim["avanco_norte_mm"] - medido
    bancada.aprovar_se("Uma célula, estimado x régua", abs(erro) <= ERRO_UMA_CELULA_MM,
                       f"estimado {fim['avanco_norte_mm']:.0f} mm, medido {medido:.0f} mm, erro "
                       f"{erro:+.0f} mm (limite ±{ERRO_UMA_CELULA_MM:.0f})")
    linha_real = int(round(medido / CELULA_MM))
    bancada.aprovar_se("Uma célula, célula estimada", estado["linha"] == linha_real,
                       f"estimada ({estado['linha']},{estado['coluna']}), real pela régua: linha {linha_real}")


def verificar_corredor(bancada):
    titulo("4. Corredor com correção pelo ToF")
    print("Monte um corredor reto com paredes do labirinto: o robô no centro da primeira célula,")
    print("parede no FIM da quarta célula (a 3,5 células = 630 mm do centro de partida) e, se der,")
    print("uma abertura lateral no meio (por exemplo, sem a parede direita da segunda célula).")
    print("O robô anda ~450 mm (2,5 células), parando bem antes da parede.")
    potencia = int(perguntar_numero("Potência", 400))
    velocidade = bancada.velocidade(potencia)
    duracao = duracao_para(2.5 * CELULA_MM, velocidade)
    if duracao == 0:
        print(f"{VERMELHO}A tabela dá velocidade zero para {potencia}.{NORMAL}")
        return
    pausar("Ponha o robô no centro da primeira célula, virado para a parede do fim.")
    bancada.comando("ZERAR", "estado")
    fim, estado, linhas = bancada.mover("ANDAR", potencia, duracao, correcao=True)
    if fim is None or estado is None:
        return
    medido = perguntar_numero("Distância andada, medida na régua, em mm")
    erro = fim["avanco_norte_mm"] - medido
    tipos = {l.get("resultado") for l in linhas if l.get("tipo") == "correcao"}
    bancada.aprovar_se("Corredor, houve correção", fim["correcoes"] >= 1,
                       f"{fim['correcoes']} aplicada(s) ({', '.join(sorted(tipos)) or 'nenhuma'}), "
                       f"{fim['descartadas']} descartada(s)")
    bancada.aprovar_se("Corredor, estimado x régua", abs(erro) <= ERRO_CORREDOR_MM,
                       f"estimado {fim['avanco_norte_mm']:.0f} mm, medido {medido:.0f} mm, erro "
                       f"{erro:+.0f} mm (limite ±{ERRO_CORREDOR_MM:.0f})")
    bancada.aprovar_se("Corredor, pose válida no fim", estado["pose_valida"],
                       f"pose_valida {estado['pose_valida']}, sem correção {estado['sem_correcao_mm']} mm")
    if "pela_lateral" not in tipos:
        bancada.registrar("Corredor, quina lateral", "OBSERVADO",
                          "nenhuma correção pela lateral: confira se havia abertura e o limiar lateral")


def verificar_confianca(bancada):
    titulo("5. Confiança da pose sem correção")
    print("Sem o ToF, depois de 360 mm a telemetria deve passar a dizer pose_valida: false.")
    potencia = int(perguntar_numero("Potência", 400))
    duracao = duracao_para(2.5 * CELULA_MM, bancada.velocidade(potencia))
    if duracao == 0:
        return
    pausar("O robô vai andar ~450 mm em linha reta. Deixe o caminho livre.")
    bancada.comando("ZERAR", "estado")
    fim, estado, _ = bancada.mover("ANDAR", potencia, duracao, correcao=False)
    if estado is None:
        return
    bancada.aprovar_se("Pose deixa de ser válida", not estado["pose_valida"] and
                       estado["sem_correcao_mm"] > 360,
                       f"pose_valida {estado['pose_valida']}, sem correção {estado['sem_correcao_mm']} mm")
    estado = bancada.comando("ZERAR", "estado")
    if estado is not None:
        bancada.aprovar_se("ZERAR devolve a confiança", estado["pose_valida"],
                           f"pose_valida {estado['pose_valida']}")


def verificar_giro(bancada):
    titulo("6. Giro de 90° para a direita (sem correção)")
    potencia = int(perguntar_numero("Potência", 400))
    velocidade = bancada.velocidade(potencia)
    duracao = duracao_para(ARCO_POR_RODA_90_GRAUS_MM, velocidade)
    if duracao == 0:
        return
    pausar(f"O robô vai girar no lugar por {duracao} ms. Marque a direção inicial dele no chão.")
    bancada.comando("ZERAR", "estado")
    _, estado, _ = bancada.mover("GIRAR", potencia, duracao, correcao=False)
    if estado is None:
        return
    real = perguntar_numero("Quantos graus ele girou de verdade (aproximado)", 90)
    bancada.aprovar_se("Giro, direção estimada", estado["orientacao"] == "L",
                       f"estimada {estado['orientacao']} (esperado L, Leste)")
    bancada.aprovar_se("Giro, ângulo real", abs(real - 90) <= GIRO_TOLERANCIA_GRAUS,
                       f"girou {real:.0f}° (limite 90 ± {GIRO_TOLERANCIA_GRAUS:.0f}°). Se errar sempre "
                       "para o mesmo lado, ajuste a tabela ou PULSOS_DIFERENCIAIS_POR_90_GRAUS_PROVISORIO")


def avaliar_celulas(celulas, medido_mm):
    """Erro de parada depois de `celulas` avanços, contra a régua: (erro em mm, aprovado)."""
    erro = medido_mm - celulas * CELULA_MM
    return erro, abs(erro) <= ERRO_EXECUCAO_MM


def avaliar_giro(nominal_graus, real_graus):
    """Erro de um giro executado, contra o transferidor: (erro em graus, aprovado)."""
    erro = real_graus - nominal_graus
    return erro, abs(erro) <= GIRO_EXECUTADO_TOLERANCIA_GRAUS


def verificar_execucao_celulas(bancada):
    titulo("7. Execução: avançar uma célula (#184)")
    celulas = int(perguntar_numero("Quantas células seguidas (1 a 8)", 3))
    celulas = max(1, min(8, celulas))
    pausar(f"Ponha o robô no centro de uma célula de um corredor reto com pelo menos {celulas} "
           "célula(s) livre(s) à frente, alinhado, e marque no chão a posição do centro dele. O "
           "robô usa os ToF e o giroscópio como na corrida.")
    bancada.comando("ZERAR", "estado")
    fins, fim, _ = bancada.executar(f"CELULA {celulas}", "fim_celulas", espera_s=celulas * 8 + 5)
    if fim is None:
        return
    if fim["feitas"] < celulas:
        falha = fins[-1]["falha"] if fins else "?"
        bancada.registrar("Execução, avanço de célula", "REPROVADO",
                          f"{fim['feitas']} de {celulas} célula(s); o último terminou em {falha}")
        return
    medido = perguntar_numero("Distância da marca de partida ao centro do robô agora, em mm",
                              celulas * CELULA_MM)
    erro, ok = avaliar_celulas(celulas, medido)
    paradas = ", ".join(f"{f['restante_mm']:+d}" for f in fins)
    bancada.aprovar_se("Execução, parada depois de n células", ok,
                       f"{celulas} célula(s): medido {medido:.0f} mm, esperado {celulas * CELULA_MM:.0f} mm, "
                       f"erro {erro:+.0f} mm (limite ±{ERRO_EXECUCAO_MM:.0f}); restante estimado por "
                       f"célula: {paradas} mm")
    lateral = perguntar_numero("Desvio do centro do robô para o lado no fim, em mm (+ = direita)", 0)
    bancada.aprovar_se("Execução, desvio lateral no fim", abs(lateral) <= ERRO_LATERAL_MM,
                       f"{lateral:+.0f} mm (limite ±{ERRO_LATERAL_MM:.0f}, abaixo da folga de 5,9 mm "
                       "do giro no lugar)")


def verificar_execucao_giros(bancada):
    titulo("8. Execução: girar 90° e 180° pelo giroscópio (#184)")
    pausar("Robô parado no centro de uma célula: o giroscópio vai ser calibrado (2 s).")
    calibracao = bancada.comando("CALIBRAR", "calibracao", espera_s=8)
    if calibracao is None or calibracao.get("estado") != "calibrado":
        bancada.registrar("Execução, giros", "REPROVADO", "o giroscópio não calibrou (MPU6050?)")
        return
    bancada.comando("ZERAR", "estado")
    for comando, nome, nominal in GIROS_EXECUTADOS:
        pausar(f"O robô vai girar {nome}. Marque a direção atual dele no chão (ou alinhe o "
               "transferidor).")
        fins, fim, _ = bancada.executar(comando, "fim_execucao", espera_s=8)
        if fim is None or fim.get("estado") != "concluido":
            falha = fim.get("falha") if fim else "sem resposta"
            bancada.registrar(f"Execução, giro {nome}", "REPROVADO", f"terminou em {falha}")
            continue
        real = perguntar_numero("Quantos graus ele girou de verdade (transferidor)", nominal)
        erro, ok = avaliar_giro(nominal, real)
        bancada.aprovar_se(f"Execução, giro {nome}", ok,
                           f"girou {real:.1f}°, erro {erro:+.1f}° (limite ±{GIRO_EXECUTADO_TOLERANCIA_GRAUS:.0f}°); "
                           f"o giroscópio mediu {abs(fim['giro_medido_graus']):.1f}° em {fim['tempo_ms']} ms")


def avaliar_desvio(medido_mm, real_mm):
    """Desvio lateral medido pelos ToF contra a régua: (erro em mm, aprovado)."""
    erro = medido_mm - real_mm
    return erro, abs(erro) <= ERRO_DESVIO_MEDIDO_MM


def avaliar_percurso(fim, linha_real, coluna_real):
    """Célula estimada no fim do percurso contra a real: (texto, aprovado)."""
    estimada = (fim["linha"], fim["coluna"])
    real = (int(linha_real), int(coluna_real))
    completo = fim["feitos"] == fim["movimentos"]
    texto = (f"estimada ({estimada[0]},{estimada[1]}) {fim['orientacao']}, real ({real[0]},{real[1]}); "
             f"{fim['feitos']} de {fim['movimentos']} movimentos, {fim['realinhamentos']} realinhamento(s)")
    if not completo:
        texto += f"; parou em {fim['falha']}"
    return texto, completo and estimada == real


def verificar_realinhamento(bancada):
    titulo("9. Desvio lateral e realinhamento ao centro (#188)")
    pausar("Robô parado no centro de uma célula: o giroscópio vai ser calibrado (2 s).")
    calibracao = bancada.comando("CALIBRAR", "calibracao", espera_s=8)
    if calibracao is None or calibracao.get("estado") != "calibrado":
        bancada.registrar("Realinhamento", "REPROVADO", "o giroscópio não calibrou (MPU6050?)")
        return
    pausar("Ponha o robô num corredor com parede dos dois lados e a célula de trás livre, alinhado, "
           "mas uns 8 mm fora do centro (para qualquer lado). Meça o desvio com a régua.")
    real = perguntar_numero("Desvio real do centro do robô, em mm (+ = direita)", 8)
    medido = bancada.comando("DESVIO", "desvio", espera_s=3)
    if medido is None or medido.get("referencia") == "sem_parede":
        bancada.registrar("Desvio lateral medido", "REPROVADO", "os ToF laterais não viram parede")
        return
    erro, ok = avaliar_desvio(medido["desvio_mm"], real)
    bancada.aprovar_se("Desvio lateral medido", ok,
                       f"medido {medido['desvio_mm']:+.1f} mm ({medido['referencia']}), régua {real:+.1f} mm, "
                       f"erro {erro:+.1f} mm (limite ±{ERRO_DESVIO_MEDIDO_MM:.0f})")
    pausar("O robô vai se realinhar: giro pequeno, ré, volta ao rumo e frente.")
    fins, fim, _ = bancada.executar("REALINHAR", "fim_execucao", espera_s=15)
    if fim is None or fim.get("estado") != "concluido":
        falha = fim.get("falha") if fim else "sem resposta"
        bancada.registrar("Realinhamento", "REPROVADO", f"terminou em {falha}")
        return
    final = perguntar_numero("Desvio do centro do robô depois do realinhamento, na régua, em mm", 0)
    bancada.aprovar_se("Realinhamento ao centro", abs(final) <= ERRO_REALINHAMENTO_MM,
                       f"de {real:+.1f} mm para {final:+.1f} mm (limite ±{ERRO_REALINHAMENTO_MM:.0f}) em "
                       f"{fim['tempo_ms']} ms; erro de rumo {fim['erro_rumo_graus']:+.1f}°")
    encostou = input("O chassi encostou em alguma parede? (s/N): ").strip().lower() == "s"
    bancada.aprovar_se("Realinhamento sem encostar", not encostou, "conferido a olho")


def verificar_percurso(bancada):
    titulo("10. Percurso: a célula estimada coincide com a real (#188)")
    print("  Sequência: A avança uma célula, D e E giram 90° (direita e esquerda), V gira 180°.")
    sequencia = input(f"Sequência do percurso [{PERCURSO_PADRAO}]: ").strip().upper() or PERCURSO_PADRAO
    pausar("Ponha o robô no centro da célula de partida (0, 0), virado para o Norte, num labirinto em "
           "que a sequência não bata em parede. O giroscópio vai ser calibrado (2 s).")
    calibracao = bancada.comando("CALIBRAR", "calibracao", espera_s=8)
    if calibracao is None or calibracao.get("estado") != "calibrado":
        bancada.registrar("Percurso", "REPROVADO", "o giroscópio não calibrou (MPU6050?)")
        return
    bancada.comando("ZERAR", "estado")
    _, fim, _ = bancada.executar(f"PERCURSO {sequencia}", "fim_percurso", espera_s=len(sequencia) * 15 + 10)
    if fim is None:
        return
    linha = perguntar_numero("Linha real da célula em que o robô parou (Norte aumenta)", fim["linha"])
    coluna = perguntar_numero("Coluna real (Leste aumenta)", fim["coluna"])
    texto, ok = avaliar_percurso(fim, linha, coluna)
    bancada.aprovar_se("Percurso, célula estimada x real", ok, f"{sequencia}: {texto}")


def movimento_livre(bancada):
    titulo("Movimento livre com log ao vivo")
    tipo = input("ANDAR ou GIRAR? [ANDAR]: ").strip().upper() or "ANDAR"
    if tipo not in ("ANDAR", "GIRAR"):
        return
    potencia = int(perguntar_numero("Potência (-1000 a 1000)", 400))
    duracao = int(perguntar_numero("Duração em ms (até 10000)", 1000))
    correcao = input("Correção pelo ToF ligada? (S/n): ").strip().lower() != "n"
    if input("Zerar a pose antes? (S/n): ").strip().lower() != "n":
        bancada.comando("ZERAR", "estado")
    bancada.mover(tipo, potencia, duracao, correcao)


def ver_estado(bancada):
    titulo("Estado atual (sem mover)")
    estado = bancada.comando("LER", "estado")
    if estado is not None:
        for chave in CAMPOS_CSV[6:17]:
            print(f"  {chave:16s} {estado.get(chave)}")


def anotar_condicoes(bancada):
    titulo("Condições do teste")
    for chave, pergunta in (
        ("responsaveis", "Quem executou o teste"),
        ("piso", "Piso (ex.: MDF do labirinto, mesa, chão da sala)"),
        ("bateria", "Bateria (tensão no início, se medida)"),
        ("montagem", "Montagem (ex.: chassi completo, rodas, posição dos ToF)"),
    ):
        valor = input(f"{pergunta} [{bancada.anotacoes.get(chave, '')}]: ").strip()
        if valor:
            bancada.anotacoes[chave] = valor


def salvar_relatorio(bancada):
    if not bancada.resultados:
        print("Nenhum resultado para salvar ainda.")
        return
    commit = subprocess.run(["git", "rev-parse", "--short", "HEAD"], cwd=PASTA_FIRMWARE,
                            capture_output=True, text=True).stdout.strip() or "?"
    agora = datetime.now()
    os.makedirs(bancada.pasta_logs, exist_ok=True)
    caminho = os.path.join(bancada.pasta_logs, "relatorio.md")
    linhas = [
        f"# Bancada do deslocamento sem encoder ({agora:%d/%m/%Y %H:%M})",
        "",
        f"- Commit do firmware: `{commit}` (ambiente `bancada_deslocamento`)",
        f"- Porta: `{bancada.porta}`",
    ]
    nomes = {"responsaveis": "Responsáveis", "piso": "Piso", "bateria": "Bateria",
             "montagem": "Montagem"}
    for chave, rotulo in nomes.items():
        linhas.append(f"- {rotulo}: {bancada.anotacoes.get(chave, 'não informado')}")
    if bancada.info:
        tabela = ", ".join(f"{p} → {v} mm/s" for p, v in bancada.info["tabela"])
        linhas.append(f"- Tabela de velocidade em uso: {tabela}")
    linhas += ["", "| Etapa | Resultado | Detalhe |", "|---|:---:|---|"]
    for etapa, situacao, detalhe in bancada.resultados:
        linhas.append(f"| {etapa} | {situacao} | {detalhe.replace('|', '/')} |")
    reprovados = sum(1 for _, s, _ in bancada.resultados if s == "REPROVADO")
    linhas += ["", f"**Total:** {len(bancada.resultados)} verificações, {reprovados} reprovadas."]
    if bancada.velocidades_medidas:
        linhas += ["", "**Tabela medida:**", "", "| Potência | mm/s medidos |", "|---:|---:|"]
        for potencia in sorted(bancada.velocidades_medidas):
            linhas.append(f"| {potencia} | {bancada.velocidades_medidas[potencia]:.0f} |")
    linhas += ["", f"Logs de cada movimento (CSV) na mesma pasta: `{bancada.pasta_logs}`."]
    with open(caminho, "w", encoding="utf-8") as arquivo:
        arquivo.write("\n".join(linhas) + "\n")
    print(f"{VERDE}Relatório salvo em {caminho}{NORMAL}")


def roteiro_completo(bancada):
    anotar_condicoes(bancada)
    for etapa in (verificar_estado_inicial, calibrar_tabela, verificar_uma_celula,
                  verificar_corredor, verificar_confianca, verificar_giro, verificar_execucao_celulas,
                  verificar_execucao_giros, verificar_realinhamento, verificar_percurso):
        if bancada.serial is None and not bancada.conectar():
            print(f"{VERMELHO}Sem conexão com a placa: roteiro interrompido.{NORMAL}")
            break
        etapa(bancada)
    reprovados = sum(1 for _, s, _ in bancada.resultados if s == "REPROVADO")
    cor = VERDE if reprovados == 0 else VERMELHO
    print(f"\n{cor}Roteiro concluído: {reprovados} verificação(ões) reprovada(s).{NORMAL}")
    salvar_relatorio(bancada)


# ---- menu ------------------------------------------------------------------------------------


def escolher_porta():
    portas = list(serial.tools.list_ports.comports())
    if not portas:
        print(f"{VERMELHO}Nenhuma porta serial encontrada. Conecte a placa pela USB.{NORMAL}")
        sys.exit(1)
    if len(portas) == 1:
        print(f"Porta encontrada: {portas[0].device} ({portas[0].description})")
        return portas[0].device
    for i, porta in enumerate(portas, 1):
        print(f"  {i}. {porta.device} ({porta.description})")
    while True:
        escolha = input("Número da porta da placa: ").strip()
        if escolha.isdigit() and 1 <= int(escolha) <= len(portas):
            return portas[int(escolha) - 1].device


MENU = (
    ("Gravar o firmware de bancada na placa", gravar_firmware),
    ("Conectar (firmware de bancada já gravado)", lambda b: b.conectar()),
    ("Roteiro completo (etapas 1 a 10 + relatório)", roteiro_completo),
    ("1. Estado inicial (sem mover)", verificar_estado_inicial),
    ("2. Tabela de velocidade e zona morta", calibrar_tabela),
    ("3. Avanço de uma célula", verificar_uma_celula),
    ("4. Corredor com correção pelo ToF", verificar_corredor),
    ("5. Confiança da pose sem correção", verificar_confianca),
    ("6. Giro de 90°", verificar_giro),
    ("7. Execução: avançar n células (#184)", verificar_execucao_celulas),
    ("8. Execução: giros de 90° e 180° (#184)", verificar_execucao_giros),
    ("9. Desvio lateral e realinhamento (#188)", verificar_realinhamento),
    ("10. Percurso: célula estimada x real (#188)", verificar_percurso),
    ("Movimento livre com log ao vivo", movimento_livre),
    ("Ver o estado atual", ver_estado),
    ("Anotar condições do teste", anotar_condicoes),
    ("Salvar relatório", salvar_relatorio),
)


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--porta", help="porta serial da placa (ex.: /dev/ttyACM0, COM5)")
    args = parser.parse_args()

    print("=" * 66)
    print("  BANCADA DO DESLOCAMENTO (#380), DOS MOVIMENTOS (#184) E DAS PAREDES (#188)")
    print("  As etapas acionam os motores: robô no chão, espaço livre.")
    print("  Ctrl+C durante um movimento freia os motores na hora.")
    print("=" * 66)
    bancada = Bancada(args.porta or escolher_porta())
    print("\nSe a placa ainda não tem o firmware de bancada, use a opção 1.")
    if not bancada.conectar():
        print("(Tudo bem se for a primeira vez: grave o firmware pela opção 1.)")

    while True:
        print("\n" + "-" * 66)
        for i, (texto, _) in enumerate(MENU, 1):
            print(f"  {i:2d}) {texto}")
        print("   0) Sair")
        escolha = input("Opção: ").strip()
        if escolha == "0":
            break
        if not escolha.isdigit() or not 1 <= int(escolha) <= len(MENU):
            continue
        try:
            MENU[int(escolha) - 1][1](bancada)
        except KeyboardInterrupt:
            if bancada.serial is not None:
                bancada.serial.write(b"\n")  # freia, se algo estiver girando
            print("\nEtapa interrompida.")
        except serial.SerialException as erro:
            print(f"{VERMELHO}Conexão perdida: {erro}. Use a opção 2 para reconectar.{NORMAL}")
            bancada.fechar()

    if bancada.resultados and input("Salvar o relatório antes de sair? (s/N): ").strip().lower() == "s":
        salvar_relatorio(bancada)
    if bancada.serial is not None:
        bancada.serial.write(b"\n")
    bancada.fechar()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print()
