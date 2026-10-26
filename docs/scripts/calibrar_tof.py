#!/usr/bin/env python3
"""Calibração estatística de um VL53L0X com régua (bancada do Grupo 01).

Lê as linhas CSV do firmware do repositório micromouse (src/main.cpp), em que a 11ª coluna
(índice 10) é a distância do sensor da frente em mm (-1 = sem alvo, -2 = sensor não respondeu).

Para cada distância da régua, coleta N leituras válidas e salva:
  - leituras_brutas.csv  todas as leituras, uma por linha
  - resumo.csv           média, desvio, mín., máx., mediana, erro e % de leituras válidas
  - calibracao.png       gráfico: medido × real, erro × distância e dispersão
  - relatorio.md         tabela, reta de correção (real = a · medido + b) e R²

O desconto padrão é 68 mm, o mesmo do visualizador (escolha da equipe). A calibração de 26/10/2026
(pasta calibracao_20261026_192249) mediu ~78 mm a mais, constante de 5 a 30 cm.
Use --desconto 0 para uma calibração nova, com as leituras cruas.

Uso:
  .venv/bin/python calibrar_tof.py                       # validação: desconto de 68 mm, de 5 a 30 cm
  .venv/bin/python calibrar_tof.py --desconto 0          # calibração: leituras cruas
  .venv/bin/python calibrar_tof.py --porta /dev/cu.usbmodem1101
  .venv/bin/python calibrar_tof.py --distancias 50,100,200,400 --amostras 200
  .venv/bin/python calibrar_tof.py --simular             # sem placa, só para conferir o script
  .venv/bin/python calibrar_tof.py --visualizador        # com o visualizador aberto ao mesmo tempo
"""
import argparse
import csv
import datetime
import glob
import pathlib
import random
import statistics
import sys
import time

DISTANCIAS_PADRAO = [50, 100, 150, 200, 300]  # faixa usada pelo robô; abaixo de ~5 cm o sensor não mede
DESCONTO_PADRAO_MM = 68  # escolhido pela equipe; a calibração calibracao_20261026_192249 mediu ~78
TOLERANCIA_MM = 10      # critério da bancada do projeto: ±10 mm ou ±5%, o que for maior
TOLERANCIA_RELATIVA = 0.05
LINHAS_DESCARTADAS = 10  # leituras logo depois de mexer na parede


def achar_porta():
    portas = sorted(glob.glob("/dev/cu.usbmodem*") + glob.glob("/dev/ttyACM*") + glob.glob("/dev/ttyUSB*"))
    if not portas:
        sys.exit("Nenhuma placa encontrada. Confira o cabo USB-C (precisa ser de dados) e use --porta.")
    if len(portas) > 1:
        print("Mais de uma porta encontrada:", ", ".join(portas), "- usando", portas[0], "(ou use --porta)")
    return portas[0]


class FonteSerial:
    def __init__(self, porta, coluna):
        import serial  # só precisa do pyserial com a placa

        self.coluna = coluna
        self.conexao = serial.Serial(porta, 115200, timeout=1)
        time.sleep(2)  # a placa pode reiniciar ao abrir a porta

    def ler(self):
        """Devolve a próxima distância (int) ou None se a linha não for de dados."""
        linha = self.conexao.readline().decode("utf-8", errors="ignore").strip()
        campos = linha.split(",")
        if len(campos) <= self.coluna:
            if linha:
                print("  [placa]", linha)  # mensagens de início, por exemplo "VL53L0X frente ok"
            return None
        try:
            return int(float(campos[self.coluna]))
        except ValueError:
            return None

    def limpar(self):
        """Joga fora tudo o que se acumulou na porta enquanto o script esperava o ENTER."""
        self.conexao.reset_input_buffer()
        timeout = self.conexao.timeout
        self.conexao.timeout = 0.05
        fim = time.time() + 0.3
        while time.time() < fim:
            self.conexao.read(self.conexao.in_waiting or 1)
        self.conexao.timeout = timeout
        self.conexao.readline()  # termina a linha que estava no meio


