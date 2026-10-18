#ifndef GALAGA_SYNTH_H
#define GALAGA_SYNTH_H

/* Small original sound synthesiser: square / triangle / pitched-noise
   voices with pitch sweeps, vibrato and envelopes, plus note-sequence
   jingles. No samples. Pure C (no SDL) so the tests can drive it. */

#include <stdint.h>
#include "galaga.h"

enum {
    SND_SHOT, SND_HIT, SND_BOSS_HIT, SND_PLAYER_DIE, SND_DIVE,
    SND_CAPTURED, SND_RESCUE, SND_EXTRA_LIFE,
    SND_START, SND_STAGE, SND_CHALLENGE, SND_RESULT, SND_PERFECT, SND_GAME_OVER,
    SND_COUNT
};

void synth_init(int sample_rate);
void synth_play(int sound);
void synth_beam(int on);                 /* looping tractor-beam warble */
void synth_set_mute(int mute);
int  synth_muted(void);
void synth_render(int16_t *out, int frames);  /* mono, adds nothing when muted */
int  synth_active_voices(void);

/* Watches GameState between frames and turns changes into sounds, so the
   simulation (asm and C) stays sound-free. */
typedef struct {
    int valid;
    int phase, banner, next_extra_life, capt_state, chal_result, chal_timer;
    uint8_t bullet_on[MAX_BULLETS];
    int     bullet_y[MAX_BULLETS];
    uint8_t expl_on[MAX_EXPLOSIONS];
    int     expl_timer[MAX_EXPLOSIONS];
    uint8_t enemy_on[MAX_ENEMIES];
    int     enemy_hp[MAX_ENEMIES], enemy_state[MAX_ENEMIES];
    uint32_t played;                     /* bitmask of SND_* fired last update */
} SoundWatch;

void sound_watch_update(SoundWatch *w, const GameState *s);

#endif
