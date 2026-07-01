# Galaga-assembly

[![Build](https://github.com/danfour649/Galaga-assembly/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/danfour649/Galaga-assembly/actions/workflows/build.yml)

A cross-platform **Galaga** clone for Windows and macOS, built with **SDL2** and **x86-64 NASM assembly**.

## Download

Prebuilt binaries for Linux, macOS, and Windows are published from every successful build of `main`:

**[Download the latest build](https://github.com/danfour649/Galaga-assembly/releases/tag/latest)**

## Assembly is still central

This is not a pure C/SDL game. The architecture deliberately keeps gameplay logic in assembly:

- **C + SDL** — window, input, rendering, main loop
- **NASM (x86-64)** — collision, entities, player, enemies, scoring, game state

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the full split. On Apple Silicon (ARM64) and **WebAssembly**, C fallbacks in `src/game_fallback.c` are used instead of NASM.

## Quick start

### Prerequisites

| Platform | Install |
|----------|---------|
| **Linux** | `sudo apt install build-essential cmake libsdl2-dev nasm` |
| **macOS** | `brew install cmake sdl2 nasm` |
| **Windows** | [CMake](https://cmake.org/), [SDL2 via vcpkg](https://vcpkg.io/), [NASM](https://www.nasm.us/) |
| **Web** | [Emscripten SDK](https://emscripten.org/) — see [docs/WEB.md](docs/WEB.md) |

### Build & run

```bash
cmake -B build
cmake --build build
./build/galaga          # Linux / macOS
# build\galaga.exe      # Windows
```

### Web build

```bash
emcmake cmake -B build-web -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-web
python3 -m http.server 8080 --directory build-web
# open http://localhost:8080/galaga.html
```

Full details: [docs/WEB.md](docs/WEB.md).

### CI builds

Every push/PR to `main` runs [GitHub Actions](.github/workflows/build.yml) on **Linux, macOS, Windows, Linux C fallback, and WebAssembly**. Native executables are uploaded as workflow artifacts and, on a clean build of `main`, published to the [latest release](https://github.com/danfour649/Galaga-assembly/releases/tag/latest). The WASM build is available as the `galaga-wasm` artifact (`galaga.html`, `.js`, `.wasm`).

### Controls

| Key | Action |
|-----|--------|
| Left / A | Move left |
| Right / D | Move right |
| Space | Fire |
| Escape | Quit |

## Documentation

| Doc | Contents |
|-----|----------|
| [docs/NEXT_STEPS.md](docs/NEXT_STEPS.md) | Phased implementation plan |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | SDL + assembly split |
| [docs/ASM_STYLE.md](docs/ASM_STYLE.md) | NASM conventions (HelloAssembly-inspired) |
| [docs/WEB.md](docs/WEB.md) | Emscripten / WebAssembly build |

## Project layout

```
include/galaga.h     Shared types and APIs
src/                 C/SDL modules (main, input, render, game glue)
asm/                 NASM gameplay modules
web/                 Emscripten HTML shell
assets/              Sprites and audio (future)
docs/                Architecture and next steps
```

## Style references

Assembly style follows conventions from [PlummersSoftwareLLC/HelloAssembly](https://github.com/PlummersSoftwareLLC/HelloAssembly) and related repos — readable comments, transparent control flow, gameplay logic in asm.

## License

TBD
