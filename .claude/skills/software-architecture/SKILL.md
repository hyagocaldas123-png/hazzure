---
name: software-architecture
description: >
  Maintains clean separation of concerns, reduces module coupling, organizes internal
  subsystems, and prevents improvised hacks in Hazzure.
license: MIT
---

# Software Architecture (Hazzure)

Use when planning structural changes, adding major feature suites, or refactoring subsystems.

## Architectural Boundaries
1. **Separation of Layers**:
   - **Render / GUI (`HazzureUI`)**: Reads configuration state (`g_cfg`) and diagnostics (`g_frameDiagnostics`). Renders ImGui widgets and layout. Never runs game physics, entity traces, or network command serialization.
   - **Core Engine / Hooks (`InternalStartup`)**: Intercepts DirectX 11 `Present`, input `WndProc`, and game `CreateMove`. Coordinates tick timing and dispatches to subsystems.
   - **Physics & Gameplay (`AdvancedMovement`, `CombatRender`)**: Executes duck-aware sweeps, friction calculations, wishdir generation, and subtick protobuf assembly.
   - **Persistence (`OverlayPersistence`)**: Reads/writes configuration files and synchronizes with IPC buffers.
2. **Coupling & Anti-Hacks**:
   - Do not invent speculative hardcoded offsets: verify in IDA or run runtime offset dumpers before committing magic numbers.
   - Do not mix telemetry logging into per-frame critical loops (`GS_Log` writes to disk on each call; use frame throttling).
   - Preserve single translation unit (unity build) constraints in `HazzureInternal/src`.