class FonteVisualizador:
    """Lê as linhas pelo servidor do visualizador (http://localhost:8000/dados), que é quem abre a porta
    USB. Assim o visualizador continua aberto, mostrando ao vivo, enquanto o script coleta."""

    def __init__(self, url, coluna):
        self.url = url.rstrip("/") + "/dados"
        self.coluna = coluna
        self.resposta = None
        self.limpar()

    def limpar(self):
        # Reconectar descarta as linhas antigas da fila do servidor: só chegam leituras de agora.
        import urllib.request

        if self.resposta is not None:
            self.resposta.close()
        try:
            self.resposta = urllib.request.urlopen(self.url, timeout=5)
        except OSError as erro:
            sys.exit(f"Não consegui falar com o visualizador em {self.url} ({erro}). "
                     "Ele está rodando? (visualizador/servidor.py)")

    def ler(self):
        linha = self.resposta.readline().decode("utf-8", errors="ignore").strip()
        if not linha.startswith("data:"):
            return None
        campos = linha[5:].strip().split(",")
        if len(campos) <= self.coluna:
            return None
        try:
            return int(float(campos[self.coluna]))
        except ValueError:
            return None


class FonteSimulada:
    """Leituras inventadas (SIMULAÇÃO), só para testar o script sem a placa."""

    def __init__(self):
        self.real = 0

    def ler(self):
        time.sleep(0.002)
        if random.random() < 0.02:
            return -1
        return int(round(self.real * 1.03 + 12 + random.gauss(0, 2 + self.real * 0.01)))

    def limpar(self):
        pass


DESCONTO_MM = 0  # definido por --desconto: subtraído de cada leitura válida


def com_desconto(valor):
    """Desconta só leituras válidas: os negativos da placa são códigos (-1 sem alvo, -2 sem sensor).
    Uma leitura válida pode ficar negativa depois do desconto e continua valendo."""
    return valor - DESCONTO_MM if valor is not None and valor >= 0 else valor


def ver_ao_vivo(fonte, segundos=4):
    """Mostra as leituras por alguns segundos, para conferir se o sensor vê o alvo antes de medir."""
    fonte.limpar()
    fim = time.time() + segundos
    vistas = []
    while time.time() < fim:
        cru = fonte.ler()
        if cru is None:
            continue
        vistas.append((cru, com_desconto(cru)))
        if len(vistas) % 10 == 0:
            print(f"  ao vivo: {vistas[-1][1]} mm" + (f" (cru {cru} mm)" if DESCONTO_MM and cru >= 0 else ""))
    validas = [corrigido for cru, corrigido in vistas if cru >= 0]
    taxa = len(vistas) / segundos
    if taxa > 80:
        print(f"  AVISO: {taxa:.0f} leituras/s é mais que o firmware envia (50/s): ainda há leituras antigas na porta.")
    if validas:
        print(f"  -> média ao vivo {statistics.fmean(validas):.0f} mm ({len(validas)} leituras, {taxa:.0f}/s)")
    else:
        print("  -> nenhuma leitura válida: nada no alcance ou sensor sem resposta")


def coletar(fonte, real, amostras, tempo_maximo_s):
    fonte.limpar()
    descartadas = 0
    while descartadas < LINHAS_DESCARTADAS:
        if fonte.ler() is not None:
            descartadas += 1
    validas, invalidas = [], 0
    inicio = time.time()
    while len(validas) < amostras and time.time() - inicio < tempo_maximo_s:
        cru = fonte.ler()
        if cru is None:
            continue
        if cru < 0:
            invalidas += 1
            continue
        valor = com_desconto(cru)
        validas.append(valor)
        if len(validas) % 20 == 0:
            print(f"  {len(validas)}/{amostras} leituras (última {valor} mm)")
    return validas, invalidas


