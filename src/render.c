#include "galaga.h"

#include <SDL.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static SDL_Window   *window;
static SDL_Renderer *renderer;

static void draw_sprite(SDL_Texture *tex, int x, int y, int w, int h)
{
    SDL_Rect dst = { x, y, w, h };
    SDL_RenderCopy(renderer, tex, NULL, &dst);
}

static void draw_rect(int x, int y, int w, int h, Uint8 r, Uint8 g, Uint8 b)
{
    SDL_Rect rect = { x, y, w, h };
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_RenderFillRect(renderer, &rect);
}

/* --- 8x8 arcade-style font (src/font8.h), drawn from a white glyph atlas
   tinted per call. Each character cell is 8x8 game pixels times scale. --- */

#include "font8.h"

#define FONT8_COUNT ((int)(sizeof(FONT8) / sizeof(FONT8[0])))
static SDL_Texture *font_tex;

static void font_init(void)
{
    Uint32 px[FONT8_COUNT * 8 * 8];
    for (int g = 0; g < FONT8_COUNT; g++)
        for (int row = 0; row < 8; row++)
            for (int col = 0; col < 8; col++)
                px[row * FONT8_COUNT * 8 + g * 8 + col] =
                    (FONT8[g][row] & (0x80 >> col)) ? 0xFFFFFFFFu : 0x00FFFFFFu;
    font_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                 SDL_TEXTUREACCESS_STATIC, FONT8_COUNT * 8, 8);
    if (font_tex) {
        SDL_UpdateTexture(font_tex, NULL, px, FONT8_COUNT * 8 * 4);
        SDL_SetTextureBlendMode(font_tex, SDL_BLENDMODE_BLEND);
    }
}

static int font_index(char c)
{
    if (c >= 'a' && c <= 'z')
        c = (char)(c - 'a' + 'A');
    const char *p = strchr(FONT8_CHARS, c);
    return (c && p) ? (int)(p - FONT8_CHARS) : -1;
}

static void draw_text(const char *s, int x, int y, int scale,
                      Uint8 r, Uint8 g, Uint8 b)
{
    if (!font_tex)
        return;
    SDL_SetTextureColorMod(font_tex, r, g, b);
    for (; *s; s++, x += 8 * scale) {
        int gi = font_index(*s);
        if (gi < 0)
            continue;
        SDL_Rect src = { gi * 8, 0, 8, 8 };
        SDL_Rect dst = { x, y, 8 * scale, 8 * scale };
        SDL_RenderCopy(renderer, font_tex, &src, &dst);
    }
}

static int text_width(const char *s, int scale)
{
    return (int)strlen(s) * 8 * scale - scale;
}

static void draw_text_centered(const char *s, int y, int scale,
                               Uint8 r, Uint8 g, Uint8 b)
{
    draw_text(s, (GALAGA_WIDTH - text_width(s, scale)) / 2, y, scale, r, g, b);
}

static void draw_text_right(const char *s, int right_x, int y,
                            Uint8 r, Uint8 g, Uint8 b)
{
    draw_text(s, right_x - (int)strlen(s) * 8, y, 1, r, g, b);
}

/* --- Starfield: one fixed set of stars that scrolls continuously in every
   phase (no per-stage reseed, no wrap-back jump), two speeds, and the
   arcade's multicolour twinkle: stars blink in four interleaved groups. --- */

#define STAR_COUNT 72
static struct { int16_t x, y0; uint8_t speed, col, group; } stars[STAR_COUNT];

static const Uint8 STAR_COLOURS[8][3] = {
    { 255, 255, 255 }, { 255,  70,  70 }, { 255, 230,  80 }, {  80, 255, 120 },
    {  80, 220, 255 }, {  90, 110, 255 }, { 255,  90, 230 }, { 255, 160,  60 },
};

static void starfield_init(uint32_t seed)
{
    for (int i = 0; i < STAR_COUNT; i++) {
        seed = seed * 1103515245u + 12345u;
        stars[i].x = (int16_t)((seed >> 16) % GALAGA_WIDTH);
        seed = seed * 1103515245u + 12345u;
        stars[i].y0 = (int16_t)((seed >> 16) % GALAGA_HEIGHT);
        seed = seed * 1103515245u + 12345u;
        stars[i].speed = (uint8_t)(1 + ((seed >> 16) % 3 == 0));
        stars[i].col = (uint8_t)((seed >> 20) & 7);
        stars[i].group = (uint8_t)(i & 3);
    }
}

