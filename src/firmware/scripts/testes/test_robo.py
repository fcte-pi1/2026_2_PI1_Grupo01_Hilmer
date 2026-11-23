"""Testes do controle do robô pelo terminal (`scripts/robo.py`).

Não precisam de rede nem do Supabase: a chamada HTTP é trocada por uma que responde o que a função
`robo` e o PostgREST devolveriam. Conferem as chamadas (endereço, cabeçalhos e corpo), as mensagens de
erro e a saída de cada subcomando.

Uso (a partir de `src/firmware`):
    python3 -m unittest discover -s scripts/testes -v
"""

import io
import json
import os
import sys
import tempfile
import unittest
from unittest import mock

PASTA_SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, PASTA_SCRIPTS)
import robo  # noqa: E402

AMBIENTE = {"SUPABASE_URL": "https://projeto.supabase.co/", "SUPABASE_ANON_KEY": "anon", "PAINEL_TOKEN": "senha"}


class Servidor:
    """Faz as vezes do Supabase: guarda as chamadas e responde pelo caminho."""

    def __init__(self):
        self.chamadas = []
        self.respostas = {}  # trecho do caminho -> (código, corpo) ou lista de respostas em sequência

    def __call__(self, metodo, url, cabecalhos, corpo=None):
        self.chamadas.append((metodo, url, cabecalhos, corpo))
        for trecho, resposta in self.respostas.items():
            if trecho in url:
                if isinstance(resposta, list):
                    return resposta.pop(0) if len(resposta) > 1 else resposta[0]
                return resposta
        return 404, None


def rodar(argv, servidor, ambiente=AMBIENTE, **kwargs):
    saida = io.StringIO()
    args = robo.montar_parser().parse_args(argv)
    codigo = robo.executar(args, ambiente, saida=saida, requisitar=servidor, **kwargs)
    return codigo, saida.getvalue()


def log(id, mensagem, criado="2026-11-17T12:00:01+00:00"):
    return {"id": id, "corrida_id": None, "criado_em": criado, "mensagem": mensagem}


class TestComandos(unittest.TestCase):
    def test_iniciar_chama_a_funcao_com_a_senha_do_painel(self):
        s = Servidor()
        s.respostas["/robo/comandos"] = (202, {"id": 12, "comando": "iniciar"})
        codigo, saida = rodar(["iniciar"], s)
        self.assertEqual(codigo, 0)
        self.assertIn("iniciar enviado (id 12)", saida)
        metodo, url, cabecalhos, corpo = s.chamadas[0]
        self.assertEqual((metodo, url), ("POST", "https://projeto.supabase.co/functions/v1/robo/comandos"))
        self.assertEqual(cabecalhos["x-painel-token"], "senha")
        self.assertEqual(cabecalhos["apikey"], "anon")
        self.assertEqual(cabecalhos["Authorization"], "Bearer anon")
        self.assertEqual(corpo, {"comando": "iniciar"})

    def test_parar_e_zerar_mandam_o_comando_certo(self):
        for comando in ("parar", "zerar"):
            s = Servidor()
            s.respostas["/robo/comandos"] = (202, {"id": 1})
            self.assertEqual(rodar([comando], s)[0], 0)
            self.assertEqual(s.chamadas[0][3], {"comando": comando})

    def test_erros_do_servidor_viram_mensagens_claras(self):
        casos = {401: "senha do painel incorreta", 404: "não está publicada", 500: "código 500"}
        for status, trecho in casos.items():
            s = Servidor()
            s.respostas["/robo/comandos"] = (status, {"erro": "x"})
            with self.assertRaises(robo.ErroDoRobo) as erro:
                rodar(["iniciar"], s)
            self.assertIn(trecho, str(erro.exception))

    def test_esperar_diz_quanto_o_robo_demorou(self):
        s = Servidor()
        s.respostas["/robo/comandos"] = (202, {"id": 5})
        s.respostas["comandos_robo"] = [(200, [{"id": 5, "entregue_em": None}]), (200, [{"id": 5, "entregue_em": "2026-11-17T12:00:02+00:00"}])]
        relogio = iter([0.0, 0.1, 0.6, 1.2, 1.3])
        codigo, saida = rodar(["iniciar", "--esperar"], s, dormir=lambda _: None, agora=lambda: next(relogio))
        self.assertEqual(codigo, 0)
        self.assertIn("entregue ao robô em 1.2 s", saida)

    def test_esperar_falha_se_o_robo_nao_busca(self):
        s = Servidor()
        s.respostas["/robo/comandos"] = (202, {"id": 5})
        s.respostas["comandos_robo"] = (200, [{"id": 5, "entregue_em": None}])
        tempo = {"t": 0.0}

        def dormir(segundos):
            tempo["t"] += segundos * 20  # o tempo anda rápido: passa dos 30 s

        codigo, saida = rodar(["iniciar", "--esperar"], s, dormir=dormir, agora=lambda: tempo["t"])
        self.assertEqual(codigo, 1)
        self.assertIn("não buscou o comando", saida)