def resumir(real, validas, invalidas):
    total = len(validas) + invalidas
    media = statistics.fmean(validas)
    erro = media - real
    tolerancia = max(TOLERANCIA_MM, TOLERANCIA_RELATIVA * real)
    return {
        "real_mm": real,
        "n_validas": len(validas),
        "pct_validas": round(100 * len(validas) / total, 1) if total else 0.0,
        "media_crua_mm": round(media + DESCONTO_MM, 2),
        "media_mm": round(media, 2),
        "desvio_mm": round(statistics.stdev(validas), 2) if len(validas) > 1 else 0.0,
        "min_mm": min(validas),
        "max_mm": max(validas),
        "mediana_mm": statistics.median(validas),
        "erro_mm": round(erro, 2),
        "erro_pct": round(100 * erro / real, 2),
        "tolerancia_mm": round(tolerancia, 1),
        "dentro_da_tolerancia": "sim" if abs(erro) <= tolerancia else "não",
    }


def ajustar_reta(xs, ys):
    """Mínimos quadrados de y = a·x + b, com o R²."""
    mx, my = statistics.fmean(xs), statistics.fmean(ys)
    sxx = sum((x - mx) ** 2 for x in xs)
    a = sum((x - mx) * (y - my) for x, y in zip(xs, ys)) / sxx
    b = my - a * mx
    ss_res = sum((y - (a * x + b)) ** 2 for x, y in zip(xs, ys))
    ss_tot = sum((y - my) ** 2 for y in ys)
    return a, b, 1 - ss_res / ss_tot if ss_tot else 1.0


def desenhar(pasta, resumo, brutas, titulo):
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    reais = [r["real_mm"] for r in resumo]
    medias = [r["media_mm"] for r in resumo]
    desvios = [r["desvio_mm"] for r in resumo]
    erros = [r["erro_mm"] for r in resumo]
    tolerancias = [r["tolerancia_mm"] for r in resumo]

    fig, eixos = plt.subplots(1, 3, figsize=(17, 5.2))
    fig.suptitle(titulo)

    ax = eixos[0]
    ax.errorbar(reais, medias, yerr=desvios, fmt="o", capsize=4, label="média ± desvio")
    limite = max(reais + medias) * 1.05
    ax.plot([0, limite], [0, limite], "--", color="gray", label="ideal (medido = real)")
    if len(reais) >= 2:
        a, b, r2 = ajustar_reta(reais, medias)
        ax.plot([0, limite], [b, a * limite + b], color="tab:red", label=f"ajuste: medido = {a:.3f}·real {b:+.1f} (R² {r2:.4f})")
    ax.set_xlabel("Distância real na régua (mm)")
    ax.set_ylabel("Distância medida pelo sensor (mm)")
    ax.set_title("Medido × real")
    ax.grid(alpha=0.3)
    ax.legend(fontsize=8)

    ax = eixos[1]
    ax.errorbar(reais, erros, yerr=desvios, fmt="o", capsize=4, color="tab:orange", label="erro médio ± desvio")
    ax.fill_between(reais, [-t for t in tolerancias], tolerancias, color="tab:green", alpha=0.12,
                    label="tolerância (±10 mm ou ±5%)")
    ax.axhline(0, color="gray", lw=1)
    ax.set_xlabel("Distância real na régua (mm)")
    ax.set_ylabel("Erro = medido − real (mm)")
    ax.set_title("Erro por distância")
    ax.grid(alpha=0.3)
    ax.legend(fontsize=8)

    ax = eixos[2]
    grupos = [[v - r for v in brutas[r]] for r in reais]
    ax.boxplot(grupos, tick_labels=[str(r) for r in reais])
    ax.axhline(0, color="gray", lw=1)
    ax.set_xlabel("Distância real na régua (mm)")
    ax.set_ylabel("Erro de cada leitura (mm)")
    ax.set_title("Dispersão das leituras")
    ax.grid(alpha=0.3)

    fig.tight_layout()
    caminho = pasta / "calibracao.png"
    fig.savefig(caminho, dpi=150)
    return caminho


