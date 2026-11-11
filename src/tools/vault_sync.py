#!/usr/bin/env python3
"""Gera a parte derivavel do inventario do vault a partir do codigo e do deploy.

Escreve em C:/Codex2/Hazzure_Projeto/30_Inventario/:
  MENU.md         aba -> secao -> widget -> campo de g_cfg -> default -> arquivo:linha
  CONFIG_KEYS.md  chaves nomeadas lidas por RuntimeBool/Float/Int
  OFFSETS.md      offsets gerados do client.dll + RVAs de funcoes assinadas
  ARTEFATOS.md    DLL instalada, tamanho, SHA-256, rollbacks

Deterministico: o corpo so muda quando muda o codigo (ou o deploy, no caso de
ARTEFATOS.md). `--check` compara apenas o corpo, ignora o rodape, e sai 1 quando o
vault esta atrasado. Depois disso o mesmo `--check` roda `tools/vault_lint.py`
(integridade do vault em si: encoding, frontmatter, links, duplicatas) e sai 1 se
houver achado — as duas checagens andam juntas.

Heuristicas assumidas (documentadas no proprio MENU.md):
  - aba vem do roteador `DrawGSPanelSection()` em GSLayout.inl (case N -> funcoes
    chamadas / widgets inline daquele span); widget dentro de funcao nao chamada pelo
    roteador cai em "(fora do roteador)".
  - secao = GS_SectionHeader / BeginCleanCard / ImGui::CollapsingHeader mais proximo
    acima do widget.
  - rotulo `##id` (oculto) e trocado pelo TextUnformatted/Text mais proximo acima.
  - uma linha pode ter varios widgets; todos sao coletados.

Uso:  python tools/vault_sync.py [--check] [--only MENU,OFFSETS]
"""
import hashlib
import re
import sys
import time
from pathlib import Path

SRC = Path(r"C:/Codex2/gpt/HazzureInternal/src")
OFF_H = Path(r"C:/Codex2/gpt/Compartilhado/offsets_generated.h")
KERNEL = Path(r"C:/HazzureKernel")
OUT = Path(r"C:/Codex2/Hazzure_Projeto/30_Inventario")
FOOTER = "<!-- gerado por tools/vault_sync.py"
LAYOUT = SRC / "GSLayout.inl"

HEADER = """<!-- NAO EDITAR A MAO ({what}): gerado por tools/vault_sync.py
     Fonte da verdade = codigo-fonte + C:/HazzureKernel. Rodar: python tools/vault_sync.py -->
"""


def sources():
    files = list(SRC.glob("*.inl")) + list(SRC.glob("*.h")) + \
        list(SRC.glob("*.cpp")) + list((SRC / "cards").glob("*.inl"))
    return sorted(p for p in files if ".bak" not in p.name and p.exists())


WIDGET_RE = re.compile(
    r'(?:GSWidgets::)?(?P<kind>GS_ToggleWithKeybind|GS_Toggle|GS_SliderFloat|GS_SliderInt|'
    r'GS_Combo|GS_MultiCombo|GS_ColorRow|GS_Keybind|GS_InventorySlot|'
    r'CleanToggle|CleanSliderFloat|CleanSliderInt|CleanCombo|CleanColorRow|CleanKeybind|'
    r'ImGui::Checkbox|ImGui::Combo|ImGui::SliderFloat|ImGui::SliderInt|ImGui::InputText|'
    r'ImGui::InputFloat|ImGui::InputInt)\s*\(\s*"(?P<label>[^"]*)"(?P<rest>[^;\n]*)')
SECTION_RE = re.compile(
    r'(?:GS_SectionHeader|BeginCleanCard|ImGui::CollapsingHeader)\(\s*"([^"#][^"]*)"')
FUNC_RE = re.compile(r'(?:static|inline)\s+(?:void|bool|int|float)\s+(Draw[A-Za-z0-9_]+)\s*\(')
FN_ANY_RE = re.compile(
    r'^(?:static\s+|inline\s+|extern\s+)?'
    r'(?:void|bool|int|float|unsigned\s+int|size_t|const\s+char\s*\*|auto)\s+\**'
    r'([A-Za-z0-9_]+)\s*\(')
