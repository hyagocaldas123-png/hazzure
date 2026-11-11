#!/bin/sh
# vigilancia do debug_log durante o teste em partida (t049) - dedup persistido entre lotes
LOG="/c/HazzureKernel/debug_log.txt"
SEEN="/tmp/hazzure_seen.txt"
: > "$SEEN"
prev=$(stat -c %s "$LOG")
echo "watch ON offset=$prev"
n=0
while true; do
  sleep 4
  n=$((n + 1))
  cur=$(stat -c %s "$LOG" 2>/dev/null)
  [ -z "$cur" ] && { echo "log sumiu"; break; }
  if [ "$cur" -lt "$prev" ]; then
    echo "log recriado: $prev -> $cur"
    prev=$cur
  elif [ "$cur" -gt "$prev" ]; then
    tail -c +$((prev + 1)) "$LOG" 2>/dev/null |
      grep -aE '\[STRAFE\]|\[AUTOFIRE\]|\[3P\]|\[AUTOWALL\]|\[START\]|\[CLIMA\] Transicao|\[C4\]|\[BOMB|\[GLOVEBG\] ok=0|\[ERROR|\[ERRO|EXCEPTION|falhou|Falhou|nao confirmado|Unverified|Fail=[1-9A-Fa-f]|RestoreFail=[1-9]|\[ANTIAIM\].*applied=0|\[SCOPE\].*writes=0' |
      grep -avE 'timer sem bomba|\[ANTIAIM\].*applied=1|\[SCOPE\] filtro: medido=0\.000 entregue=0\.000 alvo=0\.000' |
      awk -v sf="$SEEN" '
        BEGIN {
          cmd = "date +%s"; cmd | getline now; close(cmd)
          while ((getline line < sf) > 0) {
            p = index(line, "\t"); if (p < 2) continue
            e = substr(line, 1, p - 1) + 0; k = substr(line, p + 1)
            if (e > seen[k]) seen[k] = e
          }
        }
        { k = $0; sub(/^\[[0-9:.]+\] /, "", k); gsub(/-?[0-9]+(\.[0-9]+)?/, "#", k) }
        !(k in seen) || (now - seen[k] >= 60) { print; seen[k] = now; print now "\t" k >> sf; fflush(sf) }
      ' | tail -6
    prev=$cur
  fi
  if [ $((n % 10)) -eq 0 ]; then
    if ! tasklist //FI "IMAGENAME eq cs2.exe" 2>/dev/null | grep -qi "cs2.exe"; then
      echo "cs2.exe NAO ESTAB MAIS RODANDO (offset final $prev)"
      break
    fi
  fi
done
echo "watch OFF"
