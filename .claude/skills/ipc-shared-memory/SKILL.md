---
name: ipc-shared-memory
description: >
  Audits and verifies IPC (Inter-Process Communication), shared memory structs,
  memory layouts, synchronization, alignment, and race conditions between C++ and C#/tools.
license: MIT
---

# IPC & Shared Memory (Hazzure)

Use when editing or reviewing `Compartilhado/ipc.h`, named pipes, shared memory mapped files, or telemetry channels.

## Protocols
1. **Binary Layout & Alignment**:
   - Shared structs must maintain explicit packing (`#pragma pack(push, 1)` or explicit 4/8-byte alignments) across all consuming compilers.
   - Use fixed-width primitive types (`uint32_t`, `int32_t`, `float`, `bool`) instead of architecture-variable types (`long`, `size_t`, `enum` without underlying type).
2. **Synchronization & Concurrency**:
   - Prevent torn reads/writes across processes using memory barriers (`MemoryBarrier()`), atomic sequence numbers, or dirty flags.
   - Read from shared buffers into local stack copies before processing to avoid race conditions during continuous frame updates.
3. **Backward Compatibility**:
   - Append new fields strictly at the end of existing structures to prevent offset shifts for older consumers.
   - Increment layout version headers when introducing breaking changes.
