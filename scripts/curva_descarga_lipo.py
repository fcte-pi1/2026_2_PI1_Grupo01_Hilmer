#!/usr/bin/env python3
"""
Mapeamento Teórico da Curva de Descarga - Bateria LiPo 2S 650 mAh (Tattu 75C)
Projeto Micromouse - PI1 (FGA/UnB) - Issue #139

Este script implementa o modelo eletroquimico de Shepherd / Tremblay-Dessaint
para caracterizar a celula de Polimero de Litio (LiPo) sob carga constante,
gerando a curva temporal de tensao, tabela de Estado de Carga (SoC)
e exportando o grafico vetorial SVG para a documentacao oficial.
"""

import math
import os

# Parametros Nominais do Pack LiPo 2S (Tattu 650mAh 75C)
CAPACIDADE_NOMINAL_AH = 0.650      # Q = 650 mAh = 0.650 Ah
NUM_CELULAS = 2                   # Topologia 2S1P
TENSAO_PLENA_CARGA_CELULA = 4.20  # 8.40 V no pack
TENSAO_CORTE_UVLO_CELULA = 3.30   # 6.60 V no pack (limiar logico de protecao)
RESISTENCIA_INTERNA_CELULA = 0.015 # R = 15 mOhm por celula (30 mOhm pack)

# Parametros do Modelo de Shepherd calibrados para LiPo (Tattu 650mAh)
# Curva: 8.4V plena -> 7.7V (80%) -> 7.4V (50% platô) -> 7.0V (20%) -> 6.6V (corte UVLO a ~7% residual)
E0 = 3.740                         # Tensao base no patamar nominal (V)
K = 0.085                          # Constante de polarizacao de fim de descarga (V)
A = 0.440                          # Amplitude da zona exponencial inicial (V)
B = 16.0                           # Coeficiente da zona exponencial (1/Ah)

# Corrente de Missao Nominal do Micromouse
# Buck 6V U4 (Motores N20) + Buck 5V U2 (ESP32-C3 Super Mini) + Buck 3V3 U5 (3x VL53L0X + MPU6050) -> ~3.15 W / 7.4 V = ~0.426 A
CORRENTE_MISSAO_A = 0.426


def calcular_tensao_celula(q_descarregada_ah, corrente_a):
    """
    Calcula a tensao nos terminais de uma celula LiPo individual
    pela equacao modificada de Shepherd.
    """
    q_util = max(0.0, min(q_descarregada_ah, CAPACIDADE_NOMINAL_AH * 0.999))
    q_total = CAPACIDADE_NOMINAL_AH

    # Termo constante e queda ohmica
    v_base = E0 - (RESISTENCIA_INTERNA_CELULA * corrente_a)

    # Termo de polarizacao (decaimento acelerado perto do esgotamento)
    # Evita singularidade com fator de amortecimento
    fracao_restante = max(0.005, (q_total - q_util) / q_total)
    v_pol = K * (1.0 / fracao_restante - 1.0) * (q_util / q_total)

    # Termo exponencial (decaimento rapido dos primeiros 15% de carga)
    v_exp = A * math.exp(-B * q_util)

    v_cell = v_base - v_pol + v_exp
    return max(2.80, min(TENSAO_PLENA_CARGA_CELULA, v_cell))


def calcular_tensao_pack(tempo_segundos, corrente_a):
    """
    Retorna a tensao total do pack 2S para um determinado instante de tempo.
    """
    q_d = corrente_a * (tempo_segundos / 3600.0)
    v_cell = calcular_tensao_celula(q_d, corrente_a)
    return v_cell * NUM_CELULAS


