#!/usr/bin/env python3
"""Menu de bancada dos 3 sensores VL53L0X (issues #344 e #175).

Conduz, junto com a eletrônica, a verificação do driver `TofVl53l0x` no hardware montado:
grava o firmware de bancada (ambiente `bancada_tof_devkit`), confere a fiação de cada XSHUT, a
atribuição dos endereços, a taxa de medição, o tempo da leitura, o erro contra a régua e o
comportamento fora da faixa. Cada etapa sai como APROVADO/REPROVADO/OBSERVADO e pode ser salva
num relatório em Markdown para registrar em `docs/07-testes/software.md`.

Uso (a partir de `src/firmware`):
    ./scripts/bancada_tof.py              # escolhe a porta no menu
    ./scripts/bancada_tof.py --porta /dev/ttyUSB0

Requer `pyserial` (pip install pyserial) e, para gravar, o PlatformIO (`pio`).
"""

import argparse
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
LADOS = ("frente", "esquerda", "direita")
LETRA_DO_LADO = {"frente": "F", "esquerda": "E", "direita": "D"}

ENDERECO_FABRICA = 0x29
ENDERECO_MPU6050 = 0x68
ENDERECOS_ESPERADOS = {"frente": 0x31, "esquerda": 0x30, "direita": 0x32}

# Critérios provisórios: ajuste aqui se a equipe combinar outros valores.
TAXA_MINIMA_HZ = 20.0  # 33 ms de tempo de medição dão cerca de 30 Hz por sensor
TAXA_MAXIMA_HZ = 40.0
TEMPO_LEITURA_MAXIMO_US = 5000  # metade do ciclo de 10 ms da navegação
LEITURAS_VALIDAS_MINIMO = 0.95  # fração das leituras válidas com obstáculo à frente dos 3
TOLERANCIA_REGUA_MM = 10.0  # erro aceito contra a régua: o maior entre 10 mm e 5%
TOLERANCIA_REGUA_PERC = 5.0
# Faixa do `ConfiguracaoTof.h`: fora dela, a leitura vale com a distância limitada a estes valores.
DISTANCIA_MINIMA_MM = 30
DISTANCIA_MAXIMA_MM = 1200

VERDE = "\033[32m" if sys.stdout.isatty() else ""
VERMELHO = "\033[31m" if sys.stdout.isatty() else ""
AMARELO = "\033[33m" if sys.stdout.isatty() else ""
NORMAL = "\033[0m" if sys.stdout.isatty() else ""


