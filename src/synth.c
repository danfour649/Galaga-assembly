#include "synth.h"

#include <math.h>
#include <string.h>

#define MAX_VOICES 12
#define PI2 6.28318530718f

enum { W_SQUARE, W_TRIANGLE, W_NOISE };

typedef struct {
    int   active, wave, sound;
    int   t, len;             /* samples */
    float freq, ratio;        /* current Hz, per-sample sweep multiplier */
    float vol, duty;
    float lfo_rate, lfo_depth;
    float phase;
    uint32_t lfsr;
    float noise_val;
    int   hold;               /* 1 = flat envelope with short release */
    /* jingles */
    const int8_t *notes;      /* MIDI note numbers, 0 = rest, -1 = end */
    const uint8_t *steps;     /* length of each note in sequencer steps */
    int   note_i, note_t, step_len;
} Voice;

static Voice voices[MAX_VOICES];
static int   rate = 48000;
static int   muted;
static int   beam_on;
static float beam_phase, beam_lfo;

void synth_init(int sample_rate)
{
    memset(voices, 0, sizeof(voices));
    rate = sample_rate > 0 ? sample_rate : 48000;
    beam_on = 0;
}

void synth_set_mute(int m) { muted = m ? 1 : 0; if (muted) memset(voices, 0, sizeof(voices)); }
int  synth_muted(void) { return muted; }
void synth_beam(int on) { beam_on = on ? 1 : 0; }

int synth_active_voices(void)
{
    int n = beam_on;
    for (int i = 0; i < MAX_VOICES; i++)
        n += voices[i].active;
    return n;
}

static float midi_hz(int n) { return 440.0f * powf(2.0f, (float)(n - 69) / 12.0f); }

static Voice *alloc_voice(int sound)
{
    Voice *best = &voices[0];
    for (int i = 0; i < MAX_VOICES; i++) {
        if (!voices[i].active) { best = &voices[i]; break; }
        if (voices[i].t > best->t) best = &voices[i];   /* steal the oldest */
    }
    memset(best, 0, sizeof(*best));
    best->active = 1;
    best->sound = sound;
    best->lfsr = 0xACE1u;
    best->duty = 0.5f;
    return best;
}

static void tone(int sound, int wave, float f0, float f1, float secs, float vol)
{
    Voice *v = alloc_voice(sound);
    v->wave = wave;
    v->len = (int)(secs * (float)rate);
    if (v->len < 1) v->len = 1;
    v->freq = f0;
    v->ratio = powf(f1 / f0, 1.0f / (float)v->len);
    v->vol = vol;
}

static void jingle(int sound, int wave, const int8_t *notes, const uint8_t *steps,
                   float step_secs, float vol, float duty)
{
    Voice *v = alloc_voice(sound);
    v->wave = wave;
    v->notes = notes;
    v->steps = steps;
    v->step_len = (int)(step_secs * (float)rate);
    v->len = 0x7fffffff;
    v->vol = vol;
    v->duty = duty;
    v->hold = 1;
    v->freq = notes[0] > 0 ? midi_hz(notes[0]) : 0.0f;
    v->ratio = 1.0f;
}

