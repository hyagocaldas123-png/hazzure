"""Procura em .text todo sitio que toca um displacement de 4 bytes (ex.: [reg+0x1EA0]) e desmonta ao redor.
Uso: python tools/scratch_field_xref.py a01e0000 [mais_displacements_hex...]"""
import re, sys, capstone

CLIENT = (r'C:\Program Files (x86)\Steam\steamapps\common\Counter-Strike Global '
          r'Offensive\game\csgo\bin\win64\client.dll')
blob = open(CLIENT, 'rb').read()
pe_at = int.from_bytes(blob[0x3C:0x40], 'little')
sec_num = int.from_bytes(blob[pe_at + 6:pe_at + 8], 'little')
opt_size = int.from_bytes(blob[pe_at + 20:pe_at + 22], 'little')
secs, off = [], pe_at + 24 + opt_size
for _ in range(sec_num):
    e = blob[off:off + 40]
    secs.append((e[:8].rstrip(b'\0').decode(), int.from_bytes(e[12:16], 'little'),
                 int.from_bytes(e[8:12], 'little'), int.from_bytes(e[20:24], 'little')))
    off += 40
text = next(s for s in secs if s[0] == '.text')
tva, tvsize, tfo = text[1], text[2], text[3]
tbody = blob[tfo:tfo + tvsize]

pats = [bytes.fromhex(a) for a in sys.argv[1:] if not a.startswith('-')]
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
for pat in pats:
    sites = [m.start() for m in re.finditer(re.escape(pat), tbody)]
    print(f'== {pat[::-1].hex()} ({int.from_bytes(pat,"little"):#x}) -> {len(sites)} sitios ==')
    for s in sites[:60]:
        rva = tva + s
        fo = tfo + s - 0x18
        print(f'--- {rva:#x} ---')
        for ins in md.disasm(blob[fo:fo + 0x60], rva - 0x18):
            mark = '  <==' if ins.address <= rva < ins.address + ins.size else ''
            print(f'  {ins.address:#x} {ins.mnemonic} {ins.op_str}{mark}')
