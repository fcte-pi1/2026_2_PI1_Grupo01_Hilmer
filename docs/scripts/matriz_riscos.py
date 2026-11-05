"""Gera as notas e o mapa de calor das páginas de docs/matriz-riscos.

Uso: python docs/scripts/matriz_riscos.py

Para cada página, lê a tabela de riscos (colunas Probabilidade e Impacto, no formato "Possível (3)"
ou só a palavra), escreve a coluna Nota (probabilidade × impacto) e redesenha o mapa de calor entre
os marcadores <!-- mapa:inicio --> e <!-- mapa:fim -->. Rodar de novo não muda nada se as tabelas
não mudaram.
"""
import pathlib
import re

PASTA = pathlib.Path(__file__).resolve().parent.parent / "matriz-riscos"

PROBABILIDADE = {1: "Rara", 2: "Improvável", 3: "Possível", 4: "Provável", 5: "Muito provável"}
IMPACTO = {1: "Muito baixo", 2: "Baixo", 3: "Médio", 4: "Alto", 5: "Muito alto"}
# Palavras da primeira versão (3 níveis de probabilidade, 4 de impacto) para a escala de 1 a 5.
ANTIGAS = {"baixa": 2, "média": 3, "alta": 4, "crítico": 5}


def faixa(nota):
    """Faixa da nota (1 a 25): baixa, moderada, alta ou crítica."""
    if nota >= 15:
        return "critica", "Crítica"
    if nota >= 10:
        return "alta", "Alta"
    if nota >= 5:
        return "moderada", "Moderada"
    return "baixa", "Baixa"


def valor(texto, escala):
    """Lê "Possível (3)", "Possível" ou uma palavra da versão anterior e devolve o número."""
    texto = texto.strip()
    m = re.search(r"\((\d)\)", texto)
    if m:
        return int(m.group(1))
    palavra = texto.lower()
    for n, nome in escala.items():
        if nome.lower() == palavra:
            return n
    if palavra in ANTIGAS:
        return ANTIGAS[palavra]
    raise ValueError(f"valor desconhecido: {texto!r}")


def celulas(linha):
    return [c.strip() for c in linha.strip().strip("|").split("|")]


def processar_tabela(linhas):
    """Reescreve a tabela de riscos com Probabilidade, Impacto e Nota; devolve as linhas e os riscos."""
    cab = celulas(linhas[0])
    nomes = [re.sub(r"\*", "", c).strip().lower() for c in cab]
    ip, ii = nomes.index("probabilidade"), nomes.index("impacto")
    fixas = [i for i in range(len(cab)) if i not in (ip, ii) and nomes[i] != "nota"]
    novo_cab = [cab[i] for i in fixas] + ["**Probabilidade**", "**Impacto**", "**Nota**"]
    alinh = celulas(linhas[1])
    novo_alinh = [alinh[i] for i in fixas] + [":---:", ":---:", ":---:"]
    saida = ["| " + " | ".join(novo_cab) + " |", "|" + "|".join(novo_alinh) + "|"]
    riscos = []
    for linha in linhas[2:]:
        c = celulas(linha)
        p, i = valor(c[ip], PROBABILIDADE), valor(c[ii], IMPACTO)
        nota = p * i
        classe, _ = faixa(nota)
        riscos.append((c[0], p, i, nota))
        saida.append("| " + " | ".join([c[k] for k in fixas] + [
            f"{PROBABILIDADE[p]} ({p})", f"{IMPACTO[i]} ({i})",
            f'<span class="nota nota-{classe}">{nota}</span>']) + " |")
    return saida, riscos


def mapa(riscos):
    """HTML do mapa de calor 5 × 5 com os números dos riscos em cada célula."""
    linhas = ['<div class="mapa-calor" markdown="0">', "<table>",
              '<tr><th class="canto" rowspan="2">Probabilidade ↓</th><th colspan="5">Impacto →</th></tr>',
              "<tr>" + "".join(f"<th>{IMPACTO[i]}<br><small>{i}</small></th>" for i in range(1, 6)) + "</tr>"]
    for p in range(5, 0, -1):
        tds = []
        for i in range(1, 6):
            classe, nome = faixa(p * i)
            nums = [n for n, rp, ri, _ in riscos if rp == p and ri == i]
            conteudo = "".join(f'<span class="risco">{n}</span>' for n in nums)
            tds.append(f'<td class="nota-{classe}" title="Nota {p * i} ({nome})">'
                       f'<span class="ponto">{p * i}</span>{conteudo}</td>')
        linhas.append(f"<tr><th>{PROBABILIDADE[p]}<br><small>{p}</small></th>" + "".join(tds) + "</tr>")
    linhas += ["</table>", "</div>", ""]
    ordem = sorted(riscos, key=lambda r: (-r[3], int(r[0])))
    grupos = []
    for classe, nome in (("critica", "Crítica"), ("alta", "Alta"), ("moderada", "Moderada"), ("baixa", "Baixa")):
        itens = [f"{n} ({nota})" for n, _, _, nota in ordem if faixa(nota)[0] == classe]
        if itens:
            grupos.append(f'<span class="nota nota-{classe}">{nome}</span> {", ".join(itens)}')
    linhas.append("**Prioridade (maior nota primeiro):** " + " · ".join(grupos))
    return linhas


def processar(caminho):
    texto = caminho.read_text(encoding="utf-8").split("\n")
    # tabela de riscos: a que tem colunas chamadas exatamente Probabilidade e Impacto
    def eh_tabela_de_riscos(linha):
        nomes = {re.sub(r"\*", "", c).strip().lower() for c in celulas(linha)}
        return linha.startswith("|") and {"probabilidade", "impacto"} <= nomes

    ini = next(k for k, l in enumerate(texto) if eh_tabela_de_riscos(l))
    fim = ini
    while fim < len(texto) and texto[fim].startswith("|"):
        fim += 1
    nova, riscos = processar_tabela(texto[ini:fim])
    texto[ini:fim] = nova
    a = texto.index("<!-- mapa:inicio -->")
    b = texto.index("<!-- mapa:fim -->")
    texto[a + 1:b] = mapa(riscos)
    caminho.write_text("\n".join(texto), encoding="utf-8")
    print(f"{caminho.name}: {len(riscos)} riscos")


if __name__ == "__main__":
    for pagina in sorted(PASTA.glob("*.md")):
        processar(pagina)