def gerar_grafico_svg(caminho_svg):
    """
    Gera um grafico vetorial SVG de alta qualidade da curva de descarga.
    """
    # Dimensoes do grafico
    width = 800
    height = 480
    margin_left = 70
    margin_right = 40
    margin_top = 60
    margin_bottom = 60

    plot_width = width - margin_left - margin_right
    plot_height = height - margin_top - margin_bottom

    # Limites dos eixos
    t_max_min = 85.0   # Minutos
    v_min = 6.0        # Volts
    v_max = 8.6        # Volts

    def x_pixel(t_min):
        return margin_left + (t_min / t_max_min) * plot_width

    def y_pixel(v_val):
        return margin_top + ((v_max - v_val) / (v_max - v_min)) * plot_height

    # Gerar trajetoria da curva nominal (426 mA)
    pontos_nominal = []
    tempo_passo = 30  # segundos
    t_seg = 0
    t_uvlo_nominal = None

    while True:
        t_min = t_seg / 60.0
        v_pack = calcular_tensao_pack(t_seg, CORRENTE_MISSAO_A)
        if t_min > t_max_min or v_pack <= 6.0:
            pontos_nominal.append((t_min, max(6.0, v_pack)))
            break
        if v_pack <= 6.60 and t_uvlo_nominal is None:
            t_uvlo_nominal = t_min
        pontos_nominal.append((t_min, v_pack))
        t_seg += tempo_passo

    # Gerar trajetoria de baixa corrente (150 mA - Standby / Sensor)
    pontos_baixa = []
    t_seg = 0
    while True:
        t_min = t_seg / 60.0
        v_pack = calcular_tensao_pack(t_seg, 0.150)
        if t_min > t_max_min or v_pack <= 6.0:
            pontos_baixa.append((t_min, max(6.0, v_pack)))
            break
        pontos_baixa.append((t_min, v_pack))
        t_seg += 60

    # Construir linhas SVG
    def path_data(pontos):
        return " ".join(f"{'M' if i == 0 else 'L'} {x_pixel(p[0]):.1f},{y_pixel(p[1]):.1f}" for i, p in enumerate(pontos))

    d_nom = path_data(pontos_nominal)
    d_low = path_data(pontos_baixa)

    # Iniciar SVG
    svg = []
    svg.append(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" width="100%" height="100%" style="font-family: -apple-system, BlinkMacSystemFont, \'Segoe UI\', Roboto, sans-serif; background: #ffffff;">')
    
    # Titulo
    svg.append(f'<text x="{width/2}" y="28" text-anchor="middle" font-size="16" font-weight="bold" fill="#1e293b">Mapeamento Teórico da Curva de Descarga — LiPo 2S 650 mAh (Tattu 75C)</text>')
    svg.append(f'<text x="{width/2}" y="46" text-anchor="middle" font-size="12" fill="#64748b">Modelo Eletroquímico de Shepherd para o perfil de missão do Micromouse</text>')

    # Linhas de grade e marcadores verticais
    for t_step in range(0, 90, 10):
        xp = x_pixel(t_step)
        svg.append(f'<line x1="{xp:.1f}" y1="{margin_top}" x2="{xp:.1f}" y2="{margin_top + plot_height}" stroke="#e2e8f0" stroke-width="1" />')
        svg.append(f'<text x="{xp:.1f}" y="{margin_top + plot_height + 18}" text-anchor="middle" font-size="11" fill="#64748b">{t_step}</text>')

    # Marcadores de tensao horizontais
    for v_step_int in range(60, 88, 4):
        v_step = v_step_int / 10.0
        yp = y_pixel(v_step)
        svg.append(f'<line x1="{margin_left}" y1="{yp:.1f}" x2="{margin_left + plot_width}" y2="{yp:.1f}" stroke="#e2e8f0" stroke-width="1" />')
        svg.append(f'<text x="{margin_left - 10}" y="{yp + 4:.1f}" text-anchor="end" font-size="11" fill="#64748b">{v_step:.1f} V</text>')

    # Regiao de seguranca da missao (0 a 30 min)
    x30 = x_pixel(30)
    svg.append(f'<rect x="{margin_left}" y="{margin_top}" width="{x30 - margin_left:.1f}" height="{plot_height}" fill="#22c55e" fill-opacity="0.08" />')
    svg.append(f'<line x1="{x30:.1f}" y1="{margin_top}" x2="{x30:.1f}" y2="{margin_top + plot_height}" stroke="#16a34a" stroke-width="1.5" stroke-dasharray="4,4" />')
    svg.append(f'<text x="{x30 - 6:.1f}" y="{margin_top + 20}" text-anchor="end" font-size="11" font-weight="bold" fill="#15803d">Meta da Missão RNF-04 (30 min)</text>')

    # Linha horizontal de corte UVLO (6,60 V)
    y_uvlo = y_pixel(6.60)
    svg.append(f'<line x1="{margin_left}" y1="{y_uvlo:.1f}" x2="{margin_left + plot_width}" y2="{y_uvlo:.1f}" stroke="#ef4444" stroke-width="1.5" stroke-dasharray="5,3" />')
    svg.append(f'<text x="{margin_left + plot_width - 10}" y="{y_uvlo - 6:.1f}" text-anchor="end" font-size="11" font-weight="bold" fill="#dc2626">Corte de Subtensão UVLO (6,60 V)</text>')

    # Linha dos eixos principais
    svg.append(f'<line x1="{margin_left}" y1="{margin_top}" x2="{margin_left}" y2="{margin_top + plot_height}" stroke="#475569" stroke-width="1.5" />')
    svg.append(f'<line x1="{margin_left}" y1="{margin_top + plot_height}" x2="{margin_left + plot_width}" y2="{margin_top + plot_height}" stroke="#475569" stroke-width="1.5" />')

    # Titulos dos eixos
    svg.append(f'<text x="{margin_left + plot_width/2}" y="{height - 18}" text-anchor="middle" font-size="12" font-weight="bold" fill="#334155">Tempo de Operação (minutos)</text>')
    svg.append(f'<text transform="rotate(-90)" x="{-margin_top - plot_height/2}" y="20" text-anchor="middle" font-size="12" font-weight="bold" fill="#334155">Tensão do Pack LiPo 2S (V)</text>')

    # Curvas de dados
    svg.append(f'<path d="{d_low}" fill="none" stroke="#94a3b8" stroke-width="2" stroke-dasharray="6,4" />')
    svg.append(f'<path d="{d_nom}" fill="none" stroke="#2563eb" stroke-width="3" />')

    # Marcador no ponto dos 30 minutos
    v30 = calcular_tensao_pack(30 * 60, CORRENTE_MISSAO_A)
    y30 = y_pixel(v30)
    svg.append(f'<circle cx="{x30:.1f}" cy="{y30:.1f}" r="5" fill="#2563eb" stroke="#ffffff" stroke-width="2" />')
    svg.append(f'<text x="{x30 + 10:.1f}" y="{y30 - 8:.1f}" font-size="11" font-weight="bold" fill="#1d4ed8">{v30:.2f} V aos 30 min (~67% SoC)</text>')

    # Marcador no ponto de corte UVLO
    if t_uvlo_nominal:
        x_uvlo = x_pixel(t_uvlo_nominal)
        svg.append(f'<circle cx="{x_uvlo:.1f}" cy="{y_uvlo:.1f}" r="5" fill="#dc2626" stroke="#ffffff" stroke-width="2" />')
        svg.append(f'<text x="{x_uvlo - 8:.1f}" y="{y_uvlo + 18:.1f}" text-anchor="end" font-size="11" font-weight="bold" fill="#dc2626">Corte aos {t_uvlo_nominal:.1f} min</text>')

    # Legenda
    leg_x = margin_left + 20
    leg_y = margin_top + plot_height - 60
    svg.append(f'<rect x="{leg_x}" y="{leg_y}" width="250" height="50" rx="4" fill="#ffffff" stroke="#cbd5e1" stroke-width="1" />')
    svg.append(f'<line x1="{leg_x + 10}" y1="{leg_y + 16}" x2="{leg_x + 35}" y2="{leg_y + 16}" stroke="#2563eb" stroke-width="3" />')
    svg.append(f'<text x="{leg_x + 42}" y="{leg_y + 20}" font-size="11" fill="#1e293b">Regime Nominal de Missão (426 mA)</text>')
    svg.append(f'<line x1="{leg_x + 10}" y1="{leg_y + 34}" x2="{leg_x + 35}" y2="{leg_y + 34}" stroke="#94a3b8" stroke-width="2" stroke-dasharray="5,3" />')
    svg.append(f'<text x="{leg_x + 42}" y="{leg_y + 38}" font-size="11" fill="#475569">Standby / Eletrônica Apenas (150 mA)</text>')

    svg.append('</svg>')

    os.makedirs(os.path.dirname(caminho_svg), exist_ok=True)
    with open(caminho_svg, 'w', encoding='utf-8') as f:
        f.write("\n".join(svg))
    print(f"[OK] Grafico vetorial exportado com sucesso para: {caminho_svg}")


