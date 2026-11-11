"""Scan pak01_dir.vpk printable runs for scope/sniper/zoom file names."""
import re, sys

DIR = r"C:/Program Files (x86)/Steam/steamapps/common/Counter-Strike Global Offensive/game/csgo/pak01_dir.vpk"
keys = [k.lower() for k in sys.argv[1:]] or ["scope", "sniper"]

data = open(DIR, "rb").read()
print("dir vpk bytes", len(data))
seen = set()
for m in re.finditer(rb"[ -~]{4,}", data):
    s = m.group().decode("latin1")
    low = s.lower()
    if any(k in low for k in keys):
        if s in seen:
            continue
        seen.add(s)
        print(hex(m.start()), repr(s))
print("total unique", len(seen))
