"""De quem sao os enderecos na pilha do thread que faultou.

Responde, sem abrir o jogo, se o crash numa RVA de client.dll tem na pilha itens de
retorno que pertencem a regiao anonima (executable fora de modulo listado — o nosso
manual map aparece aqui) ou se a cadeia inteira e de modulos do jogo.

Uso:
    python tools/crash_stack_ownership.py            # todos os dumps de cs2_2026_*
    python tools/crash_stack_ownership.py 2026_1105  # filtro por substring do carimbo
"""
import glob
import os
import struct
import sys
from collections import Counter

GAME = (r'C:\Program Files (x86)\Steam\steamapps\common\Counter-Strike Global '
        r'Offensive\game\bin\win64')
OURS = (r'C:\HazzureKernel\Hazzure.Internal.dll')
REG_ORDER = ['r15', 'r14', 'r13', 'r12', 'rbp', 'rbx', 'r11', 'r10', 'r9', 'r8',
             'rax', 'rcx', 'rdx', 'rsi', 'rdi', 'rsp', 'rip']
FINGERPRINTS = [b'[CLIMA]', b'[GLOVEVIEW]', b'Hazzure.Internal']

dll = open(OURS, 'rb').read()
pe = struct.unpack_from('<I', dll, 0x3C)[0]
nsec = struct.unpack_from('<H', dll, pe + 6)[0]
optsz = struct.unpack_from('<H', dll, pe + 20)[0]
dsec = []
for i in range(nsec):
    e = dll[pe + 24 + optsz + i * 40: pe + 24 + optsz + i * 40 + 40]
    dsec.append((int.from_bytes(e[12:16], 'little'), int.from_bytes(e[8:12], 'little'),
                 int.from_bytes(e[16:20], 'little'), int.from_bytes(e[20:24], 'little')))


def our_file_off(rva):
    for vaddr, vsize, rawsize, raddr in dsec:
        if vaddr <= rva < vaddr + max(vsize, rawsize):
            return raddr + (rva - vaddr)
    return None


def parse(path):
    raw = open(path, 'rb').read()
    if raw[:4] != b'MDMP':
        return None
    _, _, nstreams, dirrva, _, _, _ = struct.unpack_from('<IIIIIIQ', raw, 0)
    streams = {}
    for i in range(nstreams):
        kind, size, rva = struct.unpack_from('<III', raw, dirrva + i * 12)
        streams.setdefault(kind, (size, rva))
    if 6 not in streams or 4 not in streams:
        return None

    size, rva = streams[4]
    count, = struct.unpack_from('<I', raw, rva)
    mods, off = [], rva + 4
    for _ in range(count):
        base, img, _, tds, name_rva = struct.unpack_from('<QIIII', raw, off)
        nlen, = struct.unpack_from('<I', raw, name_rva)
        name = raw[name_rva + 4:name_rva + 4 + nlen].decode('utf-16-le', 'replace')
        mods.append((base, img, os.path.basename(name).lower()))
        off += 108

    ranges = []
    if 16 in streams:
        size, rva = streams[16]
        cnt, data_rva = struct.unpack_from('<QQ', raw, rva)
        off = rva + 16
        cursor = data_rva
        for _ in range(cnt):
            start, length = struct.unpack_from('<QQ', raw, off)
            ranges.append((start, length, cursor))
            cursor += length
            off += 16
    if 5 in streams:
        size, rva = streams[5]
        cnt, = struct.unpack_from('<I', raw, rva)
        off = rva + 4
        for _ in range(cnt):
            start, dsz, rva2 = struct.unpack_from('<QII', raw, off)
            ranges.append((start, dsz, rva2))
            off += 16

    size, rva = streams[6]
    tid, _ = struct.unpack_from('<II', raw, rva)
    code, _, nested, addr, nparams, _ = struct.unpack_from('<IIQQII', raw, rva + 8)
    _, ctx_rva = struct.unpack_from('<II', raw, rva + 8 + 152)
    reg = {n: struct.unpack_from('<Q', raw, ctx_rva + i * 8)[0] for i, n in enumerate(REG_ORDER)}

    def peek(where, ln):
        for start, length, r in ranges:
            if start <= where and where + ln <= start + length:
                return raw[r + (where - start):r + (where - start) + ln]
        return None

    def owner(where):
        for base, img, name in mods:
            if base <= where < base + img:
                return f'{name}+{where - base:#x}'
        for start, length, r in ranges:
            if start <= where < start + length:
                head = raw[r:r + 2]
                return ('anon-exec-region' if head == b'MZ' else 'anon')
        return 'no-dump'

    cli = next((m for m in mods if m[2] == 'client.dll'), None)
    fault = (addr - cli[0]) if cli and cli[0] <= addr < cli[0] + cli[1] else None

    # Onde a nossa imagem mapeada aparece neste dump (base = endereco - RVA do arquivo).
    fp = FINGERPRINTS[0]
    fpos = dll.find(fp)
    fp_rva = None
    for vaddr, _, _, raddr in dsec:
        if raddr <= fpos < raddr + 0x200000:
            fp_rva = fpos - raddr + vaddr
            break
    our_base = Counter()
    if fp_rva is not None:
        for start, length, r in ranges:
            if length < 0x40:
                continue
            blob = raw[r:r + length]
            pos = blob.find(fp)
            while pos >= 0:
                our_base[start + pos - fp_rva] += 1
                pos = blob.find(fp, pos + 1)
    return dict(stamp=os.path.basename(path)[4:17], tid=tid, code=code, fault=fault,
                addr=owner(addr), rsp=reg['rsp'], rbp=reg['rbp'], ret=owner(reg['rip']),
                stack=[owner(v) for v in
                       [struct.unpack_from('<Q', peek(reg['rsp'] + i * 8, 8)[0], 0)[0]
                        for i in range(64) if peek(reg['rsp'] + i * 8, 8)]],
                our_base=our_base.most_common(1))


def main():
    needle = sys.argv[1] if len(sys.argv) > 1 else ''
    tally = Counter()
    for path in sorted(glob.glob(os.path.join(GAME, 'cs2_2026_*.mdmp'))):
        if needle and needle not in os.path.basename(path):
            continue
        try:
            info = parse(path)
        except Exception as exc:
            print(os.path.basename(path)[4:17], 'ERR', exc)
            continue
        if not info:
            continue
        anon = [s for s in info['stack'] if s.startswith('anon') or s == 'no-dump']
        kinds = Counter(s.split('+')[0] for s in info['stack'])
        tally[('anon' if anon else 'so-modulos',
               'client' if info['fault'] is not None else 'fora')] += 1
        print(f'{info["stamp"]:14} code={info["code"]:#x} fault='
              f'{hex(info["fault"]) if info["fault"] is not None else "fora":>10} '
              f'em {info["addr"][:34]:34} nossa-base={info["our_base"]}')
        print(f'   pilha: {dict(kinds.most_common(5))}')
        if anon:
            print(f'   itens anonimos/fora de modulo na pilha: {len(anon)} -> {anon[:6]}')
    print('\nresumo (tem anon no backtrace?, fault em module?):', dict(tally))


if __name__ == '__main__':
    main()
