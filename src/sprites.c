#include "galaga.h"

#include <SDL.h>
#include <string.h>

/*
 * Pixel-art sprites authored as 16x16 character maps.
 * '.' is transparent; other characters index into a per-sprite palette.
 * Enemies have two animation frames (wing flap); the boss has a second
 * palette for its damaged (1 hp) form, like the arcade original.
 */

typedef struct {
    char  key;
    Uint8 r, g, b;
} PalEntry;

static SDL_Texture *g_player;
static SDL_Texture *g_bee[2];
static SDL_Texture *g_butterfly[2];
static SDL_Texture *g_boss[2];
static SDL_Texture *g_boss_hit[2];
static SDL_Texture *g_bullet;
static SDL_Texture *g_ebullet;
static SDL_Renderer *g_renderer;

static Uint32 pal_lookup(SDL_Surface *s, const PalEntry *pal, int n, char key)
{
    for (int i = 0; i < n; i++) {
        if (pal[i].key == key)
            return SDL_MapRGBA(s->format, pal[i].r, pal[i].g, pal[i].b, 255);
    }
    return SDL_MapRGBA(s->format, 0, 0, 0, 0);
}

static SDL_Texture *tex_from_art(const char *const rows[], int w, int h,
                                 const PalEntry *pal, int pal_len)
{
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA8888);
    Uint32 *px = (Uint32 *)s->pixels;
    int pitch = s->pitch / 4;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            char c = rows[y][x];
            px[y * pitch + x] = (c == '.')
                ? SDL_MapRGBA(s->format, 0, 0, 0, 0)
                : pal_lookup(s, pal, pal_len, c);
        }
    }
    SDL_Texture *t = SDL_CreateTextureFromSurface(g_renderer, s);
    SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
    SDL_FreeSurface(s);
    return t;
}

/* --- player fighter: white hull, red trim, blue cockpit --- */

static const PalEntry PAL_PLAYER[] = {
    { 'W', 236, 236, 236 },
    { 'R', 224,  48,  48 },
    { 'B',  80, 160, 255 },
};

static const char *const ART_PLAYER[16] = {
    ".......WW.......",
    ".......WW.......",
    "......WWWW......",
    "......WBBW......",
    "......WBBW......",
    "..R...WWWW...R..",
    "..R..WWWWWW..R..",
    "..W..WWWWWW..W..",
    "..W.WWWRRWWW.W..",
    "..WWWWRRRRWWWW..",
    ".WWWWWWRRWWWWWW.",
    ".WWWWWWWWWWWWWW.",
    ".WW..WWWWWW..WW.",
    ".W....WWWW....W.",
    "......W..W......",
    "................",
};

/* --- bee (Zako): yellow wings, blue body --- */

static const PalEntry PAL_BEE[] = {
    { 'Y', 255, 216,  48 },
    { 'B',  64, 120, 255 },
};

static const char *const ART_BEE_A[16] = {
    "................",
    "....B......B....",
    ".....B....B.....",
    "..Y..BBBBBB..Y..",
    ".YYY.BYBBYB.YYY.",
    ".YYYYBBBBBBYYYY.",
    "..YYYBBBBBBYYY..",
    "...YYBBBBBBYY...",
    "....YBBBBBBY....",
    "....YYBBBBYY....",
    ".....Y.BB.Y.....",
    ".......BB.......",
    "......B..B......",
    "................",
    "................",
    "................",
};

static const char *const ART_BEE_B[16] = {
    "................",
    "....B......B....",
    ".....B....B.....",
    ".....BBBBBB.....",
    "....BBYBBYBB....",
    ".YYYBBBBBBBBYYY.",
    ".YYYYBBBBBBYYYY.",
    "..YY.BBBBBB.YY..",
    "....YBBBBBBY....",
    "....YYBBBBYY....",
    ".....Y.BB.Y.....",
    ".......BB.......",
    "......B..B......",
    "................",
    "................",
    "................",
};

/* --- butterfly (Goei): red body, white wings --- */

static const PalEntry PAL_BUTTERFLY[] = {
    { 'R', 230,  40,  60 },
    { 'W', 240, 240, 240 },
};

static const char *const ART_BUTTERFLY_A[16] = {
    "................",
    "....R......R....",
    ".....R....R.....",
    "....WRRRRRRW....",
    "...WWRRWWRRWW...",
    "..WWWRRRRRRWWW..",
    ".WWWWRRRRRRWWWW.",
    ".WWW.RRRRRR.WWW.",
    "..WW.RRRRRR.WW..",
    "..W...RRRR...W..",
    "......RRRR......",
    ".......RR.......",
    "................",
    "................",
    "................",
    "................",
};

static const char *const ART_BUTTERFLY_B[16] = {
    "................",
    "....R......R....",
    ".WW..R....R..WW.",
    ".WWWWRRRRRRWWWW.",
    "..WWWRRWWRRWWW..",
    "...WWRRRRRRWW...",
    "....WRRRRRRW....",
    ".....RRRRRR.....",
    "......RRRR......",
    "......RRRR......",
    ".......RR.......",
    "................",
    "................",
    "................",
    "................",
    "................",
};

/* --- boss Galaga: green, turns purple after the first hit --- */

