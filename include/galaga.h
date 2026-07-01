#ifndef GALAGA_H
#define GALAGA_H

#include <stdbool.h>
#include <stdint.h>

#define GALAGA_WIDTH  224
#define GALAGA_HEIGHT 288
#define GALAGA_SCALE  2

#define MAX_ENEMIES 40
#define MAX_BULLETS 8
#define MAX_COLLISION_HITS 8
#define MAX_EXPLOSIONS 12

#define ENTITY_KIND_ENEMY  0
#define ENTITY_KIND_BULLET 1

#define ENEMY_TYPE_BEE       0
#define ENEMY_TYPE_BUTTERFLY 1
#define ENEMY_TYPE_BOSS      2

#define ENEMY_STATE_FORMATION 0
#define ENEMY_STATE_DIVING    1
#define ENEMY_STATE_ENTERING  2

#define HIT_PLAYER_BULLET_ENEMY 0
#define HIT_ENEMY_BULLET_PLAYER 1
#define HIT_ENEMY_BODY_PLAYER   2

#define GAME_PHASE_TITLE       0
#define GAME_PHASE_PLAYING     1
#define GAME_PHASE_STAGE_CLEAR 2
#define GAME_PHASE_GAME_OVER   3

#define SCORE_BEE_FORMATION       50
#define SCORE_BEE_DIVING          100
#define SCORE_BUTTERFLY_FORMATION 80
#define SCORE_BUTTERFLY_DIVING    160
#define SCORE_BOSS_FORMATION      150
#define SCORE_BOSS_DIVING         400

#define STAGE_CLEAR_FRAMES 90
#define EXPLOSION_FRAMES   12

#define HIGH_SCORE_FILE "galaga_hi.txt"

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
    int count;
    int type[MAX_COLLISION_HITS];
    int bullet_idx[MAX_COLLISION_HITS];
    int enemy_idx[MAX_COLLISION_HITS];
} CollisionHits;

typedef struct {
    bool active;
    int  x, y;
    int  timer;
} Explosion;

typedef struct {
    Player        player;
    Enemy         enemies[MAX_ENEMIES];
    Bullet        bullets[MAX_BULLETS];
    bool          running;
    bool          game_over;
    uint32_t      frame;
    int           phase;
    int           clear_timer;
    int           next_extra_life;
    int           high_score;
    CollisionHits last_hits;
    Explosion     explosions[MAX_EXPLOSIONS];
} GameState;

int  rect_overlap(const Rect *a, const Rect *b);
void collision_resolve(GameState *state, CollisionHits *hits);

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

void score_add_points(GameState *state, int points);

void game_state_init(GameState *state);
void game_state_tick(GameState *state, int fire_pressed);
void game_state_begin_stage_clear(GameState *state);
int  game_state_is_playing(const GameState *state);

void effects_spawn_explosion(GameState *gs, int x, int y);
void effects_process_hits(GameState *gs);
void effects_tick(GameState *gs);

void high_score_load(GameState *state);
void high_score_save(const GameState *state);

void game_init(GameState *state);
void game_tick(GameState *state, float dt, bool move_left, bool move_right, bool fire);
void input_poll(bool *move_left, bool *move_right, bool *fire, bool *quit);

struct SDL_Renderer;
struct SDL_Texture;
void sprites_init(struct SDL_Renderer *renderer);
void sprites_shutdown(void);
struct SDL_Texture *sprites_player(void);
struct SDL_Texture *sprites_bullet(void);
struct SDL_Texture *sprites_enemy_bullet(void);
struct SDL_Texture *sprites_enemy(int type);
void render_init(void);
void render_shutdown(void);
void render_frame(const GameState *state);

#endif /* GALAGA_H */