/* --- original jingles (MIDI numbers; 0 = rest, -1 = end) ------------------ */
/* game start: a rising C-major fanfare, melody + bass */
static const int8_t  START_M[] = { 67, 72, 76, 79, 76, 79, 84, 0, 83, 81, 79, 76, 77, 74, 71, 67, 72, 76, 79, 84, -1 };
static const uint8_t START_MS[] = { 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4 };
static const int8_t  START_B[] = { 48, 0, 43, 0, 48, 0, 52, 0, 41, 0, 43, 0, 43, 0, 47, 0, 48, -1 };
static const uint8_t START_BS[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 6 };
static const int8_t  STAGE_N[] = { 76, 79, 84, -1 };
static const uint8_t STAGE_S[] = { 1, 1, 3 };
static const int8_t  CHAL_N[] = { 74, 78, 81, 86, 81, 86, 90, -1 };
static const uint8_t CHAL_S[] = { 1, 1, 1, 1, 1, 1, 4 };
static const int8_t  PERF_N[] = { 84, 88, 91, 96, 91, 96, 91, 96, -1 };
static const uint8_t PERF_S[] = { 1, 1, 1, 2, 1, 1, 1, 4 };
static const int8_t  LIFE_N[] = { 81, 85, 88, 93, 81, 85, 88, 93, -1 };
static const uint8_t LIFE_S[] = { 1, 1, 1, 1, 1, 1, 1, 3 };
static const int8_t  CAPT_N[] = { 76, 75, 74, 73, 72, 71, 70, 69, -1 };
static const uint8_t CAPT_S[] = { 2, 2, 2, 2, 2, 2, 2, 5 };
static const int8_t  RESC_N[] = { 72, 76, 79, 84, 88, -1 };
static const uint8_t RESC_S[] = { 1, 1, 1, 1, 4 };
static const int8_t  OVER_N[] = { 67, 0, 64, 0, 60, -1 };
static const uint8_t OVER_S[] = { 2, 1, 2, 1, 6 };
static const int8_t  RES_N[] = { 79, 84, -1 };
static const uint8_t RES_S[] = { 1, 2 };

void synth_play(int s)
{
    if (muted)
        return;
    switch (s) {
    case SND_SHOT:
        tone(s, W_SQUARE, 1700.0f, 480.0f, 0.07f, 0.16f);
        break;
    case SND_HIT:
        tone(s, W_NOISE, 3200.0f, 260.0f, 0.20f, 0.30f);
        tone(s, W_SQUARE, 640.0f, 140.0f, 0.06f, 0.10f);
        break;
    case SND_BOSS_HIT:
        tone(s, W_SQUARE, 1250.0f, 1150.0f, 0.09f, 0.12f);
        tone(s, W_SQUARE, 1870.0f, 1720.0f, 0.09f, 0.08f);
        break;
    case SND_PLAYER_DIE:
        tone(s, W_NOISE, 1800.0f, 55.0f, 1.40f, 0.45f);
        tone(s, W_SQUARE, 320.0f, 40.0f, 0.80f, 0.12f);
        break;
    case SND_DIVE: {
        tone(s, W_TRIANGLE, 1500.0f, 380.0f, 0.50f, 0.20f);
        Voice *v = NULL;
        for (int i = 0; i < MAX_VOICES; i++)
            if (voices[i].active && voices[i].sound == SND_DIVE && voices[i].t == 0)
                v = &voices[i];
        if (v) { v->lfo_rate = 14.0f; v->lfo_depth = 0.04f; }
        break;
    }
    case SND_CAPTURED:
        jingle(s, W_TRIANGLE, CAPT_N, CAPT_S, 0.11f, 0.30f, 0.5f);
        break;
    case SND_RESCUE:
        jingle(s, W_SQUARE, RESC_N, RESC_S, 0.07f, 0.14f, 0.25f);
        break;
    case SND_EXTRA_LIFE:
        jingle(s, W_SQUARE, LIFE_N, LIFE_S, 0.06f, 0.12f, 0.25f);
        break;
    case SND_START:
        jingle(s, W_SQUARE, START_M, START_MS, 0.11f, 0.12f, 0.25f);
        jingle(s, W_TRIANGLE, START_B, START_BS, 0.11f, 0.30f, 0.5f);
        break;
    case SND_STAGE:
        jingle(s, W_SQUARE, STAGE_N, STAGE_S, 0.09f, 0.12f, 0.25f);
        break;
    case SND_CHALLENGE:
        jingle(s, W_SQUARE, CHAL_N, CHAL_S, 0.08f, 0.12f, 0.25f);
        break;
    case SND_RESULT:
        jingle(s, W_SQUARE, RES_N, RES_S, 0.07f, 0.12f, 0.5f);
        break;
    case SND_PERFECT:
        jingle(s, W_SQUARE, PERF_N, PERF_S, 0.07f, 0.13f, 0.25f);
        break;
    case SND_GAME_OVER:
        jingle(s, W_TRIANGLE, OVER_N, OVER_S, 0.14f, 0.32f, 0.5f);
        break;
    default:
        break;
    }
}

