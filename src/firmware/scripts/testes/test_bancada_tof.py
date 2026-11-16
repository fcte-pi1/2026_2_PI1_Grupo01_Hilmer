"""Testes do menu de bancada dos VL53L0X (`scripts/bancada_tof.py`, issues #344 e #175).

Não precisam da placa nem do `pyserial`: a porta serial é trocada por um falso que devolve as
linhas JSON do firmware de bancada, e a `Bancada` por uma que responde a cada comando com o JSON
combinado no teste. Assim cada etapa do roteiro é conferida (APROVADO, REPROVADO ou OBSERVADO)
com respostas do formato que o `bancada_tof.cpp` imprime.

Uso (a partir de `src/firmware`):
    python3 -m unittest discover -s scripts/testes -v
"""

import contextlib
import io
import json
import os
import sys
import tempfile
import types
import unittest
from unittest import mock

PASTA_SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# O script sai se não achar o `pyserial`; os testes não usam a porta real.
_serial = types.ModuleType("serial")
_serial.Serial = None
_serial.SerialException = Exception
_serial.tools = types.ModuleType("serial.tools")
_serial.tools.list_ports = types.ModuleType("serial.tools.list_ports")
sys.modules.setdefault("serial", _serial)
sys.modules.setdefault("serial.tools", _serial.tools)
sys.modules.setdefault("serial.tools.list_ports", _serial.tools.list_ports)

sys.path.insert(0, PASTA_SCRIPTS)
import bancada_tof  # noqa: E402  (precisa do `serial` falso acima)


# ---- falsos ----------------------------------------------------------------------------------


class SerialFalsa:
    """Porta serial que devolve as linhas dadas, uma por `readline()`."""

    def __init__(self, linhas):
        self.linhas = [linha.encode() for linha in linhas]
        self.escrito = b""

    def reset_input_buffer(self):
        pass

    def write(self, dados):
        self.escrito += dados

    def readline(self):
        return self.linhas.pop(0) if self.linhas else b""

    def close(self):
        pass


class BancadaFalsa(bancada_tof.Bancada):
    """`Bancada` cujos comandos devolvem as respostas combinadas, em ordem, por comando."""

    def __init__(self, respostas):
        super().__init__("/dev/falsa")
        self.serial = object()
        self.respostas = {texto: list(lista) for texto, lista in respostas.items()}
        self.enviados = []

    def comando(self, texto, tipo, espera_s=5.0, ao_receber=None):
        self.enviados.append(texto)
        fila = self.respostas.get(texto)
        return fila.pop(0) if fila else None

    def conectar(self):
        return True

    def fechar(self):
        pass

    def situacoes(self):
        return {etapa: situacao for etapa, situacao, _ in self.resultados}


def amostra(mm, estado="valida"):
    return {"mm": mm, "estado": estado, "idade_ms": 10}


def leitura(valida, frente, esquerda, direita, limitada=None):
    dados = {"tipo": "leitura", "valida": valida, "idade_ms": 10, "tempo_leitura_us": 900,
             "frente": frente, "esquerda": esquerda, "direita": direita}
    if limitada is not None:
        dados["limitada"] = limitada
    return dados


def inicio(frente=True, esquerda=True, direita=True):
    return {"tipo": "inicio", "ok": frente and esquerda and direita, "frente": frente,
            "esquerda": esquerda, "direita": direita, "duracao_ms": 120}


def varredura(*enderecos):
    return {"tipo": "varredura", "enderecos": list(enderecos)}


def estatisticas(hz=30.0, tempo_max=1500, validas=100, leituras=100, por_lado=None):
    dados = {"tipo": "estatisticas", "leituras": leituras, "leituras_validas": validas,
             "tempo_leitura_us_medio": 800.0, "tempo_leitura_us_max": tempo_max}
    for lado in bancada_tof.LADOS:
        dados[lado] = {"hz": hz, "validas": 90, "media_mm": 100.0, "desvio_mm": 1.5, "min_mm": 97,
                       "max_mm": 103, "abaixo_do_minimo": 0, "fora_de_alcance": 0, "falhas": 0}
        if por_lado and lado in por_lado:
            dados[lado].update(por_lado[lado])
    return dados


