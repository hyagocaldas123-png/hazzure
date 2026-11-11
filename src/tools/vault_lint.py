"""Verificação de integridade do vault (só leitura).

`python tools/vault_lint.py` -> imprime cada achado e sai 1 se houver algum; sem achados
sai 0. O `python tools/vault_sync.py --check` chama `run()` daqui, então as duas checagens
(deploy/código e vault em si) já andam juntas por lá.

Cobre o que o Obsidian usa para navegar e renderizar: encoding, frontmatter fechado,
[[wikilink]] e links relativos resolvendo, colunas de tabela, seções duplicadas, texto
colado e CRLF misturado.
"""
import io, os, re, sys

VAULT = os.environ.get("HAZZURE_VAULT", r"C:/Codex2/Hazzure_Projeto")
SKIP_DIRS = (".git", ".obsidian", ".trash")


def run(vault=VAULT):
    """Achados como (tipo, arquivo, detalhe); não escreve nada em lugar nenhum."""
    bad = []
    files = []
    for root, dirs, names in os.walk(vault):
        dirs[:] = [d for d in dirs if d not in SKIP_DIRS]
        for n in sorted(names):
            if n.endswith(".md"):
                files.append(os.path.join(root, n))
    files.sort()

    texts = {}
    for p in files:
        raw = io.open(p, "rb").read()
        try:
            t = raw.decode("utf-8")
        except UnicodeDecodeError as e:
            bad.append(("encoding", p, str(e)))
            continue
        texts[p] = t
        rel = os.path.relpath(p, vault)
        if "\ufffd" in t:
            bad.append(("mojibake U+FFFD", rel, t.count("\ufffd")))
        # Sequencia de mojibake vista JA decodificada como utf-8 ("é" virou "Ã©").
        # Testar a classe de bytes 0xC3 nao prova nada: todo acento valido comeca com 0xC3.
        m2 = re.search(r"Ã[\x80-\xbf\u00c0-\u00ff]|â€[\x90-\xbf\u00c0-\u00ff]", t)
        if m2:
            bad.append(("dupla codificacao", rel, repr(m2.group(0))))
        crlf = t.count("\r\n")
        if crlf and crlf != t.count("\n"):
            bad.append(("CRLF misturado", rel,
                        f"{crlf} CRLF de {t.count(chr(10))} linhas"))
        lines = t.split("\n")
        if lines and lines[0].strip() == "---":
            close = next((i for i in range(1, len(lines))
                          if lines[i].strip() == "---"), None)
            if close is None:
                bad.append(("frontmatter aberto", rel,
                            "nunca fechado — o Obsidian para de renderizar"))
            elif close > 30:
                bad.append(("frontmatter suspeito", rel,
                            f"{close} linhas antes do fechamento"))

    # [[wikilink]]: fora de code-span, porque [[maybe_unused]] é C++ e não link
    allnames = {os.path.splitext(os.path.basename(p))[0].lower() for p in files}
    relpaths = {os.path.relpath(p, vault).replace("\\", "/").lower() for p in files}
    for p, t in texts.items():
        for m in re.finditer(r"\[\[([^\]|#]+)", re.sub(r"`[^`\n]*`", "", t)):
            target = m.group(1).strip().replace("\\", "/").lower()
            if target in relpaths or os.path.basename(target) in allnames:
                continue
            bad.append(("link quebrado", os.path.relpath(p, vault), m.group(1)))

    # links markdown relativos tambem tem que apontar para algo que existe
    for p, t in texts.items():
        for m in re.finditer(r"\]\(([^)\s#]+)", t):
            link = m.group(1)
            if link.startswith(("http", "mailto", "file://")):
                continue
            tgt = link if os.path.isabs(link) else \
                os.path.normpath(os.path.join(vault, link))
            if not os.path.exists(tgt.replace("/", "\\")):
                bad.append(("arquivo apontado nao existe", os.path.relpath(p, vault), link))

    for p, t in texts.items():
        lines = t.split("\n")
        for i in range(len(lines) - 1):
            a, b = lines[i], lines[i + 1]
            if a.startswith("|") and re.fullmatch(r"\|[\s:|-]+\|", b.strip() or " ") \
               and a.count("|") != b.count("|"):
                bad.append(("tabela inconsistente", os.path.relpath(p, vault),
                            f"linha {i+1}: {a.count('|')} vs {b.count('|')}"))

    for p, t in texts.items():
        # "naoEstado"; exige maiuscula no segundo lado, senao "de+mais" de "demais" vira achado
        for m in re.finditer(r"\b(n[ãa]o|que|em|de|e)([A-ZÀ-Ü][a-zà-ü])", t):
            bad.append(("possivel texto colado", os.path.relpath(p, vault), m.group(0)))

    for p, t in texts.items():
        lines = t.split("\n")
        nb = [l.strip() for l in lines if l.strip()]
        for i in range(len(nb) - 1):
            if nb[i] == nb[i + 1] and len(nb[i]) > 20:
                bad.append(("linha duplicada", os.path.relpath(p, vault), nb[i][:60]))
        for i, l in enumerate(lines):
            s = l.strip()
            if not (len(s) > 2 and s.endswith("—")):
                continue
            # "frase —\n continua" e quebra normal; o defeito e o travessao fechar o bloco
            nxt = (lines[i + 1] if i + 1 < len(lines) else "").strip()
            if nxt == "" or nxt.startswith("#"):
                bad.append(("trava de texto", os.path.relpath(p, vault),
                            f"linha {i+1}: {s[:60]}"))

    # titulos duplicados no mesmo arquivo: diff de titulos entre arquivos prova que nada
    # se perdeu, nao que nada se duplicou — esta checagem e a que falta na mesclagem de 07/11
    for p, t in texts.items():
        titles = [l[3:].strip() for l in t.split("\n") if l.startswith("## ")]
        for dup in sorted({x for x in titles if titles.count(x) > 1}):
            bad.append(("secao duplicada", os.path.relpath(p, vault), dup[:60]))

    return bad, len(files)


def main():
    bad, n = run()
    for kind, p, info in bad:
        print(f"{kind:26} {p}  {info}")
    if not bad:
        print(f"vault: sem problemas ({n} .md)")
        return 0
    print(f"vault: {len(bad)} achado(s) em {n} arquivos .md")
    return 1


if __name__ == "__main__":
    sys.exit(main())
