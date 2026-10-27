"""Gráficos do teste de integração dos sensores I²C (26/10/2026).

Uso: python grafico_integracao_i2c.py <pasta dos registros> <pasta de saída>
Lê serial.csv (saída crua do firmware) e painel_1hz.csv (valores transcritos do visualizador).
"""
import csv
import math
import statistics as st
import sys

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

registros, saida = sys.argv[1], sys.argv[2]
PERIODO_S = 0.020  # o firmware envia uma linha a cada 20 ms
AZUL, LARANJA, AQUA = "#2a78d6", "#eb6834", "#1baf7a"
TINTA, TINTA2, GRADE, FUNDO = "#1f1f1e", "#5f5e58", "#e6e5e0", "#fcfcfb"
SENSORES = (("frente", AZUL, "o"), ("esquerda", LARANJA, "s"), ("direita", AQUA, "^"))

plt.rcParams.update({"font.size": 10, "axes.edgecolor": TINTA2, "axes.labelcolor": TINTA, "xtick.color": TINTA2,
                     "ytick.color": TINTA2, "text.color": TINTA})


def eixos(n):
    fig, ax = plt.subplots(1, n, figsize=(13, 4.6), facecolor=FUNDO)
    for e in ax:
        e.set_facecolor(FUNDO)
        e.grid(color=GRADE, lw=0.8)
        e.spines[["top", "right"]].set_visible(False)
    return fig, ax


def yaw(w, x, y, z):
    return math.degrees(math.atan2(2 * (w * z + x * y), 1 - 2 * (y * y + z * z)))


# --- serial cru: robô parado, anteparo da frente em três posições ---
serial = [{k: float(v) for k, v in linha.items()} for linha in csv.DictReader(open(f"{registros}/serial.csv"))]
t = [a["amostra"] * PERIODO_S for a in serial]
fig, (e1, e2) = eixos(2)
for nome, cor, _ in SENSORES:
    e1.plot(t, [a[f"{nome}_mm"] for a in serial], color=cor, lw=1.6, label=nome)
PATAMARES = ((0, 220), (262, 356), (405, len(serial)))
for ini, fim in PATAMARES:
    v = [a["frente_mm"] for a in serial[ini:fim]]
    e1.annotate(f"{st.fmean(v):.0f} ± {st.pstdev(v):.0f} mm", xy=(t[ini], max(v) + 15), color=TINTA2, fontsize=9)
e1.set_title("A. Distância crua dos 3 VL53L0X (serial)", loc="left", fontsize=11)
e1.set_xlabel("Tempo desde o início do registro (s, 20 ms por amostra)")
e1.set_ylabel("Leitura crua (mm)")
e1.set_ylim(0, 900)
e1.legend(frameon=False, fontsize=9, loc="center right")

e2.plot(t, [yaw(a["w"], a["x"], a["y"], a["z"]) - yaw(serial[0]["w"], serial[0]["x"], serial[0]["y"], serial[0]["z"])
            for a in serial], color=AZUL, lw=1.6, label="variação do yaw (°)")
e2.plot(t, [a["gz_dps"] for a in serial], color=LARANJA, lw=1, label="giroscópio Z (°/s)")
e2.plot(t, [math.sqrt(a["ax_g"] ** 2 + a["ay_g"] ** 2 + a["az_g"] ** 2) - 1 for a in serial], color=AQUA, lw=1,
        label="|aceleração| − 1 g")
e2.axhline(0, color=TINTA2, lw=1)
e2.set_title("B. MPU6050 parado durante o mesmo registro", loc="left", fontsize=11)
e2.set_xlabel("Tempo desde o início do registro (s, 20 ms por amostra)")
e2.set_ylabel("Valor")
e2.set_ylim(-1, 1)
e2.legend(frameon=False, fontsize=9, loc="lower left")
fig.suptitle("Integração I²C, 26/10/2026: saída serial crua, 499 amostras", x=0.01, ha="left", fontsize=12)
fig.tight_layout()
fig.savefig(f"{saida}/integracao-i2c-serial.png", dpi=150, facecolor=FUNDO)

# --- painel do visualizador a 1 leitura/s: protoboard girada à mão ---
painel = list(csv.DictReader(open(f"{registros}/painel_1hz.csv")))
tp = [float(a["tempo_s"]) for a in painel]
fig, (e1, e2) = eixos(2)
for nome, cor, marca in (("yaw", AZUL, "o"), ("pitch", LARANJA, "s"), ("roll", AQUA, "^")):
    e1.plot(tp, [float(a[f"{nome}_graus"]) for a in painel], marca + "-", color=cor, lw=1.6, ms=6, label=nome)
e1.axhline(0, color=TINTA2, lw=1)
e1.set_title("C. Orientação exibida no painel", loc="left", fontsize=11)
e1.set_xlabel("Tempo do vídeo (s)")
e1.set_ylabel("Ângulo (graus)")
e1.legend(frameon=False, fontsize=9, loc="lower left")

for nome, cor, marca in SENSORES:
    # "livre" vira NaN para a linha não ligar leituras separadas por um instante sem parede
    valores = [math.nan if a[f"{nome}_painel_mm"] == "livre" else float(a[f"{nome}_painel_mm"]) for a in painel]
    e2.plot(tp, valores, marca + "-", color=cor, lw=1.6, ms=6, label=nome)
livres = [x for x, a in zip(tp, painel) if a["frente_painel_mm"] == "livre"]
e2.plot(livres, [1150] * len(livres), "x", color=AZUL, ms=8, label="frente: livre (> 2 m)")
e2.set_title("D. Distância exibida no painel (já com −68 mm)", loc="left", fontsize=11)
e2.set_xlabel("Tempo do vídeo (s)")
e2.set_ylabel("Distância no painel (mm)")
e2.legend(frameon=False, fontsize=9, loc="center right")
fig.suptitle("Integração I²C, 26/10/2026: painel do visualizador transcrito a 1 leitura/s", x=0.01, ha="left",
             fontsize=12)
fig.tight_layout()
fig.savefig(f"{saida}/integracao-i2c-painel.png", dpi=150, facecolor=FUNDO)

# --- números usados na página ---
for nome, _, _ in SENSORES:
    v = [a[f"{nome}_mm"] for a in serial]
    print(f"{nome}: min {min(v):.0f} max {max(v):.0f} media {st.fmean(v):.1f} dp {st.pstdev(v):.1f}")
for ini, fim in PATAMARES:
    print(f"patamar {ini}-{fim - 1} ({t[ini]:.1f}-{t[fim - 1]:.1f} s):",
          {n: (round(st.fmean(a[f'{n}_mm'] for a in serial[ini:fim]), 1),
               round(st.pstdev([a[f'{n}_mm'] for a in serial[ini:fim]]), 1)) for n, _, _ in SENSORES})
for c in ("ax_g", "ay_g", "az_g", "gx_dps", "gy_dps", "gz_dps"):
    v = [a[c] for a in serial]
    print(f"{c}: media {st.fmean(v):.3f} dp {st.pstdev(v):.3f} min {min(v)} max {max(v)}")
y = [yaw(a["w"], a["x"], a["y"], a["z"]) for a in serial]
print(f"yaw: inicio {y[0]:.2f} fim {y[-1]:.2f} min {min(y):.2f} max {max(y):.2f}")
print("repetidas (3 distancias iguais a anterior):",
      sum(1 for a, b in zip(serial, serial[1:]) if all(a[f"{n}_mm"] == b[f"{n}_mm"] for n, _, _ in SENSORES)))
print("codigos negativos:", sum(1 for a in serial for n, _, _ in SENSORES if a[f"{n}_mm"] < 0))