@contextlib.contextmanager
def sem_interacao(respostas_input=()):
    """Responde ao `input()` com os textos dados (ENTER depois deles) e não espera no `sleep`."""
    fila = list(respostas_input)
    with mock.patch("builtins.input", side_effect=lambda *_: fila.pop(0) if fila else ""), \
            mock.patch.object(bancada_tof.time, "sleep"), \
            contextlib.redirect_stdout(io.StringIO()):
        yield


# ---- protocolo serial ------------------------------------------------------------------------


class TestComando(unittest.TestCase):
    def comando(self, linhas, tipo="leitura", ao_receber=None):
        bancada = bancada_tof.Bancada("/dev/falsa")
        bancada.serial = SerialFalsa(linhas)
        with contextlib.redirect_stdout(io.StringIO()):
            return bancada.comando("LER", tipo, espera_s=0.5, ao_receber=ao_receber), bancada

    def test_envia_o_comando_com_quebra_de_linha(self):
        _, bancada = self.comando(['{"tipo":"leitura"}'])
        self.assertEqual(b"LER\n", bancada.serial.escrito)

    def test_ignora_log_e_json_quebrado_ate_a_resposta(self):
        resposta, _ = self.comando(["ets Jun  8 2016 00:22:57", "{quebrado", '{"tipo":"leitura","valida":true}'])
        self.assertEqual({"tipo": "leitura", "valida": True}, resposta)

    def test_repassa_as_outras_linhas_json_enquanto_espera(self):
        recebidas = []
        resposta, _ = self.comando(['{"tipo":"fluxo","n":1}', '{"tipo":"fluxo","n":2}',
                                    '{"tipo":"fim_fluxo"}'], tipo="fim_fluxo", ao_receber=recebidas.append)
        self.assertEqual({"tipo": "fim_fluxo"}, resposta)
        self.assertEqual([1, 2], [d["n"] for d in recebidas])

    def test_erro_da_placa_devolve_none(self):
        resposta, _ = self.comando(['{"tipo":"erro","mensagem":"comando desconhecido"}'])
        self.assertIsNone(resposta)

    def test_sem_resposta_no_prazo_devolve_none(self):
        resposta, _ = self.comando([])
        self.assertIsNone(resposta)

    def test_sem_conexao_devolve_none(self):
        bancada = bancada_tof.Bancada("/dev/falsa")
        with contextlib.redirect_stdout(io.StringIO()):
            self.assertIsNone(bancada.comando("LER", "leitura"))


# ---- etapas do roteiro -----------------------------------------------------------------------


class TestFiacao(unittest.TestCase):
    def test_cada_xshut_liga_so_o_seu_sensor(self):
        bancada = BancadaFalsa({
            "ISOLAR N": [varredura(0x68)],
            "ISOLAR F": [varredura(0x29, 0x68)],
            "ISOLAR E": [varredura(0x29, 0x68)],
            "ISOLAR D": [varredura(0x29, 0x68)],
            "INIT": [inicio()],
        })
        with sem_interacao():
            self.assertTrue(bancada_tof.verificar_fiacao(bancada))
        self.assertEqual("INIT", bancada.enviados[-1])  # devolve os sensores ao estado normal

    def test_sensor_no_0x29_com_todos_em_reset_reprova(self):
        bancada = BancadaFalsa({
            "ISOLAR N": [varredura(0x29)],
            "ISOLAR F": [varredura(0x29)], "ISOLAR E": [varredura(0x29)], "ISOLAR D": [varredura(0x29)],
        })
        with sem_interacao():
            self.assertFalse(bancada_tof.verificar_fiacao(bancada))
        self.assertEqual("REPROVADO", bancada.situacoes()["Todos os ToF em reset"])

    def test_xshut_que_nao_liga_o_sensor_reprova_so_aquele_lado(self):
        bancada = BancadaFalsa({
            "ISOLAR N": [varredura()],
            "ISOLAR F": [varredura(0x29)], "ISOLAR E": [varredura()], "ISOLAR D": [varredura(0x29)],
        })
        with sem_interacao():
            self.assertFalse(bancada_tof.verificar_fiacao(bancada))
        situacoes = bancada.situacoes()
        self.assertEqual("REPROVADO", situacoes["XSHUT do sensor esquerda"])
        self.assertEqual("APROVADO", situacoes["XSHUT do sensor frente"])


