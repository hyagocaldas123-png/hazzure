#!/usr/bin/env python3
# coding: utf-8
"""
Hazzure Automated Offset & Schema Pipeline (Static PE64 SigScan + Schema XRef)
Analyzes CS2 client.dll / engine2.dll directly via byte signatures and
SchemaClassFieldData_t cross-references, with JSON validation and fallback.
"""

import os
import re
import sys
import json
import struct
from datetime import datetime, timezone

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
OFFSETS_DIR = os.path.join(ROOT, "Hazzure", "offsets")
COMPARTILHADO_DIR = os.path.join(ROOT, "Compartilhado")
OUTPUT_HEADER = os.path.join(COMPARTILHADO_DIR, "offsets_generated.h")
BUILD_IDENTITY_HEADER = os.path.join(ROOT, "HazzureInternal", "src", "BuildIdentity.h")

# Default / Fallback offsets for Build 14192
DEFAULT_BUTTONS = {
    "jump": 0x22363E0,
}

DEFAULT_GLOBALS = {
    "dwCSGOInput": 0x257F980,
    "dwEntityList": 0x271EB28,
    "dwGameRules": 0x2569378,
    "dwGlobalVars": 0x22320E8,
    "dwGlowManager": 0x25660A0,
    "dwLocalPlayerController": 0x25413F8,
    "dwLocalPlayerPawn": 0x25698C8,
    "dwPlantedC4": 0x24D0B70,
    "dwPrediction": 0x25697D0,
    "dwSensitivity": 0x2566BC8,
    "dwViewAngles": 0x2580008,
    "dwViewMatrix": 0x256F1D0,
    "dwViewRender": 0x256FBA8,
    "dwWeaponC4": 0x24CE108,
}

DEFAULT_SCHEMA = {
    "m_iHealth": 0x34C,
    "m_iTeamNum": 0x3E7,
    "m_vecAbsOrigin": 0xC8,
    "m_vOldOrigin": 0x14A4,
    "m_lifeState": 0x354,
    "m_hPlayerPawn": 0x92C,
    "m_hObserverPawn": 0x930,
    "m_iszPlayerName": 0x6FC,
    "dwSensitivity_sensitivity": 0x58,
    "m_fFlags": 0x3F4,
    "m_pWeaponServices": 0x12F0,
    "m_hActiveWeapon": 0x60,
    "m_hMyWearables": 0x1268,
    "m_AttributeManager": 0x1290,
    "m_Item": 0x50,
    "m_iItemDefinitionIndex": 0x149A,
    "m_nFallbackPaintKit": 0x18A8,
    "m_nFallbackSeed": 0x18AC,
    "m_flFallbackWear": 0x18B0,
    "m_nFallbackStatTrak": 0x18B4,
    "m_hSkyMaterial": 0x1098,
    "m_pSceneObjectCache": 0x10E8,
    "m_pGameSceneNode": 0x330,
    "m_vecVelocity": 0x430,
    "m_hOwnerEntity": 0x520,
    "m_nSubclassID": 0x380,
    "m_pCameraServices": 0x1328,
    "m_hActivePostProcessingVolume": 0x200,
    "m_flMinLogExposure": 0x119C,
    "m_iItemIDHigh": 0x14B0,
    "m_iItemIDLow": 0x14B4,
    "m_AttributeList": 0x1560,
    "m_vTintColor": 0x10AC,
    "m_flBrightnessScale": 0x10B4,
    "m_modelState": 0x140,
    "m_iShotsFired": 0x1EB8,
    "m_pAimPunchServices": 0x1598,
    "m_predictableBaseAngle": 0x50,
    "m_unpredictableBaseAngle": 0xA4,
    "m_angEyeAngles": 0x3600,
    "m_vecViewOffset": 0xF60,
    "m_iIDEntIndex": 0x36DC,
}

# Byte-pattern signatures in client.dll (.text) -> (regex_bytes, disp32_offset, inst_end_offset, rva_delta)
CLIENT_SIGS = {
    "dwEntityList": (
        rb"\x48\x8B\x05....\x48\x8B\xB0\x10\x02\x00\x00",
        3, 7, 0
    ),
    "dwLocalPlayerController": (
        rb"\x48\x8D\x0D....\x33\xF6\x4C\x89\x34\xF9",
        3, 7, 0
    ),
    "dwViewMatrix": (
        rb"\x48\x8D\x0D....\x48\xC1\xE0\x06\x48\x03\xC1\xC3",
        3, 7, 0
    ),
}


