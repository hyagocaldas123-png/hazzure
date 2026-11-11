#!/usr/bin/env python3
# coding: utf-8
# Verify dwGlobalVars + curtime offset by inspecting rip-relative refs to the global.
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

def r2o(rva):
    for s in pe.sections:
        if s.VirtualAddress <= rva < s.VirtualAddress + max(s.Misc_VirtualSize, s.SizeOfRawData):
            return rva - s.VirtualAddress + s.PointerToRawData
    return None

def dis(va, n):
    off = r2o(va - ib)
    return [(i.address, i.mnemonic, i.op_str) for i in md.disasm(data[off:off+n], va)]

GV = 0x22320E8
print(f"=== rip-relative refs to dwGlobalVars 0x{GV:X} ===")
count = 0
for i in range(len(code) - 7):
    if code[i] in (0x48, 0x4C) and code[i+1] in (0x8B, 0x8D, 0x89):
        modrm = code[i+2]
        if (modrm & 0xC7) == 0x05:
            disp = struct.unpack_from("<i", code, i+3)[0]
            target = text_va + i + 7 + disp
            if target == ib + GV:
                count += 1
                # show the instruction + next 5
                ins = dis(text_va + i, 24)
                for a, m, o in ins[:6]:
                    print(f"  0x{a:X}: {m} {o}")
                print("   ---")
print(f"total refs: {count}")