static const PalEntry PAL_BOSS[] = {
    { 'G',  80, 200,  80 },
    { 'P', 190,  90, 255 },
};

static const PalEntry PAL_BOSS_HIT[] = {
    { 'G', 170,  80, 255 },
    { 'P', 255, 100, 180 },
};

static const char *const ART_BOSS_A[16] = {
    "................",
    "....G......G....",
    ".....G....G.....",
    "...GGGGGGGGGG...",
    "..GGGPPGGPPGGG..",
    ".GGGGPPGGPPGGGG.",
    ".GGGGGGGGGGGGGG.",
    ".GG.GGGGGGGG.GG.",
    ".G..GGGGGGGG..G.",
    "....GGG..GGG....",
    "....GG....GG....",
    "....G......G....",
    "................",
    "................",
    "................",
    "................",
};

static const char *const ART_BOSS_B[16] = {
    "................",
    "....G......G....",
    ".....G....G.....",
    "...GGGGGGGGGG...",
    "..GGGPPGGPPGGG..",
    ".GGGGPPGGPPGGGG.",
    ".GGGGGGGGGGGGGG.",
    ".G..GGGGGGGG..G.",
    ".GG.GGGGGGGG.GG.",
    "....GGG..GGG....",
    "....GG....GG....",
    ".....G....G.....",
    "................",
    "................",
    "................",
    "................",
};

/* --- bullets --- */

static const PalEntry PAL_BULLET[] = {
    { 'W', 255, 255, 255 },
    { 'C',  90, 200, 255 },
};

static const char *const ART_BULLET[8] = {
    ".W..",
    ".W..",
    ".WW.",
    ".WW.",
    ".WW.",
    ".CC.",
    ".CC.",
    ".C..",
};

static const PalEntry PAL_EBULLET[] = {
    { 'Y', 255, 220,  80 },
    { 'R', 255,  80,  40 },
};

static const char *const ART_EBULLET[8] = {
    ".Y..",
    ".YY.",
    ".YY.",
    ".RR.",
    ".RR.",
    ".RR.",
    ".R..",
    ".R..",
};

#define PAL_N(p) ((int)(sizeof(p) / sizeof((p)[0])))

void sprites_init(SDL_Renderer *renderer)
{
    g_renderer = renderer;
    g_player       = tex_from_art(ART_PLAYER,      16, 16, PAL_PLAYER,    PAL_N(PAL_PLAYER));
    g_bee[0]       = tex_from_art(ART_BEE_A,       16, 16, PAL_BEE,       PAL_N(PAL_BEE));
    g_bee[1]       = tex_from_art(ART_BEE_B,       16, 16, PAL_BEE,       PAL_N(PAL_BEE));
    g_butterfly[0] = tex_from_art(ART_BUTTERFLY_A, 16, 16, PAL_BUTTERFLY, PAL_N(PAL_BUTTERFLY));
    g_butterfly[1] = tex_from_art(ART_BUTTERFLY_B, 16, 16, PAL_BUTTERFLY, PAL_N(PAL_BUTTERFLY));
    g_boss[0]      = tex_from_art(ART_BOSS_A,      16, 16, PAL_BOSS,      PAL_N(PAL_BOSS));
    g_boss[1]      = tex_from_art(ART_BOSS_B,      16, 16, PAL_BOSS,      PAL_N(PAL_BOSS));
    g_boss_hit[0]  = tex_from_art(ART_BOSS_A,      16, 16, PAL_BOSS_HIT,  PAL_N(PAL_BOSS_HIT));
    g_boss_hit[1]  = tex_from_art(ART_BOSS_B,      16, 16, PAL_BOSS_HIT,  PAL_N(PAL_BOSS_HIT));
    g_bullet       = tex_from_art(ART_BULLET,       4,  8, PAL_BULLET,    PAL_N(PAL_BULLET));
    g_ebullet      = tex_from_art(ART_EBULLET,      4,  8, PAL_EBULLET,   PAL_N(PAL_EBULLET));
}

void sprites_shutdown(void)
{
    SDL_DestroyTexture(g_player);
    for (int i = 0; i < 2; i++) {
        SDL_DestroyTexture(g_bee[i]);
        SDL_DestroyTexture(g_butterfly[i]);
        SDL_DestroyTexture(g_boss[i]);
        SDL_DestroyTexture(g_boss_hit[i]);
        g_bee[i] = g_butterfly[i] = g_boss[i] = g_boss_hit[i] = NULL;
    }
    SDL_DestroyTexture(g_bullet);
    SDL_DestroyTexture(g_ebullet);
    g_player = g_bullet = g_ebullet = NULL;
}

SDL_Texture *sprites_player(void) { return g_player; }
SDL_Texture *sprites_bullet(void) { return g_bullet; }
SDL_Texture *sprites_enemy_bullet(void) { return g_ebullet; }

SDL_Texture *sprites_enemy(int type, int frame, int hp)
{
    frame &= 1;
    switch (type) {
    case ENEMY_TYPE_BUTTERFLY: return g_butterfly[frame];
    case ENEMY_TYPE_BOSS:      return (hp <= 1) ? g_boss_hit[frame] : g_boss[frame];
    default:                   return g_bee[frame];
    }
}
