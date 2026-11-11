"""Acha referencias rip-relative (mov/lea/movss/cmp/mov r32) a um RVA alvo dentro de .text.

Uso: python tools/scratch_global_xref.py <targetRva> [-d]
"""
import capstone, re, struct, sys

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


target = int(sys.argv[1], 16)
text = next(s for s in secs if s[0] == '.text')
tva, tvsize, tfo = text[1], text[2], text[3]
tblob = blob[tfo:tfo + tvsize]

# 48/4c 8b|8d modrm(mod=00, rm=101) disp32  |  f3 0f 10|11 05 disp32  |  8b 0d / 3b 0d / 83 3d / 39 0d
pats = [
    (rb'[\x48\x4c][\x8b\x8d][\x05\x0d\x15\x1d\x25\x2d\x35\x3d](.{4})', 7),
    (rb'\xf3\x0f[\x10\x11][\x05\x0d\x15\x1d](.{4})', 8),
    (rb'[\x8b\x83\x3b\x39\x81\xc7][\x05\x0d\x15\x1d\x25\x2d\x35\x3d](.{4})', 6),
]
hits = []
for pat, size in pats:
    for m in re.finditer(pat, tblob, re.S):
        disp = int.from_bytes(m.group(1), 'little', signed=True)
        site = tva + m.start()
        if site + size + disp == target:
            hits.append((site, size))
hits.sort()
print(f'== {len(hits)} sitio(s) referindo {target:#x} ==')
for site, size in hits:
    print(f'{site:#x}')

if '-d' in sys.argv:
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
    for site, size in hits[:24]:
        s = site - 0x40
        fo = rva_to_fo(s)
        print(f'--- {site:#x} ---')
        for ins in md.disasm(blob[fo:fo + 0xA0], BASE + s):
            rel = ins.address - BASE
            mark = '  <== ref' if rel == site else ''
            extra = ''
            if mark and ins.mnemonic in ('mov', 'lea'):
                print(f'  {rel:#09x} {ins.mnemonic} {ins.op_str}{mark}')
                continue
            print(f'  {rel:#09x} {ins.mnemonic} {ins.op_str}{extra}{mark}')