def locate_steam_cs2():
    """Locate CS2 client.dll from common Steam library paths."""
    candidates = [
        r"C:\Program Files (x86)\Steam\steamapps\common\Counter-Strike Global Offensive\game\csgo\bin\win64\client.dll",
        r"D:\SteamLibrary\steamapps\common\Counter-Strike Global Offensive\game\csgo\bin\win64\client.dll",
        r"E:\SteamLibrary\steamapps\common\Counter-Strike Global Offensive\game\csgo\bin\win64\client.dll",
    ]
    for path in candidates:
        if os.path.isfile(path):
            return path
    return None


def parse_pe_sections(data):
    """Parse PE64 header, ImageBase, TimeDateStamp, SizeOfImage and sections."""
    if len(data) < 0x100 or data[:2] != b"MZ":
        return None
    pe_off = struct.unpack_from("<I", data, 0x3C)[0]
    if pe_off + 0xF0 > len(data) or data[pe_off:pe_off + 4] != b"PE\x00\x00":
        return None

    num_sec = struct.unpack_from("<H", data, pe_off + 6)[0]
    timestamp = struct.unpack_from("<I", data, pe_off + 8)[0]
    opt_size = struct.unpack_from("<H", data, pe_off + 20)[0]
    image_base = struct.unpack_from("<Q", data, pe_off + 24 + 24)[0]
    size_of_image = struct.unpack_from("<I", data, pe_off + 24 + 56)[0]

    sec_off = pe_off + 24 + opt_size
    sections = []
    for i in range(num_sec):
        s = sec_off + i * 40
        if s + 40 > len(data):
            break
        name = data[s:s + 8].rstrip(b"\x00").decode("ascii", "ignore")
        vsize, va, rsize, rptr = struct.unpack_from("<IIII", data, s + 8)
        sections.append((name, va, vsize, rptr, rsize))

    return {
        "image_base": image_base,
        "timestamp": timestamp,
        "size_of_image": size_of_image,
        "sections": sections,
    }


def raw_to_rva(raw_offset, sections):
    for _, va, vsize, rptr, rsize in sections:
        if rptr <= raw_offset < rptr + rsize:
            return va + (raw_offset - rptr)
    return None


def extract_schema_field_offsets(data, pe_info, field_name):
    """Scan PE64 .rdata/.data for SchemaClassFieldData_t entries referencing field_name."""
    target = field_name.encode("ascii") + b"\x00"
    sections = pe_info["sections"]
    image_base = pe_info["image_base"]

    rvas = []
    pos = 0
    while True:
        idx = data.find(target, pos)
        if idx == -1:
            break
        if idx == 0 or data[idx - 1] == 0:
            rva = raw_to_rva(idx, sections)
            if rva is not None:
                rvas.append(rva)
        pos = idx + 1

    offsets = []
    for rva in rvas:
        ptr_bytes = struct.pack("<Q", image_base + rva)
        p2 = 0
        while True:
            hit = data.find(ptr_bytes, p2)
            if hit == -1:
                break
            if hit + 24 <= len(data):
                off, meta_sz = struct.unpack_from("<ii", data, hit + 16)
                if 0 < off < 0x10000 and 0 <= meta_sz <= 32:
                    if off not in offsets:
                        offsets.append(off)
            p2 = hit + 8
    return offsets


