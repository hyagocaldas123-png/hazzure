#!/usr/bin/env python3
# coding: utf-8
# Offline disasm: verify the true dwPlantedC4 global for the live client.dll
import struct, sys
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

def va_to_off(va):
    return rva_to_off(va - image_base)

md = Cs(CS_ARCH_X86, CS_MODE_64)
md.detail = False

def disasm(va, nbytes):
    off = va_to_off(va)
    if off is None: return []
    code = data[off:off+nbytes]
    out = []
    for ins in md.disasm(code, va):
        out.append((ins.address, ins.mnemonic, ins.op_str))
    return out

# Targets to investigate
targets = [0x24CD840, 0x24CD848]

# 1) find all rip-relative references to the targets
def find_rip_refs(target_rva):
    # rip-relative disp32: search for any instruction whose next IP + disp == target
    hits = []
    # scan .text
    text = pe.sections[0]
    text_off = text.PointerToRawData
    text_va = image_base + text.VirtualAddress
    text_size = min(text.Misc_VirtualSize, text.SizeOfRawData)
    code = data[text_off:text_off+text_size]
    for i in range(len(code) - 4):
        # try disp32 at i..i+3, instruction must end at i+4 (next IP)
        disp = struct.unpack_from("<i", code, i)[0]
        # the disp is at offset i within instruction; next instruction IP = text_va + (i+4)
        next_ip = text_va + i + 4
        target = next_ip + disp
        if target == image_base + target_rva:
            hits.append((text_va + i, i))  # va of the disp field
    return hits

for t in targets:
    hits = find_rip_refs(t)
    print(f"=== refs to 0x{t:X} (rva {t}) === count={len(hits)}")
    for disp_va, _ in hits[:20]:
        window = disasm(disp_va - 12, 20)
        for a, m, o in window:
            marker = " <== disp here" if a <= disp_va < a + 12 else ""
            print(f"  0x{a:X}: {m} {o}{marker}")
        print("  ---")

# 2) find READ refs (mov reg, [rip+X]) to 0x24CD848 to confirm it is loaded as a pointer
print()
print("=== reads of 0x24CD848 (mov r??, [rip+X]) ===")
text = pe.sections[0]
code = data[text.PointerToRawData:text.PointerToRawData + min(text.Misc_VirtualSize, text.SizeOfRawData)]
text_va = image_base + text.VirtualAddress
for i in range(len(code) - 7):
    b0 = code[i]
    # mov reg, [rip+disp32]: 48 8B /r (mod=00, rm=101) or 48 8B 0D / 15 / 1D / 05 etc
    if b0 == 0x48 and code[i+1] == 0x8B:
        modrm = code[i+2]
        if (modrm & 0xC7) == 0x05:  # mod=00, rm=101 -> [rip+disp32]
            disp = struct.unpack_from("<i", code, i+3)[0]
            target = text_va + i + 7 + disp
            if target == image_base + 0x24CD848 or target == image_base + 0x24CD840:
                print(f"  0x{text_va+i:X}: mov r?, [rip+0x{target-image_base:X}]")
# also movsxd/lea/other 8B and 89 (write) and 8D (lea)
print()
print("=== all refs to 0x24CD848 (any rip-rel op) ===")
for i in range(len(code) - 7):
    if code[i] in (0x48, 0x4C) and code[i+1] in (0x8B, 0x89, 0x8D):
        modrm = code[i+2]
        if (modrm & 0xC7) == 0x05:
            disp = struct.unpack_from("<i", code, i+3)[0]
            target = text_va + i + 7 + disp
            if target == image_base + 0x24CD848:
                ins = disasm(text_va + i, 8)
                for a, m, o in ins:
                    print(f"  0x{a:X}: {m} {o}")
