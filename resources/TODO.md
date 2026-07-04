# TODO

Remaining polish after the 2026-07-03 visuals/difficulty pass
(pixel-art sprites, bitmap-font HUD, faster bullets, more aggressive dives).

## Visuals
- [ ] Player death explosion + brief respawn invulnerability (currently the
      ship never visibly dies; a life just disappears from the HUD).
- [ ] Enemy entry flight paths — arcade Galaga flies loops/S-curves into
      formation; ours drops straight down 24px (`ENTER_DROP`).
- [ ] Diving enemies should curve (sine/arc x-motion) instead of constant `vx`.
- [ ] Stage badge icons bottom-right (arcade uses flag icons, we print text).
- [ ] Two-tone starfield twinkle + occasional colored stars like the arcade.

## Gameplay
- [ ] Boss Galaga tractor beam / captured-ship + dual fighter mechanic.
- [ ] Challenging stages (bonus waves every 4th stage, no enemy fire).
- [ ] Diving enemies should aim bullets at the player's x-position (currently
      bullets fall straight down from the diver).
- [ ] Score popups (e.g. "400") where a diving boss is destroyed.

## Tech
- [ ] Difficulty tuning pass on real hardware after playtesting: dive interval
      (`DIVE_INTERVAL_BASE 60`, min 24), diver fire cooldown
      (`DIVE_FIRE_COOLDOWN 30`, min 12), bullet speed multipliers
      (player 3x, enemy 1.5x) live in `include/galaga.inc` and must be kept
      in sync with `src/game_fallback.c`.
- [ ] Sound: SDL_mixer or simple square-wave synth (fire, explosion, dive,
      stage-clear jingle).
- [ ] The animation frame is derived in `render.c` from `state->frame`
      (`>> 3`); if game logic ever needs it, move it into state.
