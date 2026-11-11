#!/usr/bin/env python3
"""Testes offline do `tools/log_desync.py` — não precisa de jogo, nem do log real.

    python tools/test_log_desync.py

Cobre as decisões do veredito (as cinco saídas possíveis), o recorte de sessão por
offset de byte (com BLOCO encolhido, para forçar marcador atravessando fronteira de
bloco), CRLF, --tail, --csv, linha malformada, e o --watch incluindo a propriedade que
justifica ele existir: não segurar handle do log entre as sondas.
"""
import io
import math
import os
import re
import subprocess
import sys
import tempfile
import time

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)
import log_desync as ld  # noqa: E402

SCRIPT = os.path.join(AQUI, "log_desync.py")
FALHAS = []


def checa(nome, cond, detalhe=""):
    print(("PASS: " if cond else "FALHOU: ") + nome + ("" if cond else "  <- " + str(detalhe)))
    if not cond:
        FALHAS.append(nome)


def linha(rel, cfg, apl, real, sent, limit=35, ok=0, fail=0):
    delta = ((sent - real + 540.0) % 360.0) - 180.0
    return ("[%s.000] [DESYNC] cfg=%d aplicado=%d yawReal=%.2f yawSent=%.2f delta=%.2f "
            "limit=%.0f ok=%d fail=%d" % (rel, cfg, apl, real, sent, delta, limit, ok, fail))


def amostra(cfg, apl, real, sent, ok=0, fail=0, limit=35.0):
    delta = ((sent - real + 540.0) % 360.0) - 180.0
    return ("00:00:00", "000", {"cfg": float(cfg), "aplicado": float(apl), "yawReal": real,
                                "yawSent": sent, "delta": delta, "limit": float(limit),
                                "ok": float(ok), "fail": float(fail)})


def texto_veredito(dados):
    return "\n".join(ld.veredito(dados))


def caso_veredito():
    checa("sem linha [DESYNC] -> manda conferir o build",
          "SEM linhas [DESYNC]" in texto_veredito([]), texto_veredito([]))

    v = texto_veredito([amostra(0, 0, 10.0, 10.0), amostra(0, 0, 12.0, 12.0)])
    checa("cfg=0 em todas -> diz que o desync estava desligado", "cfg=0 em todas" in v, v)
    checa("cfg=0 nao dispara o alerta de canal falseado", "NAO escreveu" not in v, v)

    v = texto_veredito([amostra(1, 0, 10.0, 0.0, ok=0), amostra(1, 0, 11.0, 0.0, ok=0)])
    checa("cfg=1 + aplicado=0 -> canal falseado", "NAO escreveu" in v, v)
    checa("aplicado=0 nao gera leitura de delta", "delta |deg|" not in v, v)

    v = texto_veredito([amostra(1, 1, 20.0, 20.0, ok=5), amostra(1, 1, 21.0, 21.02, ok=9)])
    checa("aplicado=1 com delta ~0 -> nao e desync de verdade", "nao e desync" in v, v)

    v = texto_veredito([amostra(1, 1, 20.0, 50.0, ok=5), amostra(1, 1, 22.0, 55.0, ok=9)])
    checa("aplicado=1 com delta real -> sem alarme", "nao e desync" not in v and "NAO escreveu" not in v, v)
    checa("aplicado=1 -> resume o delta medido", "delta |deg|:" in v and "min=30.00 mediana=31.50 max=33.00" in v, v)
    checa("ok subindo nao dispara 'parou de subir'", "parou de subir" not in v, v)

    v = texto_veredito([amostra(1, 1, 20.0, 50.0, ok=7), amostra(1, 1, 25.0, 55.0, ok=7)])
    checa("ok congelado em 2 amostras -> disparou", "parou de subir" in v, v)

    v = texto_veredito([amostra(1, 1, 20.0, 50.0, ok=7)])
    checa("uma unica amostra NAO grita 'parou de subir'", "parou de subir" not in v, v)

    v = texto_veredito([amostra(1, 1, 20.0, 50.0, ok=1, fail=4)])
    checa("mostra ok e fail nas pontas", "ok: 1->1" in v and "fail: 4->4" in v, v)

    checa("mediana impar/par/vazio",
          ld.mediana([1, 2, 3]) == 2 and ld.mediana([1, 2, 3, 4]) == 2.5
          and math.isnan(ld.mediana([])))


def escreve_log(caminho, sessoes):
    """sessoes: lista de listas de linhas. Grava com CRLF, como o LogDebug."""
    with io.open(caminho, "w", encoding="utf-8", newline="") as f:
        for linhas in sessoes:
            f.write("[00:00:00.000] [KIERO] Resident Present/ResizeBuffers ready=1\r\n")
            f.write("[00:00:00.001] [START] Start() finalizado com sucesso.\r\n")
            for l in linhas:
                f.write(l + "\r\n")


