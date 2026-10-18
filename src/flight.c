/*
 * flight.c: arcade-style enemy flight, shared by every backend.
 *
 *  - 40-enemy rack in the arcade layout: 4 Boss Galaga, 16 butterflies
 *    (Goei) in two rows of 8, 20 bees (Zako) in two rows of 10.
 *  - Five waves of eight fly in on curved paths, single file, from the top
 *    and from the lower sides. The rack sways side to side while they
 *    arrive, then "breathes" (spreads out and back) once everyone is home.
 *  - Dives start with a half loop up and outward, then curve toward the
 *    player. Butterflies weave. A Boss can take up to two butterfly escorts.
 *    Divers that leave the screen re-enter from the top and fly home.
 *
 * Paths are "turtle" programs: a heading, a speed, and segments of
 * (steps, degrees turned per step). Mirroring a path flips it left/right.
 *
 * This replaces enemies_tick_all / enemies_spawn_formation in game.c on
 * every platform. Those routines still exist in asm/enemies.asm and
 * src/game_fallback.c (and are still unit-tested) but are not used by the
 * game loop. Flight data lives at the end of GameState, so none of the NASM
 * struct offsets change.
 */
#include "galaga.h"

#include <math.h>
#include <string.h>

#define FLIGHT_NONE      0  /* not driven by flight.c (tests, legacy) */
#define FLIGHT_WAIT      1  /* off screen, waiting for its wave */
#define FLIGHT_PATH      2  /* following an entry path program */
#define FLIGHT_SEEK      3  /* homing on its formation slot */
#define FLIGHT_FORMED    4  /* sitting in the rack */
#define FLIGHT_DIVE_LOOP 5  /* half loop at the start of a dive */
#define FLIGHT_ATTACK    6  /* diving at the player */
#define FLIGHT_ESCORT    7  /* following a diving Boss */

#define PI_F 3.14159265f
#define DEG(a) ((a) * (PI_F / 180.0f))

#define RACK_CX      112.0f
#define RACK_TOP     34.0f
#define COL_GAP      16.0f
#define ROW_GAP      16.0f
#define HALF         7.0f      /* enemy hitbox is 14x14 */

typedef struct { int steps; float turn; } Seg;
typedef struct { float x, y, heading, speed; Seg seg[4]; } Prog;

enum { PROG_TOP, PROG_BOTTOM, PROG_COUNT };

/* Defined for side = +1; side = -1 mirrors around x = 112. */
static const Prog PROGS[PROG_COUNT] = {
    /* down from the top centre, sweep out and loop back up */
    { 98.0f, -10.0f, 90.0f, 2.6f, { {26, 0.0f}, {70, 3.6f}, {0, 0} } },
    /* in from the lower left, climb and loop toward the centre */
    { -10.0f, 250.0f, -38.0f, 2.6f, { {30, 0.0f}, {16, -1.5f}, {64, 5.4f}, {0, 0} } },
};

/* --- small helpers ------------------------------------------------------ */

static uint32_t rnd(GameState *s)
{
    uint32_t x = s->flight.rng ? s->flight.rng : 0x9E3779B9u;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    s->flight.rng = x;
    return x;
}

/* Triangle wave in [-1, 1] that starts at 0 and rises. */
static float tri(int t, int period)
{
    float p = (float)(t % period) / (float)period;
    if (p < 0.25f) return p * 4.0f;
    if (p < 0.75f) return 2.0f - p * 4.0f;
    return p * 4.0f - 4.0f;
}

static float wrap_deg(float a)
{
    while (a > 180.0f) a -= 360.0f;
    while (a < -180.0f) a += 360.0f;
    return a;
}

static void steer(Flight *f, float tx, float ty, float max_turn)
{
    float want = atan2f(ty - f->fy, tx - f->fx) * (180.0f / PI_F);
    float d = wrap_deg(want - f->heading);
    if (d > max_turn) d = max_turn;
    if (d < -max_turn) d = -max_turn;
    f->heading += d;
}

