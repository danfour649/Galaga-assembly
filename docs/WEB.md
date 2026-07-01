# Web build (Emscripten / WebAssembly)

The browser build uses **Emscripten** to compile the C/SDL shell and **`game_fallback.c`** gameplay implementations to WebAssembly. NASM x86-64 modules are not linked on the web target (same strategy as Apple Silicon).

## Prerequisites

Install the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) and activate it in your shell:

```bash
source /path/to/emsdk/emsdk_env.sh
```

## Build

From the repository root:

```bash
emcmake cmake -B build-web -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-web
```

Output files:

| File | Purpose |
|------|---------|
| `build-web/galaga.html` | Page shell (`web/shell.html` + loader) |
| `build-web/galaga.js` | Emscripten runtime and SDL glue |
| `build-web/galaga.wasm` | Game binary (C fallbacks + SDL) |

## Run locally

Serve the build directory over HTTP (required for WASM MIME type):

```bash
python3 -m http.server 8080 --directory build-web
```

Open `http://localhost:8080/galaga.html`, click the canvas to focus, then play.

## Controls

| Key | Action |
|-----|--------|
| Left / A | Move left |
| Right / D | Move right |
| Space | Fire / start / continue |

Escape does not quit the browser build (unlike desktop).

## High scores

Desktop builds persist to `galaga_hi.txt`. The web build uses `localStorage` under the key `galaga_hi`.

## CI

Every push/PR compiles the WASM target in GitHub Actions (`wasm` job) and uploads `galaga-wasm` artifacts. A separate **Linux C fallback** job builds with `-DGALAGA_USE_ASM=OFF` to catch parity drift before WASM.

## Future assets

When bitmap/audio assets land in `assets/`, add Emscripten preload to `CMakeLists.txt`:

```cmake
target_link_options(galaga PRIVATE --preload-file "${CMAKE_SOURCE_DIR}/assets@/assets")
```

Load files from `/assets/...` in C only (not assembly).