FIELD_RE = re.compile(r'&g_cfg\.([A-Za-z0-9_]+)')
TEXT_RE = re.compile(r'ImGui::(?:TextUnformatted|Text|TextColored)\(\s*"([^"#][^"]*)"')
RUNTIME_RE = re.compile(r'Runtime(Bool|Float|Int)\(\s*"([A-Za-z0-9_]+)"\s*,\s*'
                        r'((?:\([^()]*\)|[^,\n)])+)')
DECL_RE = re.compile(r'^\s*(?:bool|int|unsigned|signed|float|double|char|uint\d+_t|int\d+_t|size_t)\b')
NAME_VAL_RE = re.compile(r'([A-Za-z_][A-Za-z0-9_]*)\s*(?:\[[^\]]*\])?\s*=\s*(\{[^}]*\}|[^,;]+)')


def read_lines(path):
    return path.read_text(encoding="utf-8", errors="replace").splitlines()


def md(s):
    return str(s).replace("|", "\\|").replace("\n", " ").strip()


def overlay_defaults():
    """campo -> default, lido de `struct OverlayConfig` (UIState.inl)."""
    out, inside = {}, False
    for ln in read_lines(SRC / "UIState.inl"):
        if ln.startswith("struct OverlayConfig"):
            inside = True
            continue
        if not inside:
            continue
        if re.match(r"^\};", ln):
            break
        code = re.sub(r"//.*", "", ln)
        if not DECL_RE.match(code):
            continue
        for name, val in NAME_VAL_RE.findall(code):
            v = re.sub(r"(?<=[\d.])f$", "", val.strip())
            out.setdefault(name, v)
    return out


def navigation():
    """(abas, secoes_por_aba, ocultas) a partir das tabelas C de GSLayout.inl."""
    lines = read_lines(LAYOUT)
    tabs, sections, hidden = [], [], []
    idx = next((i for i, ln in enumerate(lines) if "gs_mainTabs[" in ln), None)
    if idx is not None:
        end = next(i for i in range(idx, len(lines)) if lines[i].startswith("};"))
        for ln in lines[idx:end]:
            m = re.search(r'"([^"]+)"', ln)
            if m:
                tabs.append(m.group(1))
    hidx = next((i for i, ln in enumerate(lines) if "gs_hiddenTabs[" in ln and "=" in ln),
                None)
    if hidx is not None:
        eidx = next(i for i in range(hidx, len(lines)) if lines[i].startswith("};"))
        blob = " ".join(lines[hidx:eidx])
        hidden = [v == "true" for v in re.findall(r"\b(true|false)\b", blob)]
    sidx = next((i for i, ln in enumerate(lines) if "gs_tabSections[" in ln), None)
    if sidx is not None:
        eidx = next(i for i in range(sidx, len(lines)) if lines[i].startswith("};"))
        row = None
        for ln in lines[sidx:eidx]:
            if "{" in ln:
                row = re.findall(r'"([^"]+)"', ln)
            if "}" in ln and row is not None:
                sections.append(row)
                row = None
    return tabs, sections, hidden


