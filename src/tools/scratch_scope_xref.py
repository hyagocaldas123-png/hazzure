"""Acha strings no client.dll, mostra vizinhos e procura referencias rip-relative em .text.
Uso: python tools/scratch_scope_xref.py [-d] palavra1 palavra2 ...
Todos os enderecos impressos sao RVAs (iguais aos do resto do projeto)."""
import re, sys, capstone

CLIENT = (r'C:\Program Files (x86)\Steam\steamapps\common\Counter-Strike Global '
          r'Offensive\game\csgo\bin\win64\client.dll')
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


def fo_to_rva(fo):
    for name, vaddr, vsize, raddr, rsize in secs:
        if raddr <= fo < raddr + rsize:
            return vaddr + (fo - raddr)
    return None


KEYS = [k for k in sys.argv[1:] if not k.startswith('-')]
targets = {}
for m in re.finditer(rb'[ -~]{3,}', blob):
    s = m.group().decode('latin1')
    low = s.lower()
    if any(k.lower() in low for k in KEYS):
        rva = fo_to_rva(m.start())
        if rva is not None:
            targets.setdefault(rva, s)

print('== strings candidatas (RVA) ==')
for rva in sorted(targets):
    print(f'{rva:#x} {targets[rva]!r}')

print('== vizinhos ==')
for rva in sorted(targets):
    fo = rva_to_fo(rva)
    chunk = blob[max(0, fo - 128):fo + 128]
    near = [r.group().decode('latin1') for r in re.finditer(rb'[ -~]{4,}', chunk)]
    print(f'{rva:#x} :: {near}')

text = next(s for s in secs if s[0] == '.text')
tva, tvsize, tfo = text[1], text[2], text[3]
tbody = blob[tfo:tfo + tvsize]
hits = {}
for m in re.finditer(rb'[\x48\x4c][\x8b\x8d][\x05\x0d\x15\x1d\x25\x2d\x35\x3d](.{4})', tbody, re.S):
    disp = int.from_bytes(m.group(1), 'little', signed=True)
    site = tva + m.start()
    tgt = site + 7 + disp
    if tgt in targets:
        hits.setdefault(tgt, []).append(site)

print('== xrefs (RVA) ==')
for tgt in sorted(hits):
    print(f'{targets[tgt]!r} em {tgt:#x} <- {len(hits[tgt])} sitio(s): ' +
          ', '.join(f'{h:#x}' for h in hits[tgt][:16]))

if '-d' in sys.argv:
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
    for tgt in sorted(hits):
        for site in hits[tgt][:8]:
            fo = rva_to_fo(site - 0x70)
            print(f'--- sitio {site:#x} para {targets[tgt]!r} ---')
            for ins in md.disasm(blob[fo:fo + 0x180], site - 0x70):
                mark = '  <== ref' if ins.address == site else ''
                print(f'  {ins.address:#x} {ins.mnemonic} {ins.op_str}{mark}')
