import ctypes, struct
from ctypes import wintypes

PROCESS_VM_READ = 0x0010
PROCESS_QUERY_INFORMATION = 0x0400

k32 = ctypes.windll.kernel32
psapi = ctypes.windll.psapi

k32.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
k32.OpenProcess.restype = wintypes.HANDLE

k32.ReadProcessMemory.argtypes = [wintypes.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
k32.ReadProcessMemory.restype = wintypes.BOOL

psapi.EnumProcessModulesEx.argtypes = [wintypes.HANDLE, ctypes.POINTER(ctypes.c_void_p), wintypes.DWORD, ctypes.POINTER(wintypes.DWORD), wintypes.DWORD]
psapi.EnumProcessModulesEx.restype = wintypes.BOOL

psapi.GetModuleBaseNameA.argtypes = [wintypes.HANDLE, ctypes.c_void_p, ctypes.c_char_p, wintypes.DWORD]
psapi.GetModuleBaseNameA.restype = wintypes.DWORD

pid = 16136
hProcess = k32.OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, False, pid)
if not hProcess:
    print('Failed to open process:', k32.GetLastError())
    exit()

hMods = (ctypes.c_void_p * 1024)()
cbNeeded = wintypes.DWORD()
psapi.EnumProcessModulesEx(hProcess, hMods, ctypes.sizeof(hMods), ctypes.byref(cbNeeded), 0x03)

client_base = 0
for i in range(cbNeeded.value // 8):
    mod_name = ctypes.create_string_buffer(260)
    psapi.GetModuleBaseNameA(hProcess, hMods[i], mod_name, 260)
    if mod_name.value.lower() == b'client.dll':
        client_base = hMods[i]
        break

print(f'client.dll base: 0x{client_base:X}')

def rpm(addr, size):
    buf = ctypes.create_string_buffer(size)
    bytesRead = ctypes.c_size_t()
    if k32.ReadProcessMemory(hProcess, ctypes.c_void_p(addr), buf, size, ctypes.byref(bytesRead)):
        return buf.raw
    return None

input_addr = client_base + 0x257D780
data = rpm(input_addr + 0xb50, 16)
if data:
    count = struct.unpack('<I', data[:4])[0]
    records = struct.unpack('<Q', data[8:16])[0]
    print(f'count={count}, records=0x{records:X}')
    fb = rpm(input_addr + 0x688, 8)
    if fb:
        fx, fy = struct.unpack('<ff', fb)
        print(f'fallback view: pitch={fx:.3f}, yaw={fy:.3f}')
    if count > 0 and records > 0x10000:
        rec_addr = records + (count - 1) * 0x440 + 0x430
        rb = rpm(rec_addr, 8)
        if rb:
            rx, ry = struct.unpack('<ff', rb)
            print(f'record view (count-1): pitch={rx:.3f}, yaw={ry:.3f}')

local_pawn_ptr = rpm(client_base + 0x25676D8, 8)
if local_pawn_ptr:
    local_pawn = struct.unpack('<Q', local_pawn_ptr)[0]
    print(f'local_pawn: 0x{local_pawn:X}')
    if local_pawn:
        ang = rpm(local_pawn + 0x3600, 8)
        if ang:
            ax, ay = struct.unpack('<ff', ang)
            print(f'm_angEyeAngles: pitch={ax:.3f}, yaw={ay:.3f}')