def analyze_live_pe_binary(client_path):
    """Perform static PE64 pattern scanning and SchemaClassFieldData_t extraction on client.dll."""
    try:
        with open(client_path, "rb") as f:
            data = f.read()
    except Exception as e:
        print(f"[WARN] Could not read {client_path}: {e}")
        return None

    pe_info = parse_pe_sections(data)
    if not pe_info:
        return None

    text_sec = next((s for s in pe_info["sections"] if s[0] == ".text"), None)
    if not text_sec:
        return None

    _, text_va, text_vsize, text_raw, text_rsize = text_sec
    text_bytes = data[text_raw:text_raw + min(text_vsize, text_rsize)]

    scanned_globals = {}
    for name, (pat, disp_off, inst_end, delta) in CLIENT_SIGS.items():
        m = re.search(pat, text_bytes, re.DOTALL)
        if m:
            idx = m.start()
            disp = struct.unpack_from("<i", text_bytes, idx + disp_off)[0]
            rva = text_va + idx + inst_end + disp + delta
            if 0 < rva < pe_info["size_of_image"]:
                scanned_globals[name] = rva

    # Resolve dwLocalPlayerPawn via Source2ClientPrediction001 interface registration
    pred_str_idx = data.find(b"Source2ClientPrediction001\x00")
    pred_str_rva = raw_to_rva(pred_str_idx, pe_info["sections"]) if pred_str_idx != -1 else None
    if pred_str_rva:
        for idx in range(len(text_bytes) - 24):
            if text_bytes[idx] == 0x4C and text_bytes[idx + 1] == 0x8D and text_bytes[idx + 2] == 0x05:
                disp = struct.unpack_from("<i", text_bytes, idx + 3)[0]
                if text_va + idx + 7 + disp == pred_str_rva:
                    if text_bytes[idx + 7] == 0x48 and text_bytes[idx + 8] == 0x8D and text_bytes[idx + 9] == 0x15:
                        fn_disp = struct.unpack_from("<i", text_bytes, idx + 10)[0]
                        fn_va = text_va + idx + 14 + fn_disp
                        fn_idx = fn_va - text_va
                        if 0 <= fn_idx <= len(text_bytes) - 8 and text_bytes[fn_idx:fn_idx + 3] == b"\x48\x8D\x05":
                            pred_disp = struct.unpack_from("<i", text_bytes, fn_idx + 3)[0]
                            pred_rva = fn_va + 7 + pred_disp
                            scanned_globals["dwLocalPlayerPawn"] = pred_rva + 0xF8
                    break

    # Resolve dwSensitivity via ConVar "sensitivity" registration
    sens_str_idx = data.find(b"\x00sensitivity\x00")
    sens_str_rva = raw_to_rva(sens_str_idx + 1, pe_info["sections"]) if sens_str_idx != -1 else None
    if sens_str_rva:
        for idx in range(len(text_bytes) - 24):
            if text_bytes[idx] == 0x48 and text_bytes[idx + 1] == 0x8D and text_bytes[idx + 2] == 0x15:
                disp = struct.unpack_from("<i", text_bytes, idx + 3)[0]
                if text_va + idx + 7 + disp == sens_str_rva:
                    if text_bytes[idx + 12:idx + 15] == b"\x48\x8D\x0D":
                        cvar_disp = struct.unpack_from("<i", text_bytes, idx + 15)[0]
                        cvar_rva = text_va + idx + 19 + cvar_disp
                        scanned_globals["dwSensitivity"] = cvar_rva + 0x08
                    break

    # Extract schema offsets from SchemaClassFieldData_t structures
    scanned_schema = {}
    single_fields = [
        "m_iHealth",
        "m_iTeamNum",
        "m_vecAbsOrigin",
        "m_pWeaponServices",
        "m_hActiveWeapon",
        "m_hMyWearables",
        "m_Item",
        "m_nFallbackPaintKit",
        "m_nFallbackSeed",
        "m_flFallbackWear",
        "m_nFallbackStatTrak",
        "m_pGameSceneNode",
        "m_vecVelocity",
        "m_pCameraServices",
        "m_hActivePostProcessingVolume",
        "m_flMinLogExposure",
        "m_modelState",
        "m_iShotsFired",
        "m_pAimPunchServices",
        "m_predictableBaseAngle",
        "m_unpredictableBaseAngle",
        "m_angEyeAngles",
        "m_vecViewOffset",
    ]
    for fname in single_fields:
        cands = extract_schema_field_offsets(data, pe_info, fname)
        if len(cands) == 1:
            scanned_schema[fname] = cands[0]

    # Disambiguate multi-class fields by known range
    flags_cands = [x for x in extract_schema_field_offsets(data, pe_info, "m_fFlags") if x >= 0x100]
    if flags_cands:
        scanned_schema["m_fFlags"] = flags_cands[0]

    sky_cands = [x for x in extract_schema_field_offsets(data, pe_info, "m_hSkyMaterial") if x >= 0x1000]
    if sky_cands:
        scanned_schema["m_hSkyMaterial"] = sky_cands[0]
        scanned_schema["m_pSceneObjectCache"] = sky_cands[0] + 0x50

    attr_cands = [x for x in extract_schema_field_offsets(data, pe_info, "m_AttributeManager") if 0x1100 <= x <= 0x12D0]
    if attr_cands:
        scanned_schema["m_AttributeManager"] = attr_cands[0]

    item_def_cands = extract_schema_field_offsets(data, pe_info, "m_iItemDefinitionIndex")
    if (
        item_def_cands
        and "m_AttributeManager" in scanned_schema
        and "m_Item" in scanned_schema
    ):
        scanned_schema["m_iItemDefinitionIndex"] = (
            scanned_schema["m_AttributeManager"]
            + scanned_schema["m_Item"]
            + item_def_cands[0]
        )

    return {
        "pe_info": pe_info,
        "globals": scanned_globals,
        "schema": scanned_schema,
    }


