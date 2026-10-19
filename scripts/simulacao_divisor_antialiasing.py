#!/usr/bin/env python3
"""
Simulação Analítica do Divisor Resistivo e Filtro Anti-Aliasing
Projeto Micromouse - PI1 (FGA/UnB) - Issue #203

Este script modela o condicionamento de sinal do canal ADC do ESP32-C3:
- Divisor resistivo R1 (100k) e R2 (33k)
- Filtro passa-baixas anti-aliasing RC com C1 (100nF)
- Resposta em frequencia (Diagrama de Bode de Magnitude e Fase)
- Rejeicao de ruido de comutacao PWM dos motores (20 kHz)
- Exporta grafico vetorial SVG para docs/figs/bode-filtro-antialiasing.svg
"""

import math
import os

# Parametros dos Componentes Dimensionados (Issue #203)
R1 = 100_000.0       # 100 kOhms (1% precisao)
R2 = 33_000.0        # 33 kOhms (1% precisao)
C1 = 100e-9          # 100 nF (Capacitor ceramico MLCC 50V)

# Resistencia equivalente Thevenin e Ganho DC
K_DIV = R2 / (R1 + R2)                      # Ganho estatico DC = 33 / 133 ~= 0.24812
R_TH = (R1 * R2) / (R1 + R2)               # Impedancia Thevenin ~= 24.812 kOhms
F_CORTE = 1.0 / (2.0 * math.pi * R_TH * C1) # Frequencia de corte (-3 dB) ~= 64.14 Hz

# Frequencia de comutacao PWM dos motores DC
F_PWM_MOTORES = 20_000.0  # 20 kHz
F_SW_BUCK = 150_000.0     # 150 kHz (Chaveamento do LM2596)


def calcular_resposta_frequencia(f):
    """
    Retorna o ganho em dB e a fase em graus para uma dada frequencia f (Hz).
    Funcao de transferencia: H(s) = K_div / (1 + s * R_th * C1)
    """
    if f <= 0:
        f = 1e-3
    w = 2.0 * math.pi * f
    w_c = 2.0 * math.pi * F_CORTE

    # Magnitude em relacao a entrada (incluindo o divisor resistivo)
    modulo_h = K_DIV / math.sqrt(1.0 + (w / w_c)**2)
    ganho_db = 20.0 * math.log10(modulo_h)

    # Magnitude normalizada em relacao a banda passante DC (0 dB em DC)
    ganho_rel_db = -20.0 * math.log10(math.sqrt(1.0 + (w / w_c)**2))

    # Fase em graus
    fase_graus = -math.degrees(math.atan(w / w_c))

    return ganho_db, ganho_rel_db, fase_graus


