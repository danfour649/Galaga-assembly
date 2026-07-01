# Galaga-assembly

A cross-platform **Galaga** clone for Windows and macOS, built with **SDL2** and **x86-64 NASM assembly**.

## Assembly is still central

This is not a pure C/SDL game. The architecture deliberately keeps gameplay logic in assembly:

- **C + SDL** — window, input, rendering, main loop
- **NASM (x86-64)** — collision, and (planned) entity updates, enemy AI, scoring

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the full split. On Apple Silicon (ARM64), C fallbacks are used until `asm/aarch64/` modules exist.

## Quick start

### Prerequisites

| Platform | Install |
|----------|---------|
| **Linux** | `sudo apt install build-essential cmake libsdl2-dev nasm` |
| **macOS** | `brew install cmake sdl2 nasm` |
| **Windows** | [CMake](https://cmake.org/), [SDL2 via vcpkg](https://vcpkg.io/), [NASM](https://www.nasm.us/) |

### Build & run

```bash
cmake -B build
cmake --build build
./build/galaga          # Linux / macOS
# build\galaga.exe      # Windows
```

### CI builds

Every push/PR to `main` runs [GitHub Actions](.github/workflows/build.yml) on **Linux, macOS, and Windows**. Built executables are uploaded as workflow artifacts (`galaga-linux-x86_64`, `galaga-macos`, `galaga-windows-x86_64`).

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

## Project layout

```
include/galaga.h     Shared types and APIs
src/                 C/SDL modules (main, input, render, game glue)
asm/                 NASM gameplay modules
assets/              Sprites and audio (future)
docs/                Architecture and next steps
```

## Style references

Assembly style follows conventions from [PlummersSoftwareLLC/HelloAssembly](https://github.com/PlummersSoftwareLLC/HelloAssembly) and related repos — readable comments, transparent control flow, gameplay logic in asm.

## License

TBD