def load_offsets(client_path=None):
    """Combine defaults, JSON dumps, and live PE64 static analysis."""
    offsets_file = os.path.join(OFFSETS_DIR, "offsets.json")
    client_file = os.path.join(OFFSETS_DIR, "client_dll.json")
    info_file = os.path.join(OFFSETS_DIR, "info.json")

    buttons_file = os.path.join(OFFSETS_DIR, "buttons.json")
    build_num = 14192
    globals_dict = dict(DEFAULT_GLOBALS)
    schema_dict = dict(DEFAULT_SCHEMA)
    buttons_dict = dict(DEFAULT_BUTTONS)
    pe_meta = None

    if os.path.isfile(buttons_file):
        try:
            with open(buttons_file, "r", encoding="utf-8") as f:
                bdata = json.load(f)
                client_buttons = bdata.get("client.dll", {})
                for k, v in client_buttons.items():
                    if isinstance(v, int):
                        buttons_dict[k] = v
        except Exception as e:
            print(f"[WARN] Failed reading buttons.json: {e}")

    if os.path.isfile(info_file):
        try:
            with open(info_file, "r", encoding="utf-8") as f:
                info = json.load(f)
                build_num = info.get("build_number", build_num)
        except Exception as e:
            print(f"[WARN] Failed reading info.json: {e}")

    if os.path.isfile(offsets_file):
        try:
            with open(offsets_file, "r", encoding="utf-8") as f:
                data = json.load(f)
                client_data = data.get("client.dll", {})
                for k, v in client_data.items():
                    if k in globals_dict and isinstance(v, int):
                        globals_dict[k] = v
        except Exception as e:
            print(f"[WARN] Failed reading offsets.json: {e}")

    if os.path.isfile(client_file):
        try:
            with open(client_file, "r", encoding="utf-8") as f:
                cdata = json.load(f).get("client.dll", {}).get("classes", {})
                if "C_BaseEntity" in cdata:
                    fields = cdata["C_BaseEntity"].get("fields", {})
                    for fname in ("m_fFlags", "m_iHealth", "m_iTeamNum", "m_pGameSceneNode", "m_vecVelocity"):
                        if fname in fields:
                            schema_dict[fname] = fields[fname]
                if "CGameSceneNode" in cdata:
                    fields = cdata["CGameSceneNode"].get("fields", {})
                    if "m_vecAbsOrigin" in fields:
                        schema_dict["m_vecAbsOrigin"] = fields["m_vecAbsOrigin"]
                if "C_BasePlayerPawn" in cdata:
                    fields = cdata["C_BasePlayerPawn"].get("fields", {})
                    if "m_pWeaponServices" in fields:
                        schema_dict["m_pWeaponServices"] = fields["m_pWeaponServices"]
                if "C_CSPlayerPawnBase" in cdata:
                    fields = cdata["C_CSPlayerPawnBase"].get("fields", {})
                    if "m_hMyWearables" in fields:
                        schema_dict["m_hMyWearables"] = fields["m_hMyWearables"]
        except Exception as e:
            print(f"[WARN] Failed reading client_dll.json: {e}")

    if client_path and os.path.isfile(client_path):
        live = analyze_live_pe_binary(client_path)
        if live:
            pe_meta = live["pe_info"]
            globals_dict.update(live["globals"])
            schema_dict.update(live["schema"])
            print(
                f"[PE64] Static SigScan resolved {len(live['globals'])}/5 globals "
                f"and {len(live['schema'])} schema fields directly from client.dll "
                f"(TimeDateStamp=0x{pe_meta['timestamp']:X}, SizeOfImage=0x{pe_meta['size_of_image']:X})"
            )

    return build_num, globals_dict, schema_dict, buttons_dict, pe_meta