def gerar_grafico_bode_svg(caminho_svg):
    """
    Gera um diagrama de Bode vetorial (Magnitude e Fase) em SVG.
    """
    width = 820
    height = 540
    margin_left = 75
    margin_right = 45
    margin_top = 65
    margin_bottom = 60

    plot_width = width - margin_left - margin_right
    plot_height = height - margin_top - margin_bottom

    # Eixo X em escala logaritmica: 0.1 Hz ate 100 kHz (6 decadas)
    f_min = 0.1
    f_max = 100_000.0
    log_f_min = math.log10(f_min)
    log_f_max = math.log10(f_max)

    def x_pixel(freq):
        lf = math.log10(max(f_min, freq))
        return margin_left + ((lf - log_f_min) / (log_f_max - log_f_min)) * plot_width

    # Eixo Y de Magnitude: de 0 dB ate -70 dB (ganho relativo)
    db_max = 5.0
    db_min = -70.0

    def y_pixel(db_val):
        return margin_top + ((db_max - db_val) / (db_max - db_min)) * plot_height

    svg = []
    svg.append(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" width="100%" height="100%" style="font-family: -apple-system, BlinkMacSystemFont, \'Segoe UI\', Roboto, sans-serif; background: #ffffff;">')

    # Titulos
    svg.append(f'<text x="{width/2}" y="28" text-anchor="middle" font-size="16" font-weight="bold" fill="#0f172a">Diagrama de Bode — Filtro Passa-Baixas Anti-Aliasing (Issue #203)</text>')
    svg.append(f'<text x="{width/2}" y="48" text-anchor="middle" font-size="12" fill="#475569">Resposta em frequência da malha R_th = 24,81 kΩ e C1 = 100 nF (fc = {F_CORTE:.1f} Hz)</text>')

    # Linhas de grade verticais (decadas)
    decadas = [0.1, 1.0, 10.0, 100.0, 1_000.0, 10_000.0, 100_000.0]
    rotulos_decadas = ["0,1 Hz", "1 Hz", "10 Hz", "100 Hz", "1 kHz", "10 kHz", "100 kHz"]

    for d_val, d_label in zip(decadas, rotulos_decadas):
        xp = x_pixel(d_val)
        svg.append(f'<line x1="{xp:.1f}" y1="{margin_top}" x2="{xp:.1f}" y2="{margin_top + plot_height}" stroke="#cbd5e1" stroke-width="1" />')
        svg.append(f'<text x="{xp:.1f}" y="{margin_top + plot_height + 18}" text-anchor="middle" font-size="11" fill="#64748b">{d_label}</text>')
        # Sub-divisoes menores da decada
        for sub in range(2, 10):
            sub_val = d_val * sub
            if sub_val < f_max:
                x_sub = x_pixel(sub_val)
                svg.append(f'<line x1="{x_sub:.1f}" y1="{margin_top}" x2="{x_sub:.1f}" y2="{margin_top + plot_height}" stroke="#f1f5f9" stroke-width="0.8" />')

    # Linhas de grade horizontais (dB)
    for db_tick in range(-70, 10, 10):
        yp = y_pixel(db_tick)
        svg.append(f'<line x1="{margin_left}" y1="{yp:.1f}" x2="{margin_left + plot_width}" y2="{yp:.1f}" stroke="#e2e8f0" stroke-width="1" />')
        svg.append(f'<text x="{margin_left - 10}" y="{yp + 4:.1f}" text-anchor="end" font-size="11" fill="#64748b">{db_tick} dB</text>')

    # Linha dos eixos principais
    svg.append(f'<line x1="{margin_left}" y1="{margin_top}" x2="{margin_left}" y2="{margin_top + plot_height}" stroke="#334155" stroke-width="1.5" />')
    svg.append(f'<line x1="{margin_left}" y1="{margin_top + plot_height}" x2="{margin_left + plot_width}" y2="{margin_top + plot_height}" stroke="#334155" stroke-width="1.5" />')

    # Faixa da frequencia PWM dos motores (20 kHz)
    x_pwm = x_pixel(F_PWM_MOTORES)
    _, att_pwm, _ = calcular_resposta_frequencia(F_PWM_MOTORES)
    y_att_pwm = y_pixel(att_pwm)

    svg.append(f'<line x1="{x_pwm:.1f}" y1="{margin_top}" x2="{x_pwm:.1f}" y2="{margin_top + plot_height}" stroke="#ef4444" stroke-width="1.5" stroke-dasharray="4,4" />')
    svg.append(f'<text x="{x_pwm - 8:.1f}" y="{margin_top + 30}" text-anchor="end" font-size="11" font-weight="bold" fill="#dc2626">Ruído PWM Motores (20 kHz)</text>')

    # Marcador de fc (-3 dB)
    x_fc = x_pixel(F_CORTE)
    y_fc = y_pixel(-3.0)
    svg.append(f'<line x1="{x_fc:.1f}" y1="{margin_top}" x2="{x_fc:.1f}" y2="{margin_top + plot_height}" stroke="#0ea5e9" stroke-width="1.2" stroke-dasharray="3,3" />')
    svg.append(f'<line x1="{margin_left}" y1="{y_fc:.1f}" x2="{margin_left + plot_width}" y2="{y_fc:.1f}" stroke="#0ea5e9" stroke-width="1" stroke-dasharray="3,3" />')
    svg.append(f'<circle cx="{x_fc:.1f}" cy="{y_fc:.1f}" r="5" fill="#0284c7" stroke="#ffffff" stroke-width="2" />')
    svg.append(f'<text x="{x_fc + 8:.1f}" y="{y_fc - 8:.1f}" font-size="11" font-weight="bold" fill="#0369a1">fc = {F_CORTE:.1f} Hz (-3 dB)</text>')

    # Gerar curva de Magnitude (Bode)
    pontos_curva = []
    num_passos = 250
    for i in range(num_passos + 1):
        lf = log_f_min + (i / num_passos) * (log_f_max - log_f_min)
        freq = 10.0**lf
        _, ganho_rel, _ = calcular_resposta_frequencia(freq)
        pontos_curva.append((freq, ganho_rel))

    d_bode = " ".join(f"{'M' if i == 0 else 'L'} {x_pixel(p[0]):.1f},{y_pixel(p[1]):.1f}" for i, p in enumerate(pontos_curva))
    svg.append(f'<path d="{d_bode}" fill="none" stroke="#2563eb" stroke-width="3" />')

    # Ponto no ruído PWM
    svg.append(f'<circle cx="{x_pwm:.1f}" cy="{y_att_pwm:.1f}" r="6" fill="#dc2626" stroke="#ffffff" stroke-width="2" />')
    svg.append(f'<text x="{x_pwm + 10:.1f}" y="{y_att_pwm + 4:.1f}" font-size="11" font-weight="bold" fill="#b91c1c">{att_pwm:.1f} dB (Atenuação de {10**(-att_pwm/20):.0f}×)</text>')

    # Titulos dos eixos
    svg.append(f'<text x="{margin_left + plot_width/2}" y="{height - 18}" text-anchor="middle" font-size="12" font-weight="bold" fill="#334155">Frequência (Hz) — Escala Logarítmica</text>')
    svg.append(f'<text transform="rotate(-90)" x="{-margin_top - plot_height/2}" y="22" text-anchor="middle" font-size="12" font-weight="bold" fill="#334155">Ganho Relativo (dB)</text>')

    # Legenda explicativa
    leg_x = margin_left + 15
    leg_y = margin_top + plot_height - 75
    svg.append(f'<rect x="{leg_x}" y="{leg_y}" width="290" height="60" rx="4" fill="#ffffff" stroke="#cbd5e1" stroke-width="1" />')
    svg.append(f'<line x1="{leg_x + 10}" y1="{leg_y + 16}" x2="{leg_x + 35}" y2="{leg_y + 16}" stroke="#2563eb" stroke-width="3" />')
    svg.append(f'<text x="{leg_x + 42}" y="{leg_y + 20}" font-size="11" fill="#1e293b">Atenuação Passa-Baixas (Roll-off -20 dB/déc)</text>')
    svg.append(f'<circle cx="{leg_x + 22}" cy="{leg_y + 38}" r="4" fill="#dc2626" />')
    svg.append(f'<text x="{leg_x + 42}" y="{leg_y + 42}" font-size="11" fill="#991b1b">Rejeição a 20 kHz: -49,9 dB (redução de 312×)</text>')

    svg.append('</svg>')

    os.makedirs(os.path.dirname(caminho_svg), exist_ok=True)
    with open(caminho_svg, 'w', encoding='utf-8') as f:
        f.write("\n".join(svg))
    print(f"[OK] Diagrama de Bode exportado para: {caminho_svg}")


