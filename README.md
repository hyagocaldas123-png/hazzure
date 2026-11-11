# Hazzure — CS2 Internal Project (x64 / Source 2)

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
- **Primary Binary**: `Hazzure.Internal.dll` (Deploy target: `C:\HazzureKernel\Hazzure.Internal.dll` & `modules\`)

---

## 📖 Canonical Navigation for AI Models

When analyzing tasks or features in this repository:
1. Start at `docs/vault/Inicio.md` and `docs/vault/00_Gestao/Estado.md`.
2. Check `docs/vault/00_Gestao/VALIDACAO.md` to see whether a feature is merely compiled or validated in-game.
3. Inspect `docs/vault/20_Tarefas/INDICE.md` to review past engineering tasks before modifying code.