static void advance(Flight *f)
{
    f->fx += cosf(DEG(f->heading)) * f->speed;
    f->fy += sinf(DEG(f->heading)) * f->speed;
}

static void sync_enemy(Enemy *e, const Flight *f)
{
    e->x = (int)lroundf(f->fx - HALF);
    e->y = (int)lroundf(f->fy - HALF);
}

static int is_diving_mode(int m)
{
    return m == FLIGHT_DIVE_LOOP || m == FLIGHT_ATTACK || m == FLIGHT_ESCORT;
}

/* Centre of a formation slot, including sway (arriving) or breathing (formed). */
void flight_slot_pos(const GameState *s, int col, int row, float *x, float *y)
{
    float cx = RACK_CX + ((float)col - 4.5f) * COL_GAP;
    float cy = RACK_TOP + (row == 0 ? 0.0f : 2.0f + (float)row * ROW_GAP);
    if (!s->flight.formed) {
        cx += 10.0f * tri(s->flight.t, 240);
    } else {
        /* spread 0 -> 14% -> 0, repeating */
        float k = 0.14f * fabsf(tri(s->flight.breath_t, 360));
        cx = RACK_CX + (cx - RACK_CX) * (1.0f + k);
        cy = RACK_TOP + (cy - RACK_TOP) * (1.0f + k);
    }
    *x = cx;
    *y = cy;
}

static float type_speed(int type)
{
    if (type == ENEMY_TYPE_BUTTERFLY) return 2.8f;
    if (type == ENEMY_TYPE_BOSS) return 2.3f;
    return 2.6f;
}

static void enemy_fire(GameState *s, const Enemy *e)
{
    if (e->y < 0 || e->y > 200)
        return;
    entity_spawn(s, ENTITY_KIND_BULLET, 0, e->x + e->w / 2 - 1, e->y + e->h);
}

/* --- stage setup ---------------------------------------------------------- */

typedef struct { int type, col, row; } SlotDef;

static int add_enemy(GameState *s, int idx, int type, int col, int row,
                     int prog, int side, int delay)
{
    Enemy *e = &s->enemies[idx];
    Flight *f = &s->flight.f[idx];
    float hx, hy;

    memset(e, 0, sizeof(*e));
    memset(f, 0, sizeof(*f));
    e->active = true;
    e->w = 14;
    e->h = 14;
    e->type = type;
    e->hp = (type == ENEMY_TYPE_BOSS) ? 2 : 1;
    e->state = ENEMY_STATE_ENTERING;
    /* home_* = un-swayed slot, kept for code that still reads it */
    hx = RACK_CX + ((float)col - 4.5f) * COL_GAP;
    hy = RACK_TOP + (row == 0 ? 0.0f : 2.0f + (float)row * ROW_GAP);
    e->home_x = (int)lroundf(hx - HALF);
    e->home_y = (int)lroundf(hy - HALF);

    f->mode = FLIGHT_WAIT;
    f->timer = delay;
    f->prog = prog;
    f->side = side;
    f->col = col;
    f->row = row;
    f->leader = -1;
    f->fx = -64.0f;
    f->fy = -64.0f;
    sync_enemy(e, f);
    return idx + 1;
}

