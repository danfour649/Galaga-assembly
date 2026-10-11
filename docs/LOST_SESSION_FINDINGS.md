# Lost Session Findings

Source: External paste from lost development session (2026-10-11 context)

These findings document critical bugs that existed in the native Linux NASM build versus browser/C fallback, along with fixes and testing improvements that were developed but need to be re-applied or verified as to-dos.

## What was wrong

- **Spawn bug on System V (Linux and Intel macOS)**: `entity_spawn` kept `x` in `ecx` and the bullet type in `edx`, then reused both as loop counters. Ships and shots spawned at `x = slot index`. The first player shot was stored as an enemy bullet, so it fell and the 2-shot cap never applied. Windows was fine, because those arguments are not in `ecx`/`edx`.
- **Player could not die on the NASM path**: Overlap treated every box like an Enemy (x at offset 4). `Player.x` is at offset 0, so bullet-vs-ship and body-vs-ship tested a phantom rect off the right edge. The C fallback (WASM and Apple Silicon) was correct.
- **High scores never hit disk**: `game.c` copied the new score into `high_score` and then called `high_score_save`, which returns immediately when `score <= high_score`.
- **Movement was refresh-rate locked**: `(int)(100 * dt)` is 0 above about 100 Hz, so the ship froze on a high-refresh display. The stage-clear timer ran at the monitor's rate.

## What changed (fixes implemented in lost session)

- Fixed-step simulation at 60 Hz (player 1 px, world 2 px — the old 60 Hz truncations). At most four steps per rendered frame.
- Starfield is four `SDL_RenderDrawPoints` batches instead of 56 fill-rects. The bitmap font is one atlas. The renderer draws in a 224×288 logical size with integer scale and nearest filtering.
- `include/galaga.h` `_Static_assert`s every offset in `include/galaga.inc`.
- Win64 `entity_spawn` calls are 16-byte aligned. A failed spawn no longer writes through index -1. A dive with `dx == 0` now aims +x on both backends.

## Testing improvements

- Before this, CI compiled five targets and ran nothing. macOS CI is ARM and WASM uses the C fallback, so the Linux NASM binary — the one with the spawn and hit bugs — was never executed.
- `tests/sim_test.c` has no SDL and no framework. It checks overlap, spawn coordinates, the fire cap, bullet speed, clamp, scores, boss HP, player hits, formation entry, the frame-54 dive, stage clear, and `galaga_hi.txt`. `ctest` runs that file on every native job, once with NASM and once with `-DGALAGA_USE_ASM=OFF`.
- Left out on purpose: pixel goldens, a frame-time benchmark (40×8 is not the cost), and a Node WASM smoke test (the Linux fallback job already runs that C file). The next harness worth adding is a scripted input trace hashed at the end of `GameState`, when the tractor beam lands. ASan/UBSan is worth a local run on the fallback only. It cannot see the NASM objects.
- Not changed: both backends always launch two divers, while `docs/NEXT_STEPS.md` says the second diver starts at stage 2. Stage clear is 91 sim ticks, because tick 90 reaches 0 and the next tick notices.
- `render.c` was not compiled here (no SDL2 headers). Win64 objects assembled with `nasm -w+error` but were not executed.

## Action items / To-dos

- [ ] Verify/re-apply spawn fix in `asm/entities.asm` (register handling for System V ABI)
- [ ] Verify/re-apply collision/overlap fix in `asm/collision.asm` (correct offsets for Player vs Enemy)
- [ ] Fix high score persistence logic in `game.c` / `high_score_save`
- [ ] Implement fixed-step 60 Hz simulation (decouple from refresh rate)
- [ ] Add `_Static_assert`s for struct offsets in `include/galaga.h`
- [ ] Ensure Win64 calling convention alignment for `entity_spawn`
- [ ] Integrate `tests/sim_test.c` into CI for both NASM and C fallback paths
- [ ] Update `docs/NEXT_STEPS.md` to reflect actual diver behavior and stage clear timing
- [ ] Consider adding scripted input trace test for tractor beam (future)

These ensure the native x86-64 Linux build matches the intended behavior and that regressions are caught early.
