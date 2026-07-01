#ifndef GALAGA_H
#define GALAGA_H

#include <stdbool.h>
#include <stdint.h>

/* Logical game resolution (classic arcade aspect; scaled up at render time) */
#define GALAGA_WIDTH  224
#define GALAGA_HEIGHT 288
#define GALAGA_SCALE  2

#define MAX_ENEMIES 40
#define MAX_BULLETS 8

typedef struct {
    int x, y, w, h;
} Rect;

typedef struct {
    bool active;
    int  x, y;
    int  w, h;
    int  type;   /* 0=bee, 1=butterfly, 2=boss (future) */
    int  hp;
} Enemy;

typedef struct {
    bool active;
    int  x, y;
    int  w, h;
    bool from_player;
} Bullet;

typedef struct {
    int  x, y;
    int  w, h;
    int  lives;
    int  score;
    int  stage;
} Player;

typedef struct {
    Player  player;
    Enemy   enemies[MAX_ENEMIES];
    Bullet  bullets[MAX_BULLETS];
    bool    running;
    bool    game_over;
} GameState;

/* --- Assembly-backed (or C fallback) --- */

/* Returns 1 if rectangles overlap, 0 otherwise. */
int rect_overlap(const Rect *a, const Rect *b);

/* --- C-only modules --- */

void game_init(GameState *state);
void game_tick(GameState *state, float dt, bool move_left, bool move_right, bool fire);
void input_poll(bool *move_left, bool *move_right, bool *fire, bool *quit);

void render_init(void);
void render_shutdown(void);
void render_frame(const GameState *state);

#endif /* GALAGA_H */
