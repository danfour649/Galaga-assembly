#ifndef GALAGA_H
#define GALAGA_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#define GALAGA_WIDTH  224
#define GALAGA_HEIGHT 288
#define GALAGA_SCALE  2   /* minimum initial window scale */

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

#define EXPLOSION_ENEMY  0
#define EXPLOSION_PLAYER 1
#define PLAYER_EXPLOSION_FRAMES 48

typedef struct {
    bool active;
    int  x, y;
    int  timer;
    int  kind;      /* EXPLOSION_* */
} Explosion;

#define BANNER_NONE     0
#define BANNER_PLAYER1  1
#define BANNER_STAGE    2
#define BANNER_READY    3

#define BANNER_PLAYER1_STEPS 110
#define BANNER_STAGE_STEPS   100
#define PLAYER_DEAD_STEPS    170   /* explosion + pause + READY */
#define PLAYER_READY_STEPS   70    /* last part of the pause shows READY */

/* Presentation state owned by game.c (C on every backend). */
typedef struct {
    uint32_t ticks;        /* sim steps in every phase (blink, starfield) */
    int      banner;       /* BANNER_* */
    int      banner_timer;
    int      dead_timer;   /* >0 while the fighter is exploded / respawning */
} Hud;

/* Per-enemy flight data for src/flight.c. Lives at the END of GameState so
   every offset the NASM modules use (include/galaga.inc) is unchanged. */
typedef struct {
    float fx, fy;        /* sub-pixel position */
    float heading;       /* degrees, 0 = right, 90 = down (screen space) */
    float speed;         /* px per 60 Hz sim step */
    int   mode;          /* FLIGHT_* */
    int   prog;          /* path program index */
    int   seg;           /* current segment in the program */
    int   seg_t;         /* steps spent in the current segment */
    int   timer;         /* wait countdown / attack step counter */
    int   col, row;      /* formation slot */
    int   side;          /* +1 / -1 mirror for paths */
    int   leader;        /* escort: index of the Boss it follows, else -1 */
    float esc_dx, esc_dy;
    int   escorts_lost;  /* Boss: escorts shot down during this dive */
} Flight;

typedef struct {
    Flight   f[MAX_ENEMIES];
    uint32_t rng;
    int      t;            /* steps since the stage started */
    int      dive_timer;
    int      formed;       /* every wave has arrived */
    int      breath_t;     /* breathing phase once formed */
} FlightState;

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
    FlightState   flight;   /* C-only from here: not mirrored in galaga.inc */
    Hud           hud;
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

/* Arcade-style flight (src/flight.c, portable C on every backend). */
void flight_begin_stage(GameState *state);
void flight_tick(GameState *state);
void flight_on_hits(GameState *state);
void flight_slot_pos(const GameState *state, int col, int row, float *x, float *y);
void enemies_tick_all(GameState *state, int delta_px);

void score_add_points(GameState *state, int points);

void game_state_init(GameState *state);
void game_state_tick(GameState *state, int fire_pressed);
void game_state_begin_stage_clear(GameState *state);
int  game_state_is_playing(const GameState *state);

void effects_spawn_explosion(GameState *gs, int x, int y);
void effects_spawn_explosion_kind(GameState *gs, int x, int y, int kind);
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
struct SDL_Texture *sprites_enemy(int type, int frame, int hp);
void render_init(void);
void render_shutdown(void);
void render_toggle_fullscreen(void);
void render_frame(const GameState *state);

/* Offsets must match include/galaga.inc. A drift here silently breaks NASM. */
_Static_assert(offsetof(Player, x) == 0, "P_X");
_Static_assert(offsetof(Player, y) == 4, "P_Y");
_Static_assert(offsetof(Player, w) == 8, "P_W");
_Static_assert(offsetof(Player, h) == 12, "P_H");
_Static_assert(offsetof(Player, lives) == 16, "P_LIVES");
_Static_assert(offsetof(Player, score) == 20, "P_SCORE");
_Static_assert(offsetof(Player, stage) == 24, "P_STAGE");
_Static_assert(sizeof(Player) == 28, "player size");

_Static_assert(offsetof(Enemy, active) == 0, "E_ACTIVE");
_Static_assert(offsetof(Enemy, x) == 4, "E_X");
_Static_assert(offsetof(Enemy, y) == 8, "E_Y");
_Static_assert(offsetof(Enemy, w) == 12, "E_W");
_Static_assert(offsetof(Enemy, h) == 16, "E_H");
_Static_assert(offsetof(Enemy, type) == 20, "E_TYPE");
_Static_assert(offsetof(Enemy, state) == 24, "E_STATE");
_Static_assert(offsetof(Enemy, home_x) == 28, "E_HOME_X");
_Static_assert(offsetof(Enemy, home_y) == 32, "E_HOME_Y");
_Static_assert(offsetof(Enemy, vx) == 36, "E_VX");
_Static_assert(offsetof(Enemy, fire_cd) == 40, "E_FIRE_CD");
_Static_assert(offsetof(Enemy, hp) == 44, "E_HP");
_Static_assert(sizeof(Enemy) == 48, "ENEMY_SIZE");

_Static_assert(offsetof(Bullet, active) == 0, "B_ACTIVE");
_Static_assert(offsetof(Bullet, x) == 4, "B_X");
_Static_assert(offsetof(Bullet, y) == 8, "B_Y");
_Static_assert(offsetof(Bullet, w) == 12, "B_W");
_Static_assert(offsetof(Bullet, h) == 16, "B_H");
_Static_assert(offsetof(Bullet, from_player) == 20, "B_FROM_PLAYER");
_Static_assert(sizeof(Bullet) == 24, "BULLET_SIZE");

_Static_assert(offsetof(CollisionHits, count) == 0, "H_OFF_COUNT");
_Static_assert(offsetof(CollisionHits, type) == 4, "H_OFF_TYPE");
_Static_assert(offsetof(CollisionHits, bullet_idx) == 36, "H_OFF_BULLET");
_Static_assert(offsetof(CollisionHits, enemy_idx) == 68, "H_OFF_ENEMY");

_Static_assert(offsetof(GameState, player) == 0, "GS_PLAYER");
_Static_assert(offsetof(GameState, enemies) == 28, "GS_ENEMIES");
_Static_assert(offsetof(GameState, bullets) == 1948, "GS_BULLETS");
_Static_assert(offsetof(GameState, running) == 2140, "GS_RUNNING");
_Static_assert(offsetof(GameState, game_over) == 2141, "GS_GAME_OVER");
_Static_assert(offsetof(GameState, frame) == 2144, "GS_FRAME");
_Static_assert(offsetof(GameState, phase) == 2148, "GS_PHASE");
_Static_assert(offsetof(GameState, clear_timer) == 2152, "GS_CLEAR_TIMER");
_Static_assert(offsetof(GameState, next_extra_life) == 2156, "GS_NEXT_EXTRA_LIFE");
_Static_assert(offsetof(GameState, high_score) == 2160, "GS_HIGH_SCORE");

_Static_assert(offsetof(Rect, x) == 0, "Rect.x");
_Static_assert(offsetof(Rect, y) == 4, "Rect.y");
_Static_assert(offsetof(Rect, w) == 8, "Rect.w");
_Static_assert(offsetof(Rect, h) == 12, "Rect.h");

#endif /* GALAGA_H */
