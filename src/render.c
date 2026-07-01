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

    for (int i = 0; i < MAX_ENEMIES; i++) {
        const Enemy *e = &state->enemies[i];
        if (!e->active)
            continue;
        SDL_Texture *tex = sprites_enemy(e->type);
        if (e->state == ENEMY_STATE_DIVING && e->type == ENEMY_TYPE_BOSS && e->hp == 1) {
            SDL_SetTextureColorMod(tex, 255, 120, 120);
        } else {
            SDL_SetTextureColorMod(tex, 255, 255, 255);
        }
        draw_sprite(tex, e->x, e->y, e->w, e->h);
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

    draw_sprite(
        sprites_player(),
        state->player.x, state->player.y,
        state->player.w, state->player.h
    );

    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (state->explosions[i].active)
            draw_explosion(&state->explosions[i]);
    }

    char buf[160];
    snprintf(buf, sizeof(buf), "SCORE %d   HI %d   LIVES %d   STAGE %d",
             state->player.score, state->high_score,
             state->player.lives, state->player.stage);
    if (state->phase == GAME_PHASE_TITLE)
        strncat(buf, "   PRESS FIRE", sizeof(buf) - strlen(buf) - 1);
    else if (state->phase == GAME_PHASE_STAGE_CLEAR)
        strncat(buf, "   STAGE CLEAR", sizeof(buf) - strlen(buf) - 1);
    else if (state->phase == GAME_PHASE_GAME_OVER)
        strncat(buf, "   GAME OVER", sizeof(buf) - strlen(buf) - 1);
    SDL_SetWindowTitle(window, buf);

    if (state->game_over) {
        SDL_SetRenderDrawColor(renderer, 200, 40, 40, 180);
        SDL_Rect overlay = { 0, GALAGA_HEIGHT * GALAGA_SCALE / 2 - 20,
                             GALAGA_WIDTH * GALAGA_SCALE, 40 };
        SDL_RenderFillRect(renderer, &overlay);
    } else if (state->phase == GAME_PHASE_TITLE) {
        SDL_SetRenderDrawColor(renderer, 40, 40, 120, 200);
        SDL_Rect banner = { 20, GALAGA_HEIGHT * GALAGA_SCALE / 2 - 30,
                            GALAGA_WIDTH * GALAGA_SCALE - 40, 60 };
        SDL_RenderFillRect(renderer, &banner);
    } else if (state->phase == GAME_PHASE_STAGE_CLEAR) {
        SDL_SetRenderDrawColor(renderer, 40, 120, 40, 180);
        SDL_Rect banner = { 20, GALAGA_HEIGHT * GALAGA_SCALE / 2 - 20,
                            GALAGA_WIDTH * GALAGA_SCALE - 40, 40 };
        SDL_RenderFillRect(renderer, &banner);
    }

    SDL_RenderPresent(renderer);
}
