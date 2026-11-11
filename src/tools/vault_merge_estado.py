#!/usr/bin/env python3
"""One-off: mescla os dois ESTADO_ATUAL.md (vault + repo) em 00_Gestao/Estado.md.

Motivo: a partir de 2026-11-07 o vault e a fonte canonica de estado. Os dois
arquivos divergiam e nenhum era subconjunto do outro, entao a mesclagem e feita
por fatiamento de secoes (sem transcricao manual) e com nota de procedencia.

Uso: python tools/vault_merge_estado.py [--dry]
"""
import re
import sys
import hashlib
from pathlib import Path

VAULT = Path(r"C:/Codex2/Hazzure_Projeto/ESTADO_ATUAL.md")
REPO = Path(r"C:/Codex2/gpt/ESTADO_ATUAL.md")
OUT = Path(r"C:/Codex2/Hazzure_Projeto/00_Gestao/Estado.md")
DRY = "--dry" in sys.argv

TASK_RE = re.compile(r"T0(\d\d)")


def split_sections(text, origin):
    """Preamble + lista de secoes nivel-2 (### fica dentro da secao pai)."""
    lines = text.splitlines()
    starts = [i for i, ln in enumerate(lines) if ln.startswith("## ")]
    pre = "\n".join(lines[: starts[0] if starts else len(lines)]).strip()
    if not starts:
        return pre, []
    out = []
    for n, s in enumerate(starts):
        e = starts[n + 1] if n + 1 < len(starts) else len(lines)
        block = "\n".join(lines[s:e]).rstrip()
        title = lines[s][3:].strip()
        m = TASK_RE.search(title)
        key = "T%02d" % int(m.group(1)) if m else "N:" + title.lower()[:48]
        out.append({"key": key, "title": title, "body": block, "origin": origin})
    return pre, out


def norm(title):
    return re.sub(r"[^a-z0-9]+", "", title.lower())


def merge(vault, repo):
    """Nada é descartado: só títulos E corpos idênticos colapsam.

    Dedupe por id de tarefa seria destrutivo aqui — `T039b` colide com `T039` e o
    registro histórico de crash cita `T045` no título. A regra é literal.
    """
    out = list(vault)
    seen = {(norm(s["title"]), s["body"]) for s in vault}
    vault_titles = {norm(s["title"]) for s in vault}
    repo_only, identical = [], []
    for s in repo:
        if (norm(s["title"]), s["body"]) in seen:
            identical.append(s)
            continue
        out.append(s)
        if norm(s["title"]) not in vault_titles:
            repo_only.append(s)
    return out, repo_only, identical


def main():
    vtext = VAULT.read_text(encoding="utf-8", errors="replace")
    rtext = REPO.read_text(encoding="utf-8", errors="replace")
    vpre, vse = split_sections(vtext, "vault")
    rpre, rse = split_sections(rtext, "repo")
    merged, repo_only, identical = merge(vse, rse)

    # ordem: blocos do vault primeiro, com as secoes exclusivas do repo encaixadas
    # logo apos o bloco T047 (a janela de datas 05-06/11 que falta no vault)
    vault_keys = [id(s) for s in vse]
    anchor = max((i for i, s in enumerate(vse) if norm(s["title"]).find("t047") >= 0),
                 default=len(vse) - 1)
    ordered = vse[:anchor + 1] + repo_only + vse[anchor + 1:]
    # secoes do repo cujo titulo casa com uma do vault mas o conteudo difere
    # vao coladas logo apos a gêmea, como bloco rotulado — nada vai para o fim
    for s in merged:
        if s in vse or s in repo_only or s in identical:
            continue
        twin = max((v for v in vse if norm(v["title"]) == norm(s["title"])),
                   key=lambda v: v["body"], default=None)
        pos = ordered.index(twin) + 1 if twin else len(ordered) - 1
        s["body"] = s["body"].replace("## ", "## (repo) ", 1)
        ordered.insert(pos, s)

    head = HEAD.format(
        vault_sha=hashlib.sha256(vtext.encode()).hexdigest()[:12],
        repo_sha=hashlib.sha256(rtext.encode()).hexdigest()[:12],
        n_vault=len(vse), n_repo=len(rse), n_out=len(ordered),
    )
    dup_note = "\n".join(
        f"- `{s['key']}` — \"{s['title'][:64]}\" veio **só do repo** e foi encaixado após a T047."
        for s in repo_only
    ) + "\n" + ("\n".join(
        f"- repetição exata descartada: `{s['key']}` \"{s['title'][:56]}\" (título e corpo idênticos)."
        for s in identical
    ) or "- (nenhuma repetição exata entre os dois arquivos)")

    body = "\n\n".join(s["body"] for s in ordered)
    preambles = ("\n\n---\n\n## Preâmbulo histórico (arquivado, não é fila)\n\n"
                 + vpre + "\n\n" + rpre)
    result = head + "\n\n## Nota de mesclagem\n\n" + dup_note + "\n\n---\n\n" + body + preambles + "\n"

    if DRY:
        print(f"vault={len(vse)} repo={len(rse)} repo_only={len(repo_only)} "
              f"identicos={len(identical)} out={len(ordered)} bytes={len(result)}")
        for s in ordered:
            print(f"  {s['origin']:5} {s['key']:12} {s['title'][:72]}")
        return 0
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(result, encoding="utf-8", newline="\n")
    print(f"escrito {OUT} ({len(result)} B, {len(ordered)} secoes)")
    return 0


