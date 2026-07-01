# Galaga-assembly

Cross-platform Galaga clone: **SDL2** (C) for portability, **NASM x86-64** for gameplay logic. See `README.md` and `docs/`.

## Cursor Cloud specific instructions

### Repository state
- **SDL2 + CMake** build system is in place (`CMakeLists.txt`).
- Minimal playable scaffold: window, player movement, shooting, demo enemy formation, collision via `asm/collision.asm`.
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

### Assembly vs C

- **`asm/collision.asm`** — `rect_overlap()` (linked on x86-64 when NASM is present)
- **`asm/player.asm`** — `player_init`, `player_tick`, `player_clamp_x`, `player_try_fire`, `player_lose_life`
- `src/game_fallback.c` — C fallback when `GALAGA_USE_ASM=0` (ARM64, or no NASM)
- Future gameplay modules go in `asm/` per `docs/NEXT_STEPS.md`; do not put game logic in `render.c`.

### Platform notes

| Platform | SDL2 | NASM | Notes |
|----------|------|------|-------|
| Linux x86-64 | `libsdl2-dev` | `nasm` | Full asm path |
| macOS Intel | `brew install sdl2 nasm` | yes | Full asm path |
| macOS ARM | `brew install sdl2` | optional | C fallback until aarch64 asm |
| Windows | vcpkg SDL2 | nasm.us | CMake + MSVC or MinGW |

### Style

Follow `docs/ASM_STYLE.md` (HelloAssembly-inspired conventions, adapted for NASM).
