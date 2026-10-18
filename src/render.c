#include "galaga.h"

#include <SDL.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static SDL_Window   *window;
static SDL_Renderer *renderer;
static uint32_t      star_seed = 0;

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

/* --- 3x5 bitmap font, one Uint8 per row, low 3 bits used --- */

static const Uint8 FONT_DIGITS[10][5] = {
    {7,5,5,5,7}, {2,6,2,2,7}, {7,1,7,4,7}, {7,1,3,1,7}, {5,5,7,1,1},
    {7,4,7,1,7}, {7,4,7,5,7}, {7,1,1,2,2}, {7,5,7,5,7}, {7,5,7,1,7},
};

static const Uint8 FONT_LETTERS[26][5] = {
    {2,5,7,5,5}, /* A */ {6,5,6,5,6}, /* B */ {3,4,4,4,3}, /* C */
    {6,5,5,5,6}, /* D */ {7,4,6,4,7}, /* E */ {7,4,6,4,4}, /* F */
    {3,4,5,5,3}, /* G */ {5,5,7,5,5}, /* H */ {7,2,2,2,7}, /* I */
    {1,1,1,5,2}, /* J */ {5,6,4,6,5}, /* K */ {4,4,4,4,7}, /* L */
    {5,7,7,5,5}, /* M */ {6,5,5,5,5}, /* N */ {7,5,5,5,7}, /* O */
    {6,5,6,4,4}, /* P */ {7,5,5,7,1}, /* Q */ {6,5,6,5,5}, /* R */
    {3,4,2,1,6}, /* S */ {7,2,2,2,2}, /* T */ {5,5,5,5,7}, /* U */
    {5,5,5,5,2}, /* V */ {5,5,7,7,5}, /* W */ {5,5,2,5,5}, /* X */
    {5,5,2,2,2}, /* Y */ {7,1,2,4,7}, /* Z */
};

static const Uint8 FONT_DASH[5] = {0,0,7,0,0};

static const Uint8 *font_glyph(char c)
{
    if (c >= '0' && c <= '9') return FONT_DIGITS[c - '0'];
    if (c >= 'A' && c <= 'Z') return FONT_LETTERS[c - 'A'];
    if (c >= 'a' && c <= 'z') return FONT_LETTERS[c - 'a'];
    if (c == '-') return FONT_DASH;
    return NULL;
}

/* Each character cell is 4x6 game pixels (3x5 glyph + spacing) times scale. */
static void draw_text(const char *s, int x, int y, int scale,
                      Uint8 r, Uint8 g, Uint8 b)
{
    for (; *s; s++, x += 4 * scale) {
        const Uint8 *gl = font_glyph(*s);
        if (!gl)
            continue;
        for (int row = 0; row < 5; row++) {
            for (int col = 0; col < 3; col++) {
                if (gl[row] & (4 >> col))
                    draw_rect(x + col * scale, y + row * scale,
                              scale, scale, r, g, b);
            }
        }
    }
}

static int text_width(const char *s, int scale)
{
    return (int)strlen(s) * 4 * scale - scale;
}

static void draw_text_centered(const char *s, int y, int scale,
                               Uint8 r, Uint8 g, Uint8 b)
{
    draw_text(s, (GALAGA_WIDTH - text_width(s, scale)) / 2, y, scale, r, g, b);
}

static void draw_starfield(uint32_t frame, int stage)
{
    unsigned seed = star_seed + stage * 7919u;
    int layer = (int)(frame % 48);
    for (int i = 0; i < 56; i++) {
        seed = seed * 1103515245u + 12345u;
        int x = (int)((seed >> 16) % GALAGA_WIDTH);
        seed = seed * 1103515245u + 12345u;
        int y = (int)(((seed >> 16) + layer * (1 + (i & 3))) % GALAGA_HEIGHT);
        Uint8 b = (Uint8)(60 + (i % 4) * 20);
        draw_rect(x, y, 1, 1, b, b, (Uint8)(b + 20));
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

static void draw_hud(const GameState *state)
{
    char buf[32];

    /* 1UP blinks while a game is on, as in the arcade */
    if (state->phase == GAME_PHASE_TITLE || ((state->hud.ticks >> 4) & 1) == 0)
        draw_text("1UP", 16, 2, 1, 255, 40, 40);
    snprintf(buf, sizeof(buf), "%d", state->player.score);
    draw_text(buf, 12, 9, 1, 255, 255, 255);

    draw_text_centered("HIGH SCORE", 2, 1, 255, 40, 40);
    snprintf(buf, sizeof(buf), "%d", state->high_score);
    draw_text_centered(buf, 9, 1, 255, 255, 255);

    /* reserve ships, bottom-left */
    int reserve = state->player.lives - 1;
    if (reserve > 6)
        reserve = 6;
    for (int i = 0; i < reserve; i++)
        draw_sprite(sprites_player(), 4 + i * 12, GALAGA_HEIGHT - 12, 10, 10);

    if (state->phase != GAME_PHASE_TITLE)
        draw_stage_badges(state->player.stage);
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

    star_seed = (uint32_t)time(NULL);
    sprites_init(renderer);
}

void render_toggle_fullscreen(void)
{
    Uint32 flags = SDL_GetWindowFlags(window);
    SDL_SetWindowFullscreen(window,
        (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
}

void render_shutdown(void)
{
    sprites_shutdown();
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

void render_frame(const GameState *state)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 12, 255);
    SDL_RenderClear(renderer);

    draw_starfield(state->frame, state->player.stage);

    int anim = (int)(state->frame >> 3) & 1;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        const Enemy *e = &state->enemies[i];
        if (!e->active)
            continue;
        /* art is 16x16 around a 14x14 hitbox; draw centered on the box */
        draw_sprite(sprites_enemy(e->type, anim, e->hp),
                    e->x - 1, e->y - 1, 16, 16);
    }

    for (int i = 0; i < MAX_BULLETS; i++) {
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
        draw_sprite(
            sprites_player(),
            state->player.x, state->player.y,
            state->player.w, state->player.h
        );
    }

    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (state->explosions[i].active)
            draw_explosion(&state->explosions[i]);
    }

    draw_hud(state);

    if (state->phase == GAME_PHASE_TITLE) {
        draw_text_centered("GALAGA", 96, 3, 255, 40, 40);
        if (!((state->hud.ticks >> 5) & 1))
            draw_text_centered("PRESS FIRE TO START", 150, 1, 255, 255, 255);
        draw_text_centered("ASSEMBLY EDITION", 170, 1, 90, 200, 255);
    } else if (state->phase == GAME_PHASE_PLAYING) {
        char banner[24];
        if (state->hud.banner == BANNER_PLAYER1) {
            draw_text_centered("PLAYER 1", 140, 2, 90, 230, 255);
        } else if (state->hud.banner == BANNER_STAGE) {
            snprintf(banner, sizeof(banner), "STAGE %d", state->player.stage);
            draw_text_centered(banner, 140, 2, 90, 230, 255);
        } else if (state->hud.dead_timer > 0 && state->hud.dead_timer <= PLAYER_READY_STEPS) {
            draw_text_centered("READY", 140, 2, 90, 230, 255);
        }
    } else if (state->phase == GAME_PHASE_GAME_OVER) {
        draw_text_centered("GAME OVER", 130, 2, 255, 40, 40);
        draw_text_centered("PRESS FIRE", 150, 1, 255, 255, 255);
    }

    SDL_RenderPresent(renderer);
}
