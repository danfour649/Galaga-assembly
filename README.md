# Galaga-assembly

[![Build](https://github.com/danfour649/Galaga-assembly/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/danfour649/Galaga-assembly/actions/workflows/build.yml)

A cross-platform **Galaga** clone for Windows and macOS, built with **SDL2** and **x86-64 NASM assembly**.

## Download

Prebuilt binaries for Linux, macOS, and Windows are published from every successful build of `main`:

**[Download the latest build](https://github.com/danfour649/Galaga-assembly/releases/tag/latest)**

The Windows download is a `.zip` containing `galaga.exe` and its required `SDL2.dll` — unzip and run, no separate SDL2 install needed.

### Play in browser

Every successful build of `main` deploys the WebAssembly version to GitHub Pages:

**[Play Galaga-assembly online](https://danfour649.github.io/Galaga-assembly/galaga.html)**

Click the game canvas to focus, then press Space to start. No download required.

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
| **Windows** | [MSYS2](https://www.msys2.org/) (`winget install MSYS2.MSYS2`), then from any shell: `C:\msys64\usr\bin\pacman -S --needed mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-nasm` |
| **Web** | [Emscripten SDK](https://emscripten.org/) — see [docs/WEB.md](docs/WEB.md) |

### Build & run

```bash
cmake -B build
cmake --build build
./build/galaga          # Linux / macOS
```

On Windows, use the MSYS2 UCRT64 toolchain (same as CI). This is the CI recipe and is verified to build with the NASM assembly modules enabled:

```bash
# Git Bash / MSYS2 shell
export PATH="/c/msys64/ucrt64/bin:$PATH"
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/galaga.exe      # needs /c/msys64/ucrt64/bin on PATH for SDL2.dll,
                        # or copy C:\msys64\ucrt64\bin\SDL2.dll next to the exe
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

Every push/PR to `main` runs [GitHub Actions](.github/workflows/build.yml) on **Linux, macOS, Windows, Linux C fallback, and WebAssembly**. Native executables are uploaded as workflow artifacts and, on a clean build of `main`, published to the [latest release](https://github.com/danfour649/Galaga-assembly/releases/tag/latest). The WASM build is available as the `galaga-wasm` artifact (`galaga.html`, `.js`, `.wasm`) and is deployed to [GitHub Pages](https://danfour649.github.io/Galaga-assembly/galaga.html) after each successful `main` build.

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