static float osc(Voice *v, float f)
{
    v->phase += f / (float)rate;
    if (v->phase >= 1.0f) {
        v->phase -= floorf(v->phase);
        if (v->wave == W_NOISE) {
            /* pitched noise: new random level each cycle */
            uint32_t x = v->lfsr;
            x ^= x << 13; x ^= x >> 17; x ^= x << 5;
            v->lfsr = x;
            v->noise_val = (float)(x & 0xffff) / 32768.0f - 1.0f;
        }
    }
    switch (v->wave) {
    case W_SQUARE:   return v->phase < v->duty ? 1.0f : -1.0f;
    case W_TRIANGLE: return 4.0f * fabsf(v->phase - 0.5f) - 1.0f;
    default:         return v->noise_val;
    }
}

static float voice_sample(Voice *v)
{
    float env, f = v->freq;

    if (v->notes) {
        if (v->note_t >= v->steps[v->note_i] * v->step_len) {
            v->note_i++;
            v->note_t = 0;
            if (v->notes[v->note_i] < 0) { v->active = 0; return 0.0f; }
            v->freq = v->notes[v->note_i] > 0 ? midi_hz(v->notes[v->note_i]) : 0.0f;
            f = v->freq;
        }
        int nlen = v->steps[v->note_i] * v->step_len;
        int nt = v->note_t++;
        v->t++;
        if (f <= 0.0f)
            return 0.0f;
        /* short attack, gentle decay, release before the next note */
        env = nt < 120 ? (float)nt / 120.0f : 1.0f - 0.35f * (float)nt / (float)nlen;
        if (nlen - nt < 300) env *= (float)(nlen - nt) / 300.0f;
        return osc(v, f) * env * v->vol;
    }

    if (v->t >= v->len) { v->active = 0; return 0.0f; }
    if (v->lfo_depth > 0.0f)
        f *= 1.0f + v->lfo_depth * sinf(PI2 * v->lfo_rate * (float)v->t / (float)rate);
    float p = (float)v->t / (float)v->len;
    env = (v->t < 60) ? (float)v->t / 60.0f : (1.0f - p) * (1.0f - p);
    v->t++;
    v->freq *= v->ratio;
    return osc(v, f) * env * v->vol;
}

void synth_render(int16_t *out, int frames)
{
    for (int n = 0; n < frames; n++) {
        float mix = 0.0f;
        if (!muted) {
            for (int i = 0; i < MAX_VOICES; i++)
                if (voices[i].active)
                    mix += voice_sample(&voices[i]);
            if (beam_on) {
                /* tractor beam: triangle warbling between ~200 and ~520 Hz */
                beam_lfo += 5.5f / (float)rate;
                if (beam_lfo >= 1.0f) beam_lfo -= 1.0f;
                float f = 360.0f + 160.0f * sinf(PI2 * beam_lfo);
                beam_phase += f / (float)rate;
                if (beam_phase >= 1.0f) beam_phase -= 1.0f;
                mix += (4.0f * fabsf(beam_phase - 0.5f) - 1.0f) * 0.16f;
            }
        }
        /* soft clip */
        if (mix > 1.0f) mix = 1.0f;
        if (mix < -1.0f) mix = -1.0f;
        out[n] = (int16_t)(mix * 26000.0f);
    }
}

/* --- state watcher -------------------------------------------------------- */

static void fire(SoundWatch *w, int snd)
{
    if (w->played & (1u << snd))
        return;                 /* once per update */
    w->played |= 1u << snd;
    synth_play(snd);
}

