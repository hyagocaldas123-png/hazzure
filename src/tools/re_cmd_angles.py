"""RE do CUserCmd/CSGOUserCmdPB no client.dll 0x6AEBC4DA (build 14194).

Uso:
  python re_cmd_angles.py --func 0xD04300 n          # desmonta n instrucoes de CreateMove
  python re_cmd_angles.py --calls 0xD04300           # sites E8 que chamam o RVA
  python re_cmd_angles.py --xrefs 0x24D0B70 0x1000   # varre .text procurando leituras do RVA
"""
import sys, struct, re
import capstone

CLIENT = (r'C:\Program Files (x86)\Steam\steamapps\common\Counter-Strike Global '
          r'Offensive\game\csgo\bin\win64\client.dll')
IMAGE_BASE = 0x180000000

blob = open(CLIENT, 'rb').read()
pe_at = int.from_bytes(blob[0x3C:0x40], 'little')
sec_num = int.from_bytes(blob[pe_at + 6:pe_at + 8], 'little')
opt_size = int.from_bytes(blob[pe_at + 20:pe_at + 22], 'little')
secs = []
off = pe_at + 24 + opt_size
for _ in range(sec_num):
    e = blob[off:off + 40]
    secs.append((e[:8].rstrip(b'\0').decode(),
                 int.from_bytes(e[12:16], 'little'),
                 int.from_bytes(e[8:12], 'little'),
                 int.from_bytes(e[20:24], 'little')))
    off += 40

def locate(rva):
    for name, vaddr, vsize, raddr in secs:
        if vaddr <= rva < vaddr + vsize:
            return raddr + (rva - vaddr), vaddr, vsize, name
    return None

def read(rva, n):
    got = bytearray()
    while len(got) < n:
        hit = locate(rva + len(got))
        if not hit:
            break
        fo, _, _, _ = hit
        chunk = blob[fo:fo + min(0x400, n - len(got))]
        if not chunk:
            break
        got += chunk
    return bytes(got)

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
md.detail = True

def disasm(rva, count):
    data = read(rva, count * 16)
    out = []
    for ins in md.disasm(data, IMAGE_BASE + rva):
        out.append(ins)
        if len(out) >= count:
            break
    return out

def resolve_rip(ins):
    """Se a instrucao usa [rip+disp], devolve (rva_alvo, None|'disp') e o valor efetivo absoluto."""
    for op in ins.operands:
        if op.type == capstone.x86.X86_OP_MEM and op.mem.base == capstone.x86.X86_REG_RIP:
            disp = op.mem.disp
            target_abs = ins.address + ins.size + disp
            return (target_abs - IMAGE_BASE, disp)
    return None

def cmd_func():
    addr, vaddr, vsize, name = locate(0xD04300)
    return addr, vaddr, vsize, name

def main():
    if sys.argv[1] == '--func':
        rva = int(sys.argv[2], 16)
        count = int(sys.argv[3])
        for ins in disasm(rva, count):
            r = resolve_rip(ins)
            extra = f"  ; -> rva=0x{r[0]:X}" if r else ""
            print(f"{ins.address-IMAGE_BASE:08X}: {ins.mnemonic:<10} {ins.op_str}{extra}")
    elif sys.argv[1] == '--calls':
        rva = int(sys.argv[2], 16)
        # varre .text atras de E8 relativo a rva
        text = locate(rva)
        if not text:
            print('erro: rva fora de .text'); return
        fo, vaddr, vsize, name = text
        tcode = blob[fo:fo + vsize]
        hits = []
        for i in range(len(tcode) - 4):
            if tcode[i] != 0xE8:
                continue
            rel = int.from_bytes(tcode[i+1:i+5], 'little', signed=True)
            target = (vaddr + i + 5 + rel) & 0xFFFFFFFF
            if target == rva:
                hits.append(vaddr + i)
        print(f'{len(hits)} site(s) E8 -> 0x{rva:X}:')
        for h in hits:
            print(f'  0x{h:08X} (caller)')
    elif sys.argv[1] == '--xrefs':
        rva = int(sys.argv[2], 16)
        span = int(sys.argv[3]) if len(sys.argv) > 3 else 0x1000
        # procura usos de [rva] como leitura (mov reg, [rva] absoluto) em .text
        text = locate(0xD00000)
        fo, vaddr, vsize, name = text
        tcode = blob[fo:fo + vsize]
        p = re.compile(r'.' * 0)  # placeholder
        hi = 0
        lo = max(0, rva - span)
        hits = []
        for i in range(len(tcode) - 6):
            # 48 8B 0D <rel32>  = mov rcx, [rip+rel]
            if tcode[i] in (0x8B, 0x8D) and i >= 3 and tcode[i-1] == 0x48 and tcode[i-2] in (0x8B, 0x89):
                # forma generica: procurar 4 bytes rel32 a partir de i e checar alvo
                pass
            # simpler: scan for any rip-relative disp whose target == rva
            # we decode instruction-by-instruction over .text (slow on full .text)
        # decode-based scan, limited range
        start = max(0, rva - span)
        end = min(vsize, rva + span)
        for a in range(start, end, 16):
            for ins in md.disasm(tcode[a:a+200], vaddr + a):
                r = resolve_rip(ins)
                if r and lo <= r[0] <= hi if False else (r and r[0] == rva):
                    hits.append((ins.address - IMAGE_BASE, ins.mnemonic, ins.op_str))
                if ins.address - IMAGE_BASE > rva + span:
                    break
                if ins.size == 0:
                    break
        print(f'refs a 0x{rva:X} (janela 0x{start:X}..0x{end:X}):')
        for h in hits[:40]:
            print(f'  {h[0]:08X}: {h[1]} {h[2]}')

if __name__ == '__main__':
    main()