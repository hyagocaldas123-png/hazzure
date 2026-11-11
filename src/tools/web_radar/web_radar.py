#!/usr/bin/env python3
# coding: utf-8
"""
Hazzure Web Radar Server
Lightweight zero-injection web radar for secondary monitors and mobile devices.
Serves an interactive HTML5 Canvas radar at http://127.0.0.1:8080
"""

import os
import sys
import json
import time
import math
import struct
import ctypes
from ctypes import wintypes
from http.server import HTTPServer, BaseHTTPRequestHandler
import threading

# Win32 API Definitions for RPM
PROCESS_VM_READ = 0x0010
PROCESS_QUERY_INFORMATION = 0x0400
TH32CS_SNAPPROCESS = 0x00000002

class PROCESSENTRY32W(ctypes.Structure):
    _fields_ = [
        ("dwSize", wintypes.DWORD),
        ("cntUsage", wintypes.DWORD),
        ("th32ProcessID", wintypes.DWORD),
        ("th32DefaultHeapID", ctypes.POINTER(wintypes.ULONG)),
        ("th32ModuleID", wintypes.DWORD),
        ("cntThreads", wintypes.DWORD),
        ("th32ParentProcessID", wintypes.DWORD),
        ("pcPriClassBase", wintypes.LONG),
        ("dwFlags", wintypes.DWORD),
        ("szExeFile", wintypes.WCHAR * 260)
    ]

class MODULEENTRY32W(ctypes.Structure):
    _fields_ = [
        ("dwSize", wintypes.DWORD),
        ("th32ModuleID", wintypes.DWORD),
        ("th32ProcessID", wintypes.DWORD),
        ("GlblcntUsage", wintypes.DWORD),
        ("ProccntUsage", wintypes.DWORD),
        ("modBaseAddr", ctypes.POINTER(ctypes.c_byte)),
        ("modBaseSize", wintypes.DWORD),
        ("hModule", wintypes.HMODULE),
        ("szModule", wintypes.WCHAR * 256),
        ("szExePath", wintypes.WCHAR * 260)
    ]

kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)

# Offsets Build 14192
dwEntityList = 0x271C8B8
dwLocalPlayerPawn = 0x25676D8
dwViewMatrix = 0x256CFE0
m_iHealth = 0x34C
m_iTeamNum = 0x3E7
m_vecAbsOrigin = 0x1264

radar_data = {
    "status": "Aguardando CS2...",
    "local": {"x": 0, "y": 0, "z": 0, "yaw": 0, "team": 0},
    "players": []
}
data_lock = threading.Lock()

def get_pid_and_client():
    snap = kernel32.CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)
    if snap == -1: return None, 0
    pe = PROCESSENTRY32W()
    pe.dwSize = ctypes.sizeof(pe)
    pid = None
    if kernel32.Process32FirstW(snap, ctypes.byref(pe)):
        while True:
            if pe.szExeFile.lower() == "cs2.exe":
                pid = pe.th32ProcessID
                break
            if not kernel32.Process32NextW(snap, ctypes.byref(pe)):
                break
    kernel32.CloseHandle(snap)
    if not pid: return None, 0

    # Get client.dll base
    snap_m = kernel32.CreateToolhelp32Snapshot(0x00000008 | 0x00000010, pid)
    me = MODULEENTRY32W()
    me.dwSize = ctypes.sizeof(me)
    client_base = 0
    if kernel32.Module32FirstW(snap_m, ctypes.byref(me)):
        while True:
            if me.szModule.lower() == "client.dll":
                client_base = ctypes.cast(me.modBaseAddr, ctypes.c_void_p).value
                break
            if not kernel32.Module32NextW(snap_m, ctypes.byref(me)):
                break
    kernel32.CloseHandle(snap_m)
    return pid, client_base

def read_mem(hProcess, address, size):
    buf = ctypes.create_string_buffer(size)
    bytes_read = ctypes.c_size_t(0)
    if kernel32.ReadProcessMemory(hProcess, ctypes.c_void_p(address), buf, size, ctypes.byref(bytes_read)):
        return buf.raw
    return None