def gerar_tabela_soc():
    """
    Imprime e retorna a tabela de mapeamento de Estado de Carga (SoC).
    """
    print("\n" + "="*70)
    print(" TABELA DE MAPEAMENTO: ESTADO DE CARGA (SoC) vs TENSAO LiPo 2S")
    print(" Pack: Tattu 650mAh 75C | Corrente Nominal: 426 mA")
    print("="*70)
    print(f"{'SoC (%)':<10} | {'V_pack Carga (V)':<18} | {'V_celula Carga (V)':<20} | {'Capacidade Rest. (mAh)'}")
    print("-" * 70)

    tabela = []
    # 100% ate 0% de 10 em 10
    for soc in range(100, -1, -10):
        q_d = CAPACIDADE_NOMINAL_AH * (1.0 - (soc / 100.0))
        # Para carga a 426 mA
        v_cell_carga = calcular_tensao_celula(q_d, CORRENTE_MISSAO_A)
        v_pack_carga = v_cell_carga * 2
        cap_rest = CAPACIDADE_NOMINAL_AH * (soc / 100.0) * 1000

        print(f"{soc:<10} | {v_pack_carga:<18.3f} | {v_cell_carga:<20.3f} | {cap_rest:<.1f} mAh")
        tabela.append((soc, v_pack_carga, v_cell_carga, cap_rest))

    print("="*70 + "\n")
    return tabela


