#!/usr/bin/env python3
"""Controle do robô pelo terminal: a mesma largada e os mesmos logs do painel, para testes.

Fala com a Edge Function `robo` do Supabase (veja o README do backend), o mesmo canal do painel web:
os comandos vão para o robô, que os busca em até cerca de 1,5 s, e os logs dos eventos do robô voltam
pela tabela `logs_robo`. Serve para testar sem abrir o painel, em scripts e no CI do laboratório.

Uso (a partir de `src/firmware`; só usa a biblioteca padrão do Python):
    ./scripts/robo.py iniciar                 # larga o robô (como o INICIAR do painel)
    ./scripts/robo.py parar                   # freia e volta a esperar a largada
    ./scripts/robo.py zerar                   # robô recolocado na partida: zera e aguarda a largada
    ./scripts/robo.py iniciar --esperar       # espera o robô buscar o comando e diz quanto demorou
    ./scripts/robo.py logs                    # os últimos 20 logs
    ./scripts/robo.py logs --seguir           # acompanha os logs ao vivo (Ctrl+C para sair)
    ./scripts/robo.py estado                  # último comando e últimos logs

Configuração, por variáveis de ambiente (ou por um arquivo `.env`, por `--env`):
    SUPABASE_URL        URL do projeto (https://xxxx.supabase.co)
    SUPABASE_ANON_KEY   chave pública anon (a mesma do painel)
    PAINEL_TOKEN        senha do operador, só para os comandos (o segredo `PAINEL_TOKEN` do Supabase)

Código de saída: 0 deu certo, 1 falhou (senha errada, função não publicada, robô não buscou o comando...).
"""

import argparse
import json
import os
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from datetime import datetime

COMANDOS = ("iniciar", "parar", "zerar")
VALIDADE_COMANDO_S = 30
PERIODO_SEGUIR_S = 1.0
TEMPO_LIMITE_HTTP_S = 15

NIVEL_POR_STATUS = {
    "falha_imu": "erro",
    "falha_barramento_i2c": "erro",
    "motor_travado": "erro",
    "motores_travados_i2c": "erro",
    "envio_pausado": "erro",
    "largada_expirada": "aviso",
    "parada_remota": "aviso",
    "colisao": "aviso",
    "wifi_caiu": "aviso",
    "falha_criar_corrida": "aviso",
}
COR = {"erro": "\033[31m", "aviso": "\033[33m", "info": ""}
FIM_DA_COR = "\033[0m"


class ErroDoRobo(Exception):
    """Falha que o operador resolve (configuração, senha, rede, função não publicada)."""


def ler_env(caminho):
    """Lê um arquivo `.env` simples (`CHAVE=valor`, comentários com `#`)."""
    valores = {}
    with open(caminho, encoding="utf-8") as arquivo:
        for linha in arquivo:
            linha = linha.strip()
            if not linha or linha.startswith("#") or "=" not in linha:
                continue
            chave, valor = linha.split("=", 1)
            valores[chave.strip()] = valor.strip().strip('"').strip("'")
    return valores


def configuracao(ambiente, arquivo_env=None, precisa_senha=False):
    """Junta as variáveis do ambiente e do `.env` (o ambiente vence) e confere o que falta."""
    valores = {}
    if arquivo_env:
        try:
            valores.update(ler_env(arquivo_env))
        except OSError as erro:
            raise ErroDoRobo(f"não consegui ler {arquivo_env}: {erro}") from erro
    for chave in ("SUPABASE_URL", "SUPABASE_ANON_KEY", "PAINEL_TOKEN"):
        if ambiente.get(chave):
            valores[chave] = ambiente[chave]
    faltando = [c for c in ("SUPABASE_URL", "SUPABASE_ANON_KEY") if not valores.get(c)]
    if precisa_senha and not valores.get("PAINEL_TOKEN"):
        faltando.append("PAINEL_TOKEN")
    if faltando:
        raise ErroDoRobo("faltam as variáveis: " + ", ".join(faltando) + " (veja --help)")
    valores["SUPABASE_URL"] = valores["SUPABASE_URL"].rstrip("/")
    return valores