class TestConfiguracao(unittest.TestCase):
    def test_comando_sem_senha_e_recusado_mas_logs_nao_precisam_dela(self):
        sem_senha = {"SUPABASE_URL": "https://p.supabase.co", "SUPABASE_ANON_KEY": "anon"}
        with self.assertRaises(robo.ErroDoRobo) as erro:
            rodar(["parar"], Servidor(), ambiente=sem_senha)
        self.assertIn("PAINEL_TOKEN", str(erro.exception))
        s = Servidor()
        s.respostas["logs_robo"] = (200, [])
        self.assertEqual(rodar(["logs"], s, ambiente=sem_senha)[0], 0)

    def test_sem_url_ou_chave_diz_o_que_falta(self):
        with self.assertRaises(robo.ErroDoRobo) as erro:
            rodar(["logs"], Servidor(), ambiente={})
        self.assertIn("SUPABASE_URL", str(erro.exception))
        self.assertIn("SUPABASE_ANON_KEY", str(erro.exception))

    def test_arquivo_env_e_o_ambiente_vence(self):
        with tempfile.NamedTemporaryFile("w", suffix=".env", delete=False) as arquivo:
            arquivo.write('# comentário\nSUPABASE_URL="https://do-arquivo.supabase.co"\nSUPABASE_ANON_KEY=chave-do-arquivo\nPAINEL_TOKEN=senha-do-arquivo\n')
        try:
            cfg = robo.configuracao({"PAINEL_TOKEN": "senha-do-ambiente"}, arquivo.name, precisa_senha=True)
        finally:
            os.unlink(arquivo.name)
        self.assertEqual(cfg["SUPABASE_URL"], "https://do-arquivo.supabase.co")
        self.assertEqual(cfg["SUPABASE_ANON_KEY"], "chave-do-arquivo")
        self.assertEqual(cfg["PAINEL_TOKEN"], "senha-do-ambiente")

    def test_env_inexistente_e_erro_claro(self):
        with self.assertRaises(robo.ErroDoRobo):
            robo.configuracao({}, "/nao/existe/.env")


