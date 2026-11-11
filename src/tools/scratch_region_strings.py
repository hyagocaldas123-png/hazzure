"""Imprime todas as strings imprimiveis dentro de um intervalo de RVA no client.dll."""
import re, sys

CLIENT = (r'C:\Program Files (x86)\Steam\steamapps\common\Counter-Strike Global '
          r'Offensive\game\csgo\bin\win64\client.dll')
BASE = 0x180000000
blob = open(CLIENT, 'rb').read()

pe_at = int.from_bytes(blob[0x3C:0x40], 'little')
sec_num = int.from_bytes(blob[pe_at + 6:pe_at + 8], 'little')
opt_size = int.from_bytes(blob[pe_at + 20:pe_at + 22], 'little')
secs = []
off = pe_at + 24 + opt_size
for _ in range(sec_num):
    e = blob[off:off + 40]
    secs.append((e[:8].rstrip(b'\0').decode(), int.from_bytes(e[12:16], 'little'),
                 int.from_bytes(e[8:12], 'little'), int.from_bytes(e[20:24], 'little'),
                 int.from_bytes(e[16:20], 'little')))
    off += 40


def rva_to_fo(rva):
    for name, vaddr, vsize, raddr, rsize in secs:
        if vaddr <= rva < vaddr + max(vsize, rsize):
            return raddr + (rva - vaddr)
    return None


start = int(sys.argv[1], 16)
end = int(sys.argv[2], 16)
fo = rva_to_fo(start)
chunk = blob[fo:fo + (end - start)]
print(f'== strings {start:#x}..{end:#x} ==')
for m in re.finditer(rb'[ -~]{3,}', chunk):
    print(f'{start + m.start():#x} {m.group().decode("latin1")!r}')
