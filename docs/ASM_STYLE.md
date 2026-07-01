# Assembly Style Guide

Conventions for `asm/*.asm` files in this project. Derived from [PlummersSoftwareLLC/HelloAssembly](https://github.com/PlummersSoftwareLLC/HelloAssembly) and [TinyRetroPad](https://github.com/PlummersSoftwareLLC/TinyRetroPad), adapted for **NASM x86-64** and a larger game (readability over byte-count).

## Principles

1. **Readable first** — HelloAssembly says: *"Please keep it readable and explain what you're doing in the comments!"*
2. **Transparent control flow** — Avoid macros that hide logic. No `.if` / `invoke` style abstractions.
3. **Assembly owns gameplay** — Collision, AI, scoring, and entity updates live here; not in C.
4. **Document the ABI** — Every exported function documents arguments, return value, and clobbered registers.

## File header template

```nasm
;------------------------------------------------------------------------------
; collision.asm
;
; Galaga-assembly — AABB collision helpers (x86-64, NASM)
;
; Calling convention: System V AMD64 (Linux, macOS)
;   Windows builds use separate win64 exports (see CMakeLists.txt)
;
; History:
;   2026-07-01  Initial AABB overlap test
;------------------------------------------------------------------------------
```

## Naming

| Element | Convention | Example |
|---------|------------|---------|
| Exported symbols | `snake_case` | `rect_overlap`, `entity_tick_all` |
| Internal labels | `.local_name` | `.no_overlap`, `.next_entity` |
| Constants | `UPPER_SNAKE` | `MAX_BULLETS`, `SCORE_BEE_DIVE` |
| Struct field offsets | `TYPE_FIELD` | `ENTITY_X`, `ENTITY_FLAGS` |

Match C struct layout exactly — offsets are shared via `include/galaga.inc` (generated or hand-maintained) or documented in both files.

## Comments

- **Section banners** with `;---` between major routines.
- **Inline**: explain *why*, especially branch decisions:
  ```nasm
  cmp eax, ebx
  jge .no_overlap          ; a.left >= b.right → no hit
  ```
- **Register usage block** at the top of each function when non-obvious.

## Formatting

- One instruction per line.
- Align instructions in columns where practical (NASM doesn't enforce this; consistency matters).
- Labels on their own line, flush left.

## Exported functions

- Mark with `global symbol_name` (Linux/macOS) or `global symbol_name` under `section .text` for Windows.
- Preserve callee-saved registers (`rbx`, `rbp`, `r12`–`r15`) if used.
- Return integers in `rax`, pointers in `rax`.

Example:

```nasm
; int rect_overlap(const Rect* a, const Rect* b)
;   rdi = a, rsi = b
;   returns 1 if overlapping, 0 otherwise
global rect_overlap
rect_overlap:
    mov eax, [rdi + RECT_X]
    ; ...
    ret
```

## What not to do

- Do not call SDL or libc from assembly (keep platform code in C).
- Do not use MASM-only syntax — this project uses **NASM** for cross-platform builds.
- Do not optimize for executable size (unlike HelloAssembly's 383-byte demo). Optimize for clarity until profiling says otherwise.

## C ↔ assembly boundary

- C declares functions in `include/galaga.h`.
- Assembly implements them in `asm/*.asm`.
- `src/game_fallback.c` provides C equivalents when `GALAGA_USE_ASM` is off.
- Never duplicate gameplay logic in both C and asm — fallback files must mirror asm behavior exactly (test both paths).

## Related repos

| Repo | Reuse |
|------|-------|
| [HelloAssembly](https://github.com/PlummersSoftwareLLC/HelloAssembly) | Comment style, section layout, history headers |
| [TinyRetroPad](https://github.com/PlummersSoftwareLLC/TinyRetroPad) | Feature flags, growth documentation |
| [PETClock](https://github.com/PlummersSoftwareLLC/PETClock) | 6502 reference only — different CPU, not used here |
