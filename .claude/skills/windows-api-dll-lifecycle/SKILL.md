---
name: windows-api-dll-lifecycle
description: >
  Reviews Windows API usage, DLL entrypoint/lifecycle (DllMain, threads, teardown),
  handle management, and structured exception handling in Hazzure.
license: MIT
---

# Windows API & DLL Lifecycle (Hazzure)

Use when working on DLL injection, hook installation/removal, thread lifecycles, and Windows kernel/usermode interactions.

## Lifecycle Rules
1. **DllMain Discipline**:
   - `DLL_PROCESS_ATTACH`: Perform minimal work only. Spawn a dedicated worker thread (`CreateThread`) and return `TRUE`. Never acquire loader locks or execute complex initialization inside `DllMain`.
   - `DLL_PROCESS_DETACH`: Execute clean teardown if unhooking is initiated, releasing hooks in reverse order of attachment.
2. **Handle & Thread Hygiene**:
   - Every opened handle (`OpenProcess`, `CreateFile`, `CreateEvent`) must have a deterministic `CloseHandle`.
   - Threads spawned by the DLL must support graceful cancellation/exit signals without hanging host processes.
3. **Hook Safety (MinHook / Kiero)**:
   - Ensure target function signatures and calling conventions match the engine (`__fastcall`, `__vectorcall`, `__stdcall`).
   - Unified hook RVAs: never install two conflicting hooks on the same entry point.
   - Trampoline safety: ensure hook targets have adequate function preamble padding to avoid tearing instructions.
4. **Structured Exception Handling (SEH)**:
   - Enclose volatile memory reads across game memory in `__try / __except` blocks when validating unknown pointers or dynamic offsets.