def routing():
    """funcoes chamadas pelo roteador -> (tab, subsecao) + spans de linha do switch."""
    lines = read_lines(LAYOUT)
    start = next((i for i, ln in enumerate(lines)
                  if ln.startswith("static void DrawGSPanelSection(")), None)
    if start is None:
        return {}, []
    depth, end = 0, len(lines)
    for i in range(start, len(lines)):
        depth += lines[i].count("{") - lines[i].count("}")
        if i > start and depth <= 0:
            end = i
            break
    body = lines[start:end]
    cases = []
    for off, ln in enumerate(body):
        m = re.match(r"\s*case (\d+):\s*(?://(.*)?)?", ln)
        if m:
            cases.append((start + off, int(m.group(1)), (m.group(2) or "").strip()))
    spans = []
    for n, (line, tab, comment) in enumerate(cases):
        stop = cases[n + 1][0] if n + 1 < len(cases) else end
        spans.append((line, stop, tab, comment))
    funcs, line_tab = {}, {}
    for line, stop, tab, _ in spans:
        section = 0
        for off in range(line, stop):
            ln = body[off - start]
            sm = re.search(r"section\s*==\s*(\d+)", ln)
            if sm:
                section = int(sm.group(1))
            called = [fm.group(1) for fm in re.finditer(r'\b(Draw[A-Za-z0-9_]+)\s*\(', ln)
                      if fm.group(1) not in ("DrawGSPanelSection", "DrawGSPanel")]
            if called:
                for name in called:
                    funcs.setdefault(name, (tab, section))
            elif WIDGET_RE.search(ln):
                line_tab[off + 1] = (tab, section)
    return funcs, line_tab


def callgraph():
    """funcao -> funcoes de desenho que ela chama (para atravessar os forwarders)."""
    edges = {}
    for path in sources():
        cur = None
        for ln in read_lines(path):
            f = FUNC_RE.search(ln)
            if f:
                cur = f.group(1)
            if not cur:
                continue
            for fm in re.finditer(r'\b(Draw[A-Za-z0-9_]+)\s*\(', ln):
                if fm.group(1) != cur:
                    edges.setdefault(cur, []).append(fm.group(1))
    return edges


def sub_name(tab_sections, hit):
    """nome real da sub-aba (topbar) para (tab, indice) vindo do roteador."""
    tab, sec = hit
    if tab < len(tab_sections) and sec < len(tab_sections[tab]):
        return tab_sections[tab][sec]
    return f"sub-aba {sec}"


def split_args(s):
    """Divide por vírgula de nível 0, respeitando parênteses, colchetes e aspas.

    `s` é o resto da linha depois do rótulo: o parêntese da chamada já foi consumido
    pelo WIDGET_RE, então os argumentos estão no nível 0 e o `)` que fecha a chamada
    encerra a lista.
    """
    out, buf, depth, quote = [], "", 0, False
    for ch in s:
        if quote:
            buf += ch
            if ch == '"':
                quote = False
            continue
        if ch == '"':
            quote = True
            buf += ch
            continue
        if ch in "([{":
            depth += 1
        elif ch == ")":
            if depth == 0:
                break                      # fecha a chamada; o resto nao e argumento
            depth -= 1
        elif ch in "]}":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(buf.strip())
            buf = ""
        else:
            buf += ch
    out.append(buf.strip())
    return [a for a in out if a]


def visible_label(lines, n, raw, prefix=""):
    if not raw.startswith("##"):
        return raw
    same = TEXT_RE.findall(prefix)          # rotulo visivel na mesma linha, antes do widget
    if same:
        return same[-1]
    for back in range(n - 1, max(0, n - 5), -1):
        t = TEXT_RE.search(lines[back - 1])
        if t:
            return t.group(1)
    return raw


SUBTAB_RE = re.compile(r'(?:s_[A-Za-z0-9_]*[Ss]ubTab|[A-Za-z0-9_]*SubTab)\s*==\s*(\d+)')
QUOTED_RE = re.compile(r'"([^"]+)"')
TABARRAY_RE = re.compile(r'const char\*\s*([A-Za-z0-9_]+)\s*\[\]\s*=\s*\{([^}]*)\}')


def subtab_arrays(path):
    """funcao -> nomes dos botoes de sub-aba. Heuristica: o unico array de `const char*`
    da funcao cujo nome contiene "tab"; se nao houver, a sub-aba nao e refinada."""
    out = {}
    fn, names = None, None
    for ln in read_lines(path):
        f = FUNC_RE.search(ln)
        if f:
            fn, names = f.group(1), None
        if not fn:
            continue
        arr = TABARRAY_RE.search(ln)
        if arr and "tab" in arr.group(1).lower() and names is None:
            names = QUOTED_RE.findall(arr.group(2))
        if names:
            out[fn] = names
    return out