def requisitar(metodo, url, cabecalhos, corpo=None, abrir=urllib.request.urlopen):
    """Faz uma chamada HTTP e devolve (código, corpo JSON ou None). Erros de rede viram `ErroDoRobo`."""
    dados = json.dumps(corpo).encode("utf-8") if corpo is not None else None
    pedido = urllib.request.Request(url, data=dados, method=metodo, headers=cabecalhos)
    try:
        with abrir(pedido, timeout=TEMPO_LIMITE_HTTP_S) as resposta:
            codigo, texto = resposta.status, resposta.read().decode("utf-8")
    except urllib.error.HTTPError as erro:
        codigo, texto = erro.code, erro.read().decode("utf-8", errors="replace")
    except (urllib.error.URLError, TimeoutError, OSError) as erro:
        raise ErroDoRobo(f"sem conexão com o Supabase: {erro}") from erro
    try:
        return codigo, json.loads(texto) if texto else None
    except ValueError:
        return codigo, None


def cabecalhos_base(cfg):
    return {
        "apikey": cfg["SUPABASE_ANON_KEY"],
        "Authorization": f"Bearer {cfg['SUPABASE_ANON_KEY']}",
        "Content-Type": "application/json",
    }


def enviar_comando(cfg, comando, requisitar=requisitar):
    """`POST /functions/v1/robo/comandos`. Devolve o `id` do comando."""
    cabecalhos = cabecalhos_base(cfg)
    cabecalhos["x-painel-token"] = cfg["PAINEL_TOKEN"]
    codigo, corpo = requisitar("POST", f"{cfg['SUPABASE_URL']}/functions/v1/robo/comandos", cabecalhos, {"comando": comando})
    if codigo == 401:
        raise ErroDoRobo("senha do painel incorreta (ou PAINEL_TOKEN não definido no Supabase)")
    if codigo == 404:
        raise ErroDoRobo("a função `robo` não está publicada (supabase functions deploy robo)")
    if codigo != 202 or not isinstance(corpo, dict):
        raise ErroDoRobo(f"o Supabase recusou o comando (código {codigo}): {corpo}")
    return corpo.get("id")


def consultar(cfg, tabela, parametros, requisitar=requisitar):
    """`GET /rest/v1/<tabela>` (PostgREST): só leitura, com a chave anon, como o painel."""
    consulta = urllib.parse.urlencode(parametros)
    codigo, corpo = requisitar("GET", f"{cfg['SUPABASE_URL']}/rest/v1/{tabela}?{consulta}", cabecalhos_base(cfg))
    if codigo != 200 or not isinstance(corpo, list):
        raise ErroDoRobo(f"não consegui ler `{tabela}` (código {codigo}); a migração dos logs foi aplicada?")
    return corpo


def ultimos_logs(cfg, quantidade, requisitar=requisitar):
    """Os últimos logs, do mais antigo ao mais novo."""
    linhas = consultar(cfg, "logs_robo", {"select": "*", "order": "id.desc", "limit": str(quantidade)}, requisitar)
    return sorted(linhas, key=lambda l: l["id"])


def logs_depois(cfg, ultimo_id, requisitar=requisitar):
    return consultar(cfg, "logs_robo", {"select": "*", "id": f"gt.{ultimo_id}", "order": "id.asc", "limit": "100"}, requisitar)


def ultimo_comando(cfg, requisitar=requisitar):
    linhas = consultar(cfg, "comandos_robo", {"select": "*", "order": "id.desc", "limit": "1"}, requisitar)
    return linhas[0] if linhas else None


def comando_entregue(cfg, comando_id, requisitar=requisitar):
    """A linha do comando (com `entregue_em`), ou None se ela não existe."""
    linhas = consultar(cfg, "comandos_robo", {"select": "*", "id": f"eq.{comando_id}"}, requisitar)
    return linhas[0] if linhas else None


def hora(iso):
    """`2026-11-17T12:00:01.123+00:00` -> `12:00:01` na hora local."""
    try:
        return datetime.fromisoformat(iso.replace("Z", "+00:00")).astimezone().strftime("%H:%M:%S")
    except (ValueError, AttributeError):
        return iso or "?"


def formatar_log(log, cor=False):
    """Uma linha de terminal: hora e o log (o JSON do robô, com a cor do nível)."""
    mensagem = log.get("mensagem", "")
    nivel = "info"
    try:
        evento = json.loads(mensagem)
        if isinstance(evento, dict) and isinstance(evento.get("status"), str):
            nivel = NIVEL_POR_STATUS.get(evento["status"], "info")
            if evento["status"] == "bateria" and evento.get("situacao") == "fraca":
                nivel = "erro"
            if evento["status"] == "navegacao" and evento.get("estado") == "falha":
                nivel = "erro"
            if evento["status"] == "i2c" and evento.get("situacao") == "em_falha":
                nivel = "erro"
    except ValueError:
        pass
    texto = f"{hora(log.get('criado_em'))}  {mensagem}"
    return f"{COR[nivel]}{texto}{FIM_DA_COR}" if cor and COR[nivel] else texto