void flight_begin_stage(GameState *s)
{
    uint32_t rng = s->flight.rng;
    int i = 0, j;
    const int gap = 130;      /* steps between waves */
    const int file = 7;       /* steps between ships in a file */

    memset(s->enemies, 0, sizeof(s->enemies));
    memset(&s->flight, 0, sizeof(s->flight));
    s->flight.rng = rng;
    for (j = 0; j < MAX_ENEMIES; j++)
        s->flight.f[j].leader = -1;

    /* Wave 1: two files from the top. Butterflies loop left, bees loop right. */
    for (j = 0; j < 4; j++) {
        i = add_enemy(s, i, ENEMY_TYPE_BUTTERFLY, 3 + j, 1, PROG_TOP, +1, 20 + j * file);
        i = add_enemy(s, i, ENEMY_TYPE_BEE, 4 + (j & 1), 3 + (j >> 1), PROG_TOP, -1, 20 + j * file);
    }
    /* Wave 2: Bosses with butterfly partners from the lower left. */
    {
        static const int bcol[4] = { 3, 4, 5, 6 };
        static const int fcol[4] = { 1, 2, 7, 8 };
        for (j = 0; j < 4; j++) {
            i = add_enemy(s, i, ENEMY_TYPE_BOSS, bcol[j], 0, PROG_BOTTOM, +1, 20 + gap + (j * 2) * file);
            i = add_enemy(s, i, ENEMY_TYPE_BUTTERFLY, fcol[j], 1, PROG_BOTTOM, +1, 20 + gap + (j * 2 + 1) * file);
        }
    }
    /* Wave 3: butterflies from the lower right. */
    for (j = 0; j < 8; j++)
        i = add_enemy(s, i, ENEMY_TYPE_BUTTERFLY, 1 + j, 2, PROG_BOTTOM, -1, 20 + 2 * gap + j * file);
    /* Waves 4 and 5: bees from the top, right then left. */
    {
        static const int cols[8] = { 6, 7, 8, 9, 3, 2, 1, 0 };
        for (j = 0; j < 8; j++)
            i = add_enemy(s, i, ENEMY_TYPE_BEE, cols[j], 3, PROG_TOP, -1, 20 + 3 * gap + j * file);
        for (j = 0; j < 8; j++)
            i = add_enemy(s, i, ENEMY_TYPE_BEE, cols[7 - j], 4, PROG_TOP, +1, 20 + 4 * gap + j * file);
    }

    s->flight.dive_timer = 60;
}

/* --- dives ---------------------------------------------------------------- */

static void start_dive(GameState *s, int idx)
{
    Enemy *e = &s->enemies[idx];
    Flight *f = &s->flight.f[idx];
    float cx = f->fx;

    f->mode = FLIGHT_DIVE_LOOP;
    f->heading = -90.0f;
    f->speed = type_speed(e->type);
    f->seg_t = 0;
    f->timer = 0;
    f->escorts_lost = 0;
    /* loop outward, away from the centre (mirror for the right half) */
    f->side = (cx < RACK_CX) ? -1 : +1;
    e->state = ENEMY_STATE_DIVING;
}

static void pick_divers(GameState *s)
{
    int stage = s->player.stage;
    int divers = 0, alive = 0, i, tries;
    int max_divers = 2 + stage / 2;
    if (max_divers > 6)
        max_divers = 6;

    for (i = 0; i < MAX_ENEMIES; i++) {
        if (!s->enemies[i].active) continue;
        alive++;
        if (s->enemies[i].state == ENEMY_STATE_DIVING) divers++;
    }
    if (divers >= max_divers)
        return;

    for (tries = 0; tries < 10; tries++) {
        int idx = (int)(rnd(s) % MAX_ENEMIES);
        Enemy *e = &s->enemies[idx];
        Flight *f = &s->flight.f[idx];
        if (!e->active || f->mode != FLIGHT_FORMED)
            continue;
        start_dive(s, idx);
        if (e->type == ENEMY_TYPE_BOSS) {
            int taken = 0;
            for (i = 0; i < MAX_ENEMIES && taken < 2; i++) {
                Enemy *b = &s->enemies[i];
                Flight *g = &s->flight.f[i];
                if (!b->active || b->type != ENEMY_TYPE_BUTTERFLY ||
                    g->mode != FLIGHT_FORMED || g->row != 1 ||
                    g->col < f->col - 1 || g->col > f->col + 1)
                    continue;
                g->mode = FLIGHT_ESCORT;
                g->leader = idx;
                g->esc_dx = g->fx - f->fx;
                g->esc_dy = g->fy - (f->fy);
                g->speed = f->speed;
                g->timer = 0;
                b->state = ENEMY_STATE_DIVING;
                taken++;
            }
        }
        break;
    }

    {
        int base = 100 - stage * 8;
        if (base < 34) base = 34;
        if (alive <= 8) base /= 2;
        s->flight.dive_timer = base + (int)(rnd(s) % 40);
    }
}

