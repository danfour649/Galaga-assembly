# Next Steps

Working plan for the SDL + assembly Galaga clone. Tasks are ordered so each phase produces a runnable build.

**Assembly confirmation:** Every gameplay-critical module below lists its planned `.asm` file. C code is limited to SDL glue and fallbacks for non-x86 platforms.

---

## Phase 0 — Project scaffold ✅ (current)

- [x] SDL2 window and game loop
- [x] CMake cross-platform build
- [x] NASM `collision.asm` linked and called from C
- [x] C fallback for ARM64 / builds without NASM
- [x] Documentation and repo layout

**Verify:**
```bash
cmake -B build && cmake --build build
./build/galaga
```

You should see a window with a player ship (cyan), enemies (colored rects), a starfield, and HUD text. Arrow keys move; Space fires; collisions are detected via assembly.

---

## Phase 1 — Playable prototype

Goal: one screen, shoot enemies, score, lives, game over.

### 1.1 Entity system
- [ ] Define fixed-size entity pools in `include/galaga.h` (player, enemies, bullets)
- [ ] Implement `asm/entities.asm`:
  - `entity_spawn(type, x, y)`
  - `entity_kill(index)`
  - `entity_tick_all(delta)`
- [ ] C glue in `game.c` only marshals structs; no gameplay logic in C

### 1.2 Player
- [ ] Clamp movement to playfield bounds (asm: `player_clamp_x`)
- [ ] Fire rate limit (max 2 bullets on screen — classic Galaga)
- [ ] Lives counter (start: 3)

### 1.3 Enemies (single type first)
- [ ] One enemy type (Bee) in a static 5×8 formation grid
- [ ] `asm/enemies.asm`: formation positions, idle wobble
- [ ] Random dive attack: enemy leaves formation, follows simple path toward player
- [ ] Enemy fires downward while diving

### 1.4 Collision (extend existing)
- [ ] `asm/collision.asm`: bullet↔enemy, bullet↔player, enemy body↔player
- [ ] Return hit indices so C/render can play effects (no logic in render)

### 1.5 Score & game state
- [ ] `asm/score.asm`: add points, extra life at 20k/70k/150k
- [ ] State machine: `TITLE → PLAYING → STAGE_CLEAR → GAME_OVER`

### 1.6 Rendering upgrade
- [ ] Replace colored rects with placeholder 16×16 bitmap sprites (`assets/`)
- [ ] Simple explosion animation (2–3 frames)

**Done when:** Clear all enemies → next stage; die three times → game over screen.

---

## Phase 2 — Classic Galaga feel

### 2.1 All enemy types
| Type | Rows | Hits | Asm behavior |
|------|------|------|--------------|
| Bee | 2 bottom | 1 | Basic dive |
| Butterfly | 2 middle | 1 | Faster dive, escort Boss |
| Boss Galaga | 1 top (×4) | 2 | Color change on first hit |

### 2.2 Entry animation
- [ ] Enemies fly in from off-screen into formation slots (`asm/enemies.asm`)

### 2.3 Accurate scoring
| Target | Points |
|--------|--------|
| Bee in formation | 50 |
| Bee diving | 100 |
| Butterfly in formation | 80 |
| Butterfly diving | 160 |
| Boss in formation | 150 |
| Boss diving alone | 400 |
| Boss + 1 escort | 800 |
| Boss + 2 escorts | 1600 |

### 2.4 Stage progression
- [ ] Stage counter and difficulty ramp (faster dives, more enemy shots)
- [ ] Stage clear fanfare (SDL_mixer — optional)

---

## Phase 3 — Signature Galaga mechanics

### 3.1 Tractor beam
- [ ] Boss stops mid-screen, emits beam (`asm/enemies.asm` state `TRACTOR_BEAM`)
- [ ] Player caught → ship captured, carried to formation, lose life

### 3.2 Dual Fighter
- [ ] If lives remain, shoot diving Boss holding captured ship → rescue
- [ ] Dual ship: double firepower, wider hitbox (`asm/player.asm`)

### 3.3 Challenging stages
- [ ] Stage 3, then every 4th stage (7, 11, 15…)
- [ ] Enemies fly preset paths, do not shoot
- [ ] Bonus points for destroying groups (`asm/challenge.asm`)

---

## Phase 4 — Polish

- [ ] Authentic arcade resolution (224×288) with integer scale (×2 or ×3)
- [ ] Starfield parallax (`asm/render_helpers.asm` or C — TBD)
- [ ] Sound effects via SDL_mixer
- [ ] Title screen and attract mode
- [ ] High score persistence (file or env-based for cloud CI)

---

## Platform checklist

| Task | Windows | macOS (Intel) | macOS (ARM) | Linux |
|------|---------|---------------|-------------|-------|
| SDL2 install | vcpkg / MSYS2 | `brew install sdl2` | same | `apt install libsdl2-dev` |
| NASM install | nasm.us | `brew install nasm` | same | `apt install nasm` |
| Build | CMake + MSVC or MinGW | CMake + clang | C fallback (no asm yet) | CMake + gcc |
| Run | `galaga.exe` | `./galaga` | `./galaga` | `./galaga` |

---

## Immediate next PR (recommended)

1. Merge this scaffold PR.
2. Open **Phase 1.1** PR: entity pools in `asm/entities.asm`.
3. Keep each PR runnable — no long-lived broken branches.

---

## References

- [HelloAssembly](https://github.com/PlummersSoftwareLLC/HelloAssembly) — x86 style and commenting conventions
- [Galaga gameplay (StrategyWiki)](https://strategywiki.org/wiki/Galaga/Gameplay) — scoring and enemy behavior
- [SDL2 documentation](https://wiki.libsdl.org/SDL2/FrontPage)