static void draw_starfield(uint32_t ticks)
{
    uint32_t blink = ticks / 12;
    for (int i = 0; i < STAR_COUNT; i++) {
        /* each group is dark one quarter of the time, staggered */
        if (((blink + stars[i].group) & 3) == 0)
            continue;
        /* speed 1 = 0.5 px/step (far), 2 = 1 px/step (near) */
        int y = (int)((stars[i].y0 + (ticks * stars[i].speed) / 2) % GALAGA_HEIGHT);
        const Uint8 *c = STAR_COLOURS[stars[i].col];
        int dim = stars[i].speed == 1 ? 150 : 255;
        draw_rect(stars[i].x, y, 1, 1, (Uint8)(c[0] * dim / 255),
                  (Uint8)(c[1] * dim / 255), (Uint8)(c[2] * dim / 255));
    }
}

static void px_block(int x, int y, int sz, Uint8 r, Uint8 g, Uint8 b)
{
    draw_rect(x - sz / 2, y - sz / 2, sz, sz, r, g, b);
}

/* Pixel starburst, drawn procedurally (no ROM art). Enemy: 12 frames of a
   core that blooms into eight spikes and scatters. Fighter: a bigger,
   longer white/red/yellow burst with rotating shards. */
static void draw_explosion(const Explosion *e)
{
    static const int DX[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
    static const int DY[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };

    if (e->kind == EXPLOSION_PLAYER) {
        int age = PLAYER_EXPLOSION_FRAMES - e->timer;
        int phase = age / 6;                  /* 0..7 */
        if (phase < 2) {
            px_block(e->x, e->y, 4 + phase * 4, 255, 255, 255);
            px_block(e->x, e->y, 2 + phase * 2, 255, 220, 60);
            return;
        }
        for (int k = 0; k < 8; k++) {
            int rot = (k + phase) & 7;
            int r1 = 3 + phase * 2;
            int r2 = r1 + 3 + (k & 1) * 3;
            Uint8 cr = 255, cg = (k & 1) ? 60 : 230, cb = (k & 1) ? 40 : 60;
            if (phase >= 6) { cr = 160; cg = 40; cb = 40; }
            px_block(e->x + DX[rot] * r1, e->y + DY[rot] * r1, 2, 255, 255, 255);
            px_block(e->x + DX[k] * r2, e->y + DY[k] * r2, 2, cr, cg, cb);
            if (phase < 6)
                px_block(e->x + DX[k] * (r2 + 3) + DY[k], e->y + DY[k] * (r2 + 3) - DX[k], 1, 255, 160, 40);
        }
        if (phase < 5)
            px_block(e->x, e->y, 4, 255, 120, 40);
        return;
    }

    int age = EXPLOSION_FRAMES - e->timer;    /* 0..11 */
    if (age < 3) {
        px_block(e->x, e->y, 2 + age * 2, 255, 255, 255);
        px_block(e->x, e->y, 2 + age, 255, 230, 80);
        return;
    }
    int r = 1 + age;                            /* 4..12 px */
    for (int k = 0; k < 8; k++) {
        bool diag = (k & 1);
        int rr = diag ? (r * 3) / 4 : r;
        Uint8 cr = 255, cg = diag ? 70 : 220, cb = diag ? 40 : 50;
        if (age >= 9) { cr = 200; cg = 60; cb = 30; }
        px_block(e->x + DX[k] * rr, e->y + DY[k] * rr, age < 8 ? 2 : 1, cr, cg, cb);
        if (age < 8)
            px_block(e->x + DX[k] * (rr / 2), e->y + DY[k] * (rr / 2), 1, 255, 255, 200);
    }
    if (age < 6)
        px_block(e->x, e->y, 3, 255, 140, 40);
}

/* Stage badges, bottom right: 50, 30, 20, 10, 5 and 1 pennants, drawn
   procedurally in the arcade's spirit (largest first, right to left). */
static int draw_badge(int right_x, int value)
{
    int w = (value >= 10) ? 7 : (value == 5 ? 6 : 4);
    int h = (value == 1) ? 9 : 11;
    int x = right_x - w;
    int y = GALAGA_HEIGHT - 2 - h;
    Uint8 r1, g1, b1, r2, g2, b2;
    switch (value) {
    case 50: r1 = 230; g1 = 40;  b1 = 40;  r2 = 255; g2 = 230; b2 = 60;  break;
    case 30: r1 = 255; g1 = 230; b1 = 60;  r2 = 60;  g2 = 120; b2 = 255; break;
    case 20: r1 = 230; g1 = 40;  b1 = 40;  r2 = 60;  g2 = 120; b2 = 255; break;
    case 10: r1 = 60;  g1 = 120; b1 = 255; r2 = 255; g2 = 230; b2 = 60;  break;
    case 5:  r1 = 230; g1 = 40;  b1 = 40;  r2 = 255; g2 = 255; b2 = 255; break;
    default: r1 = 255; g1 = 255; b1 = 255; r2 = 230; g2 = 40;  b2 = 40;  break;
    }
    draw_rect(x, y, w, h - 3, r1, g1, b1);                 /* flag body */
    draw_rect(x + 1, y + 2, w - 2, (h - 3) / 2, r2, g2, b2); /* stripe */
    draw_rect(x + w / 2 - 1, y + h - 4, 2, 2, r1, g1, b1);   /* point */
    draw_rect(x + w / 2, y + h - 2, 1, 2, r2, g2, b2);
    return w + 1;
}

static void draw_stage_badges(int stage)
{
    static const int VALS[6] = { 50, 30, 20, 10, 5, 1 };
    int x = GALAGA_WIDTH - 2;
    for (int v = 0; v < 6 && x > GALAGA_WIDTH / 2; v++) {
        while (stage >= VALS[v] && x > GALAGA_WIDTH / 2) {
            x -= draw_badge(x, VALS[v]);
            stage -= VALS[v];
        }
    }
}

/* Tractor beam: stacked stripes widening downward, colours cycling. */
static void draw_beam(const GameState *state)
{
    const Capture *c = &state->capture;
    if (c->beam_boss < 0 || c->beam_len <= 0)
        return;
    const Enemy *b = &state->enemies[c->beam_boss];
    int cx = b->x + 7;
    int top = b->y + 15;
    static const Uint8 COL[3][3] = { { 80, 230, 255 }, { 40, 110, 255 }, { 210, 220, 255 } };
    for (int d = 0; d < c->beam_len; d += 3) {
        int half = 8 + (d * 22) / 100;
        int k = ((d / 3) + (int)(state->hud.ticks / 3)) % 3;
        /* ragged edges like the arcade beam */
        int jag = ((d / 3) & 1) ? 2 : 0;
        draw_rect(cx - half + jag, top + d, half * 2 - jag * 2, 2, COL[k][0], COL[k][1], COL[k][2]);
    }
}

static void draw_captive(const GameState *state)
{
    const Capture *c = &state->capture;
    if (c->state == CAPT_NONE)
        return;
    SDL_Rect dst = { (int)c->cx - 8, (int)c->cy - 8, 16, 16 };
    double angle = (c->state == CAPT_HELD) ? 180.0 : (double)((c->spin * 18) % 360);
    SDL_RenderCopyEx(renderer, c->state == CAPT_RESCUE ? sprites_player() : sprites_captive(),
                     NULL, &dst, angle, NULL, SDL_FLIP_NONE);
}

static void format_score(char *buf, size_t n, int v)
{
    /* the arcade shows an empty score as "00" */
    if (v <= 0)
        snprintf(buf, n, "00");
    else
        snprintf(buf, n, "%d", v);
}

static void draw_hud(const GameState *state)
{
    char buf[32];
    bool title = state->phase == GAME_PHASE_TITLE;

    /* 1UP blinks while a game is on, as in the arcade */
    if (title || ((state->hud.ticks >> 4) & 1) == 0)
        draw_text("1UP", 24, 1, 1, 255, 40, 40);
    format_score(buf, sizeof(buf), state->player.score);
    draw_text_right(buf, 56, 9, 255, 255, 255);

    draw_text_centered("HIGH SCORE", 1, 1, 255, 40, 40);
    format_score(buf, sizeof(buf), state->high_score);
    draw_text_right(buf, 136, 9, 255, 255, 255);

    if (title) {
        draw_text("FREE PLAY", 4, GALAGA_HEIGHT - 9, 1, 255, 255, 255);
        return;
    }

    /* reserve ships, bottom-left */
    int reserve = state->player.lives - 1;
    if (reserve > 6)
        reserve = 6;
    for (int i = 0; i < reserve; i++)
        draw_sprite(sprites_player(), 4 + i * 12, GALAGA_HEIGHT - 12, 10, 10);

    draw_stage_badges(state->player.stage);
}

/* --- Attract / title screen. Pages cycle every ATTRACT_CYCLE steps:
   page 0 reveals the score table line by line, page 1 explains the
   capture-and-rescue trick. Plain text title; no logo art. --- */

#define ATTRACT_PAGE   ATTRACT_PAGE_STEPS
#define ATTRACT_CYCLE  (ATTRACT_PAGE * 2)

static void draw_score_row(int y, SDL_Texture *tex, int rack, int dive, bool flip)
{
    char buf[8];
    SDL_Rect dst = { 52, y - 4, 16, 16 };
    SDL_RenderCopyEx(renderer, tex, NULL, &dst, 0.0, NULL,
                     flip ? SDL_FLIP_VERTICAL : SDL_FLIP_NONE);
    snprintf(buf, sizeof(buf), "%d", rack);
    draw_text_right(buf, 128, y, 255, 255, 255);
    snprintf(buf, sizeof(buf), "%d", dive);
    draw_text_right(buf, 184, y, 255, 255, 255);
}

/* Post-game results, as on the arcade: shots, hits and hit-miss ratio. */
static void draw_results(const GameState *state)
{
    const Hud *h = &state->hud;
    char buf[16];
    int hits = h->hits > h->shots ? h->shots : h->hits;
    int tenths = h->shots > 0 ? (int)((hits * 1000LL + h->shots / 2) / h->shots) : 0;

    draw_text_centered("-RESULTS-", 104, 1, 255, 40, 40);
    draw_text("SHOTS FIRED", 16, 132, 1, 255, 230, 80);
    snprintf(buf, sizeof(buf), "%d", h->shots);
    draw_text_right(buf, 208, 132, 255, 255, 255);
    draw_text("NUMBER OF HITS", 16, 152, 1, 255, 230, 80);
    snprintf(buf, sizeof(buf), "%d", hits);
    draw_text_right(buf, 208, 152, 255, 255, 255);
    draw_text("HIT-MISS RATIO", 16, 172, 1, 255, 230, 80);
    snprintf(buf, sizeof(buf), "%d.%d %%", tenths / 10, tenths % 10);
    draw_text_right(buf, 208, 172, 255, 255, 255);
    if (h->over_t >= GAMEOVER_FIRE_LOCK && !((h->ticks >> 5) & 1))
        draw_text_centered("PRESS FIRE", 212, 1, 255, 255, 255);
}

/* Challenging-stage results: hit count, then the bonus (or PERFECT!). */
static void draw_challenge_result(const GameState *state)
{
    const Challenge *c = &state->challenge;
    int t = CHALLENGE_RESULT_STEPS - c->result_timer;
    char buf[32];
    bool perfect = c->hits >= CHALLENGE_GROUPS * CHALLENGE_PER_GROUP;

    if (perfect) {
        if ((t >> 4) & 1)
            draw_text_centered("PERFECT !", 120, 1, 255, 40, 40);
    }
    snprintf(buf, sizeof(buf), "NUMBER OF HITS  %d", c->hits);
    draw_text(buf, 28, 140, 1, 90, 230, 255);
    if (t >= CHALLENGE_BONUS_STEP) {
        if (perfect) {
            draw_text_centered("SPECIAL BONUS", 160, 1, 255, 230, 80);
            snprintf(buf, sizeof(buf), "%d PTS", CHALLENGE_PERFECT_BONUS);
            draw_text_centered(buf, 172, 1, 255, 230, 80);
        } else {
            snprintf(buf, sizeof(buf), "BONUS  %d", c->bonus);
            draw_text(buf, 28, 160, 1, 255, 230, 80);
        }
    }
}

static void draw_attract(const GameState *state)
{
    uint32_t t = (uint32_t)state->hud.title_t % ATTRACT_CYCLE;
    int anim = (int)(state->hud.ticks >> 3) & 1;

    /* title: red with a darker drop shadow */
    draw_text_centered("GALAGA", 37, 3, 120, 0, 0);
    draw_text_centered("GALAGA", 36, 3, 255, 40, 40);
    draw_text_centered("ASSEMBLY EDITION", 66, 1, 90, 230, 255);

    if (t < ATTRACT_PAGE) {
        int shown = (int)t / 45;              /* lines appear one by one */
        if (shown >= 1)
            draw_text_centered("-- SCORE --", 92, 1, 255, 230, 80);
        if (shown >= 2) {
            draw_text_right("RACK", 132, 110, 90, 230, 255);
            draw_text_right("DIVE", 188, 110, 90, 230, 255);
        }
        if (shown >= 3)
            draw_score_row(128, sprites_enemy(ENEMY_TYPE_BEE, anim, 1),
                           SCORE_BEE_FORMATION, SCORE_BEE_DIVING, false);
        if (shown >= 4)
            draw_score_row(148, sprites_enemy(ENEMY_TYPE_BUTTERFLY, anim, 1),
                           SCORE_BUTTERFLY_FORMATION, SCORE_BUTTERFLY_DIVING, false);
        if (shown >= 5)
            draw_score_row(168, sprites_enemy(ENEMY_TYPE_BOSS, anim, 2),
                           SCORE_BOSS_FORMATION, SCORE_BOSS_DIVING, false);
        if (shown >= 6)
            draw_score_row(188, sprites_captive(), 500, 1000, true);
        if (shown >= 7)
            draw_text_centered("BOSS + 2 ESCORTS 1600", 208, 1, 255, 230, 80);
    } else {
        uint32_t u = t - ATTRACT_PAGE;
        draw_text_centered("BEWARE THE BOSS BEAM", 92, 1, 255, 230, 80);

        /* boss lowering its beam over a fighter */
        SDL_Rect boss = { 44, 112, 16, 16 };
        SDL_RenderCopy(renderer, sprites_enemy(ENEMY_TYPE_BOSS, anim, 2), NULL, &boss);
        int len = (int)(u < 60 ? u : 60) * 40 / 60;
        static const Uint8 COL[3][3] = { { 80, 230, 255 }, { 40, 110, 255 }, { 210, 220, 255 } };
        for (int d = 0; d < len; d += 3) {
            int half = 5 + d / 6;
            int k = ((d / 3) + (int)(state->hud.ticks / 3)) % 3;
            draw_rect(52 - half, 128 + d, half * 2, 2, COL[k][0], COL[k][1], COL[k][2]);
        }
        draw_sprite(sprites_player(), 44, 172, 16, 16);

        if (u >= 90) {
            draw_text(">", 92, 146, 1, 255, 255, 255);
            /* rescued pair */
            draw_sprite(sprites_player(), 132, 140, 16, 16);
            draw_sprite(sprites_player(), 148, 140, 16, 16);
        }
        if (u >= 150) {
            draw_text_centered("SHOOT THE BOSS AS IT", 196, 1, 255, 255, 255);
            draw_text_centered("DIVES TO FREE YOUR", 208, 1, 255, 255, 255);
            draw_text_centered("FIGHTER: DOUBLE FIRE!", 220, 1, 255, 255, 255);
        }
    }

    if (!((state->hud.ticks >> 5) & 1))
        draw_text_centered("PRESS FIRE TO START", 240, 1, 255, 255, 255);
    draw_text_centered("@ 2026 GALAGA-ASSEMBLY", 260, 1, 255, 255, 255);
}

void render_init(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        exit(1);
    }

    /* Largest integer scale that fits ~90% of the desktop height. */
    int scale = GALAGA_SCALE;
#ifndef __EMSCRIPTEN__
    SDL_Rect usable;
    if (SDL_GetDisplayUsableBounds(0, &usable) == 0) {
        int fit = (usable.h * 9 / 10) / GALAGA_HEIGHT;
        int fit_w = (usable.w * 9 / 10) / GALAGA_WIDTH;
        if (fit_w < fit)
            fit = fit_w;
        if (fit > scale)
            scale = fit;
    }
#endif
    int win_w = GALAGA_WIDTH * scale;
    int win_h = GALAGA_HEIGHT * scale;

    window = SDL_CreateWindow(
        "Galaga-assembly",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        win_w,
        win_h,
#ifdef __EMSCRIPTEN__
        /* RESIZABLE makes SDL's Emscripten backend copy the canvas CSS size
           back into the window size; with the shell's border/max-width that
           shrinks the canvas to a few pixels. Fixed backbuffer; CSS scales. */
        SDL_WINDOW_SHOWN
#else
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
#endif
    );
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        exit(1);
    }

    /* Accelerated+vsync is right for a normal desktop. Dummy and some
       Xvfb setups only have the software driver, so fall back instead of
       exiting before the first frame. */
    const Uint32 renderer_flags[] = {
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC,
        SDL_RENDERER_ACCELERATED,
        SDL_RENDERER_SOFTWARE | SDL_RENDERER_PRESENTVSYNC,
        SDL_RENDERER_SOFTWARE
    };
    for (size_t i = 0; i < sizeof renderer_flags / sizeof renderer_flags[0]; i++) {
        renderer = SDL_CreateRenderer(window, -1, renderer_flags[i]);
        if (renderer)
            break;
    }
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    /* Draw in game pixels; SDL scales to the window and letterboxes. */
    SDL_RenderSetLogicalSize(renderer, GALAGA_WIDTH, GALAGA_HEIGHT);
    SDL_RenderSetIntegerScale(renderer, SDL_TRUE);

    starfield_init((uint32_t)time(NULL));
    sprites_init(renderer);
    font_init();
}