/* --- per-step update ------------------------------------------------------- */

static void go_home_from_top(GameState *s, Flight *f)
{
    float tx, ty;
    flight_slot_pos(s, f->col, f->row, &tx, &ty);
    f->fx = tx;
    f->fy = -16.0f;
    f->heading = 90.0f;
    f->speed = 2.4f;
    f->mode = FLIGHT_SEEK;
    f->leader = -1;
}

static void tick_one(GameState *s, int idx)
{
    Enemy *e = &s->enemies[idx];
    Flight *f = &s->flight.f[idx];
    int stage = s->player.stage;

    switch (f->mode) {
    case FLIGHT_WAIT:
        if (--f->timer <= 0) {
            const Prog *p = &PROGS[f->prog];
            f->mode = FLIGHT_PATH;
            f->seg = 0;
            f->seg_t = 0;
            f->speed = p->speed;
            f->fx = (f->side > 0) ? p->x : 224.0f - p->x;
            f->fy = p->y;
            f->heading = (f->side > 0) ? p->heading : 180.0f - p->heading;
        }
        break;

    case FLIGHT_PATH: {
        const Seg *sg = &PROGS[f->prog].seg[f->seg];
        f->heading += sg->turn * (float)f->side;
        advance(f);
        if (stage >= 2 && f->seg == 1 && f->seg_t == 6 && (rnd(s) & 3) == 0) {
            sync_enemy(e, f);
            enemy_fire(s, e);
        }
        if (++f->seg_t >= sg->steps) {
            f->seg++;
            f->seg_t = 0;
            if (f->seg >= 4 || PROGS[f->prog].seg[f->seg].steps == 0)
                f->mode = FLIGHT_SEEK;
        }
        break;
    }

    case FLIGHT_SEEK: {
        float tx, ty, dx, dy, d;
        flight_slot_pos(s, f->col, f->row, &tx, &ty);
        dx = tx - f->fx;
        dy = ty - f->fy;
        d = sqrtf(dx * dx + dy * dy);
        if (d <= f->speed + 0.5f) {
            f->fx = tx;
            f->fy = ty;
            f->mode = FLIGHT_FORMED;
            e->state = ENEMY_STATE_FORMATION;
        } else if (d < 26.0f) {
            f->heading = atan2f(dy, dx) * (180.0f / PI_F);
            advance(f);
        } else {
            steer(f, tx, ty, 7.0f);
            advance(f);
        }
        break;
    }

    case FLIGHT_FORMED:
        flight_slot_pos(s, f->col, f->row, &f->fx, &f->fy);
        break;

    case FLIGHT_DIVE_LOOP:
        f->heading += 6.0f * (float)f->side;
        advance(f);
        if (++f->seg_t >= 30) {
            f->mode = FLIGHT_ATTACK;
            f->timer = 0;
        }
        break;

    case FLIGHT_ESCORT: {
        const Enemy *L = (f->leader >= 0) ? &s->enemies[f->leader] : NULL;
        const Flight *lf = (f->leader >= 0) ? &s->flight.f[f->leader] : NULL;
        if (L && L->active && (lf->mode == FLIGHT_DIVE_LOOP || lf->mode == FLIGHT_ATTACK)) {
            f->fx = lf->fx + f->esc_dx;
            f->fy = lf->fy + f->esc_dy;
            f->heading = lf->heading;
            f->timer++;
            if (lf->mode == FLIGHT_ATTACK && f->timer == 44) {
                sync_enemy(e, f);
                enemy_fire(s, e);
            }
            break;
        }
        /* leader gone: carry on alone */
        f->mode = FLIGHT_ATTACK;
        f->leader = -1;
        f->timer = 40;
        /* fall through */
    }
    case FLIGHT_ATTACK: {
        float px = (float)(s->player.x + s->player.w / 2);
        float py = (float)s->player.y;
        if (f->fy < py - 36.0f) {
            float turn = (e->type == ENEMY_TYPE_BUTTERFLY) ? 1.7f
                       : (e->type == ENEMY_TYPE_BOSS) ? 1.3f : 1.1f;
            steer(f, px, py, turn);
        }
        if (e->type == ENEMY_TYPE_BUTTERFLY)
            f->heading += 2.6f * sinf((float)f->timer * 0.13f);
        advance(f);
        f->timer++;
        if (f->timer == 8 || (stage >= 2 && f->timer == 30)) {
            sync_enemy(e, f);
            enemy_fire(s, e);
        }
        break;
    }
    default:
        return;
    }

    if (is_diving_mode(f->mode) &&
        (f->fy > (float)GALAGA_HEIGHT + 12.0f || f->fx < -40.0f ||
         f->fx > (float)GALAGA_WIDTH + 40.0f || f->fy < -60.0f))
        go_home_from_top(s, f);

    sync_enemy(e, f);
}

