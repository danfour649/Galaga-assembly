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

### Apple Silicon note

Apple Silicon Macs are ARM64. Until `asm/aarch64/` modules exist, the build uses **C fallback implementations** of the same functions (see `src/game_fallback.c`). Game logic APIs are identical; only the implementation backend changes.

## Directory layout

```
Galaga-assembly/
├── CMakeLists.txt
├── docs/
│   ├── ARCHITECTURE.md      ← this file
│   ├── NEXT_STEPS.md        ← phased implementation plan
│   └── ASM_STYLE.md         ← coding conventions (HelloAssembly-inspired)
├── include/
│   └── galaga.h             ← shared types and public API
├── src/
│   ├── main.c               ← entry point, SDL lifecycle
│   ├── input.c
│   ├── render.c
│   ├── game.c               ← game state; dispatches to asm or fallback
│   └── game_fallback.c      ← C implementations when asm unavailable
├── asm/
│   ├── collision.asm        ← AABB collision (implemented)
│   ├── entities.asm         ← (stub) entity pool
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

## Data flow (one frame)

1. `main.c` polls SDL events via `input.c`.
2. `game_tick()` advances simulation (calls asm for collision / entity updates).
3. `render_frame()` draws the current state with SDL.
4. Loop until quit or game over.
