import pefile
import struct

client_path = r"C:\Program Files (x86)\Steam\steamapps\common\Counter-Strike Global Offensive\game\csgo\bin\win64\client.dll"
pe = pefile.PE(client_path, fast_load=True)

# Find dwSensitivity in cs2 client.dll
# Search for string "sensitivity"
data = open(client_path, "rb").read()
print(f"File size: {len(data)}")

# Let's inspect dwSensitivity in offset_dumper:
# In offset_dumper.py, dwSensitivity = 0x25649D8
# dwSensitivity_sensitivity = 0x58
print("Script finished")