def menu_doc():
    defaults = overlay_defaults()
    tabs, tab_sections, hidden = navigation()
    funcs, line_tab = routing()
    edges = callgraph()
    queue = [(n, t, s) for n, (t, s) in funcs.items()]
    while queue:                        # aba via forwarders (DrawSettingsCardN -> card)
        name, tab, sec = queue.pop(0)
        for callee in edges.get(name, []):
            if callee not in funcs:
                funcs[callee] = (tab, sec)
                queue.append((callee, tab, sec))
    files = sources()

    rows, seen = [], set()
    for path in files:
        rel = path.relative_to(SRC).as_posix()
        lines = read_lines(path)
        arrays = subtab_arrays(path)
        fn, section, alt = None, "(topo do arquivo)", None
        for n, ln in enumerate(lines, 1):
            f = FN_ANY_RE.search(ln)
            if f:
                fn, section, alt = f.group(1), "(sem secao)", None
            s = SECTION_RE.search(ln)
            if s:
                section = s.group(1)
            hit = line_tab.get(n) if path == LAYOUT else None
            if hit is None:
                hit = funcs.get(fn)
            st = SUBTAB_RE.search(ln)
            if st and arrays.get(fn):
                i = int(st.group(1))
                if i < len(arrays[fn]):
                    alt = arrays[fn][i]
            for w in WIDGET_RE.finditer(ln):
                kind = w.group("kind")
                if fn and (fn == kind or fn.endswith(kind.split("::")[-1])):
                    continue        # chamada dentro da propria implementacao do widget
                args = split_args(w.group("rest"))
                primary = args[0] if args else ""
                ref = FIELD_RE.match(primary)
                field = ref.group(1) if ref else primary.strip("& ")
                extra = ", ".join(a.replace("&g_cfg.", "").lstrip("&")
                                  for a in args[1:] if "&g_cfg." in a)
                label = visible_label(lines, n, w.group("label"), ln[:w.start()])
                key = (rel, n, label)
                if key in seen:
                    continue
                seen.add(key)
                tab, sub = None, None
                if hit:
                    tab, sub = hit[0], alt or sub_name(tab_sections, hit)
                rows.append({
                    "tab_idx": tab,
                    "tab": tabs[tab] if tab is not None and tab < len(tabs)
                    else "(fora do roteador)",
                    "sub": sub,
                    "section": section,
                    "kind": w.group("kind").replace("ImGui::", "ImGui "),
                    "label": label,
                    "field": field,
                    "extra": extra,
                    "default": defaults.get(field, "") if ref else "",
                    "rel": rel, "n": n,
                })

    rows.sort(key=lambda r: (r["tab_idx"] if r["tab_idx"] is not None else 999,
                             r["sub"] or "", r["rel"], r["n"]))

    parts = [HEADER.format(what="menu"),
             "# Inventário do menu (gerado)\n\n",
             "_Gerado por `python tools/vault_sync.py` a partir do código. "
             "Não edite à mão._\n\n",
             "Como ler: **aba** = barra lateral real (`gs_mainTabs`), **seção** = cabeçalho "
             "mais próximo acima do widget (`GS_SectionHeader` / `BeginCleanCard` / "
             "`CollapsingHeader`), **campo** = o que o widget escreve — `g_cfg.<campo>` quando é "
             "campo de `UIState.inl:122 struct OverlayConfig` (aí `default` vem da declaração), "
             "ou uma variável local / `shared->…` quando o card escreve por IPC no lado do jogo "
             "(nesse caso não há default no struct), **origem** = `arquivo:linha` do widget.\n\n",
             "A UI ativa é a do GSLayout (`g_useGSLayout = true` em `UIState.inl`; o roteador é "
             "`GSLayout.inl: DrawGSPanelSection`). Widget em "
             "`(fora do roteador)` está numa função que o GSLayout não chama — UI legada de "
             "`UIPanel.inl` ou código morto; trate como não visitável até prova em jogo.\n\n",
             "Limites desta tabela (parser de texto, não execução): a **sub-aba** vem do roteador "
             "ou dos botões locais da função; quando um card é chamado dentro de um bloco de "
             "sub-aba de outra função (ex.: `DrawLegitbotMemeSense`) ou quando a própria função "
             "gerencia as sub-abas internamente (`DrawLocalInventory`), a sub-aba pode estar "
             "aproximada — a **aba** e o **arquivo:linha** são exatos. Label montado por "
             "concatenação ou dentro de macro pode não aparecer aqui. Rótulo oculto do ImGui "
             "(`##id`) é resolvido pelo `ImGui::TextUnformatted` correspondente na mesma linha "
             "ou até 4 linhas acima; quando não resolve, o `##id` aparece cru.\n\n",
             "O total inclui as duas UIs do repositório: a ativa (GSLayout + cards por "
             "`DrawSettingsCardN`) e a legada (`UIPanel.inl`). Por isso o mesmo campo pode "
             "aparecer duas vezes com origens diferentes — a linha do GSLayout é a que o "
             "usuário vê; a do `UIPanel.inl` não é renderizada enquanto "
             "`g_useGSLayout = true`.\n\n"]

    if tabs:
        parts.append("## Navegação real\n\n"
                     "`visível = não` vem de `gs_hiddenTabs[i]`: a aba não aparece na sidebar "
                     "e, se for selecionada pela busca, é remapeada para outra "
                     "(`GSLayout.inl:919-923 ValidateNavigationIndices`; o remapeamento atual "
                     "está nas linhas 921-922). O `case` dela continua no roteador, mas não é "
                     "alcançável pela sidebar.\n\n"
                     "| # | aba | visível | seções (topbar) |\n|---|---|---|---|\n")
        for i, name in enumerate(tabs):
            sec = ", ".join(tab_sections[i]) if i < len(tab_sections) else ""
            vis = "não" if i < len(hidden) and hidden[i] else "sim"
            parts.append(f"| {i} | {md(name)} | {vis} | {md(sec)} |\n")
        parts.append("\n")

    parts.append("## Widgets por aba e sub-aba (topbar)\n\n")
    current = None
    for r in rows:
        key = (r["tab"], r["sub"])
        if key != current:
            if current is not None:
                parts.append("\n")
            n_same = sum(1 for x in rows if (x["tab"], x["sub"]) == key)
            head = r["tab"] + (f" › {r['sub']}" if r["sub"] else "")
            parts.append(f"### {md(head)} — {n_same} widgets\n\n"
                         "| seção | rótulo | widget | campo | default | origem |\n"
                         "|---|---|---|---|---|---|\n")
            current = key
        extra = f" + `{r['extra']}`" if r["extra"] else ""
        parts.append(f"| {md(r['section'])} | {md(r['label'])} | `{md(r['kind'])}` | "
                     f"`{r['field']}`{extra} | `{md(r['default'])}` | `{r['rel']}:{r['n']}` |\n")

    parts.append(f"\n---\n\n**{len(rows)} widgets** em {len(files)} arquivos lidos "
                 "(`.bak-*` excluídos).\n")
    return "".join(parts), len(rows)