void sound_watch_update(SoundWatch *w, const GameState *s)
{
    int i;
    w->played = 0;
    if (w->valid) {
        if (s->phase == GAME_PHASE_PLAYING || s->phase == GAME_PHASE_STAGE_CLEAR) {
            for (i = 0; i < MAX_BULLETS; i++) {
                const Bullet *b = &s->bullets[i];
                if (b->active && b->from_player &&
                    (!w->bullet_on[i] || b->y > w->bullet_y[i] + 4))
                    fire(w, SND_SHOT);
            }
            for (i = 0; i < MAX_EXPLOSIONS; i++) {
                const Explosion *e = &s->explosions[i];
                if (e->active && (!w->expl_on[i] || e->timer > w->expl_timer[i]))
                    fire(w, e->kind == EXPLOSION_PLAYER ? SND_PLAYER_DIE : SND_HIT);
            }
            for (i = 0; i < MAX_ENEMIES; i++) {
                const Enemy *e = &s->enemies[i];
                if (!e->active || !w->enemy_on[i])
                    continue;
                if (e->type == ENEMY_TYPE_BOSS && e->hp < w->enemy_hp[i])
                    fire(w, SND_BOSS_HIT);
                if (e->state == ENEMY_STATE_DIVING && w->enemy_state[i] == ENEMY_STATE_FORMATION)
                    fire(w, SND_DIVE);
            }
            if (s->next_extra_life > w->next_extra_life && w->phase == GAME_PHASE_PLAYING)
                fire(w, SND_EXTRA_LIFE);
            if (s->capture.state != w->capt_state) {
                if (s->capture.state == CAPT_PULL) fire(w, SND_CAPTURED);
                if (s->capture.state == CAPT_RESCUE) fire(w, SND_RESCUE);
            }
            if (s->challenge.result == 1 && w->chal_result == 0)
                fire(w, SND_RESULT);
            if (s->challenge.result == 1 &&
                w->chal_timer > CHALLENGE_RESULT_STEPS - CHALLENGE_BONUS_STEP &&
                s->challenge.result_timer <= CHALLENGE_RESULT_STEPS - CHALLENGE_BONUS_STEP)
                fire(w, s->challenge.hits >= CHALLENGE_GROUPS * CHALLENGE_PER_GROUP
                            ? SND_PERFECT : SND_RESULT);
        }
        if (s->phase == GAME_PHASE_PLAYING && s->hud.banner != w->banner) {
            if (s->hud.banner == BANNER_PLAYER1)
                fire(w, SND_START);
            else if (s->hud.banner == BANNER_STAGE && w->banner != BANNER_PLAYER1)
                fire(w, s->challenge.active ? SND_CHALLENGE : SND_STAGE);
        }
        if (s->phase == GAME_PHASE_GAME_OVER && w->phase != GAME_PHASE_GAME_OVER)
            fire(w, SND_GAME_OVER);
    }
    synth_beam(s->phase == GAME_PHASE_PLAYING && s->capture.beam_boss >= 0 &&
               s->capture.beam_len > 0);

    w->valid = 1;
    w->phase = s->phase;
    w->banner = s->hud.banner;
    w->next_extra_life = s->next_extra_life;
    w->capt_state = s->capture.state;
    w->chal_result = s->challenge.result;
    w->chal_timer = s->challenge.result_timer;
    for (i = 0; i < MAX_BULLETS; i++) {
        w->bullet_on[i] = s->bullets[i].active && s->bullets[i].from_player;
        w->bullet_y[i] = s->bullets[i].y;
    }
    for (i = 0; i < MAX_EXPLOSIONS; i++) {
        w->expl_on[i] = s->explosions[i].active;
        w->expl_timer[i] = s->explosions[i].timer;
    }
    for (i = 0; i < MAX_ENEMIES; i++) {
        w->enemy_on[i] = s->enemies[i].active;
        w->enemy_hp[i] = s->enemies[i].hp;
        w->enemy_state[i] = s->enemies[i].state;
    }
}
