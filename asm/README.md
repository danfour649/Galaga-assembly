# Assembly modules (`asm/`)

NASM x86-64 sources for gameplay logic. Built automatically by CMake when NASM is available.

| File | Status | Purpose |
|------|--------|---------|
| `collision.asm` | **Implemented** | AABB overlap (`rect_overlap`) |
| `entities.asm` | **Implemented** | Entity pools: spawn, kill, tick, demo formation |
| `enemies.asm` | Planned | Formation, dive AI, tractor beam |
| `score.asm` | Planned | Points and extra lives |

Style conventions: see [docs/ASM_STYLE.md](../docs/ASM_STYLE.md).