def config_keys_doc():
    rows = []
    for path in sources():
        for n, ln in enumerate(read_lines(path), 1):
            if ln.lstrip().startswith("//"):
                continue
            for m in RUNTIME_RE.finditer(ln):
                rows.append((m.group(2), m.group(1), m.group(3).strip(),
                             path.relative_to(SRC).as_posix(), n))
    rows.sort(key=lambda r: (r[0], r[3], r[4]))
    parts = [HEADER.format(what="config"),
             "# Chaves de runtime e perfil (gerado)\n\n",
             "_Gerado por `python tools/vault_sync.py`. Não edite à mão._\n\n",
             "Cada linha é uma chamada real de `RuntimeBool/RuntimeFloat/RuntimeInt(nome, "
             "fallback, contexto, classeDaArma)` (implementação em "
             "`OverlayPersistence.h:420`). A **chave** é o nome pesquisável em perfil/override "
             "por arma; o **fallback** é o que vale quando o perfil não define a chave — em "
             "quase todos os casos é o próprio campo de `g_cfg`, ou seja, o que o menu "
             "escreve.\n\n",
             "Escopo: esta lista é só a **ponte perfil/override**. Um campo de `OverlayConfig` "
             "que o runtime lê direto de `g_cfg` sem chave nomeada (ex.: `noScopeOverlay`, "
             "`scopeFilterAlpha`) não aparece aqui por construção — ele está na coluna **campo** "
             "de `MENU.md`, e é esse o caminho para responder \"qual chave controla X\": procure "
             "no MENU, e só procure aqui se existir leitura `Runtime*(\"nome\")`.\n\n",
             "| chave | tipo | fallback | lida em |\n|---|---|---|---|\n"]
    for k, t, f, p, n in rows:
        parts.append(f"| `{k}` | {t} | `{md(f)}` | `{p}:{n}` |\n")
    parts.append(f"\n---\n\n**{len(rows)} leituras nomeadas**.\n")
    return "".join(parts), len(rows)


