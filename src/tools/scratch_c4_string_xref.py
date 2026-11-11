#!/usr/bin/env python3
# coding: utf-8
# Find how the game locates the planted C4: string "planted_c4" xrefs + surrounding code
import struct
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_64

CLIENT = r"C:\Program Files (x86)\Steam\steamapps\common\Counter-Strike Global Offensive\game\csgo\bin\win64\client.dll"

pe = pefile.PE(CLIENT)
image_base = pe.OPTIONAL_HEADER.ImageBase
data = open(CLIENT, "rb").read()

def rva_to_off(rva):
    for s in pe.sections:
        if s.VirtualAddress <= rva < s.VirtualAddress + max(s.Misc_VirtualSize, s.SizeOfRawData):
            return rva - s.VirtualAddress + s.PointerToRawData
    return None

md = Cs(CS_ARCH_X86, CS_MODE_64)

def disasm(va, nbytes):
    off = rva_to_off(va - image_base)
    if off is None: return []
    code = data[off:off+nbytes]
    return [(i.address, i.mnemonic, i.op_str) for i in md.disasm(code, va)]

# 1. locate "planted_c4" strings
for needle in (b"planted_c4\x00", b"C_PlantedC4\x00", b"planted_c4"):
    print(f"=== occurrences of {needle!r} ===")
    pos = 0
    while True:
        idx = data.find(needle, pos)
        if idx == -1: break
        rva = None
        for s in pe.sections:
            if s.PointerToRawData <= idx < s.PointerToRawData + s.SizeOfRawData:
                rva = s.VirtualAddress + (idx - s.PointerToRawData)
                break
        va = image_base + rva if rva else None
        print(f"  file_off=0x{idx:X} rva=0x{rva:X} va=0x{va:X}")
        pos = idx + 1

# 2. find rip-relative refs (lea/mov) to the "planted_c4" string in .text
needle = b"planted_c4\x00"
str_off = data.find(needle)
# rva of the string
str_rva = None
for s in pe.sections:
    if s.PointerToRawData <= str_off < s.PointerToRawData + s.SizeOfRawData:
        str_rva = s.VirtualAddress + (str_off - s.PointerToRawData)
        break
str_va = image_base + str_rva
print(f"\nplanted_c4 string VA = 0x{str_va:X}")

text = pe.sections[0]
code = data[text.PointerToRawData:text.PointerToRawData + min(text.Misc_VirtualSize, text.SizeOfRawData)]
text_va = image_base + text.VirtualAddress

print("\n=== rip-relative refs to 'planted_c4' string ===")
for i in range(len(code) - 7):
    # lea reg,[rip+disp] = 48 8D /r ; mov reg,[rip+disp]=48 8B ; also 4C 8D
    if code[i] in (0x48, 0x4C) and code[i+1] in (0x8D, 0x8B):
        modrm = code[i+2]
        if (modrm & 0xC7) == 0x05:
            disp = struct.unpack_from("<i", code, i+3)[0]
            target = text_va + i + 7 + disp
            if target == str_va:
                print(f"  0x{text_va+i:X}: {disasm(text_va+i, 8)[0][2] if disasm(text_va+i,8) else '?'}")
                # disasm surrounding
                for a, m, o in disasm(text_va+i-32, 80):
                    mark = " <<<" if a == text_va+i else ""
                    print(f"     0x{a:X}: {m} {o}{mark}")
                print("     ----")
