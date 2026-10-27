"""Gráfico comparativo das rodadas de calibração do VL53L0X (dados reais da bancada de 26/10/2026)."""
import csv
import statistics as st
import sys
from collections import defaultdict

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

base, saida = sys.argv[1], sys.argv[2]
RODADAS = {
    "Calibração (crua)": (f"{base}/calibracao_20261026_192249/leituras_brutas.csv", 0),
    "Validação (crua)": (f"{base}/calibracao_VALIDACAO_desconto68_20261026_195302/leituras_brutas.csv", 68),
}
AZUL, LARANJA, AQUA = "#2a78d6", "#eb6834", "#1baf7a"
TINTA, TINTA2, GRADE, FUNDO = "#1f1f1e", "#5f5e58", "#e6e5e0", "#fcfcfb"


def medias_cruas(caminho, desconto):
    d = defaultdict(list)
    for linha in csv.DictReader(open(caminho)):
        d[int(linha["real_mm"])].append(float(linha["medido_mm"]) + desconto)  # volta à leitura crua
    return {k: (st.fmean(v), st.stdev(v)) for k, v in sorted(d.items())}


dados = {nome: medias_cruas(*arq) for nome, arq in RODADAS.items()}

# Reta de correção com as duas rodadas, de 5 a 30 cm (abaixo de 5 cm o sensor não mede)
xs, ys = [], []
for m in dados.values():
    for real, (media, _) in m.items():
        if real >= 50:
            xs.append(media)
            ys.append(real)
mx, my = st.fmean(xs), st.fmean(ys)
a = sum((x - mx) * (y - my) for x, y in zip(xs, ys)) / sum((x - mx) ** 2 for x in xs)
b = my - a * mx
print(f"reta: real = {a:.4f} * cru {b:+.1f}")

plt.rcParams.update({"font.size": 10, "axes.edgecolor": TINTA2, "axes.labelcolor": TINTA, "xtick.color": TINTA2,
                     "ytick.color": TINTA2, "text.color": TINTA})
fig, (e1, e2) = plt.subplots(1, 2, figsize=(13, 4.8), facecolor=FUNDO)
for eixo in (e1, e2):
    eixo.set_facecolor(FUNDO)
    eixo.grid(color=GRADE, lw=0.8)
    eixo.spines[["top", "right"]].set_visible(False)

# A: leitura crua × distância real
e1.plot([0, 320], [0, 320], "--", color=TINTA2, lw=1, label="ideal (leitura = régua)")
for (nome, m), cor, marca in zip(dados.items(), (AZUL, LARANJA), ("o", "s")):
    reais = list(m)
    e1.errorbar(reais, [m[r][0] for r in reais], yerr=[m[r][1] for r in reais], fmt=marca + "-", color=cor, lw=2,
                ms=8, capsize=3, label=nome)
e1.set_title("A. Leitura crua do sensor × régua", loc="left", fontsize=11)
e1.set_xlabel("Distância real (mm)")
e1.set_ylabel("Leitura do sensor (mm)")
e1.annotate("~70 a 86 mm acima da régua", xy=(150, 232), xytext=(170, 120), color=TINTA2, fontsize=9,
            arrowprops={"arrowstyle": "->", "color": TINTA2})
e1.legend(frameon=False, fontsize=9, loc="upper left")

# B: erro depois de cada correção (média das duas rodadas)
reais = [50, 100, 150, 200, 300]
tol = [max(10, 0.05 * r) for r in reais]
e2.fill_between(reais, [-t for t in tol], tol, color=AQUA, alpha=0.10, lw=0, label="tolerância (±10 mm ou ±5%)")
e2.axhline(0, color=TINTA2, lw=1)
correcoes = {
    "desconto fixo de 68 mm": lambda c: c - 68,
    "desconto fixo de 78 mm": lambda c: c - 78,
    f"reta: real = {a:.3f}·cru {b:+.1f}": lambda c: a * c + b,
}
for (nome, f), cor, marca in zip(correcoes.items(), (LARANJA, AZUL, AQUA), ("s", "o", "^")):
    erros = [st.fmean(f(m[r][0]) for m in dados.values()) - r for r in reais]
    e2.plot(reais, erros, marca + "-", color=cor, lw=2, ms=8, label=nome)
e2.set_title("B. Erro depois da correção (média das 2 rodadas)", loc="left", fontsize=11)
e2.set_xlabel("Distância real (mm)")
e2.set_ylabel("Erro = corrigido − régua (mm)")
e2.legend(frameon=False, fontsize=9, loc="lower left")

fig.suptitle("VL53L0X (sensor da frente): calibração com régua, 26/10/2026 — 100 leituras por ponto", x=0.01,
             ha="left", fontsize=12)
fig.tight_layout()
fig.savefig(saida, dpi=150, facecolor=FUNDO)
print("salvo em", saida)
