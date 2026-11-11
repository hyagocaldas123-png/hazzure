---
name: cpp17-code-reviewer
description: >
  Analyzes C++17 code for bugs, memory leaks, undefined behavior, ownership issues,
  and unnecessary complexity in Hazzure. Use when reviewing, diagnosing, or refactoring C++ code.
license: MIT
---

# C++17 Code Reviewer (Hazzure)

Use when reviewing, writing, or diagnosing C++17 code within `HazzureInternal/` and related tools.

## Review Priorities
1. **Memory Safety & Pointer Lifetime**:
   - Verify null checks before pointer dereferencing on game structures (`client.dll` base, entities, controllers).
   - Check raw pointer ownership: ensure buffers, strings, and handles do not leak on early returns or exceptions.
   - Guard entity memory reads with appropriate safety checks to avoid `0xC0000005` Access Violations.
2. **Undefined Behavior (UB)**:
   - Identify strict aliasing violations, unaligned memory access, and out-of-bounds array access in fixed-size buffers.
   - Ensure bitwise manipulation on bitmasks (e.g. subtick protobuf has-bits) respects intended field types and sizes.
3. **Simplicity & Efficiency**:
   - Flag unnecessary loops, heavy per-frame memory allocations (`new`/`malloc` inside Present or CreateMove hooks).
   - Prefer standard algorithms or direct pointer arithmetic where already established.
   - Keep functions focused: avoid monolithic blocks when modularization is cleaner, but keep unity compilation constraints intact.
