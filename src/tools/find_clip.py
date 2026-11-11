import pefile, struct

client_path = r"C:\Program Files (x86)\Steam\steamapps\common\Counter-Strike Global Offensive\game\csgo\bin\win64\client.dll"
pe = pefile.PE(client_path)

def offset_to_rva(offset):
    for s in pe.sections:
        if s.PointerToRawData <= offset < s.PointerToRawData + s.SizeOfRawData:
            return offset - s.PointerToRawData + s.VirtualAddress
    return None

data = open(client_path, "rb").read()
str_offset = data.find(b"m_iClip1\x00")
str_rva = offset_to_rva(str_offset)
va = pe.OPTIONAL_HEADER.ImageBase + str_rva
va_bytes = struct.pack("<Q", va)

pos = 0
found = []
while True:
    pos = data.find(va_bytes, pos)
    if pos == -1: break
    found.append(pos)
    pos += 8

print(f"Found {len(found)} references to m_iClip1 name pointer")
for p in found:
    field_offset = struct.unpack_from("<I", data, p + 0x10)[0]
    print(f"  at offset 0x{p:X}: schema offset = 0x{field_offset:X}")