def offsets_doc():
    lines = read_lines(OFF_H)
    preamble = [ln[3:].strip() for ln in lines[:8] if ln.startswith("//")]
    body, count = [], 0
    group = None
    for ln in lines:
        ns = re.match(r"\s*namespace ([A-Za-z0-9_]+) \{", ln)
        if ns:
            group = ns.group(1)
            body.append(f"\n### `{group}`\n\n| símbolo | valor |\n|---|---|\n")
            continue
        f = re.match(r"\s*inline (?:constexpr )?(?:uintptr_t|uint\d+_t|int\d+_t|unsigned|int|size_t)\s+"
                     r"([A-Za-z0-9_]+)\s*=\s*([^;]+);", ln)
        if not f:
            continue
        body.append(f"| `{f.group(1)}` | `{f.group(2).strip()}` |\n")
        count += 1

    parts = [HEADER.format(what="offsets"),
             "# Offsets e RVAs (gerado)\n\n",
             "_Gerado por `python tools/vault_sync.py` a partir de "
             "`Compartilhado/offsets_generated.h`, que por sua vez é produzido por "
             "`python tools/offset_dumper.py` contra o `client.dll` instalado. "
             "Não edite à mão._\n\n"]
    for h in preamble:
        parts.append(f"> {md(h)}\n\n")
    parts.append("Todos os valores são **RVAs relativos** ao handle do módulo, nunca "
                 "endereços absolutos: a base muda a cada boot.\n")
    parts.extend(body)

    sk = SRC / "SkinAddresses.h"
    if sk.exists():
        parts.append("\n## Funções assinadas (`src/SkinAddresses.h`)\n\n"
                     "Cada RVA só é usado depois de conferir os 16 bytes da assinatura; se o "
                     "build do CS2 mudar, a checagem falha e o caminho é re-dumpar a função, "
                     "não remapear no chute.\n\n"
                     "| símbolo | RVA (`client+`) | assinatura (16 bytes) |\n"
                     "|---|---|---|\n")
        for ln in read_lines(sk):
            m = re.match(r"static constexpr CheckedAddress ([A-Z_]+)=\{0x([0-9A-Fa-f]+),"
                         r"\{(.*?)\}\};", ln)
            if not m:
                continue
            sig = re.findall(r"0x[0-9A-Fa-f]+", m.group(3))
            hexs = " ".join(f"{int(b, 16):02X}" for b in sig)
            parts.append(f"| `{m.group(1)}` | `0x{m.group(2).upper()}` | `{hexs}` |\n")
    parts.append(f"\n---\n\n**{count} offsets** no header gerado + RVAs assinados acima.\n")
    return "".join(parts), count


