"""Disassemble the range around a fault RVA and, optionally, find CALL sites to a function."""
import sys, capstone

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


def dump(start, length):
    got = bytearray()
    while len(got) < length:
        hit = locate(start + len(got))
        if not hit:
            break
        fo, _, _, _ = hit
        chunk = blob[fo:fo + min(0x400, length - len(got))]
        if not chunk:
            break
        got += chunk
    return bytes(got)


TEXT = next(s for s in secs if s[0] == '.text')
text_bytes = blob[TEXT[3]:TEXT[3] + TEXT[2]]

if len(sys.argv) > 1 and sys.argv[1] == 'callers':
    fn = int(sys.argv[2], 16)
    found = []
    for i in range(len(text_bytes) - 5):
        if text_bytes[i] == 0xE8:
            rel = int.from_bytes(text_bytes[i + 1:i + 5], 'little', signed=True)
            if TEXT[1] + i + 5 + rel == fn:
                found.append(TEXT[1] + i)
        elif text_bytes[i:i + 3] == b'\xff\x15' or text_bytes[i:i + 2] == b'\x8d\x0d':
            pass
    print(f'call sites into {fn:#x}: {len(found)}')
    for c in found[:40]:
        print(f'  caller {c:#x}')
    sys.exit(0)

fault = int(sys.argv[1], 16)
back = int(sys.argv[2], 16) if len(sys.argv) > 2 else 0x2A0
fwd = int(sys.argv[3], 16) if len(sys.argv) > 3 else 0x60
start = fault - back
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
rows = []
for ins in md.disasm(dump(start, back + fwd), IMAGE_BASE + start):
    rel = ins.address - IMAGE_BASE
    rows.append((rel, f'{ins.mnemonic:<8} {ins.op_str}', rel == fault))
for rel, txt, hit in rows:
    mark = '   <== FAULT' if hit else ''
    print(f'{rel:#09x} {txt}{mark}')