def caso_sessoes(pasta):
    caminho = os.path.join(pasta, "log.txt")
    s1 = [linha("01:00:00", 1, 0, 5.0, 0.0, ok=0), linha("01:00:01", 1, 0, 6.0, 0.0, ok=0)]
    s2 = [linha("02:00:00", 0, 0, 7.0, 7.0, ok=0),
          "[02:00:01] [AIMTRIG] aimWriteOk=3 silentApplied=0 shotTick=0",
          linha("02:00:02", 0, 0, 8.0, 8.0, ok=0)]
    s3 = [linha("03:00:00", 1, 1, 10.0, 45.0, limit=35, ok=11),
          linha("03:00:01", 1, 1, 12.0, 47.0, limit=35, ok=19)]
    escreve_log(caminho, [s1, s2, s3])

    marcas = ld.sessoes(caminho)
    checa("acha as 3 sessoes", len(marcas) == 3, marcas)
    checa("rotulo traz o relogio da linha do [START]",
          [m[1] for m in marcas] == ["00:00:00", "00:00:00", "00:00:00"], marcas)

    original = ld.BLOCO
    try:
        ld.BLOCO = 37  # força o marcador a atravessar fronteira de bloco
        checa("sessoes() acha os 3 [START] com bloco de 37 bytes", len(ld.sessoes(caminho)) == 3,
              len(ld.sessoes(caminho)))
    finally:
        ld.BLOCO = original

    ultimo = marcas[-1][0]
    fim = marcas[1][0]
    d_ultimo, aim_ultimo = ld.amostras(caminho, ultimo)
    checa("sessao da frente so ve as linhas dela", len(d_ultimo) == 2 and aim_ultimo is None,
          (len(d_ultimo), aim_ultimo))
    meio, aim_meio = ld.amostras(caminho, marcas[1][0], ultimo)
    checa("recorte com fim nao vaza para a sessao seguinte", len(meio) == 2, len(meio))
    checa("guarda o ultimo [AIMTRIG] da sessao", aim_meio is not None and "aimWriteOk=3" in aim_meio,
          aim_meio)
    tudo, _ = ld.amostras(caminho, 0)
    checa("arquivo inteiro soma as 6 amostras", len(tudo) == 6, len(tudo))

    tabela = ld.tabela(d_ultimo, tail=1)
    checa("--tail corta a tabela", len(tabela) == 1 and "enviado=   45.00" in ld.tabela(d_ultimo)[0],
          tabela)

    csv = os.path.join(pasta, "saida.csv")
    ld.csv_de(d_ultimo, csv)
    linhas_csv = io.open(csv, encoding="utf-8").read().strip().split("\n")
    checa("csv com cabecalho + N linhas", len(linhas_csv) == 3 and
          linhas_csv[0] == "hhmmss,ms," + ",".join(ld.CAMPOS), linhas_csv)
    checa("csv round-tripa os campos", linhas_csv[1].startswith("030000,000,1.0,1.0,10.0,45.0,"),
          linhas_csv[1])


def rodar(args):
    p = subprocess.run([sys.executable, SCRIPT] + args, stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT, text=True, encoding="utf-8", errors="replace")
    return p.returncode, p.stdout


