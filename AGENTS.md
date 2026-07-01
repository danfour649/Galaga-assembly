# Galaga-assembly

Cross-platform Galaga clone: **SDL2** (C) for portability, **NASM x86-64** for gameplay logic. See `README.md` and `docs/`.

## Cursor Cloud specific instructions

### Repository state
- **SDL2 + CMake** build system is in place (`CMakeLists.txt`).
- Playable game: sprites, three enemy types, dive AI, explosions, scoring, title/stage-clear/game-over flow.
- **WebAssembly** build via Emscripten (`docs/WEB.md`, `web/shell.html`).
- Phased plan: `docs/NEXT_STEPS.md`.

### Toolchain (provisioned in the VM)
| Tool | Purpose |
|------|---------|
| `cmake`, `gcc`/`clang` | C build |
| `nasm` | x86-64 assembly modules (`asm/`) |
| `libsdl2-dev` | SDL2 headers and linking |
| `gdb` | Debugging |

### Build & run

```bash
cmake -B build && cmake --build build
./build/galaga
```

Headless cloud VMs have no display. To verify a graphical build compiles, the compile step is sufficient. For local/manual testing, run `./build/galaga` on a machine with a display.

**Web (compile-only in CI):**
```bash
emcmake cmake -B build-web -G Ninja && cmake --build build-web
```

### Assembly vs C

- **`asm/*.asm`** — gameplay logic on x86-64 native builds
- `src/game_fallback.c` — C fallback when `GALAGA_USE_ASM=0` (ARM64, Emscripten, or `-DGALAGA_USE_ASM=OFF`)
- Future gameplay modules go in `asm/` per `docs/NEXT_STEPS.md`; mirror each in `game_fallback.c` in the same PR
- Do not put game logic in `render.c`

### Platform notes

| Platform | SDL2 | NASM | Notes |
|----------|------|------|-------|
| Linux x86-64 | `libsdl2-dev` | `nasm` | Full asm path |
| macOS Intel | `brew install sdl2 nasm` | yes | Full asm path |
| macOS ARM | `brew install sdl2` | optional | C fallback until aarch64 asm |
| Windows | vcpkg SDL2 | nasm.us | CMake + MSVC or MinGW |
| Web (WASM) | Emscripten port | n/a | C fallback; see `docs/WEB.md` |

### Style

Follow `docs/ASM_STYLE.md` (HelloAssembly-inspired conventions, adapted for NASM).