HEAD = """# Estado — fonte canônica do Hazzure

> Este arquivo é **a** fonte de estado do projeto desde 2026-11-07. Ele substituiu os dois
> `ESTADO_ATUAL.md` que divergiam (vault `{vault_sha}`, repo `{repo_sha}`), mesclados por
> `tools/vault_merge_estado.py` sem transcrição manual. Os originais estão preservados como
> `ESTADO_ATUAL.md.bak-20261107-vault-merge` nos dois lugares. `C:\\Codex2\\gpt\\ESTADO_ATUAL.md`
> agora é só um ponteiro para cá.
>
> Seções mescladas: {n_vault} do vault + {n_repo} do repo → {n_out} únicas.

## Cabeçalho fixo (atualizar aqui, não no fim do arquivo)

### Build atual em disco
- `C:\\HazzureKernel\\Hazzure.Internal.dll` — **t050**, SHA-256 `92af769c6d204cce15c7bfa6ac3d2def717da905cf76e416614a20646ed89399`, 2.957.312 B, deployado 2026-11-06 03:06 (cópia lateral + `mv -f`, também em `modules\\`).
- Rollback do t049 intacto: `Hazzure.Internal.dll.bak-20261106-t050` — `247eb4ba…`.

### O que está em memória AGORA
- **t049**, sessão iniciada 2026-11-06 02:27 (pid 27864). O t050 **não foi injetado**.
- Método para responder isso sem tocar no processo: cruzar o timestamp de `[START] Start() finalizado` no `C:\\HazzureKernel\\debug_log.txt` com o mtime/SHA-256 da DLL instalada. DLL em disco ≠ DLL em memória; não existe hot reload.

### Alvo de engenharia
- `client.dll` TimeDateStamp `0x6AEBC4DA`, SizeOfImage `0x29A3000`, build CS2 **14194**. Todo endereço deste vault é RVA sobre essa base.

### Pendências abertas
- Reinjetar o t050 e validar os 4 técnicos: `[STRAFE]` (ângulo 84–86° e ganho real), `[AUTOFIRE]` sem o rage master, mãos sumindo em 3P + leitura de `pawnPerguntado`.
- Validações antigas ainda sem partida: plant da C4 (t048), sobreviver à troca de mapa (t045), luvas (t044), efeito visual do scope (t049).
- Tarefas do tracker: #2 refinar chams/material override, #3 portão de teardown do clima.
- Adiado por decisão do usuário: modelo de dano de penetração (tarefa separada), desync/body yaw/manual yaw/fake lag (análise-only), features do handoff MemeSense (T040, reservado, sem código).

### Riscos conhecidos
- Suprimir draw de entidades na cadeia de render (3P) pode esconder as mãos em primeira pessoa se o gate errar; por isso os contadores `[3P]`.
- Escrever modelo na entidade de mãos já foi medido e rejeitado pelo motor (mesh mask volta `0xFFFFFFFFFFFFFFFE`, "faca flutuando") — t042/t043.
- Crash de troca de mapa: banda histórica em `client+0x230daf`; a pasta de minidumps estava com **0** `.dmp` em 2026-11-06 03:1x, então qualquer dump novo é inequívoco da sessão testada.

### Regras que não mudam
- Só o usuário abre o CS2 e injeta. Nada de matar/reiniciar o processo por conta própria.
- Build, self-test e mock não comprovam runtime. Relato traz caminho, hash, exit code e log — ou diz que não há evidência.
- Feature de gameplay complexa (silent aim, backtrack, autowall de dano, prefire) só com ordem explícita nomeando a feature.
- Fake lag/choke, desync, body/manual yaw e resolver de inimigo: análise, nunca implementação.
- Reserva em `00_Gestao/Reservas.md` antes de editar; um build por ciclo; um escritor de código final.
"""


if __name__ == "__main__":
    sys.exit(main())