def caso_cli(pasta):
    caminho = os.path.join(pasta, "log.txt")

    codigo, saida = rodar(["--log", os.path.join(pasta, "nao-existe.txt")])
    checa("log inexistente -> exit 2 e mensagem", codigo == 2 and "log nao existe" in saida,
          (codigo, saida))

    with io.open(caminho, "w", encoding="utf-8", newline="") as f:
        f.write("[10:00:00.000] [CURSOR] nada de desync aqui\r\n")
    codigo, saida = rodar(["--log", caminho])
    checa("log sem [START] -> exit 2", codigo == 2 and "nenhum [START]" in saida, (codigo, saida))

    escreve_log(caminho, [
        [linha("04:00:00", 1, 1, 30.0, 65.0, limit=35, ok=100),
         linha("04:00:01", 1, 1, 31.0, 66.0, limit=35, ok=240)],
        [linha("05:00:00", 1, 0, 1.0, 0.0, ok=0)],
    ])
    codigo, saida = rodar(["--log", caminho, "--session", "last"])
    checa("CLI last: exit 0 + veredito de canal falseado",
          codigo == 0 and "NAO escreveu" in saida and "05:00:00" in saida, saida)

    codigo, saida = rodar(["--log", caminho, "--session", "2"])
    checa("CLI --session 2 pega a sessao anterior", codigo == 0 and "04:00:01" in saida
          and "05:00:00" not in saida, saida)

    codigo, saida = rodar(["--log", caminho, "--session", "9"])
    checa("sessao fora do intervalo -> exit 2", codigo == 2 and "so ha 2 sessoes" in saida,
          (codigo, saida))

    codigo, saida = rodar(["--log", caminho, "--session", "all", "--tail", "1"])
    checa("--session all enxerga as 3 amostras das duas sessoes",
          codigo == 0 and "amostras=3" in saida and "aplicado=1: 2" in saida, saida)
    linhas = [l for l in saida.split("\n") if re.match(r"^\d\d:\d\d:\d\d\.\d\d\d\s+cfg=", l)]
    checa("--tail 1 deixa uma linha de tabela", len(linhas) == 1, linhas)
    codigo, saida = rodar(["--log", caminho, "--session", "all"])
    linhas = [l for l in saida.split("\n") if re.match(r"^\d\d:\d\d:\d\d\.\d\d\d\s+cfg=", l)]
    checa("sem --tail sao 3 linhas", len(linhas) == 3, linhas)

    csv = os.path.join(pasta, "cli.csv")
    rodar(["--log", caminho, "--session", "all", "--csv", csv])
    checa("--csv pelo CLI grava as 3 amostras",
          len(io.open(csv, encoding="utf-8").read().strip().split("\n")) == 4, csv)

    escreve_log(caminho, [[linha("06:00:00", 1, 1, 4.0, 40.0, ok=3, fail=1),
                           "[06:00:01.000] [DESYNC] cfg=1 aplicado=1 yawReal=x",
                           "[06:00:02] [DESYNC] sem relogio com milissegundos"]])
    codigo, saida = rodar(["--log", caminho])
    checa("linha [DESYNC] malformada vira aviso, nao excecao",
          codigo == 0 and "campos inesperados" in saida and "enviado=   40.00" in saida
          and saida.count("campos inesperados") == 1, saida)
    checa("relogio sem milissegundos e ignorado em silencio (contrato do regex)",
          "sem relogio com milissegundos" not in saida, saida)


def caso_watch(pasta):
    """O --watch precisa mostrar as linhas novas E deixar o modulo escrever (sem handle)."""
    caminho = os.path.join(pasta, "watch.txt")
    escreve_log(caminho, [[linha("07:00:00", 1, 1, 9.0, 44.0, ok=1)]])
    saida = os.path.join(pasta, "watch.out")

    with io.open(saida, "w", encoding="utf-8") as out:
        proc = subprocess.Popen([sys.executable, "-u", SCRIPT, "--log", caminho,
                                 "--session", "last", "--watch", "1"],
                                stdout=out, stderr=subprocess.STDOUT, text=True)
        try:
            time.sleep(1.5)
            nome_aleatorio = os.path.join(pasta, "renomeavel.txt")
            if os.path.exists(nome_aleatorio):
                os.remove(nome_aleatorio)
            checa("watch nao segura handle: da para renomear o log enquanto ele dorme",
                  _tenta_renomear(caminho))
            time.sleep(1.5)
            with io.open(caminho, "a", encoding="utf-8", newline="") as f:
                f.write(linha("07:00:09", 1, 1, 15.0, 50.0, ok=77) + "\r\n")
            prazo = time.time() + 8.0
            visto = ""
            while time.time() < prazo:
                time.sleep(0.5)
                visto = io.open(saida, encoding="utf-8").read()
                if "07:00:09" in visto:
                    break
        finally:
            proc.terminate()
            try:
                proc.wait(timeout=5)
            except Exception:
                proc.kill()
                proc.wait(timeout=5)

    visto = io.open(saida, encoding="utf-8").read()
    checa("--watch imprime so a linha nova", "07:00:09" in visto and "ok=77" in visto, visto)
    depois = visto.split("watch ON", 1)[-1]
    checa("--watch nao reimprime o que o relatorio inicial ja mostrou", "07:00:00" not in depois,
          depois)
    checa("watch mostra o veredito do que viu", "delta |deg|:" in visto, visto)


def _tenta_renomear(caminho):
    destino = caminho + ".mov"
    for _ in range(20):
        try:
            os.rename(caminho, destino)
            os.rename(destino, caminho)
            return True
        except OSError:
            time.sleep(0.1)
    return False


def caso_log_real():
    """Smoke no arquivo vivo: não afirma conteúdo, só que roda e exita 0."""
    if not os.path.exists(ld.LOG_PADRAO):
        checa("log real ausente — smoke pulado", True)
        return
    codigo, saida = rodar(["--tail", "3"])
    checa("smoke no log real: exit 0 e rotulo de sessao", codigo == 0 and "sessao iniciada em" in saida,
          (codigo, saida[:400]))


def main():
    caso_veredito()
    with tempfile.TemporaryDirectory() as pasta:
        caso_sessoes(pasta)
        caso_cli(pasta)
        caso_watch(pasta)
    caso_log_real()
    print("\n%d teste(s) falho(ram)." % len(FALHAS))
    if FALHAS:
        for nome in FALHAS:
            print("  - " + nome)
    return 1 if FALHAS else 0


if __name__ == "__main__":
    sys.exit(main())
