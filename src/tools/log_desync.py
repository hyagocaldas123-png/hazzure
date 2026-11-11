#!/usr/bin/env python3
"""Filtra as linhas [DESYNC] do debug_log do Hazzure e diz o que elas significam.

Uso:
    python tools/log_desync.py                 # sessao atual (ultimo [START])
    python tools/log_desync.py --session 2     # penultima sessao
    python tools/log_desync.py --session all   # arquivo inteiro
    python tools/log_desync.py --csv d.csv     # exporta as amostras
    python tools/log_desync.py --watch 2       # acompanhar durante a partida
    python tools/log_desync.py --tail 20       # so as ultimas N amostras

Por que nao `tail -F`/Monitor direto: o LogDebug faz fopen+fclose por linha, entao
qualquer handle segurado faz todas as escritas do modulo falharem em silencio. Aqui o
arquivo e lido em blocos e fechado entre as sondas do --watch.
"""
import argparse
import io
import os
import re
import sys
import time

LOG_PADRAO = r"C:\HazzureKernel\debug_log.txt"
MARCADOR = b"[START] Start() finalizado"
BLOCO = 4 * 1024 * 1024
CAMPOS = ("cfg", "aplicado", "yawReal", "yawSent", "delta", "limit", "ok", "fail")
RE_LINHA = re.compile(r"^\[(\d\d:\d\d:\d\d)\.(\d\d\d)\] \[DESYNC\] (.*)$")
RE_KV = re.compile(r"(\w+)=(-?\d+(?:\.\d+)?)")


def sessoes(caminho):
    """(offset de byte, relogio) de cada [START] Start() finalizado."""
    achados = []
    with open(caminho, "rb") as f:
        pos = 0
        sobra = b""
        while True:
            bloco = f.read(BLOCO)
            if not bloco:
                break
            dados = sobra + bloco
            busca = 0
            while True:
                i = dados.find(MARCADOR, busca)
                if i < 0:
                    break
                inicio = dados.rfind(b"\n", 0, i) + 1
                relogio = dados[inicio + 1:inicio + 9].decode("ascii", "replace")
                achados.append((pos + inicio, relogio))
                busca = i + 1
            sobra = dados[-len(MARCADOR):]
            pos += len(dados) - len(sobra)
    return achados


def linhas_de(caminho, inicio, fim):
    com_fim = fim is not None
    with open(caminho, "rb") as f:
        f.seek(inicio)
        faltando = (fim if com_fim else os.fstat(f.fileno()).st_size) - inicio
        sobra = b""
        while faltando > 0:
            bloco = f.read(min(BLOCO, faltando))
            if not bloco:
                break
            faltando -= len(bloco)
            partes = (sobra + bloco).split(b"\n")
            sobra = partes.pop()
            for p in partes:
                yield p.decode("utf-8", "replace").rstrip("\r")
        if sobra and not com_fim:
            yield sobra.decode("utf-8", "replace").rstrip("\r")


def amostras(caminho, inicio, fim=None):
    dados = []
    aim_ultimo = None
    for linha in linhas_de(caminho, inicio, fim):
        m = RE_LINHA.match(linha)
        if m:
            pares = dict(RE_KV.findall(m.group(3)))
            if all(c in pares for c in CAMPOS):
                dados.append((m.group(1), m.group(2), {c: float(pares[c]) for c in CAMPOS}))
            else:
                print("linha [DESYNC] com campos inesperados: %s" % linha, file=sys.stderr)
        elif "[AIMTRIG]" in linha:
            aim_ultimo = linha.strip()
    return dados, aim_ultimo


