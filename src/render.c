#include "galaga.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static SDL_Window   *window;
static SDL_Renderer *renderer;
static uint32_t      star_color = 0;

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

static void draw_starfield(void)
{
    /* Deterministic star positions from stage seed */
    unsigned seed = star_color;
    for (int i = 0; i < 48; i++) {
        seed = seed * 1103515245u + 12345u;
        int x = (int)((seed >> 16) % GALAGA_WIDTH);
        seed = seed * 1103515245u + 12345u;
        int y = (int)((seed >> 16) % GALAGA_HEIGHT);
        draw_rect(x, y, 1, 1, 80, 80, 100);
    }
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

    star_color = (uint32_t)time(NULL);
}

void render_shutdown(void)
{
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

void render_frame(const GameState *state)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 12, 255);
    SDL_RenderClear(renderer);

    draw_starfield();

    for (int i = 0; i < MAX_ENEMIES; i++) {
        const Enemy *e = &state->enemies[i];
        if (!e->active)
            continue;
        Uint8 r = 80, g = 200, b = 80;
        if (e->type == 1) { r = 220; g = 200; b = 60; }
        if (e->type == 2) { r = 220; g = 60;  b = 60; }
        draw_rect(e->x, e->y, e->w, e->h, r, g, b);
    }

    for (int i = 0; i < MAX_BULLETS; i++) {
        const Bullet *b = &state->bullets[i];
        if (!b->active)
            continue;
        if (b->from_player)
            draw_rect(b->x, b->y, b->w, b->h, 255, 255, 80);
        else
            draw_rect(b->x, b->y, b->w, b->h, 255, 80, 80);
    }

    draw_rect(state->player.x, state->player.y, state->player.w, state->player.h, 80, 220, 255);

    /* Simple HUD bar */
    SDL_SetRenderDrawColor(renderer, 20, 20, 40, 255);
    SDL_Rect hud = { 0, 0, GALAGA_WIDTH * GALAGA_SCALE, 18 };
    SDL_RenderFillRect(renderer, &hud);

    char buf[128];
    snprintf(buf, sizeof(buf), "SCORE %d   LIVES %d   STAGE %d   [asm]",
             state->player.score, state->player.lives, state->player.stage);
    (void)buf; /* TODO: SDL_ttf for text; HUD values visible via window title for now */

    SDL_SetWindowTitle(window, buf);

    if (state->game_over) {
        SDL_SetRenderDrawColor(renderer, 200, 40, 40, 180);
        SDL_Rect overlay = { 0, GALAGA_HEIGHT * GALAGA_SCALE / 2 - 20,
                             GALAGA_WIDTH * GALAGA_SCALE, 40 };
        SDL_RenderFillRect(renderer, &overlay);
    }

    SDL_RenderPresent(renderer);
}