def exibir_tabela_atenuacao():
    """
    Exibe no terminal a tabela comparativa de atenuacao nas frequencias operacionais.
    """
    freqs_interesse = [
        ("DC / Banda Passante", 0.0),
        ("Frequencia de Corte (fc)", F_CORTE),
        ("Frequencia Dupla (2 fc)", 2.0 * F_CORTE),
        ("Ondulacao de Rede / Baixa", 120.0),
        ("Banda de Audio / Intermediaria", 1_000.0),
        ("Comutacao PWM Motores (DRV8833)", F_PWM_MOTORES),
        ("Chaveamento Regulador Buck LM2596", F_SW_BUCK)
    ]

    print("\n" + "="*80)
    print(" TABELA DE RESPOSTA EM FREQUENCIA — FILTRO PASSIVA RC ANTI-ALIASING")
    print(" R1 = 100 kOhm | R2 = 33 kOhm | C1 = 100 nF | R_th = 24.81 kOhm | fc = 64.14 Hz")
    print("="*80)
    print(f"{'Ponto de Frequencia':<36} | {'Freq (Hz)':<12} | {'Atenuacao':<12} | {'Fator de Reducao'}")
    print("-" * 80)

    for nome, freq in freqs_interesse:
        _, att_db, _ = calcular_resposta_frequencia(freq)
        fator_reducao = 10.0 ** (-att_db / 20.0)
        print(f"{nome:<36} | {freq:<12.1f} | {att_db:<12.2f} dB | {fator_reducao:<.1f}x")

    print("="*80 + "\n")


if __name__ == "__main__":
    exibir_tabela_atenuacao()
    caminho_svg = os.path.abspath("docs/figs/bode-filtro-antialiasing.svg")
    gerar_grafico_bode_svg(caminho_svg)
