#include "galaga.h"

#include <SDL.h>
#include <string.h>

static SDL_Texture *g_player;
static SDL_Texture *g_bee;
static SDL_Texture *g_butterfly;
static SDL_Texture *g_boss;
static SDL_Texture *g_bullet;
static SDL_Texture *g_ebullet;
static SDL_Renderer *g_renderer;

static void fill_rect(SDL_Surface *s, int x, int y, int w, int h, Uint32 c) {
    SDL_Rect r = {x, y, w, h};
    SDL_FillRect(s, &r, c);
}

static SDL_Texture *make_tex(SDL_Surface *s) {
    SDL_Texture *t = SDL_CreateTextureFromSurface(g_renderer, s);
    SDL_FreeSurface(s);
    return t;
}

static SDL_Texture *make_player(void) {
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, 16, 16, 32, SDL_PIXELFORMAT_RGBA8888);
    fill_rect(s, 6, 0, 4, 4, 0xFFFFFFFF);
    fill_rect(s, 4, 4, 8, 4, 0xFF00FFFF);
    fill_rect(s, 2, 8, 12, 4, 0xFF00CCFF);
    fill_rect(s, 0, 12, 16, 4, 0xFF0088FF);
    return make_tex(s);
}

static SDL_Texture *make_bee(void) {
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, 16, 16, 32, SDL_PIXELFORMAT_RGBA8888);
    fill_rect(s, 4, 2, 8, 4, 0xFFFFFF00);
    fill_rect(s, 2, 6, 12, 4, 0xFFFFCC00);
    fill_rect(s, 0, 10, 16, 4, 0xFFFF8800);
    fill_rect(s, 6, 0, 4, 2, 0xFFFFFFFF);
    return make_tex(s);
}

static SDL_Texture *make_butterfly(void) {
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, 16, 16, 32, SDL_PIXELFORMAT_RGBA8888);
    fill_rect(s, 0, 4, 4, 8, 0xFFFF00FF);
    fill_rect(s, 12, 4, 4, 8, 0xFFFF00FF);
    fill_rect(s, 4, 6, 8, 6, 0xFFCC00CC);
    fill_rect(s, 6, 2, 4, 4, 0xFFFFFFFF);
    return make_tex(s);
}

static SDL_Texture *make_boss(void) {
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, 16, 16, 32, SDL_PIXELFORMAT_RGBA8888);
    fill_rect(s, 2, 2, 12, 12, 0xFFFF0000);
    fill_rect(s, 4, 4, 8, 8, 0xFFCC0000);
    fill_rect(s, 6, 0, 4, 4, 0xFFFFFFFF);
    fill_rect(s, 0, 6, 2, 4, 0xFFFF4444);
    fill_rect(s, 14, 6, 2, 4, 0xFFFF4444);
    return make_tex(s);
}

static SDL_Texture *make_bullet(void) {
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, 4, 8, 32, SDL_PIXELFORMAT_RGBA8888);
    fill_rect(s, 1, 0, 2, 8, 0xFFFFFFFF);
    return make_tex(s);
}

static SDL_Texture *make_ebullet(void) {
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, 4, 8, 32, SDL_PIXELFORMAT_RGBA8888);
    fill_rect(s, 1, 0, 2, 8, 0xFFFF8800);
    return make_tex(s);
}

void sprites_init(SDL_Renderer *renderer) {
    g_renderer = renderer;
    g_player = make_player();
    g_bee = make_bee();
    g_butterfly = make_butterfly();
    g_boss = make_boss();
    g_bullet = make_bullet();
    g_ebullet = make_ebullet();
}

void sprites_shutdown(void) {
    SDL_DestroyTexture(g_player);
    SDL_DestroyTexture(g_bee);
    SDL_DestroyTexture(g_butterfly);
    SDL_DestroyTexture(g_boss);
    SDL_DestroyTexture(g_bullet);
    SDL_DestroyTexture(g_ebullet);
    g_player = g_bee = g_butterfly = g_boss = g_bullet = g_ebullet = NULL;
}

SDL_Texture *sprites_player(void) { return g_player; }
SDL_Texture *sprites_bullet(void) { return g_bullet; }
SDL_Texture *sprites_enemy_bullet(void) { return g_ebullet; }

SDL_Texture *sprites_enemy(int type) {
    switch (type) {
    case ENEMY_TYPE_BEE: return g_bee;
    case ENEMY_TYPE_BUTTERFLY: return g_butterfly;
    case ENEMY_TYPE_BOSS: return g_boss;
    default: return g_bee;
    }
}