def exportar_header_cpp(caminho_header, tabela):
    """
    Exporta uma estrutura de calibração em C++ para inclusão no firmware.
    """
    conteudo = [
        "// Lookup Table de Calibracao SoC vs Tensao LiPo 2S",
        "// Gerado automaticamente pelo script curva_descarga_lipo.py (Issue #139)",
        "#pragma once",
        "",
        "#include <stdint.h>",
        "",
        "struct PontoCalibracaoBateria {",
        "    uint8_t soc_percentual;",
        "    float tensao_pack_v;",
        "};",
        "",
        "static const PontoCalibracaoBateria TABELA_SOC_LIPO_2S[] = {"
    ]

    for soc, v_pack, _, _ in tabela:
        conteudo.append(f"    {{ {soc:3d}, {v_pack:.3f}f }},")

    conteudo.extend([
        "};",
        "",
        "static const uint8_t TAMANHO_TABELA_SOC = sizeof(TABELA_SOC_LIPO_2S) / sizeof(TABELA_SOC_LIPO_2S[0]);",
        ""
    ])

    os.makedirs(os.path.dirname(caminho_header), exist_ok=True)
    with open(caminho_header, 'w', encoding='utf-8') as f:
        f.write("\n".join(conteudo))
    print(f"[OK] Header C++ exportado para: {caminho_header}")


if __name__ == "__main__":
    tabela = gerar_tabela_soc()

    caminho_svg = os.path.abspath("docs/figs/curva-descarga-lipo-2s.svg")
    gerar_grafico_svg(caminho_svg)

    caminho_h = os.path.abspath("src/firmware/lib/simulacao/TabelaDescargaLiPo.h")
    exportar_header_cpp(caminho_h, tabela)
