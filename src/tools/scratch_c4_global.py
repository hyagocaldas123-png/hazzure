#!/usr/bin/env python3
# coding: utf-8
# Find the true planted-C4 global: locate "mov reg,[rip+X]" whose reg is then
# dereferenced at bomb-schema offsets (0x1288 ticking / 0x12B8 blow / 0x12C0 length).
import struct
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_64

CLIENT = r"C:\Program Files (x86)\Steam\steamapps\common\Counter-Strike Global Offensive\game\csgo\bin\win64\client.dll"
pe = pefile.PE(CLIENT)
ib = pe.OPTIONAL_HEADER.ImageBase
data = open(CLIENT, "rb").read()

text = pe.sections[0]
code = data[text.PointerToRawData:text.PointerToRawData + min(text.Misc_VirtualSize, text.SizeOfRawData)]
text_va = ib + text.VirtualAddress

md = Cs(CS_ARCH_X86, CS_MODE_64)
md.detail = True

BOMB_OFFS = (0x1288, 0x12B8, 0x12C0, 0x12C4, 0x12D4, 0x12D8, 0x12DC)

# Map reg name -> whether the disasm window touches [reg + off]
import re

results = []
for i in range(len(code) - 7):
    if code[i] in (0x48, 0x4C) and code[i+1] == 0x8B:
        modrm = code[i+2]
        if (modrm & 0xC7) == 0x05:  # mov r64, [rip+disp32]
            disp = struct.unpack_from("<i", code, i+3)[0]
            target = text_va + i + 7 + disp
            # target must be a plausible global (data section)
            if not (0x2400000 <= target - ib <= 0x2600000):
                continue
            # get the destination register
            reg_field = (modrm >> 3) & 7
            rex_r = (code[i] & 0x04) >> 2
            reg_names = ["rax","rcx","rdx","rbx","rsp","rbp","rsi","rdi",
                         "r8","r9","r10","r11","r12","r13","r14","r15"]
            dst = reg_names[reg_field + (8 if rex_r else 0)]
            # disasm next 48 bytes looking for [dst + bomb_off]
            window = code[i+7:i+7+64]
            instrs = list(md.disasm(window, text_va+i+7))
            for ins in instrs:
                op = ins.op_str
                m = re.search(r'\[[^]]*\+ 0x(12[0-9a-f]{2})\][^]]*$', op) or re.search(r'\[([^]]*0x12[0-9a-f]{2}[^]]*)\]', op)
                if m and dst in op and any(hex(o) in op for o in BOMB_OFFS):
                    # check it references one of our offsets
                    for o in BOMB_OFFS:
                        if hex(o) in op and dst in op:
                            results.append((text_va+i, target-ib, dst, hex(o), f"{ins.mnemonic} {ins.op_str}"))
                            break
                    break

seen = set()
for va, global_rva, dst, off, ins in results:
    key = (global_rva, dst, off)
    if key in seen: continue
    seen.add(key)
    print(f"mov @0x{va:X}: dst={dst} -> global rva=0x{global_rva:X} then {ins}")
print(f"\nunique hits: {len(seen)}")
