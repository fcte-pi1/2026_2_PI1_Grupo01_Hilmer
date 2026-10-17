#!/usr/bin/env python3
"""Script de calibração dos encoders e teste de deslocamento de 18 cm (Issue #186).

Conecta à porta Serial do ESP32-C3, recebe as leituras em formato JSON,
registra o deslocamento percorrido pelo robô em bancada ao longo de 180 mm (18 cm),
calcula o erro percentual/absoluto frente à tolerância de ±3% e sugere a nova
constante de calibração em micrômetros por pulso.
"""

import argparse
import json
import os
import sys
import time
from datetime import datetime

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("ERRO: Pacote 'pyserial' não encontrado. Instale com: pip install pyserial")
    sys.exit(1)

DISTANCIA_ALVO_MM = 180.0
TOLERANCIA_MAX_PERC = 3.0  # ±3%


def listar_portas_disponiveis():
    portas = [p.device for p in serial.tools.list_ports.comports()]
    return portas


def executar_teste(porta_serial, baudrate, distancia_alvo_mm, tolerancia_perc, arquivo_saida):
    print(f"\n=======================================================")
    print(f"  TESTE DE DESLOCAMENTO E CALIBRAÇÃO DOS ENCODERS")
    print(f"  Distância de referência: {distancia_alvo_mm:.1f} mm (18 cm)")
    print(f"  Tolerância máxima: ±{tolerancia_perc:.1f}% (±{(distancia_alvo_mm * tolerancia_perc / 100.0):.2f} mm)")
    print(f"  Porta Serial: {porta_serial} @ {baudrate} baud")
    print(f"=======================================================\n")

    try:
        ser = serial.Serial(porta_serial, baudrate, timeout=1.0)
    except serial.SerialException as e:
        print(f"ERRO: Não foi possível abrir a porta {porta_serial}: {e}")
        sys.exit(1)

    time.sleep(1.5)  # Aguarda estabilização da conexão
    ser.reset_input_buffer()

    print("Instruções:")
    print("1. Posicione o micromouse no início da régua/guia (marca 0 mm).")
    print("2. Pressione ENTER para zerar os encoders e iniciar a captura...")
    input()

    # Envia comando de zerar encoders se suportado pela interface serial
    ser.write(b"Z\n")
    ser.flush()
    time.sleep(0.2)
    ser.reset_input_buffer()

    print("\n>>> Movimente manualmente o robô até a marca de 180 mm (18 cm).")
    print(">>> Pressione CTRL+C ou aguarde a estabilização para finalizar a medição.\n")

    amostras = []
    ultima_leitura = None

    try:
        while True:
            linha = ser.readline().decode("utf-8", errors="ignore").strip()
            if not linha:
                continue

            # Procura por JSON na linha
            try:
                dados = json.loads(linha)
                if "esq_mm" in dados or "esq_pulsos" in dados:
                    ultima_leitura = dados
                    amostras.append({"timestamp": time.time(), "dados": dados})
                    esq_mm = dados.get("esq_mm", 0)
                    dir_mm = dados.get("dir_mm", 0)
                    medio_mm = dados.get("medio_mm", (esq_mm + dir_mm) // 2)
                    esq_p = dados.get("esq_pulsos", 0)
                    dir_p = dados.get("dir_pulsos", 0)

                    print(
                        f"\r[AO VIVO] Esq: {esq_mm:4d} mm ({esq_p:5d} p) | "
                        f"Dir: {dir_mm:4d} mm ({dir_p:5d} p) | "
                        f"Médio: {medio_mm:4d} mm",
                        end="",
                        flush=True,
                    )
            except json.JSONDecodeError:
                # Linha de log comum
                continue

    except KeyboardInterrupt:
        print("\n\nMedição concluída pelo operador!")

    ser.close()

    if not ultima_leitura:
        print("ERRO: Nenhuma leitura válida em JSON foi recebida pela Serial.")
        sys.exit(1)

    # Processamento dos resultados
    esq_final_mm = ultima_leitura.get("esq_mm", 0)
    dir_final_mm = ultima_leitura.get("dir_mm", 0)
    medio_final_mm = ultima_leitura.get("medio_mm", (esq_final_mm + dir_final_mm) / 2.0)
    esq_pulsos = abs(ultima_leitura.get("esq_pulsos", 0))
    dir_pulsos = abs(ultima_leitura.get("dir_pulsos", 0))

    erro_abs_esq = esq_final_mm - distancia_alvo_mm
    erro_rel_esq = (abs(erro_abs_esq) / distancia_alvo_mm) * 100.0

    erro_abs_dir = dir_final_mm - distancia_alvo_mm
    erro_rel_dir = (abs(erro_abs_dir) / distancia_alvo_mm) * 100.0

    erro_abs_medio = medio_final_mm - distancia_alvo_mm
    erro_rel_medio = (abs(erro_abs_medio) / distancia_alvo_mm) * 100.0

    aprovado = (
        erro_rel_esq <= tolerancia_perc
        and erro_rel_dir <= tolerancia_perc
        and erro_rel_medio <= tolerancia_perc
    )

    # Cálculo da calibração sugerida (distancia_alvo_mm * 1000 / pulsos)
    calib_esq = int(round((distancia_alvo_mm * 1000.0) / esq_pulsos)) if esq_pulsos > 0 else 0
    calib_dir = int(round((distancia_alvo_mm * 1000.0) / dir_pulsos)) if dir_pulsos > 0 else 0
    calib_media = int(round((calib_esq + calib_dir) / 2.0)) if (calib_esq and calib_dir) else 0

    relatorio = f"""
=======================================================
               RELATÓRIO DO TESTE (18 cm)
=======================================================
Data/Hora: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
Distância de Referência: {distancia_alvo_mm:.1f} mm

[Roda Esquerda]
  Pulsos Contados: {esq_pulsos}
  Distância Medida: {esq_final_mm:.1f} mm
  Erro Absoluto: {erro_abs_esq:+.2f} mm
  Erro Relativo: {erro_rel_esq:.2f}%
  Calibração Sugerida: {calib_esq} µm/pulso

[Roda Direita]
  Pulsos Contados: {dir_pulsos}
  Distância Medida: {dir_final_mm:.1f} mm
  Erro Absoluto: {erro_abs_dir:+.2f} mm
  Erro Relativo: {erro_rel_dir:.2f}%
  Calibração Sugerida: {calib_dir} µm/pulso

[Deslocamento Médio do Robô]
  Distância Média Medida: {medio_final_mm:.1f} mm
  Erro Absoluto Médio: {erro_abs_medio:+.2f} mm
  Erro Relativo Médio: {erro_rel_medio:.2f}%
  Calibração Média Recomendada: {calib_media} µm/pulso

-------------------------------------------------------
Critério de Aceitação (±{tolerancia_perc:.1f}%): {'[APROVADO]' if aprovado else '[REPROVADO]'}
=======================================================
"""
    print(relatorio)

    if arquivo_saida:
        os.makedirs(os.path.dirname(os.path.abspath(arquivo_saida)), exist_ok=True)
        with open(arquivo_saida, "w", encoding="utf-8") as f:
            f.write(relatorio)
        print(f"Relatório salvo em: {arquivo_saida}")


def main():
    parser = argparse.ArgumentParser(
        description="Validação do deslocamento de 18 cm e calibração de encoders."
    )
    portas = listar_portas_disponiveis()
    porta_padrao = portas[0] if portas else "/dev/ttyUSB0"

    parser.add_argument(
        "-p", "--porta", default=porta_padrao, help=f"Porta serial (padrão: {porta_padrao})"
    )
    parser.add_argument("-b", "--baud", type=int, default=115200, help="Baud rate (padrão: 115200)")
    parser.add_argument(
        "-d", "--distancia", type=float, default=DISTANCIA_ALVO_MM, help="Distância alvo em mm (padrão: 180.0)"
    )
    parser.add_argument(
        "-t", "--tolerancia", type=float, default=TOLERANCIA_MAX_PERC, help="Tolerância de erro em %% (padrão: 3.0)"
    )
    parser.add_argument(
        "-o",
        "--saida",
        default="docs/07-testes/resultados/relatorio_encoders_18cm.txt",
        help="Caminho do arquivo para salvar o relatório",
    )

    args = parser.parse_args()
    executar_teste(args.porta, args.baud, args.distancia, args.tolerancia, args.saida)


if __name__ == "__main__":
    main()
