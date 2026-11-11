"""Desmonta um intervalo de RVA no client.dll anotando alvos rip-relative resolvidos (RVA).

Uso: python tools/scratch_disasm_res.py <startRva> <lenHex>
"""
import capstone, struct, sys
from capstone.x86_const import X86_OP_MEM, X86_REG_RIP

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


def fo_to_rva(fo):
    for name, vaddr, vsize, raddr, rsize in secs:
        if raddr <= fo < raddr + rsize:
            return vaddr + (fo - raddr)
    return None


def cstring(rva, limit=120):
    fo = rva_to_fo(rva)
    if fo is None:
        return None
    out = bytearray()
    for i in range(limit):
        if fo + i >= len(blob):
            return None
        b = blob[fo + i]
        if b == 0:
            break
        if not 32 <= b < 127:
            return None
        out.append(b)
    return out.decode('latin1') if out else None


start = int(sys.argv[1], 16)
length = int(sys.argv[2], 16)
fo = rva_to_fo(start)
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
md.detail = True
for ins in md.disasm(blob[fo:fo + length], start):
    note = ''
    for op in ins.operands:
        if op.type == X86_OP_MEM and op.mem.base == X86_REG_RIP:
            tgt = ins.address + ins.size + op.mem.disp
            note += f'  -> {tgt:#x}'
            if rva_to_fo(tgt) is not None:
                s = cstring(tgt)
                if s:
                    note += f' {s!r}'
                else:
                    ffo = rva_to_fo(tgt)
                    if ffo + 4 > len(blob):
                        continue
                    fl = struct.unpack_from('<f', blob, ffo)[0]
                    if 1e-30 < abs(fl) < 1e30:
                        note += f' [float {fl:g}]'
    print(f'{ins.address:#09x} {ins.mnemonic:<8} {ins.op_str}{note}')
