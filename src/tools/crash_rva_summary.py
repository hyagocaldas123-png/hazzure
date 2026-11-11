"""Resumo forense dos minidumps do CS2: build do client.dll, RVA do fault e alvo do AV.

Uso:
    python tools/crash_rva_summary.py            # todos os dumps
    python tools/crash_rva_summary.py 2026_11     # só carimbos contendo a substring

Layout dos streams (confirmado contra crashmap.py):
  ModuleList(4):  count uint32, entradas de 108 bytes = base,size,csum,tds,nameRva...
  Exception(6):   threadId u32, align u32, depois MINIDUMP_EXCEPTION_RECORD em +8:
                  code(4) flags(4) nestedRecord(8) address(8) nParams(4) pad(4) params[15*8]
Os params ficam portanto em rva+8+32 (a grava antiga usava o ponteiro da grava aninhada
como se fosse o vetor e produzia alvos sem sentido).
"""
import glob
import os
import struct
import sys
from collections import Counter

GAME = (r'C:\Program Files (x86)\Steam\steamapps\common\Counter-Strike Global '
        r'Offensive\game\bin\win64')
REG_ORDER = ['r15', 'r14', 'r13', 'r12', 'rbp', 'rbx', 'r11', 'r10', 'r9', 'r8',
             'rax', 'rcx', 'rdx', 'rsi', 'rdi', 'rsp', 'rip']


def rows(path):
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
    client = None
    off = rva + 4
    for _ in range(count):
        base, img, _, tds, name_rva = struct.unpack_from('<QIIII', raw, off)
        nlen, = struct.unpack_from('<I', raw, name_rva)
        name = raw[name_rva + 4:name_rva + 4 + nlen].decode('utf-16-le', 'replace')
        if os.path.basename(name).lower() == 'client.dll':
            client = (base, img, tds)
        off += 108

    size, rva = streams[6]
    _, _ = struct.unpack_from('<II', raw, rva)
    rec = rva + 8
    code, _, nested, addr, nparams, _ = struct.unpack_from('<IIQQII', raw, rec)
    params = [struct.unpack_from('<Q', raw, rec + 32 + i * 8)[0] for i in range(nparams)]
    _, ctx_rva = struct.unpack_from('<II', raw, rec + 152)
    reg = {n: struct.unpack_from('<Q', raw, ctx_rva + i * 8)[0] for i, n in enumerate(REG_ORDER)}

    fault = (addr - client[0]) if client and client[0] <= addr < client[0] + client[1] else None
    return dict(stamp=os.path.basename(path)[4:17], tds=client[2] if client else 0,
                image=client[1] if client else 0, code=code, rva=fault,
                read=('unknown' if code != 0xC0000005 else ('write' if params and params[0] == 1 else 'read')),
                target=params[1] if len(params) > 1 else 0,
                rax=reg['rax'], rbx=reg['rbx'], rsi=reg['rsi'], rip=reg['rip'])


def main():
    needle = sys.argv[1] if len(sys.argv) > 1 else ''
    found = []
    for path in sorted(glob.glob(os.path.join(GAME, 'cs2_2026_*.mdmp'))):
        if needle and needle not in os.path.basename(path):
            continue
        row = rows(path)
        if row:
            found.append(row)
    print(f'{"stamp":14} {"tds":12} {"image":11} {"code":12} {"faultRVA":11} '
          f'{"AV":6} {"target":18} rax / rbx')
    for r in found:
        cells = [r['stamp'], hex(r['tds']), hex(r['image']), hex(r['code']),
                 hex(r['rva']) if r['rva'] is not None else 'fora',
                 r['read'], hex(r['target']), f"{r['rax']:#x} / {r['rbx']:#x}"]
        print(' '.join(c.ljust(w) for c, w in zip(cells, [14, 12, 11, 12, 11, 6, 18, 24])))
    print('\ncontagem por (tds, faultRVA):')
    for (tds, rva), n in Counter((r['tds'], r['rva']) for r in found).most_common(12):
        print(f'  tds={tds:#x} rva={(hex(rva) if rva is not None else "fora")}: {n}')


if __name__ == '__main__':
    main()