def generate_header(build_num, globals_dict, schema_dict, buttons_dict=None, pe_meta=None):
    """Generate C++ constexpr header for shared inclusion."""
    now_str = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC")
    mode_str = (
        f"PE64_STATIC_SIGSCAN (TimeDateStamp=0x{pe_meta['timestamp']:X}, SizeOfImage=0x{pe_meta['size_of_image']:X})"
        if pe_meta
        else "JSON_SCHEMA_FALLBACK"
    )
    lines = [
        "// Generated automatically by tools/offset_dumper.py",
        f"// Timestamp: {now_str}",
        f"// CS2 Target Build: {build_num} | Mode: {mode_str}",
        "#pragma once",
        "#include <cstdint>",
        "",
        "namespace HazzureOffsets {",
        f"    inline constexpr uint32_t kBuildNumber = {build_num};",
        "",
        "    namespace Globals {",
    ]

    for k, v in sorted(globals_dict.items()):
        lines.append(f"        inline uintptr_t {k} = 0x{v:X};")

    lines.append("    } // namespace Globals")
    lines.append("")
    lines.append("    namespace Schema {")

    for k, v in sorted(schema_dict.items()):
        lines.append(f"        inline constexpr uintptr_t {k} = 0x{v:X};")

    lines.append("    } // namespace Schema")

    if buttons_dict:
        lines.append("")
        lines.append("    namespace Buttons {")
        for k, v in sorted(buttons_dict.items()):
            lines.append(f"        inline uintptr_t {k} = 0x{v:X};")
        lines.append("    } // namespace Buttons")

    lines.append("} // namespace HazzureOffsets")
    lines.append("")

    content = "\n".join(lines)
    os.makedirs(os.path.dirname(OUTPUT_HEADER), exist_ok=True)
    with open(OUTPUT_HEADER, "w", encoding="utf-8") as f:
        f.write(content)

    print(f"[OK] Generated {OUTPUT_HEADER}")


def update_build_identity(client_path):
    """Synchronize HazzureInternal/src/BuildIdentity.h with live PE64 module headers."""
    if not client_path or not os.path.isfile(client_path):
        return
    bin_dir = os.path.dirname(client_path)
    engine_bin_dir = os.path.abspath(os.path.join(bin_dir, "..", "..", "..", "bin", "win64"))
    scene_path = os.path.join(engine_bin_dir, "scenesystem.dll")
    mat_path = os.path.join(engine_bin_dir, "materialsystem2.dll")
    if not (os.path.isfile(scene_path) and os.path.isfile(mat_path)):
        return
    try:
        with open(scene_path, "rb") as f:
            scene_pe = parse_pe_sections(f.read(0x1000))
        with open(mat_path, "rb") as f:
            mat_pe = parse_pe_sections(f.read(0x1000))
        with open(client_path, "rb") as f:
            client_pe = parse_pe_sections(f.read(0x1000))
        if not (scene_pe and mat_pe and client_pe):
            return
        lines = [
            "// Exact installed build identity; synchronized by tools/offset_dumper.py.",
            "#pragma once",
            f"#define SCENE_STAMP 0x{scene_pe['timestamp']:X}u",
            f"#define SCENE_SIZE 0x{scene_pe['size_of_image']:X}u",
            f"#define MATERIAL_STAMP 0x{mat_pe['timestamp']:X}u",
            f"#define MATERIAL_SIZE 0x{mat_pe['size_of_image']:X}u",
            f"#define CLIENT_STAMP 0x{client_pe['timestamp']:X}u",
            f"#define CLIENT_SIZE 0x{client_pe['size_of_image']:X}u",
            "",
        ]
        with open(BUILD_IDENTITY_HEADER, "w", encoding="utf-8") as f:
            f.write("\n".join(lines))
        print(f"[OK] Synchronized {BUILD_IDENTITY_HEADER}")
    except Exception as e:
        print(f"[WARN] Could not update BuildIdentity.h: {e}")


def main():
    print("==================================================")
    print(" Hazzure Automated Offset & Schema Pipeline")
    print("==================================================")
    steam_client = locate_steam_cs2()
    if steam_client:
        print(f"[INFO] Found live CS2 client.dll: {steam_client}")
    else:
        print("[INFO] Using local validated schema dump in Hazzure/offsets/")

    build_num, globals_dict, schema_dict, buttons_dict, pe_meta = load_offsets(steam_client)
    print(f"[INFO] Loaded {len(globals_dict)} globals, {len(schema_dict)} schema offsets, and {len(buttons_dict)} buttons for Build {build_num}")
    generate_header(build_num, globals_dict, schema_dict, buttons_dict, pe_meta)
    update_build_identity(steam_client)
    print("[SUCCESS] Offset pipeline synchronization complete.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