def mediana(valores):
    v = sorted(valores)
    n = len(v)
    if not n:
        return float("nan")
    return v[n // 2] if n % 2 else (v[n // 2 - 1] + v[n // 2]) / 2.0


def veredito(dados):
    if not dados:
        return ["SEM linhas [DESYNC] na sessao: o modulo em memoria \u00e9 anterior ao t071 "
                "(cruze o [START] com o mtime da DLL) ou o caminho nunca rodou \u2014 AA/Desync "
                "desligados em partida ainda geram linha com cfg=0, entao aus\u00eancia = build antigo."]
    out = []
    ligadas = [d for d in dados if d[2]["cfg"] == 1]
    aplicadas = [d for d in dados if d[2]["aplicado"] == 1]
    out.append("amostras=%d  cfg=1: %d  aplicado=1: %d  ok: %d->%d  fail: %d->%d" % (
        len(dados), len(ligadas), len(aplicadas),
        int(dados[0][2]["ok"]), int(dados[-1][2]["ok"]),
        int(dados[0][2]["fail"]), int(dados[-1][2]["fail"])))
    dit = [abs(d[2]["delta"]) for d in aplicadas]
    if dit:
        out.append("delta |deg|: min=%.2f mediana=%.2f max=%.2f  (limit=%.0f no fim)" % (
            min(dit), mediana(dit), max(dit), aplicadas[-1][2]["limit"]))
    if not ligadas:
        out.append("cfg=0 em todas: Desync desligado no config desta sessao.")
    elif not aplicadas:
        out.append("cfg=1 e aplicado=0 em todas: o StampSubtickViewAngles NAO escreveu nas "
                   "entries do input_history_ — o canal da T070/T071 esta falseado nesta build.")
    elif max(dit) < 0.5:
        out.append("aplicado=1 com delta ~0: o yaw enviado e a propria camera (offset do AA "
                   "zerado ou cortado pelo fake limit) — nao e desync de verdade.")
    oks = {int(d[2]["ok"]) for d in dados}
    if len(dados) > 1 and len(oks) == 1 and aplicadas:
        out.append("ok parou de subir entre amostras: a escrita rodou uma vez so.")
    return out


def tabela(dados, tail=None):
    recorte = dados[-tail:] if tail else dados
    return ["%s.%s  cfg=%d aplicado=%d  real=%8.2f  enviado=%8.2f  delta=%7.2f  limit=%3.0f  "
            "ok=%d  fail=%d" % (h, ms, int(c["cfg"]), int(c["aplicado"]), c["yawReal"],
                                c["yawSent"], c["delta"], c["limit"], int(c["ok"]), int(c["fail"]))
            for h, ms, c in recorte]


def csv_de(dados, caminho):
    with io.open(caminho, "w", encoding="utf-8", newline="") as f:
        f.write("hhmmss,ms,%s\n" % ",".join(CAMPOS))
        for h, ms, c in dados:
            f.write("%s,%s,%s\n" % (h.replace(":", ""), ms,
                                    ",".join(repr(c[k]) for k in CAMPOS)))
    print("csv: %s (%d amostras)" % (caminho, len(dados)))


def relatorio(caminho, inicio, fim, tail):
    dados, aim = amostras(caminho, inicio, fim)
    for linha in tabela(dados, tail):
        print(linha)
    if aim:
        print("ultimo [AIMTRIG]: %s" % aim)
    for v in veredito(dados):
        print("=> %s" % v)
    return dados


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--log", default=LOG_PADRAO)
    ap.add_argument("--session", default="last", help="last | 2 | 3 | all")
    ap.add_argument("--tail", type=int, default=None)
    ap.add_argument("--csv", default=None, metavar="ARQUIVO")
    ap.add_argument("--watch", type=int, default=0, metavar="SEG",
                    help="reabre a cada SEG segundos e imprime so as linhas novas")
    args = ap.parse_args()

    if not os.path.exists(args.log):
        print("log nao existe: %s" % args.log, file=sys.stderr)
        return 2

    if args.session == "all":
        inicio, fim, rotulo = 0, None, "arquivo inteiro"
    else:
        marcas = sessoes(args.log)
        if not marcas:
            print("nenhum [START] Start() finalizado em %s" % args.log, file=sys.stderr)
            return 2
        idx = -1 if args.session == "last" else -int(args.session)
        if abs(idx) > len(marcas):
            print("so ha %d sessoes no log" % len(marcas), file=sys.stderr)
            return 2
        pos, relogio = marcas[idx]
        fim = marcas[idx + 1][0] if idx != -1 else None
        inicio = pos
        rotulo = "sessao iniciada em %s (byte %d)%s" % (
            relogio, pos, "" if fim is None else " ate %s" % marcas[idx + 1][1])

    print("log=%s  %s" % (args.log, rotulo))
    dados = relatorio(args.log, inicio, fim, args.tail)
    if args.csv:
        csv_de(dados, args.csv)

    if args.watch > 0:
        visto = os.path.getsize(args.log)
        print("watch ON (reabre a cada %ds, sem segurar handle)" % args.watch)
        try:
            while True:
                time.sleep(args.watch)
                tamanho = os.path.getsize(args.log)
                if tamanho < visto:
                    print("log recriado: %d -> %d" % (visto, tamanho))
                    visto = 0
                if tamanho > visto:
                    novos, _ = amostras(args.log, visto, tamanho)
                    visto = tamanho
                    if novos:
                        for linha in tabela(novos):
                            print(linha)
                        for v in veredito(novos):
                            print("=> %s" % v)
        except KeyboardInterrupt:
            print("watch OFF")
    return 0


if __name__ == "__main__":
    sys.exit(main())