def memory_worker():
    global radar_data
    while True:
        pid, client = get_pid_and_client()
        if not pid or not client:
            with data_lock:
                radar_data["status"] = "CS2 nao encontrado"
                radar_data["players"] = []
            time.sleep(2)
            continue

        hProc = kernel32.OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, False, pid)
        if not hProc:
            time.sleep(2)
            continue

        with data_lock:
            radar_data["status"] = "Conectado ao CS2"

        while True:
            # Read local pawn
            lp_raw = read_mem(hProc, client + dwLocalPlayerPawn, 8)
            if not lp_raw: break
            local_pawn = struct.unpack("<Q", lp_raw)[0]
            if not local_pawn:
                time.sleep(0.1)
                continue

            local_team_raw = read_mem(hProc, local_pawn + m_iTeamNum, 1)
            local_team = struct.unpack("<B", local_team_raw)[0] if local_team_raw else 0
            local_pos_raw = read_mem(hProc, local_pawn + m_vecAbsOrigin, 12)
            lx, ly, lz = struct.unpack("<fff", local_pos_raw) if local_pos_raw else (0, 0, 0)

            # Read entity list
            ent_list_raw = read_mem(hProc, client + dwEntityList, 8)
            ent_list = struct.unpack("<Q", ent_list_raw)[0] if ent_list_raw else 0

            players = []
            if ent_list:
                for i in range(1, 64):
                    entry_raw = read_mem(hProc, ent_list + (8 * (i >> 9) + 16), 8)
                    if not entry_raw: continue
                    entry = struct.unpack("<Q", entry_raw)[0]
                    if not entry: continue

                    controller_raw = read_mem(hProc, entry + 120 * (i & 0x1FF), 8)
                    if not controller_raw: continue
                    controller = struct.unpack("<Q", controller_raw)[0]
                    if not controller: continue

                    pawn_h_raw = read_mem(hProc, controller + 0x7E4, 4) # m_hPlayerPawn
                    pawn_handle = struct.unpack("<I", pawn_h_raw)[0] if pawn_h_raw else 0
                    if not pawn_handle: continue

                    # Resolve pawn from handle
                    p_entry_raw = read_mem(hProc, ent_list + (8 * ((pawn_handle & 0x7FFF) >> 9) + 16), 8)
                    if not p_entry_raw: continue
                    p_entry = struct.unpack("<Q", p_entry_raw)[0]
                    if not p_entry: continue

                    pawn_ptr_raw = read_mem(hProc, p_entry + 120 * (pawn_handle & 0x1FF), 8)
                    pawn = struct.unpack("<Q", pawn_ptr_raw)[0] if pawn_ptr_raw else 0
                    if not pawn or pawn == local_pawn: continue

                    hp_raw = read_mem(hProc, pawn + m_iHealth, 4)
                    hp = struct.unpack("<i", hp_raw)[0] if hp_raw else 0
                    if hp <= 0 or hp > 100: continue

                    team_raw = read_mem(hProc, pawn + m_iTeamNum, 1)
                    team = struct.unpack("<B", team_raw)[0] if team_raw else 0

                    pos_raw = read_mem(hProc, pawn + m_vecAbsOrigin, 12)
                    if not pos_raw: continue
                    px, py, pz = struct.unpack("<fff", pos_raw)

                    players.append({
                        "x": px, "y": py, "z": pz,
                        "hp": hp,
                        "team": team,
                        "enemy": (team != local_team)
                    })

            with data_lock:
                radar_data["local"] = {"x": lx, "y": ly, "z": lz, "team": local_team}
                radar_data["players"] = players
                radar_data["status"] = f"Ativo ({len(players)} jogadores)"

            time.sleep(0.016) # ~60Hz poll

        kernel32.CloseHandle(hProc)
        time.sleep(1)

