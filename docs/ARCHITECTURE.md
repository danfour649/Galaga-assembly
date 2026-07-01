# Architecture

Galaga-assembly is a cross-platform Galaga clone that uses **SDL2 for portability** and **x86-64 NASM assembly for game logic**.

## Yes — assembly is still core to this project

Assembly is not being dropped. The split is intentional:

| Layer | Language | Responsibility |
|-------|----------|----------------|
| Platform shell | **C** | SDL window, input, audio, timing, asset loading |
| Game engine | **C** (thin glue) | Struct definitions, calling conventions, SDL draw helpers |
| Hot-path logic | **NASM (x86-64)** | Collision, entity updates, scoring, enemy AI state machine |

C exists because SDL is a C library and cross-platform windowing in raw assembly would duplicate enormous platform-specific code for little gain. Assembly is reserved for the parts that matter for this repo's goals: readable, maintainable game logic written close to the metal.

## High-level diagram

```
┌─────────────────────────────────────────────────────────┐
│  main.c          SDL init, main loop, quit handling     │
├─────────────────────────────────────────────────────────┤
│  input.c         Keyboard → player intent               │
│  render.c        Sprites, HUD, starfield (SDL_Renderer) │
│  game.c          Game state, calls into asm/*.asm       │
├─────────────────────────────────────────────────────────┤
│  asm/collision.asm     AABB overlap + collision_resolve       │
│  asm/entities.asm      Entity pool management                 │
│  asm/player.asm        Movement, clamp, fire, lives           │
│  asm/enemies.asm       Formation, dive AI, enemy fire         │
│  asm/score.asm         Points and extra lives                 │
│  asm/game_state.asm    Title / playing / stage clear / over   │
└─────────────────────────────────────────────────────────┘
```

## Why SDL2?

- Runs natively on **Windows** and **macOS** (and Linux).
- Handles window creation, vsync, keyboard, and 2D rendering.
- Well-supported by CMake and package managers (`brew`, `vcpkg`, `apt`).

## Why NASM + x86-64?

- Matches the [HelloAssembly](https://github.com/PlummersSoftwareLLC/HelloAssembly) / Dave's Garage x86 tradition referenced in this project.
- NASM is available on Windows, macOS, and Linux.
- CMake `enable_language(ASM_NASM)` selects the correct object format per platform (`elf64`, `macho64`, `win64`).

### Non-x86 backends (ARM64, WebAssembly)

Apple Silicon Macs are ARM64. The browser build targets **wasm32** via Emscripten. Neither can link NASM x86-64 objects. Until `asm/aarch64/` modules exist (ARM) or a deliberate WAT port (web), both use **C fallback implementations** of the same functions (see `src/game_fallback.c`). Game logic APIs are identical; only the implementation backend changes.

| Target | Toolchain | Gameplay backend |
|--------|-----------|------------------|
| Linux/Windows x86-64 | CMake + NASM | `asm/*.asm` |
| macOS ARM64 | CMake (no asm link) | `game_fallback.c` |
| Web (WASM) | Emscripten + `-sUSE_SDL=2` | `game_fallback.c` |

CMake forces `GALAGA_USE_ASM=OFF` when `CMAKE_SYSTEM_NAME` is `Emscripten`. You can also pass `-DGALAGA_USE_ASM=OFF` on native builds to exercise the fallback path (CI does this on Linux).

## Directory layout

```
Galaga-assembly/
├── CMakeLists.txt
├── docs/
│   ├── ARCHITECTURE.md      ← this file
│   ├── NEXT_STEPS.md        ← phased implementation plan
│   ├── WEB.md               ← Emscripten build
│   └── ASM_STYLE.md         ← coding conventions (HelloAssembly-inspired)
├── include/
│   └── galaga.h             ← shared types and public API
├── src/
│   ├── main.c               ← entry point, SDL lifecycle
│   ├── input.c
│   ├── render.c
│   ├── sprites.c            ← procedural placeholder textures
│   ├── effects.c            ← explosion VFX (C)
│   ├── highscore.c          ← file or localStorage persistence
│   ├── game.c               ← thin glue; dispatches to asm or fallback
│   └── game_fallback.c      ← C implementations when asm unavailable
├── web/
│   └── shell.html           ← Emscripten page template
├── asm/
│   ├── collision.asm
│   ├── entities.asm
│   ├── player.asm
│   ├── enemies.asm
│   ├── score.asm
│   ├── game_state.asm
│   └── README.md
└── assets/                  ← sprites, sounds (future)
```

## Build targets

| Command | Result |
|---------|--------|
| `cmake -B build && cmake --build build` | Debug build |
| `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build` | Release build |
| `./build/galaga` | Run (Linux/macOS) |
| `build\Release\galaga.exe` | Run (Windows, MSVC generator) |
| `emcmake cmake -B build-web && cmake --build build-web` | WebAssembly + HTML (see `docs/WEB.md`) |

## Data flow (one frame)

1. `main.c` polls SDL events via `input.c`.
2. `game_tick()` advances simulation (calls asm for collision / entity updates).
3. `render_frame()` draws the current state with SDL.
4. Loop until quit or game over.