class TestInicializacao(unittest.TestCase):
    def test_tres_sensores_nos_enderecos_esperados(self):
        bancada = BancadaFalsa({"INIT": [inicio()], "SCAN": [varredura(0x30, 0x31, 0x32, 0x68)]})
        with sem_interacao():
            self.assertTrue(bancada_tof.verificar_inicializacao(bancada))

    def test_sensor_que_ficou_no_0x29_reprova(self):
        bancada = BancadaFalsa({"INIT": [inicio(direita=False)], "SCAN": [varredura(0x29, 0x30, 0x31)]})
        with sem_interacao():
            self.assertFalse(bancada_tof.verificar_inicializacao(bancada))
        situacoes = bancada.situacoes()
        self.assertEqual("REPROVADO", situacoes["Endereço do sensor direita"])
        self.assertEqual("REPROVADO", situacoes["Endereço de fábrica livre"])
        self.assertEqual("REPROVADO", situacoes["Os 3 sensores iniciam"])


class TestTaxaETempo(unittest.TestCase):
    def test_dentro_dos_limites_aprova(self):
        bancada = BancadaFalsa({"ESTAT 5": [estatisticas()]})
        with sem_interacao():
            self.assertTrue(bancada_tof.verificar_taxa_e_tempo(bancada))

    def test_taxa_baixa_leitura_lenta_e_poucas_validas_reprovam(self):
        bancada = BancadaFalsa({"ESTAT 5": [estatisticas(hz=12.0, tempo_max=8000, validas=80)]})
        with sem_interacao():
            self.assertFalse(bancada_tof.verificar_taxa_e_tempo(bancada))
        situacoes = bancada.situacoes()
        self.assertEqual("REPROVADO", situacoes["Taxa do sensor frente"])
        self.assertEqual("REPROVADO", situacoes["Leitura não bloqueia"])
        self.assertEqual("REPROVADO", situacoes["Leituras válidas"])


class TestRegua(unittest.TestCase):
    def test_tolerancia_e_o_maior_entre_10_mm_e_5_por_cento(self):
        # 100 mm: tolerância de 10 mm; 400 mm: 5% = 20 mm. Média sempre 100 mm (veja `estatisticas`).
        bancada = BancadaFalsa({"ESTAT 3": [
            estatisticas(por_lado={"frente": {"media_mm": 109.0}}),
            estatisticas(por_lado={"frente": {"media_mm": 111.0}}),
            estatisticas(por_lado={"frente": {"media_mm": 419.0}}),
        ]})
        with sem_interacao(["100", "100", "400", "", "", ""]):
            self.assertFalse(bancada_tof.verificar_regua(bancada))
        detalhes = [(situacao, detalhe) for _, situacao, detalhe in bancada.resultados]
        self.assertEqual(["APROVADO", "REPROVADO", "APROVADO"], [s for s, _ in detalhes])
        self.assertIn("±20", detalhes[2][1])

    def test_sem_amostra_valida_reprova_com_o_motivo(self):
        bancada = BancadaFalsa({"ESTAT 3": [
            estatisticas(por_lado={"esquerda": {"validas": 0, "fora_de_alcance": 90}})]})
        with sem_interacao(["", "200", "", ""]):
            self.assertFalse(bancada_tof.verificar_regua(bancada))
        _, situacao, detalhe = bancada.resultados[0]
        self.assertEqual("REPROVADO", situacao)
        self.assertIn("90 fora de alcance", detalhe)

    def test_valor_digitado_invalido_pergunta_de_novo(self):
        bancada = BancadaFalsa({"ESTAT 3": [estatisticas()]})
        with sem_interacao(["abc", "100,0", "", "", ""]):
            self.assertTrue(bancada_tof.verificar_regua(bancada))
        self.assertEqual(["ESTAT 3"], bancada.enviados)