def escrever_relatorio(pasta, resumo, args, simulado, porta):
    reais = [r["real_mm"] for r in resumo]
    medias = [r["media_mm"] for r in resumo]
    linhas = [f"# Calibração do VL53L0X com régua{' (SIMULAÇÃO: dados inventados)' if simulado else ''}", ""]
    linhas += [
        f"- Data: {datetime.datetime.now():%d/%m/%Y %H:%M}",
        f"- Porta: {porta}",
        f"- Leituras válidas por distância: {args.amostras} (descartadas as {LINHAS_DESCARTADAS} primeiras depois de cada ajuste)",
        f"- Tolerância: ±{TOLERANCIA_MM} mm ou ±{TOLERANCIA_RELATIVA:.0%}, o que for maior",
        f"- Desconto aplicado em cada leitura: {DESCONTO_MM:g} mm" + (" (rodada de validação)" if DESCONTO_MM else " (nenhum: rodada de calibração)"),
        "",
        "| Real (mm) | Média crua (mm) | Média corrigida (mm) | Desvio (mm) | Mín. | Máx. | Mediana | Erro (mm) | Erro (%) | Válidas | Na tolerância |",
        "|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|:-:|",
    ]
    for r in resumo:
        linhas.append(
            f"| {r['real_mm']} | {r['media_crua_mm']} | {r['media_mm']} | {r['desvio_mm']} | {r['min_mm']} | {r['max_mm']} | {r['mediana_mm']} "
            f"| {r['erro_mm']:+} | {r['erro_pct']:+} | {r['pct_validas']}% | {r['dentro_da_tolerancia']} |"
        )
    if len(reais) >= 2:
        a, b, r2 = ajustar_reta(reais, medias)
        ac, bc, _ = ajustar_reta(medias, reais)
        linhas += [
            "",
            f"**Resposta do sensor:** medido = {a:.4f} · real {b:+.2f} mm (R² = {r2:.5f}).",
            "",
            f"**Correção sugerida:** real = {ac:.4f} · medido {bc:+.2f} mm.",
        ]
    linhas += ["", "![Gráfico da calibração](calibracao.png)", ""]
    (pasta / "relatorio.md").write_text("\n".join(linhas), encoding="utf-8")


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--porta", help="porta serial (padrão: a primeira /dev/cu.usbmodem*)")
    p.add_argument("--coluna", type=int, default=10, help="índice da coluna da distância (10 = sensor da frente)")
    p.add_argument("--amostras", type=int, default=100, help="leituras válidas por distância")
    p.add_argument("--distancias", default=",".join(map(str, DISTANCIAS_PADRAO)), help="distâncias da régua, em mm")
    p.add_argument("--tempo-maximo", type=float, default=30, help="segundos máximos por distância")
    p.add_argument("--simular", action="store_true", help="sem placa: gera leituras inventadas")
    p.add_argument("--desconto", type=float, default=DESCONTO_PADRAO_MM,
                   help=f"mm subtraídos de cada leitura (padrão {DESCONTO_PADRAO_MM}; 0 = leituras cruas)")
    p.add_argument("--visualizador", nargs="?", const="http://localhost:8000", metavar="URL",
                   help="lê pelo servidor do visualizador (que precisa estar rodando), em vez da porta USB")
    args = p.parse_args()

    global DESCONTO_MM
    DESCONTO_MM = args.desconto
    distancias = [int(d) for d in args.distancias.split(",") if d.strip()]
    if DESCONTO_MM:
        print(f"Validação: {DESCONTO_MM:g} mm descontados de cada leitura.")
    if args.simular:
        fonte, porta = FonteSimulada(), "simulação"
    elif args.visualizador:
        porta = f"visualizador ({args.visualizador})"
        print("Lendo pelo visualizador em", args.visualizador, "...")
        fonte = FonteVisualizador(args.visualizador, args.coluna)
    else:
        porta = args.porta or achar_porta()
        print("Conectando em", porta, "...")
        fonte = FonteSerial(porta, args.coluna)

    marcador = ("SIMULADA_" if args.simular else "") + (f"VALIDACAO_desconto{DESCONTO_MM:g}_" if DESCONTO_MM else "")
    pasta = pathlib.Path(f"calibracao_{marcador}{datetime.datetime.now():%Y%m%d_%H%M%S}")
    pasta.mkdir()  # removida no fim se nenhuma distância for medida
    brutas, resumo = {}, []
    with open(pasta / "leituras_brutas.csv", "w", newline="") as arquivo:
        saida = csv.writer(arquivo)
        saida.writerow(["real_mm", "amostra", "medido_mm"])
        for real in distancias:
            print(f"\n=== {real} mm ===")
            if not args.simular:
                while True:
                    resposta = input(f"Coloque o alvo a {real} mm ({real / 10:g} cm) da FACE do sensor e deixe tudo "
                                     "parado. ENTER mede, 'v' vê ao vivo, 'p' pula, 'q' encerra: ").strip().lower()
                    if resposta != "v":
                        break
                    ver_ao_vivo(fonte)
                if resposta == "q":
                    break
                if resposta == "p":
                    continue
            else:
                fonte.real = real
            try:
                validas, invalidas = coletar(fonte, real, args.amostras, args.tempo_maximo)
            except Exception as erro:  # placa desconectada ou porta usada por outro programa
                print(f"\n  A conexão com a placa caiu ({erro}).")
                print("  Confira o cabo e se nenhum outro programa (visualizador, monitor serial) está aberto.")
                print("  Salvando o que já foi medido.")
                break
            if not validas:
                print(f"  Nenhuma leitura válida ({invalidas} inválidas): parede fora do alcance ou sensor sem resposta.")
                continue
            if len(validas) < args.amostras:
                print(f"  Só {len(validas)} leituras válidas no tempo máximo ({invalidas} inválidas).")
            brutas[real] = validas
            for i, v in enumerate(validas, 1):
                saida.writerow([real, i, v])
            r = resumir(real, validas, invalidas)
            resumo.append(r)
            print(f"  média {r['media_mm']} mm, desvio {r['desvio_mm']} mm, erro {r['erro_mm']:+} mm "
                  f"({r['erro_pct']:+}%), válidas {r['pct_validas']}% -> tolerância: {r['dentro_da_tolerancia']}")
            if abs(r["erro_pct"]) > 30:
                print("  ATENÇÃO: a média está muito longe da régua. O sensor pode estar vendo outra coisa (mesa,"
                      " régua, fios) ou o alvo não estava na marca. Use 'v' para conferir antes de medir.")

    if not resumo:
        (pasta / "leituras_brutas.csv").unlink(missing_ok=True)
        pasta.rmdir()
        sys.exit("Nenhuma distância medida: nada foi salvo.")
    with open(pasta / "resumo.csv", "w", newline="") as arquivo:
        saida = csv.DictWriter(arquivo, fieldnames=list(resumo[0].keys()))
        saida.writeheader()
        saida.writerows(resumo)
    titulo = "Calibração do VL53L0X com régua" + (f" — validação com desconto de {DESCONTO_MM:g} mm" if DESCONTO_MM else "") \
        + (" — SIMULAÇÃO (dados inventados)" if args.simular else "")
    grafico = desenhar(pasta, resumo, brutas, titulo)
    escrever_relatorio(pasta, resumo, args, args.simular, porta)
    print(f"\nPronto. Resultados em {pasta}/: leituras_brutas.csv, resumo.csv, relatorio.md e {grafico.name}")


if __name__ == "__main__":
    main()