class TestLogs(unittest.TestCase):
    def test_logs_mostra_do_mais_antigo_ao_mais_novo(self):
        s = Servidor()
        s.respostas["logs_robo"] = (200, [log(3, '{"status":"colisao"}'), log(2, '{"status":"largada_liberada"}')])
        codigo, saida = rodar(["logs", "-n", "2"], s)
        self.assertEqual(codigo, 0)
        linhas = saida.strip().splitlines()
        self.assertIn("largada_liberada", linhas[0])
        self.assertIn("colisao", linhas[1])
        metodo, url, cabecalhos, _ = s.chamadas[0]
        self.assertEqual(metodo, "GET")
        self.assertIn("/rest/v1/logs_robo?", url)
        self.assertIn("order=id.desc", url)
        self.assertIn("limit=2", url)
        self.assertNotIn("x-painel-token", cabecalhos)  # a leitura só usa a chave anon

    def test_sem_logs_avisa(self):
        s = Servidor()
        s.respostas["logs_robo"] = (200, [])
        self.assertIn("nenhum log", rodar(["logs"], s)[1])

    def test_falha_na_leitura_sugere_a_migracao(self):
        s = Servidor()
        s.respostas["logs_robo"] = (404, {"message": "relation does not exist"})
        with self.assertRaises(robo.ErroDoRobo) as erro:
            rodar(["logs"], s)
        self.assertIn("migração", str(erro.exception))

    def test_seguir_mostra_so_os_logs_novos(self):
        s = Servidor()
        s.respostas["logs_robo"] = [
            (200, [log(2, "a"), log(1, "antigo")]),   # os últimos
            (200, [log(3, "novo1")]),                   # primeira volta
            (200, []),                                  # segunda volta
            (200, [log(4, "novo2")]),                   # terceira volta
        ]
        saida = io.StringIO()
        robo.seguir_logs({"SUPABASE_URL": "https://p", "SUPABASE_ANON_KEY": "k"}, saida, False, s, dormir=lambda _: None, ciclos=3)
        texto = saida.getvalue()
        self.assertEqual([l.split("  ", 1)[1] for l in texto.strip().splitlines()], ["antigo", "a", "novo1", "novo2"])
        urls = [c[1] for c in s.chamadas]
        self.assertIn("id=gt.2", urls[1])
        self.assertIn("id=gt.3", urls[2])

    def test_formatar_log_colore_por_nivel_so_com_terminal(self):
        erro = log(1, '{"status":"bateria","situacao":"fraca","mv":6400}')
        info = log(2, '{"status":"largada_liberada"}')
        self.assertIn("\033[31m", robo.formatar_log(erro, cor=True))
        self.assertNotIn("\033[", robo.formatar_log(erro, cor=False))
        self.assertNotIn("\033[", robo.formatar_log(info, cor=True))
        self.assertIn("\033[33m", robo.formatar_log(log(3, '{"status":"colisao"}'), cor=True))
        self.assertIn("micromouse: iniciando", robo.formatar_log(log(4, "micromouse: iniciando")))


class TestEstado(unittest.TestCase):
    def test_estado_mostra_o_ultimo_comando_e_os_logs(self):
        s = Servidor()
        s.respostas["comandos_robo"] = (200, [{"id": 9, "comando": "parar", "criado_em": "2026-11-17T12:00:00+00:00", "entregue_em": "2026-11-17T12:00:02+00:00"}])
        s.respostas["logs_robo"] = (200, [log(1, '{"status":"parada_remota"}')])
        codigo, saida = rodar(["estado"], s)
        self.assertEqual(codigo, 0)
        self.assertIn("último comando: parar (id 9", saida)
        self.assertIn("entregue às", saida)
        self.assertIn("parada_remota", saida)

    def test_estado_sem_comando_nem_log(self):
        s = Servidor()
        s.respostas["comandos_robo"] = (200, [])
        s.respostas["logs_robo"] = (200, [])
        self.assertIn("nenhum comando enviado", rodar(["estado"], s)[1])


class TestRede(unittest.TestCase):
    def test_erro_de_rede_vira_erro_do_robo(self):
        import urllib.error

        def abrir(*_args, **_kwargs):
            raise urllib.error.URLError("sem rede")

        with self.assertRaises(robo.ErroDoRobo) as erro:
            robo.requisitar("GET", "https://p", {}, abrir=abrir)
        self.assertIn("sem conexão", str(erro.exception))

    def test_resposta_que_nao_e_json_volta_sem_corpo(self):
        resposta = mock.MagicMock()
        resposta.status = 200
        resposta.read.return_value = b"<html>"
        resposta.__enter__.return_value = resposta
        self.assertEqual(robo.requisitar("GET", "https://p", {}, abrir=lambda *a, **k: resposta), (200, None))

    def test_main_devolve_1_e_escreve_o_erro_em_stderr(self):
        erro = io.StringIO()
        with mock.patch.dict(os.environ, {}, clear=True), mock.patch("sys.stderr", erro):
            self.assertEqual(robo.main(["logs"]), 1)
        self.assertIn("ERRO:", erro.getvalue())


if __name__ == "__main__":
    unittest.main()