def esperar_entrega(cfg, comando_id, limite_s=VALIDADE_COMANDO_S, requisitar=requisitar, dormir=time.sleep, agora=time.monotonic):
    """Espera o robô buscar o comando. Devolve os segundos até a entrega, ou None se expirou."""
    inicio = agora()
    while agora() - inicio < limite_s:
        linha = comando_entregue(cfg, comando_id, requisitar)
        if linha and linha.get("entregue_em"):
            return agora() - inicio
        dormir(0.5)
    return None


def seguir_logs(cfg, saida, cor, requisitar=requisitar, dormir=time.sleep, ciclos=None):
    """Mostra os logs novos a cada segundo, até Ctrl+C (ou `ciclos` voltas, nos testes)."""
    anteriores = ultimos_logs(cfg, 20, requisitar)
    for log in anteriores:
        print(formatar_log(log, cor), file=saida)
    ultimo = anteriores[-1]["id"] if anteriores else 0
    volta = 0
    while ciclos is None or volta < ciclos:
        volta += 1
        dormir(PERIODO_SEGUIR_S)
        for log in logs_depois(cfg, ultimo, requisitar):
            print(formatar_log(log, cor), file=saida, flush=True)
            ultimo = max(ultimo, log["id"])


def montar_parser():
    parser = argparse.ArgumentParser(description="Controle do robô pelo terminal (o mesmo canal do painel).")
    parser.add_argument("--env", metavar="ARQUIVO", help="arquivo .env com SUPABASE_URL, SUPABASE_ANON_KEY e PAINEL_TOKEN")
    sub = parser.add_subparsers(dest="acao", required=True)
    for comando in COMANDOS:
        p = sub.add_parser(comando, help=f"manda `{comando}` ao robô")
        p.add_argument("--esperar", action="store_true", help="espera o robô buscar o comando e diz quanto demorou")
    logs = sub.add_parser("logs", help="mostra os logs do robô")
    logs.add_argument("-n", type=int, default=20, help="quantos logs (padrão 20)")
    logs.add_argument("--seguir", action="store_true", help="acompanha os logs ao vivo (Ctrl+C para sair)")
    sub.add_parser("estado", help="mostra o último comando e os últimos logs")
    return parser


def executar(args, ambiente, saida=sys.stdout, requisitar=requisitar, dormir=time.sleep, agora=time.monotonic):
    """Roda a ação pedida. Devolve o código de saída (0 ou 1)."""
    cor = hasattr(saida, "isatty") and saida.isatty()
    cfg = configuracao(ambiente, args.env, precisa_senha=args.acao in COMANDOS)

    if args.acao in COMANDOS:
        comando_id = enviar_comando(cfg, args.acao, requisitar)
        print(f"{args.acao} enviado (id {comando_id}). O robô busca em até cerca de 1,5 s.", file=saida)
        if not args.esperar:
            return 0
        segundos = esperar_entrega(cfg, comando_id, VALIDADE_COMANDO_S, requisitar, dormir, agora)
        if segundos is None:
            print(f"o robô não buscou o comando em {VALIDADE_COMANDO_S} s (ele expirou): o robô está ligado e no Wi-Fi?", file=saida)
            return 1
        print(f"entregue ao robô em {segundos:.1f} s.", file=saida)
        return 0

    if args.acao == "logs":
        if args.seguir:
            try:
                seguir_logs(cfg, saida, cor, requisitar, dormir)
            except KeyboardInterrupt:
                print("", file=saida)
            return 0
        logs = ultimos_logs(cfg, args.n, requisitar)
        if not logs:
            print("nenhum log do robô ainda.", file=saida)
        for log in logs:
            print(formatar_log(log, cor), file=saida)
        return 0

    # estado
    comando = ultimo_comando(cfg, requisitar)
    if comando is None:
        print("nenhum comando enviado ainda.", file=saida)
    else:
        situacao = f"entregue às {hora(comando['entregue_em'])}" if comando.get("entregue_em") else "ainda não entregue"
        print(f"último comando: {comando['comando']} (id {comando['id']}, enviado às {hora(comando['criado_em'])}, {situacao})", file=saida)
    print("últimos logs:", file=saida)
    for log in ultimos_logs(cfg, 10, requisitar):
        print("  " + formatar_log(log, cor), file=saida)
    return 0


def main(argv=None):
    args = montar_parser().parse_args(argv)
    try:
        return executar(args, os.environ)
    except ErroDoRobo as erro:
        print(f"ERRO: {erro}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