void render_toggle_fullscreen(void)
{
    Uint32 flags = SDL_GetWindowFlags(window);
    SDL_SetWindowFullscreen(window,
        (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
}

void render_shutdown(void)
{
    if (font_tex) SDL_DestroyTexture(font_tex);
    font_tex = NULL;
    sprites_shutdown();
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

void render_frame(const GameState *state)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 12, 255);
    SDL_RenderClear(renderer);

    draw_starfield(state->hud.ticks);

    draw_beam(state);

    int anim = (int)(state->frame >> 3) & 1;
    bool results = state->phase == GAME_PHASE_GAME_OVER &&
                   state->hud.over_t >= GAMEOVER_RESULTS_AT;
    for (int i = 0; i < MAX_ENEMIES && !results; i++) {
        const Enemy *e = &state->enemies[i];
        if (!e->active)
            continue;
        /* art is 16x16 around a 14x14 hitbox; draw centered on the box */
        if (e->type == ENEMY_TYPE_CAPTIVE) {
            SDL_Rect dst = { e->x - 1, e->y - 1, 16, 16 };
            SDL_RenderCopyEx(renderer, sprites_captive(), NULL, &dst, 0.0, NULL, SDL_FLIP_VERTICAL);
            continue;
        }
        draw_sprite(sprites_enemy(e->type, anim, e->hp),
                    e->x - 1, e->y - 1, 16, 16);
    }

    for (int i = 0; i < MAX_BULLETS && !results; i++) {
        const Bullet *b = &state->bullets[i];
        if (!b->active)
            continue;
        draw_sprite(
            b->from_player ? sprites_bullet() : sprites_enemy_bullet(),
            b->x, b->y, b->w, b->h
        );
    }

    if (state->phase != GAME_PHASE_TITLE && state->phase != GAME_PHASE_GAME_OVER &&
        state->hud.dead_timer == 0) {
        draw_sprite(sprites_player(), state->player.x, state->player.y, 16, state->player.h);
        if (state->capture.dual)
            draw_sprite(sprites_player(), state->player.x + 16, state->player.y, 16, state->player.h);
    }

    draw_captive(state);

    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (state->explosions[i].active)
            draw_explosion(&state->explosions[i]);
    }

    draw_hud(state);

    if (state->hud.demo) {
        draw_text_centered("DEMO", 18, 1, 90, 230, 255);
        if (!((state->hud.ticks >> 5) & 1))
            draw_text_centered("PRESS FIRE", GALAGA_HEIGHT - 9, 1, 255, 255, 255);
    }

    if (state->phase == GAME_PHASE_TITLE) {
        draw_attract(state);
    } else if (state->phase == GAME_PHASE_PLAYING) {
        char banner[24];
        if (state->hud.banner == BANNER_PLAYER1) {
            draw_text_centered("PLAYER 1", 148, 1, 90, 230, 255);
        } else if (state->hud.banner == BANNER_STAGE) {
            if (state->challenge.active)
                snprintf(banner, sizeof(banner), "CHALLENGING STAGE");
            else
                snprintf(banner, sizeof(banner), "STAGE %d", state->player.stage);
            draw_text_centered(banner, 148, 1, 90, 230, 255);
        } else if (state->challenge.result == 1) {
            draw_challenge_result(state);
        } else if (state->capture.msg_timer > 0) {
            draw_text_centered("FIGHTER CAPTURED", 148, 1, 255, 40, 40);
        } else if (state->hud.dead_timer > 0 && state->hud.dead_timer <= PLAYER_READY_STEPS) {
            draw_text_centered("READY", 148, 1, 90, 230, 255);
        }
    } else if (state->phase == GAME_PHASE_GAME_OVER) {
        if (state->hud.over_t < GAMEOVER_RESULTS_AT)
            draw_text_centered("GAME OVER", 140, 1, 90, 230, 255);
        else
            draw_results(state);
    }

    SDL_RenderPresent(renderer);
}
