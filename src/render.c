#include "galaga.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static SDL_Window   *window;
static SDL_Renderer *renderer;
static uint32_t      star_seed = 0;

static void draw_sprite(SDL_Texture *tex, int x, int y, int w, int h)
{
    SDL_Rect dst = {
        x * GALAGA_SCALE,
        y * GALAGA_SCALE,
        w * GALAGA_SCALE,
        h * GALAGA_SCALE
    };
    SDL_RenderCopy(renderer, tex, NULL, &dst);
}

static void draw_rect(int x, int y, int w, int h, Uint8 r, Uint8 g, Uint8 b)
{
    SDL_Rect rect = {
        x * GALAGA_SCALE,
        y * GALAGA_SCALE,
        w * GALAGA_SCALE,
        h * GALAGA_SCALE
    };
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

static void draw_explosion(const Explosion *e)
{
    int age = EXPLOSION_FRAMES - e->timer;
    int size = 6 + age * 2;
    Uint8 alpha = (Uint8)(255 - age * 18);
    SDL_SetRenderDrawColor(renderer, 255, 200, 40, alpha);
    SDL_Rect r = {
        (e->x - size / 2) * GALAGA_SCALE,
        (e->y - size / 2) * GALAGA_SCALE,
        size * GALAGA_SCALE,
        size * GALAGA_SCALE
    };
    SDL_RenderFillRect(renderer, &r);
    SDL_SetRenderDrawColor(renderer, 255, 80, 20, alpha);
    r.x += GALAGA_SCALE;
    r.y += GALAGA_SCALE;
    r.w -= 2 * GALAGA_SCALE;
    r.h -= 2 * GALAGA_SCALE;
    if (r.w > 0 && r.h > 0)
        SDL_RenderFillRect(renderer, &r);
}

static void draw_hud(const GameState *state)
{
    char buf[32];

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

    snprintf(buf, sizeof(buf), "STAGE %d", state->player.stage);
    draw_text(buf, GALAGA_WIDTH - text_width(buf, 1) - 4, GALAGA_HEIGHT - 9,
              1, 90, 200, 255);
}

void render_init(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        exit(1);
    }

    int win_w = GALAGA_WIDTH * GALAGA_SCALE;
    int win_h = GALAGA_HEIGHT * GALAGA_SCALE;

    window = SDL_CreateWindow(
        "Galaga-assembly",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        win_w,
        win_h,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        exit(1);
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    star_seed = (uint32_t)time(NULL);
    sprites_init(renderer);
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

    if (state->phase != GAME_PHASE_TITLE) {
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
        if (!((state->frame >> 5) & 1))
            draw_text_centered("PRESS FIRE TO START", 150, 1, 255, 255, 255);
        draw_text_centered("ASSEMBLY EDITION", 170, 1, 90, 200, 255);
    } else if (state->phase == GAME_PHASE_STAGE_CLEAR) {
        draw_text_centered("STAGE CLEAR", 130, 2, 90, 200, 255);
    } else if (state->phase == GAME_PHASE_GAME_OVER) {
        draw_text_centered("GAME OVER", 130, 2, 255, 40, 40);
        draw_text_centered("PRESS FIRE", 150, 1, 255, 255, 255);
    }

    SDL_RenderPresent(renderer);
}
