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

#define ENTITY_KIND_ENEMY  0
#define ENTITY_KIND_BULLET 1

#define ENEMY_TYPE_BEE       0
#define ENEMY_STATE_FORMATION 0
#define ENEMY_STATE_DIVING    1

typedef struct {
    int x, y, w, h;
} Rect;

typedef struct {
    bool active;
    int  x, y;
    int  w, h;
    int  type;
    int  state;
    int  home_x;
    int  home_y;
    int  vx;
    int  fire_cd;
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
    Player   player;
    Enemy    enemies[MAX_ENEMIES];
    Bullet   bullets[MAX_BULLETS];
    bool     running;
    bool     game_over;
    uint32_t frame;
} GameState;

/* --- Assembly-backed (or C fallback) --- */

int rect_overlap(const Rect *a, const Rect *b);

int  entity_spawn(GameState *state, int kind, int type, int x, int y);
void entity_kill(GameState *state, int kind, int index);
void entity_tick_all(GameState *state, int delta_px);
int  entity_any_active(const GameState *state, int kind);
void entities_spawn_demo_formation(GameState *state);

void player_init(GameState *state);
void player_clamp_x(Player *player);
void player_tick(GameState *state, int delta_px, int move_left, int move_right);
void player_try_fire(GameState *state);
int  player_lose_life(GameState *state);

void enemies_spawn_formation(GameState *state);
void enemies_tick_all(GameState *state, int delta_px);

/* --- C-only modules --- */

void game_init(GameState *state);
void game_tick(GameState *state, float dt, bool move_left, bool move_right, bool fire);
void input_poll(bool *move_left, bool *move_right, bool *fire, bool *quit);

void render_init(void);
void render_shutdown(void);
void render_frame(const GameState *state);

#endif /* GALAGA_H */