HTML_PAGE = """<!DOCTYPE html>
<html lang="pt-br">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Hazzure Web Radar</title>
<style>
  * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
  body { background: #0B0B0D; color: #F5F5F9; display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100vh; overflow: hidden; }
  #header { position: absolute; top: 15px; left: 20px; font-size: 13px; color: #82828C; z-index: 10; }
  #status-badge { color: #FFFFFF; font-weight: 600; padding: 4px 8px; background: #18181D; border: 1px solid #26262D; border-radius: 6px; }
  #radar-wrap { position: relative; width: 90vmin; height: 90vmin; max-width: 700px; max-height: 700px; }
  canvas { width: 100%; height: 100%; background: #0E0E11; border: 1px solid #26262D; border-radius: 50%; box-shadow: 0 8px 32px rgba(0,0,0,0.6); }
  #controls { position: absolute; bottom: 20px; display: flex; gap: 10px; z-index: 10; }
  button { background: #18181D; border: 1px solid #26262D; color: #F5F5F9; padding: 8px 16px; border-radius: 6px; cursor: pointer; font-size: 12px; transition: 0.2s; }
  button:hover { background: #FFFFFF; color: #0B0B0D; }
</style>
</head>
<body>
<div id="header">
  Hazzure Web Radar | <span id="status-badge">Conectando...</span>
</div>
<div id="radar-wrap">
  <canvas id="radar" width="800" height="800"></canvas>
</div>
<div id="controls">
  <button id="zoom-in">Zoom +</button>
  <button id="zoom-out">Zoom -</button>
  <button id="center-btn">Centralizar</button>
</div>
<script>
  const canvas = document.getElementById('radar');
  const ctx = canvas.getContext('2d');
  const statusBadge = document.getElementById('status-badge');
  let scale = 0.12;

  document.getElementById('zoom-in').onclick = () => scale = Math.min(scale * 1.25, 0.4);
  document.getElementById('zoom-out').onclick = () => scale = Math.max(scale * 0.8, 0.04);
  document.getElementById('center-btn').onclick = () => scale = 0.12;

  async function fetchRadar() {
    try {
      const res = await fetch('/data');
      const data = await res.json();
      statusBadge.innerText = data.status;
      render(data);
    } catch(e) {
      statusBadge.innerText = "Desconectado";
    }
    requestAnimationFrame(fetchRadar);
  }

  function render(data) {
    const w = canvas.width, h = canvas.height, cx = w/2, cy = h/2;
    ctx.clearRect(0, 0, w, h);

    // Radar circle & Grid
    ctx.strokeStyle = '#26262D';
    ctx.lineWidth = 1.5;
    [100, 200, 300, 380].forEach(r => {
      ctx.beginPath(); ctx.arc(cx, cy, r, 0, Math.PI*2); ctx.stroke();
    });

    // Crosshairs
    ctx.beginPath();
    ctx.moveTo(cx, cy - 380); ctx.lineTo(cx, cy + 380);
    ctx.moveTo(cx - 380, cy); ctx.lineTo(cx + 380, cy);
    ctx.stroke();

    const lx = data.local.x, ly = data.local.y;

    // Draw other players
    data.players.forEach(p => {
      const dx = (p.x - lx) * scale;
      const dy = -(p.y - ly) * scale; // Invert Y for screen coords
      const px = cx + dx, py = cy + dy;

      const distFromCenter = Math.sqrt(dx*dx + dy*dy);
      if (distFromCenter > 380) return; // Out of bounds

      ctx.fillStyle = p.enemy ? '#FF3355' : '#00E676';
      ctx.beginPath();
      ctx.arc(px, py, 7, 0, Math.PI*2);
      ctx.fill();

      // Health bar
      ctx.fillStyle = '#FFFFFF';
      ctx.font = '10px sans-serif';
      ctx.textAlign = 'center';
      ctx.fillText(p.hp, px, py - 11);
    });

    // Local player (Center)
    ctx.fillStyle = '#00B0FF';
    ctx.beginPath();
    ctx.arc(cx, cy, 8, 0, Math.PI*2);
    ctx.fill();
    ctx.strokeStyle = '#FFFFFF';
    ctx.lineWidth = 2;
    ctx.stroke();
  }

  fetchRadar();
</script>
</body>
</html>
"""

class RadarHandler(BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        pass # Silence logs

    def do_GET(self):
        if self.path == "/data":
            with data_lock:
                payload = json.dumps(radar_data).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(payload)
        else:
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.end_headers()
            self.wfile.write(HTML_PAGE.encode("utf-8"))

def main():
    print("==================================================")
    print(" Hazzure Web Radar Server")
    print(" Servidor disponivel em: http://127.0.0.1:8080")
    print(" Para ver no celular/segundo monitor, use o IP local.")
    print("==================================================")
    t = threading.Thread(target=memory_worker, daemon=True)
    t.start()

    server = HTTPServer(("0.0.0.0", 8080), RadarHandler)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nEncerrando Web Radar...")

if __name__ == "__main__":
    sys.exit(main())