class TestForaDaFaixa(unittest.TestCase):
    LIMITADA_LONGE = {"frente": 1200, "esquerda": 60, "direita": 60}

    def test_sem_alvo_aprova_com_leitura_valida_limitada_a_1200(self):
        dados = leitura(True, amostra(8190, "fora_de_alcance"), amostra(60), amostra(60), self.LIMITADA_LONGE)
        resultado, _ = bancada_tof.avaliar_fora_da_faixa(dados, "frente", "fora_de_alcance", 1200)
        self.assertEqual("APROVADO", resultado)

    def test_muito_perto_aprova_com_leitura_valida_limitada_a_30(self):
        dados = leitura(True, amostra(200), amostra(12, "abaixo_do_minimo"), amostra(60),
                        {"frente": 200, "esquerda": 30, "direita": 60})
        resultado, _ = bancada_tof.avaliar_fora_da_faixa(dados, "esquerda", "abaixo_do_minimo", 30)
        self.assertEqual("APROVADO", resultado)

    def test_leitura_recusada_com_o_estado_certo_e_defeito_do_driver(self):
        dados = leitura(False, amostra(8190, "fora_de_alcance"), amostra(60), amostra(60), self.LIMITADA_LONGE)
        resultado, detalhe = bancada_tof.avaliar_fora_da_faixa(dados, "frente", "fora_de_alcance", 1200)
        self.assertEqual("REPROVADO", resultado)
        self.assertIn("inválida", detalhe)

    def test_distancia_sem_limitar_e_defeito_do_driver(self):
        dados = leitura(True, amostra(8190, "fora_de_alcance"), amostra(60), amostra(60),
                        {"frente": 8190, "esquerda": 60, "direita": 60})
        resultado, _ = bancada_tof.avaliar_fora_da_faixa(dados, "frente", "fora_de_alcance", 1200)
        self.assertEqual("REPROVADO", resultado)

    def test_firmware_antigo_sem_o_campo_limitada_reprova(self):
        dados = leitura(True, amostra(8190, "fora_de_alcance"), amostra(60), amostra(60))
        resultado, _ = bancada_tof.avaliar_fora_da_faixa(dados, "frente", "fora_de_alcance", 1200)
        self.assertEqual("REPROVADO", resultado)

    def test_sensor_com_outro_estado_so_e_observado(self):
        # Ex.: com luz forte, o sensor mede algo a 900 mm em vez de "sem alvo": é o sensor, não o driver.
        dados = leitura(True, amostra(900), amostra(60), amostra(60), {"frente": 900, "esquerda": 60, "direita": 60})
        resultado, detalhe = bancada_tof.avaliar_fora_da_faixa(dados, "frente", "fora_de_alcance", 1200)
        self.assertEqual("OBSERVADO", resultado)
        self.assertIn("veio valida (900 mm)", detalhe)

    def test_etapa_completa_pede_os_dois_casos_de_cada_sensor(self):
        respostas = []
        for lado in bancada_tof.LADOS:
            longe = {l: 60 for l in bancada_tof.LADOS} | {lado: 1200}
            perto = {l: 60 for l in bancada_tof.LADOS} | {lado: 30}
            amostras_longe = {l: amostra(60) for l in bancada_tof.LADOS} | {lado: amostra(8190, "fora_de_alcance")}
            amostras_perto = {l: amostra(60) for l in bancada_tof.LADOS} | {lado: amostra(5, "abaixo_do_minimo")}
            respostas.append(leitura(True, limitada=longe, **amostras_longe))
            respostas.append(leitura(True, limitada=perto, **amostras_perto))
        bancada = BancadaFalsa({"LER": respostas})
        with sem_interacao():
            self.assertTrue(bancada_tof.verificar_fora_da_faixa(bancada))
        self.assertEqual(6, len(bancada.resultados))
        self.assertTrue(all(s == "APROVADO" for _, s, _ in bancada.resultados))

    def test_etapa_reprova_se_algum_caso_reprovar(self):
        recusada = leitura(False, amostra(8190, "fora_de_alcance"), amostra(60), amostra(60), self.LIMITADA_LONGE)
        bancada = BancadaFalsa({"LER": [recusada] * 6})
        with sem_interacao():
            self.assertFalse(bancada_tof.verificar_fora_da_faixa(bancada))


