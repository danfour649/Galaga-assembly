# Galaga-assembly

A Galaga clone written in assembly language. See `README.md`.

## Cursor Cloud specific instructions

### Repository state
- This repository is currently a skeleton: only `README.md` exists and there is **no source code, build system, or dependency manifest yet**. There is no application to run until assembly source files are added.
- The intended target ISA/assembler is not yet pinned down by any committed file. The environment below is provisioned for the most common assembly game workflows (x86).

### Toolchain (provisioned in the VM)
The following are installed and available on `PATH`:
- `nasm` — Netwide Assembler (x86 / x86-64), the standard assembler for this kind of project.
- `as`, `ld` (GNU binutils) and `gcc`/`clang` — for GAS-syntax assembly and linking.
- `gdb` — debugger.
- `qemu-system-i386` / `qemu-system-x86_64` — to run bare-metal boot-sector games or full OS images.
- `make` — build driver.

### How to build & run (once source is added)
Exact commands depend on the assembler/target the project adopts. Common patterns:

- Linux userland (NASM, ELF64):
  - `nasm -f elf64 file.asm -o file.o`
  - `ld file.o -o file`  (then `./file`)
- Bare-metal boot sector (NASM, flat binary) run in QEMU:
  - `nasm -f bin boot.asm -o boot.bin`
  - `qemu-system-i386 -drive format=raw,file=boot.bin`

### Headless / cloud QEMU notes (non-obvious)
- The VM has no attached display, so run QEMU headless and capture a screenshot via the monitor instead of relying on a live window:
  - `(sleep 2; echo "screendump out.ppm"; sleep 1; echo "quit") | qemu-system-i386 -drive format=raw,file=boot.bin -display none -monitor stdio -vga std`
  - Convert the resulting `.ppm` to PNG with `ffmpeg -i out.ppm out.png` (ImageMagick `convert` is not installed by default; `ffmpeg` is available).
- `-nographic` disables the VGA device and will break `screendump`; use `-display none` (keeps the VGA device) instead.