void flight_tick(GameState *s)
{
    FlightState *fs = &s->flight;
    int i, all_home = 1;

    s->frame++;
    /* Waves wait for the PLAYER 1 / STAGE N banners, like the arcade. */
    if (s->hud.banner == BANNER_PLAYER1 || s->hud.banner == BANNER_STAGE) {
        for (i = 0; i < MAX_ENEMIES; i++)
            if (s->enemies[i].active && fs->f[i].mode == FLIGHT_FORMED)
                tick_one(s, i);
        return;
    }
    fs->t++;

    for (i = 0; i < MAX_ENEMIES; i++) {
        if (!s->enemies[i].active || fs->f[i].mode == FLIGHT_NONE)
            continue;
        tick_one(s, i);
        if (fs->f[i].mode == FLIGHT_WAIT || fs->f[i].mode == FLIGHT_PATH ||
            (fs->f[i].mode == FLIGHT_SEEK && s->enemies[i].state == ENEMY_STATE_ENTERING))
            all_home = 0;
    }

    if (!fs->formed) {
        /* switch to breathing when the sway passes through centre */
        if (all_home && fabsf(tri(fs->t, 240)) < 0.05f)
            fs->formed = 1;
        return;
    }

    fs->breath_t++;
    /* no new attacks while the fighter is exploding / respawning */
    if (s->hud.dead_timer > 0)
        return;
    if (--fs->dive_timer <= 0)
        pick_divers(s);
}

/* Escort bonus: a diving Boss shot after its escorts pays 800 / 1600. */
void flight_on_hits(GameState *s)
{
    const CollisionHits *h = &s->last_hits;
    int i;
    for (i = 0; i < h->count; i++) {
        int ei = h->enemy_idx[i];
        Flight *f;
        if (h->type[i] != HIT_PLAYER_BULLET_ENEMY || ei < 0 || ei >= MAX_ENEMIES)
            continue;
        if (s->enemies[ei].active)
            continue;
        f = &s->flight.f[ei];
        if (f->mode == FLIGHT_ESCORT && f->leader >= 0 &&
            s->enemies[f->leader].active)
            s->flight.f[f->leader].escorts_lost++;
        if (s->enemies[ei].type == ENEMY_TYPE_BOSS &&
            (f->mode == FLIGHT_DIVE_LOOP || f->mode == FLIGHT_ATTACK)) {
            if (f->escorts_lost == 1)
                score_add_points(s, 800 - SCORE_BOSS_DIVING);
            else if (f->escorts_lost >= 2)
                score_add_points(s, 1600 - SCORE_BOSS_DIVING);
        }
        f->mode = FLIGHT_NONE;
    }
}