class Bancada:
    """Conexão com o firmware de bancada e registro dos resultados."""

    def __init__(self, porta):
        self.porta = porta
        self.serial = None
        self.resultados = []  # (etapa, situação, detalhe)
        self.anotacoes = {}

    # ---- conexão ---------------------------------------------------------------------------

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
        if info is None or info.get("programa") != "bancada_tof":
            print(f"{VERMELHO}A placa não respondeu como firmware de bancada. "
                  f"Grave-o pela opção 1 do menu.{NORMAL}")
            return False
        print(f"{VERDE}Conectado ao firmware de bancada em {self.porta}.{NORMAL}")
        return True

    def fechar(self):
        if self.serial is not None:
            self.serial.close()
            self.serial = None

    def comando(self, texto, tipo, espera_s=5.0, ao_receber=None):
        """Envia um comando e devolve a primeira resposta JSON com o `tipo` pedido.

        `ao_receber` é chamado com cada outra linha JSON recebida enquanto espera (usado no FLUXO).
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

    # ---- registro --------------------------------------------------------------------------

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


def perguntar_numero(mensagem, padrao):
    texto = input(f"{mensagem} [{padrao}]: ").strip().replace(",", ".")
    if not texto:
        return padrao
    try:
        return float(texto)
    except ValueError:
        print("Valor inválido, usando o padrão.")
        return padrao


def hexa(enderecos):
    return ", ".join(f"0x{e:02X}" for e in enderecos) or "nenhum"


def titulo(texto):
    print(f"\n=== {texto} " + "=" * max(0, 60 - len(texto)))


# ---- etapas ----------------------------------------------------------------------------------


def gravar_firmware(bancada):
    titulo("Gravar o firmware de bancada")
    pio = shutil.which("pio") or os.path.expanduser("~/.platformio/penv/bin/pio")
    if not os.path.exists(pio) and shutil.which("pio") is None:
        print(f"{VERMELHO}PlatformIO (pio) não encontrado. Instale com: pipx install platformio{NORMAL}")
        return
    bancada.fechar()
    comando = [pio, "run", "-e", "bancada_tof_devkit", "-t", "upload", "--upload-port", bancada.porta]
    print("Executando:", " ".join(comando))
    resultado = subprocess.run(comando, cwd=PASTA_FIRMWARE)
    if resultado.returncode != 0:
        print(f"{VERMELHO}A gravação falhou. Se a placa não entrar em modo de gravação, segure o "
              f"botão BOOT, aperte RESET e tente de novo.{NORMAL}")
        return
    print("Aguardando a placa reiniciar...")
    time.sleep(3)
    bancada.conectar()


def verificar_fiacao(bancada):
    titulo("1. Fiação: cada XSHUT liga só o seu sensor")
    print("Cada sensor é ligado sozinho; ele deve aparecer no endereço de fábrica (0x29).")
    aprovado = True

    nenhum = bancada.comando("ISOLAR N", "varredura")
    if nenhum is None:
        return False
    enderecos = nenhum["enderecos"]
    mpu = "presente" if ENDERECO_MPU6050 in enderecos else "AUSENTE"
    print(f"  Todos em reset: {hexa(enderecos)} (MPU6050 0x68: {mpu}, apenas informativo)")
    aprovado &= bancada.aprovar_se(
        "Todos os ToF em reset", ENDERECO_FABRICA not in enderecos,
        "nenhum sensor no 0x29" if ENDERECO_FABRICA not in enderecos
        else "algum sensor responde com XSHUT baixo: XSHUT solto ou ligado no pino errado")

    for lado in LADOS:
        resposta = bancada.comando(f"ISOLAR {LETRA_DO_LADO[lado]}", "varredura")
        if resposta is None:
            return False
        enderecos = resposta["enderecos"]
        aprovado &= bancada.aprovar_se(
            f"XSHUT do sensor {lado}", ENDERECO_FABRICA in enderecos,
            f"varredura: {hexa(enderecos)}" + (
                "" if ENDERECO_FABRICA in enderecos
                else " (sensor não respondeu: confira XSHUT, alimentação, SDA e SCL)"))

    # Os sensores ficaram em reset: volta ao estado normal.
    bancada.comando("INIT", "inicio")
    return aprovado


def verificar_inicializacao(bancada):
    titulo("2. Inicialização e endereços")
    inicio = bancada.comando("INIT", "inicio")
    if inicio is None:
        return False
    estados = ", ".join(f"{lado} {'ok' if inicio[lado] else 'FALHOU'}" for lado in LADOS)
    aprovado = bancada.aprovar_se("Os 3 sensores iniciam", inicio["ok"],
                                  f"{estados}; levou {inicio['duracao_ms']} ms")

    varredura = bancada.comando("SCAN", "varredura")
    if varredura is None:
        return False
    enderecos = varredura["enderecos"]
    for lado in LADOS:
        esperado = ENDERECOS_ESPERADOS[lado]
        aprovado &= bancada.aprovar_se(f"Endereço do sensor {lado}", esperado in enderecos,
                                       f"esperado 0x{esperado:02X}; varredura: {hexa(enderecos)}")
    aprovado &= bancada.aprovar_se("Endereço de fábrica livre", ENDERECO_FABRICA not in enderecos,
                                   "0x29 não aparece depois do INIT" if ENDERECO_FABRICA not in enderecos
                                   else "algum sensor ficou no 0x29")
    return aprovado


def verificar_taxa_e_tempo(bancada):
    titulo("3. Taxa de medição, tempo da leitura e leituras válidas")
    pausar("Coloque um obstáculo entre 10 e 30 cm na frente de CADA um dos 3 sensores.")
    estat = bancada.comando("ESTAT 5", "estatisticas", espera_s=10)
    if estat is None:
        return False
    aprovado = True
    for lado in LADOS:
        hz = estat[lado]["hz"]
        aprovado &= bancada.aprovar_se(
            f"Taxa do sensor {lado}", TAXA_MINIMA_HZ <= hz <= TAXA_MAXIMA_HZ,
            f"{hz:.1f} Hz (esperado {TAXA_MINIMA_HZ:.0f} a {TAXA_MAXIMA_HZ:.0f} Hz)")
    tempo_max = estat["tempo_leitura_us_max"]
    aprovado &= bancada.aprovar_se(
        "Leitura não bloqueia", tempo_max <= TEMPO_LEITURA_MAXIMO_US,
        f"médio {estat['tempo_leitura_us_medio']:.0f} µs, máximo {tempo_max} µs "
        f"(limite {TEMPO_LEITURA_MAXIMO_US} µs)")
    fracao = estat["leituras_validas"] / max(1, estat["leituras"])
    aprovado &= bancada.aprovar_se(
        "Leituras válidas", fracao >= LEITURAS_VALIDAS_MINIMO,
        f"{fracao * 100:.1f}% de {estat['leituras']} leituras (mínimo "
        f"{LEITURAS_VALIDAS_MINIMO * 100:.0f}%)")
    return aprovado


def verificar_regua(bancada):
    titulo("4. Erro contra a régua")
    print("Para cada sensor, coloque uma parede (de preferência do labirinto) a uma distância")
    print("conhecida, medida da FACE do sensor. A #175 pede pelo menos 5 distâncias por sensor.")
    print(f"Tolerância provisória: o maior entre {TOLERANCIA_REGUA_MM:.0f} mm e "
          f"{TOLERANCIA_REGUA_PERC:.0f}% da distância.")
    aprovado = True
    for lado in LADOS:
        while True:
            texto = input(f"\nSensor {lado}: distância da régua em mm (ENTER para o próximo "
                          f"sensor): ").strip().replace(",", ".")
            if not texto:
                break
            try:
                referencia = float(texto)
            except ValueError:
                print("Valor inválido.")
                continue
            estat = bancada.comando("ESTAT 3", "estatisticas", espera_s=8)
            if estat is None:
                return False
            dados = estat[lado]
            if dados["validas"] == 0:
                aprovado &= bancada.aprovar_se(
                    f"Régua {lado} {referencia:.0f} mm", False,
                    f"nenhuma amostra válida ({dados['abaixo_do_minimo']} abaixo do mínimo, "
                    f"{dados['fora_de_alcance']} fora de alcance, {dados['falhas']} falhas)")
                continue
            erro = dados["media_mm"] - referencia
            tolerancia = max(TOLERANCIA_REGUA_MM, referencia * TOLERANCIA_REGUA_PERC / 100)
            aprovado &= bancada.aprovar_se(
                f"Régua {lado} {referencia:.0f} mm", abs(erro) <= tolerancia,
                f"média {dados['media_mm']:.1f} mm, erro {erro:+.1f} mm (±{tolerancia:.0f}), "
                f"desvio {dados['desvio_mm']:.1f} mm, faixa {dados['min_mm']}-{dados['max_mm']} mm, "
                f"{dados['validas']} amostras")
    return aprovado


def verificar_fora_da_faixa(bancada):
    titulo("5. Fora da faixa: a leitura vale, limitada à faixa, com o motivo na amostra do sensor")
    aprovado = True
    for lado in LADOS:
        for situacao, instrucao, esperado, limitada in (
            ("sem alvo", f"Aponte o sensor {lado} para um espaço aberto, sem nada a menos de 1,5 m.",
             "fora_de_alcance", DISTANCIA_MAXIMA_MM),
            ("muito perto", f"Encoste um obstáculo a menos de 2 cm do sensor {lado}.",
             "abaixo_do_minimo", DISTANCIA_MINIMA_MM),
        ):
            pausar(instrucao + " Os outros dois sensores devem ficar a menos de 1 m de um obstáculo.")
            time.sleep(0.3)
            leitura = bancada.comando("LER", "leitura")
            if leitura is None:
                return False
            resultado, detalhe = avaliar_fora_da_faixa(leitura, lado, esperado, limitada)
            # Estado diferente do esperado não é defeito do driver: é o comportamento real do sensor,
            # que a #175 quer registrar para a percepção (#379).
            bancada.registrar(f"{lado}, {situacao}", resultado, detalhe)
            aprovado = aprovado and resultado != "REPROVADO"
    return aprovado


def avaliar_fora_da_faixa(leitura, lado, esperado, limitada):
    """Classifica o passo 5 para um lado: (resultado, detalhe).

    APROVADO quando o sensor deu o estado esperado e a leitura valeu com a distância limitada;
    REPROVADO quando o estado veio certo, mas a leitura foi recusada ou não veio limitada (defeito
    do driver); OBSERVADO quando o sensor deu outro estado.
    """
    estado = leitura[lado]["estado"]
    mm = leitura[lado]["mm"]
    entregue = leitura.get("limitada", {}).get(lado)
    if estado != esperado:
        return "OBSERVADO", (f"esperado {esperado}; veio {estado} ({mm} mm), leitura "
                             f"{'válida' if leitura['valida'] else 'inválida'}")
    if leitura["valida"] and entregue == limitada:
        return "APROVADO", f"estado {estado} ({mm} mm brutos), leitura válida com {entregue} mm"
    return "REPROVADO", (f"estado {estado} ({mm} mm brutos), mas a leitura veio "
                         f"{'válida' if leitura['valida'] else 'inválida'} com {entregue} mm; "
                         f"esperado válida com {limitada} mm")


def verificar_sensor_ausente(bancada):
    titulo("6. (Opcional) Sensor ausente na inicialização")
    print("Confere que um sensor que não responde não impede os outros de iniciar.")
    print("ATENÇÃO: desligue a placa da USB antes de tirar o sensor. Não desconecte com ela ligada.")
    lado = input("Qual sensor será desconectado? (frente/esquerda/direita, ENTER para pular): ").strip()
    if lado not in LADOS:
        print("Etapa pulada.")
        return True
    bancada.fechar()
    pausar(f"Com a placa DESLIGADA, desconecte o sensor {lado}. Depois religue a USB.")
    time.sleep(2)
    if not bancada.conectar():
        return False
    inicio = bancada.comando("INIT", "inicio")
    if inicio is None:
        return False
    outros = [o for o in LADOS if o != lado]
    aprovado = bancada.aprovar_se(
        f"Sem o sensor {lado}, os outros iniciam",
        not inicio[lado] and all(inicio[o] for o in outros),
        ", ".join(f"{o} {'ok' if inicio[o] else 'falhou'}" for o in LADOS))
    leitura = bancada.comando("LER", "leitura")
    if leitura is not None:
        aprovado &= bancada.aprovar_se(
            "Leitura falha sem um sensor", not leitura["valida"] and leitura[lado]["estado"] == "falha",
            f"válida={leitura['valida']}, estado do {lado}: {leitura[lado]['estado']}")
    bancada.fechar()
    pausar(f"Desligue a placa, reconecte o sensor {lado} e religue a USB.")
    time.sleep(2)
    bancada.conectar()
    return aprovado


def leitura_ao_vivo(bancada):
    titulo("Leitura ao vivo (Ctrl+C para parar)")
    segundos = int(perguntar_numero("Duração em segundos", 30))

    def mostrar(dados):
        if dados.get("tipo") != "leitura":
            return
        partes = []
        for lado in LADOS:
            amostra = dados[lado]
            marca = "" if amostra["estado"] == "valida" else f" ({amostra['estado']})"
            partes.append(f"{lado[:3]} {amostra['mm']:5d} mm{marca}")
        situacao = f"{VERDE}válida{NORMAL}" if dados["valida"] else f"{VERMELHO}inválida{NORMAL}"
        print(f"\r  {' | '.join(partes)} | {situacao} | idade {dados['idade_ms']} ms    ",
              end="", flush=True)

    try:
        bancada.comando(f"FLUXO {segundos}", "fim_fluxo", espera_s=segundos + 3, ao_receber=mostrar)
    except KeyboardInterrupt:
        bancada.serial.write(b"\n")  # qualquer byte interrompe o FLUXO na placa
        time.sleep(0.3)
    print()


def anotar_condicoes(bancada):
    titulo("Condições do teste (pedidas pela #175)")
    for chave, pergunta in (
        ("responsaveis", "Quem executou o teste"),
        ("cor_parede", "Cor/material da parede usada"),
        ("luz_ambiente", "Luz ambiente (ex.: sala com luz fluorescente, luz do dia)"),
        ("montagem", "Montagem (ex.: sensores soltos na protoboard, montados no chassi)"),
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
    nome = f"bancada_tof_{agora.strftime('%Y%m%d_%H%M%S')}.md"
    caminho = input(f"Arquivo do relatório [{nome}]: ").strip() or nome
    linhas = [
        f"# Bancada dos VL53L0X ({agora.strftime('%d/%m/%Y %H:%M')})",
        "",
        f"- Commit do firmware: `{commit}` (ambiente `bancada_tof_devkit`)",
        f"- Porta: `{bancada.porta}`",
    ]
    nomes = {"responsaveis": "Responsáveis", "cor_parede": "Cor da parede",
             "luz_ambiente": "Luz ambiente", "montagem": "Montagem"}
    for chave, rotulo in nomes.items():
        linhas.append(f"- {rotulo}: {bancada.anotacoes.get(chave, 'não informado')}")
    linhas += ["", "| Etapa | Resultado | Detalhe |", "|---|:---:|---|"]
    for etapa, situacao, detalhe in bancada.resultados:
        linhas.append(f"| {etapa} | {situacao} | {detalhe.replace('|', '/')} |")
    reprovados = sum(1 for _, s, _ in bancada.resultados if s == "REPROVADO")
    linhas += ["", f"**Total:** {len(bancada.resultados)} verificações, {reprovados} reprovadas."]
    with open(caminho, "w", encoding="utf-8") as arquivo:
        arquivo.write("\n".join(linhas) + "\n")
    print(f"{VERDE}Relatório salvo em {os.path.abspath(caminho)}{NORMAL}")


def roteiro_completo(bancada):
    anotar_condicoes(bancada)
    for etapa in (verificar_fiacao, verificar_inicializacao, verificar_taxa_e_tempo,
                  verificar_regua, verificar_fora_da_faixa, verificar_sensor_ausente):
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
    ("Roteiro completo (etapas 1 a 6 + relatório)", roteiro_completo),
    ("1. Fiação de cada XSHUT", verificar_fiacao),
    ("2. Inicialização e endereços", verificar_inicializacao),
    ("3. Taxa, tempo da leitura e leituras válidas", verificar_taxa_e_tempo),
    ("4. Erro contra a régua", verificar_regua),
    ("5. Fora da faixa (sem alvo e muito perto)", verificar_fora_da_faixa),
    ("6. Sensor ausente na inicialização (opcional)", verificar_sensor_ausente),
    ("Leitura ao vivo", leitura_ao_vivo),
    ("Anotar condições do teste", anotar_condicoes),
    ("Salvar relatório", salvar_relatorio),
)


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--porta", help="porta serial da placa (ex.: /dev/ttyUSB0, COM5)")
    args = parser.parse_args()

    print("=" * 66)
    print("  BANCADA DOS SENSORES VL53L0X (#344, #175)")
    print("  Os motores ficam freados durante todo o teste.")
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
            print("\nEtapa interrompida.")
        except serial.SerialException as erro:
            print(f"{VERMELHO}Conexão perdida: {erro}. Use a opção 2 para reconectar.{NORMAL}")
            bancada.fechar()

    if bancada.resultados:
        if input("Salvar o relatório antes de sair? (s/N): ").strip().lower() == "s":
            salvar_relatorio(bancada)
    bancada.fechar()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print()
