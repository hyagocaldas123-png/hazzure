import pefile, struct

client_path = r"C:\Program Files (x86)\Steam\steamapps\common\Counter-Strike Global Offensive\game\csgo\bin\win64\client.dll"
pe = pefile.PE(client_path)

def rva_to_offset(rva):
    for s in pe.sections:
        if s.VirtualAddress <= rva < s.VirtualAddress + s.Misc_VirtualSize:
            return rva - s.VirtualAddress + s.PointerToRawData
    return None

def offset_to_rva(offset):
    for s in pe.sections:
        if s.PointerToRawData <= offset < s.PointerToRawData + s.SizeOfRawData:
            return offset - s.PointerToRawData + s.VirtualAddress
    return None

data = open(client_path, "rb").read()

# find m_iIDEntIndex string RVA
str_offset = data.find(b"m_iIDEntIndex\x00")
str_rva = offset_to_rva(str_offset)
print(f"m_iIDEntIndex string RVA: 0x{str_rva:X}")

# find references to str_rva in schema field table:
# SchemaClassFieldData_t:
# const char* m_pszName; (8 bytes ptr -> in 64-bit points to str VA: ImageBase + str_rva)
va = pe.OPTIONAL_HEADER.ImageBase + str_rva
va_bytes = struct.pack("<Q", va)

pos = 0
found = []
while True:
    pos = data.find(va_bytes, pos)
    if pos == -1: break
    found.append(pos)
    pos += 8

print(f"Found {len(found)} references to m_iIDEntIndex name pointer")
for p in found:
    # In SchemaClassFieldData_t:
    # m_pszName (0x00), m_pType (0x08), m_nSingleInheritanceOffset (0x10)
    field_offset = struct.unpack_from("<I", data, p + 0x10)[0]
    print(f"  at offset 0x{p:X}: schema offset = 0x{field_offset:X}")
