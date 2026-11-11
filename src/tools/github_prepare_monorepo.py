# language: python, file: C:\Codex2\gpt\tools\github_prepare_monorepo.py
import os
import shutil
import subprocess

SRC_ROOT = r"C:\Codex2\gpt"
VAULT_ROOT = r"C:\Codex2\Hazzure_Projeto"
TARGET_DIR = r"C:\Codex2\Hazzure_Hub"

def ignore_build_artifacts(dir, files):
    ignored = set()
    for f in files:
        low = f.lower()
        if low in ("build", "__pycache__", ".git", ".trash", "nul"):
            ignored.add(f)
        elif low.endswith((".obj", ".dll", ".exe", ".pdb", ".lib", ".exp", ".log", ".dmp", ".mdmp", ".i", ".pyc")):
            ignored.add(f)
    return ignored

def prepare():
    if os.path.exists(TARGET_DIR):
        print(f"Cleaning existing {TARGET_DIR}...")
        # preserve .git if exists
        git_dir = os.path.join(TARGET_DIR, ".git")
        has_git = os.path.exists(git_dir)
        for item in os.listdir(TARGET_DIR):
            if item == ".git":
                continue
            p = os.path.join(TARGET_DIR, item)
            if os.path.isdir(p):
                shutil.rmtree(p, ignore_errors=True)
            else:
                os.remove(p)
    else:
        os.makedirs(TARGET_DIR, exist_ok=True)

    print("Staging src/HazzureInternal...")
    shutil.copytree(
        os.path.join(SRC_ROOT, "HazzureInternal"),
        os.path.join(TARGET_DIR, "src", "HazzureInternal"),
        ignore=ignore_build_artifacts
    )

    print("Staging src/Compartilhado...")
    shutil.copytree(
        os.path.join(SRC_ROOT, "Compartilhado"),
        os.path.join(TARGET_DIR, "src", "Compartilhado"),
        ignore=ignore_build_artifacts
    )

    print("Staging src/tools...")
    shutil.copytree(
        os.path.join(SRC_ROOT, "tools"),
        os.path.join(TARGET_DIR, "src", "tools"),
        ignore=ignore_build_artifacts
    )

    print("Staging docs/vault...")
    shutil.copytree(
        VAULT_ROOT,
        os.path.join(TARGET_DIR, "docs", "vault"),
        ignore=ignore_build_artifacts
    )

    print("Writing .gitignore...")
    gitignore_content = """# Build & intermediate files
build/
*.obj
*.exe
*.dll
*.pdb
*.lib
*.exp
*.log
*.dmp
*.mdmp
*.i
NUL
__pycache__/
*.pyc
.obsidian/workspace.json
.trash/
.vs/
"""
    with open(os.path.join(TARGET_DIR, ".gitignore"), "w", encoding="utf-8") as f:
        f.write(gitignore_content)

    print("Writing README.md...")
    readme_content = """# Hazzure — CS2 Internal Project (x64 / Source 2)

Hazzure is an internal x64 module designed for Counter-Strike 2 (`client.dll` build 14194 / TimeDateStamp `0x6AEBC4DA`).

This repository consolidates the active codebase, reverse engineering tooling, and the complete Obsidian engineering vault for seamless exploration and AI-assisted pair-programming (ChatGPT Web, Claude, Codex, etc.).

---

## 📁 Repository Structure

- **`src/HazzureInternal/`**: Core internal dynamic library (Unity build C++17, MSVC x64).
  - `src/`: Core implementation files (`GSLayout.inl`, `UIState.inl`, `InternalStartup.inl`, `AdvancedMovement.h`, `CombatRender.h`, cards, and widgets).
  - `imgui/`: Vendored Dear ImGui 1.91.9 + DirectX 11 backend.
  - `kiero/`: DirectX 11 hook orchestrator.
  - `minhook/`: Native x64 API hook engine.
  - `build.cmd`: Self-contained MSVC build pipeline.
- **`src/Compartilhado/`**: Shared inter-process communication definitions (`ipc.h`).
- **`src/tools/`**: Automation, reverse engineering, and telemetry scripts (`vault_sync.py`, `log_desync.py`, `re_cmd_angles.py`, `crash_rva_summary.py`, etc.).
- **`docs/vault/`**: Complete canonical Obsidian engineering vault.
  - `00_Gestao/Estado.md`: Single source of truth for runtime state, deployed binary hashes, and open tasks.
  - `00_Gestao/VALIDACAO.md`: Matrix of compiled vs. runtime-validated features.
  - `20_Tarefas/INDICE.md`: Indexed register of all engineering cycles (T001 through T079).
  - `70_Ferramentas/FERRAMENTAS.md`: Operational procedures, crash triage, and build pipelines.

---

## ⚡ Core Technical Architecture

1. **Sub-Tick Protobuf Execution (Zero-Snap / Perfect Silent & Desync)**:
   - Angles are written directly to `CMsgQAngle` inside the subtick `CCSGOInputHistoryEntryPB` protobuf (`input_history_` array at `cmd + 0x30`), eliminating camera snap on the client while transmitting authoritative angles to the server.
   - Bitmask verified at `0x1` (`VIEWANGLES`).
2. **Advanced Physical Movement**:
   - Frictionless Bunnyhop (`PredictContact` duck-aware multi-tick forward sweep via `HazzurePhysics::Sweep`).
   - Air Strafe Optimizer (~89° tangential wishdir) maximizing `AirAccelerate`.
   - Vectorial viewangle movement compensation.
3. **Multi-Layer Chams & Atmospheric Engine**:
   - 6 independent layered material passes (visible, invisible, backtrack, glow, metallic, flat) synced through IPC.
   - Dynamic climate presets (Night, Snow, Rain, Sunset) and skybox synchronization.
4. **GUI Engine ("Enigma v3" / Hazzure Layout)**:
   - 1280x735 custom Dear ImGui frame with 250px sidebar, weapon-class overrides, live aim preview ("WHERE IT AIMS"), and keybinding managers.

---

## 🔨 Build & Target Requirements

- **Platform**: Windows 11 x64
- **Toolchain**: Microsoft Visual Studio 2022 (MSVC v143, C++17)
- **Target Engine**: Counter-Strike 2 (Build 14194)
- **Primary Binary**: `Hazzure.Internal.dll` (Deploy target: `C:\\HazzureKernel\\Hazzure.Internal.dll` & `modules\\`)

---

## 📖 Canonical Navigation for AI Models

When analyzing tasks or features in this repository:
1. Start at `docs/vault/Inicio.md` and `docs/vault/00_Gestao/Estado.md`.
2. Check `docs/vault/00_Gestao/VALIDACAO.md` to see whether a feature is merely compiled or validated in-game.
3. Inspect `docs/vault/20_Tarefas/INDICE.md` to review past engineering tasks before modifying code.
"""
    with open(os.path.join(TARGET_DIR, "README.md"), "w", encoding="utf-8") as f:
        f.write(readme_content)

    print("Initializing git repository in target...")
    git_dir = os.path.join(TARGET_DIR, ".git")
    if not os.path.exists(git_dir):
        subprocess.run(["git", "init"], cwd=TARGET_DIR, check=True)
        subprocess.run(["git", "branch", "-M", "master"], cwd=TARGET_DIR, check=True)
        subprocess.run(["git", "config", "user.name", "AN0N"], cwd=TARGET_DIR, check=True)
        subprocess.run(["git", "config", "user.email", "anon@hazzure.local"], cwd=TARGET_DIR, check=True)

    print("Adding files to git index...")
    subprocess.run(["git", "add", "."], cwd=TARGET_DIR, check=True)
    subprocess.run(["git", "commit", "-m", "feat: initial commit of Hazzure monorepo (code, tools, vault)"], cwd=TARGET_DIR, check=True)
    print("Done! Repository ready in", TARGET_DIR)

if __name__ == "__main__":
    prepare()