class TestSensorAusente(unittest.TestCase):
    def test_enter_pula_a_etapa(self):
        bancada = BancadaFalsa({})
        with sem_interacao([""]):
            self.assertTrue(bancada_tof.verificar_sensor_ausente(bancada))
        self.assertEqual([], bancada.enviados)

    def test_sem_um_sensor_os_outros_iniciam_e_a_leitura_falha(self):
        bancada = BancadaFalsa({
            "INIT": [inicio(direita=False)],
            "LER": [leitura(False, amostra(200), amostra(60), amostra(0, "falha"))],
        })
        with sem_interacao(["direita"]):
            self.assertTrue(bancada_tof.verificar_sensor_ausente(bancada))

    def test_sensor_ausente_que_nao_derruba_a_leitura_reprova(self):
        bancada = BancadaFalsa({
            "INIT": [inicio(direita=False)],
            "LER": [leitura(True, amostra(200), amostra(60), amostra(0, "falha"))],
        })
        with sem_interacao(["direita"]):
            self.assertFalse(bancada_tof.verificar_sensor_ausente(bancada))


class TestRelatorio(unittest.TestCase):
    def test_relatorio_tem_condicoes_resultados_e_total(self):
        bancada = BancadaFalsa({})
        bancada.anotacoes = {"responsaveis": "Equipe de firmware", "cor_parede": "branca"}
        with sem_interacao():
            bancada.registrar("Taxa do sensor frente", "APROVADO", "30 Hz")
            bancada.registrar("Régua frente 100 mm", "REPROVADO", "erro | grande")
        with tempfile.TemporaryDirectory() as pasta:
            caminho = os.path.join(pasta, "relatorio.md")
            with sem_interacao([caminho]):
                bancada_tof.salvar_relatorio(bancada)
            with open(caminho, encoding="utf-8") as arquivo:
                texto = arquivo.read()
        self.assertIn("- Responsáveis: Equipe de firmware", texto)
        self.assertIn("- Luz ambiente: não informado", texto)
        self.assertIn("| Régua frente 100 mm | REPROVADO | erro / grande |", texto)  # `|` não quebra a tabela
        self.assertIn("**Total:** 2 verificações, 1 reprovadas.", texto)

    def test_sem_resultados_nao_cria_arquivo(self):
        bancada = BancadaFalsa({})
        with sem_interacao():
            bancada_tof.salvar_relatorio(bancada)  # não pergunta o nome nem grava


class TestUtilitarios(unittest.TestCase):
    def test_hexa(self):
        self.assertEqual("0x29, 0x68", bancada_tof.hexa([0x29, 0x68]))
        self.assertEqual("nenhum", bancada_tof.hexa([]))

    def test_perguntar_numero_aceita_virgula_e_usa_o_padrao(self):
        with sem_interacao(["2,5", "", "xyz"]):
            self.assertEqual(2.5, bancada_tof.perguntar_numero("Duração", 30))
            self.assertEqual(30, bancada_tof.perguntar_numero("Duração", 30))
            self.assertEqual(30, bancada_tof.perguntar_numero("Duração", 30))

    def test_faixa_igual_a_do_firmware(self):
        # Os limites do passo 5 precisam acompanhar o `ConfiguracaoTof.h`.
        caminho = os.path.join(PASTA_SCRIPTS, "..", "lib", "tof", "src", "ConfiguracaoTof.h")
        with open(caminho, encoding="utf-8") as arquivo:
            texto = arquivo.read()
        self.assertIn(f"DISTANCIA_MINIMA_VALIDA_TOF_MM = {bancada_tof.DISTANCIA_MINIMA_MM};", texto)
        self.assertIn(f"DISTANCIA_MAXIMA_VALIDA_TOF_MM = {bancada_tof.DISTANCIA_MAXIMA_MM};", texto)


if __name__ == "__main__":
    unittest.main()