def artifacts_doc():
    parts = [HEADER.format(what="artefatos"),
             "# Artefatos instalados (gerado)\n\n",
             "_Gerado por `python tools/vault_sync.py` lendo `C:\\HazzureKernel`. "
             "Não edite à mão._\n\n",
             "**DLL em disco ≠ DLL em memória.** O módulo é injetado uma vez por processo do "
             "CS2 e não tem hot reload: para saber o que roda agora, cruze a linha "
             "`[START] Start() finalizado` de `C:\\HazzureKernel\\debug_log.txt` com o mtime e o "
             "SHA-256 abaixo. Quem abre o CS2 e injeta é sempre o usuário.\n\n",
             "| arquivo | bytes | SHA-256 (12) | modificado |\n|---|---|---|---|\n"]
    targets = sorted(KERNEL.glob("Hazzure.Internal.dll*")) if KERNEL.exists() else []
    if (KERNEL / "modules").exists():
        targets += sorted((KERNEL / "modules").glob("Hazzure.Internal.dll*"))
    for p in targets:
        data = p.read_bytes()
        where = "" if p.parent == KERNEL else "modules/"
        parts.append(f"| `{where}{p.name}` | {len(data):,} | "
                     f"`{hashlib.sha256(data).hexdigest()[:12]}` | "
                     f"{time.strftime('%Y-%m-%d %H:%M', time.localtime(p.stat().st_mtime))} |\n")
    parts.append("\nRollback: cada tarefa tem o seu `.bak-AAAAMMDD-tNNN` (ou sufixo descritivo) "
                 "listado acima; `.bak` sem sufixo é a cópia mais antiga. Build, deploy e "
                 "injeção: `70_Ferramentas/FERRAMENTAS.md`.\n")
    parts.append(f"\n---\n\n**{len(targets)} artefatos** listados.\n")
    return "".join(parts), len(targets)


def stamp(text):
    return ("\n<!-- gerado por tools/vault_sync.py — corpo "
            f"{hashlib.sha1(text.encode('utf-8')).hexdigest()[:12]} -->\n")


def main():
    check = "--check" in sys.argv
    only = [x.strip().upper() for x in
            (sys.argv[sys.argv.index("--only") + 1].split(",")
             if "--only" in sys.argv else [])]
    gens = {"MENU": menu_doc, "CONFIG_KEYS": config_keys_doc,
            "OFFSETS": offsets_doc, "ARTEFATOS": artifacts_doc}
    if only:
        gens = {k: v for k, v in gens.items() if k in only}
    OUT.mkdir(parents=True, exist_ok=True)
    stale = []
    for name in sorted(gens):
        body, count = gens[name]()
        target = OUT / f"{name}.md"
        old = target.read_text(encoding="utf-8") if target.exists() else ""
        old_body = old.split(FOOTER)[0] if old else ""
        print(f"{name:12} {count:5} itens -> {target}")
        if check:
            if old_body.rstrip() != body.rstrip():
                stale.append(name)
        else:
            target.write_text(body + stamp(body), encoding="utf-8", newline="\n")
    if check:
        if stale:
            print("DRIFT: desatualizado em " + ", ".join(stale) +
                  " — rode python tools/vault_sync.py")
            code = 1
        else:
            print("ok: inventario bate com o codigo/deploy")
            code = 0
        # o inventario bater nao diz se o vault esta internamente consistente; o --check
        # passa a rodar o verificador do vault tambem (tools/vault_lint.py, so leitura)
        import vault_lint
        bad, n = vault_lint.run()
        for kind, path, info in bad:
            print(f"VAULT-LINT {kind:26} {path}  {info}")
        if bad:
            print(f"VAULT-LINT: {len(bad)} achado(s) em {n} arquivos .md")
            return 1
        print(f"ok: vault internamente consistente ({n} .md, lido por tools/vault_lint.py)")
        return code
    return 0


if __name__ == "__main__":
    sys.exit(main())
