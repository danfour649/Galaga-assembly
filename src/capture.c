/*
 * capture.c: tractor beam, captured fighter, rescue and dual fighter.
 *
 * A diving Boss may park at BEAM_Y and lower a striped beam. A fighter
 * inside the beam is reeled up ("FIGHTER CAPTURED"), costs a life and then
 * rides above that Boss. Shoot the Boss while it is diving and the captive
 * is freed: it flies down and docks to the right of your ship, and you play
 * on as a dual fighter (two ships, double shots, wider target; a hit costs
 * one of the two ships, not a life). Shoot the Boss while it sits in the
 * rack and the captive turns hostile and joins the formation.
 *
 * Portable C on every backend. Collision with enemies/bullets still runs in
 * the asm/C collision module; this file only adds the beam, captive and
 * dual-fighter rules on top of it.
 */
#include "galaga.h"

#include <math.h>
#include <string.h>

#define CAPTIVE_DY   -15.0f    /* captive sits this far above its Boss */
#define CAPTURE_MSG_STEPS 150

static float boss_cx(const GameState *s, int i) { return (float)(s->enemies[i].x + 7); }
static float boss_cy(const GameState *s, int i) { return (float)(s->enemies[i].y + 7); }

void capture_reset(GameState *s, bool new_game)
{
    Capture *c = &s->capture;
    if (new_game) {
        memset(c, 0, sizeof(*c));
    } else if (c->state == CAPT_PULL || c->state == CAPT_HELD) {
        c->state = CAPT_NONE;   /* the Boss is gone with the old rack */
    }
    c->beam_boss = -1;
    c->beam_t = 0;
    c->beam_len = 0;
}

int capture_can_beam(const GameState *s)
{
    const Capture *c = &s->capture;
    return c->beam_boss < 0 && c->state == CAPT_NONE && !c->dual &&
           s->hud.dead_timer == 0;
}

/* Half-width of the beam d px below its top. */
static float beam_half(float d) { return 8.0f + d * 0.22f; }

static void end_beam(GameState *s, int go_home)
{
    Capture *c = &s->capture;
    int b = c->beam_boss;
    c->beam_boss = -1;
    c->beam_t = 0;
    c->beam_len = 0;
    if (b >= 0 && s->enemies[b].active)
        flight_beam_done(s, b, go_home);
}

void capture_tick(GameState *s)
{
    Capture *c = &s->capture;

    if (c->msg_timer > 0)
        c->msg_timer--;

    /* --- the beam --------------------------------------------------- */
    if (c->beam_boss >= 0) {
        int b = c->beam_boss;
        if (!s->enemies[b].active) {
            c->beam_boss = -1;
        } else if (c->beam_t >= 0 && c->state != CAPT_PULL) {
            int t = c->beam_t++;
            if (t < BEAM_EXTEND)
                c->beam_len = (t + 1) * BEAM_MAX_LEN / BEAM_EXTEND;
            else if (t < BEAM_EXTEND + BEAM_HOLD)
                c->beam_len = BEAM_MAX_LEN;
            else if (t < BEAM_EXTEND + BEAM_HOLD + BEAM_RETRACT)
                c->beam_len = (BEAM_EXTEND + BEAM_HOLD + BEAM_RETRACT - t) *
                              BEAM_MAX_LEN / BEAM_RETRACT;
            else
                end_beam(s, 0);

            /* caught? only while extending/holding, fighter alive, single */
            if (c->beam_boss >= 0 && t < BEAM_EXTEND + BEAM_HOLD &&
                s->hud.dead_timer == 0 && !c->dual && c->state == CAPT_NONE) {
                float top = boss_cy(s, b) + 8.0f;
                float pcx = (float)(s->player.x + s->player.w / 2);
                float pcy = (float)(s->player.y + s->player.h / 2);
                float d = pcy - top;
                if (d >= 0.0f && d <= (float)c->beam_len &&
                    fabsf(pcx - boss_cx(s, b)) < beam_half(d)) {
                    c->state = CAPT_PULL;
                    c->boss = b;
                    c->cx = pcx;
                    c->cy = pcy;
                    c->spin = 0;
                    c->msg_timer = CAPTURE_MSG_STEPS;
                    s->hud.dead_timer = 30000;              /* hidden until docked */
                    s->player.y = GALAGA_HEIGHT + 1000;
                    player_lose_life(s);
                }
            }
        }
    }

    /* --- the captive ------------------------------------------------- */
    switch (c->state) {
    case CAPT_PULL: {
        int b = c->boss;
        float tx = boss_cx(s, b), ty = boss_cy(s, b) + CAPTIVE_DY;
        c->spin++;
        c->cx += (tx - c->cx) * 0.06f;
        c->cy -= 0.9f;
        if (c->cy <= ty) {
            c->cx = tx;
            c->cy = ty;
            c->state = CAPT_HELD;
            c->beam_len = 0;
            end_beam(s, 1);
            s->hud.dead_timer = PLAYER_READY_STEPS + 40;   /* respawn soon */
        }
        break;
    }
    case CAPT_HELD:
        if (s->enemies[c->boss].active) {
            c->cx = boss_cx(s, c->boss);
            c->cy = boss_cy(s, c->boss) + CAPTIVE_DY;
        }
        c->spin = 0;
        break;
    case CAPT_RESCUE: {
        float tx, ty, dx, dy, d;
        if (s->hud.dead_timer > 0)
            break;                       /* wait for the fighter to respawn */
        tx = (float)(s->player.x + 16 + 8);
        ty = (float)(s->player.y + 8);
        if (tx > (float)GALAGA_WIDTH - 8.0f)
            tx = (float)(s->player.x - 8);   /* no room right: dock left */
        dx = tx - c->cx;
        dy = ty - c->cy;
        d = sqrtf(dx * dx + dy * dy);
        c->spin++;
        if (d < 2.0f) {
            if (tx < (float)s->player.x)
                s->player.x -= 16;
            c->state = CAPT_NONE;
            c->dual = 1;
            s->player.w = 32;
            player_clamp_x(&s->player);
        } else {
            float sp = 2.0f;
            c->cx += dx / d * sp;
            c->cy += dy / d * sp;
        }
        break;
    }
    default:
        break;
    }
}

/* Runs after collision_resolve, before flight_on_hits. */
void capture_on_hits(GameState *s)
{
    Capture *c = &s->capture;

    /* player shots vs the captive (it is not in the enemy pool) */
    if (c->state == CAPT_HELD || c->state == CAPT_PULL) {
        for (int bi = 0; bi < MAX_BULLETS; bi++) {
            Bullet *b = &s->bullets[bi];
            if (!b->active || !b->from_player)
                continue;
            Rect br = { b->x, b->y, b->w, b->h };
            Rect cr = { (int)c->cx - 7, (int)c->cy - 7, 14, 14 };
            if (rect_overlap(&br, &cr)) {
                b->active = false;
                effects_spawn_explosion_kind(s, (int)c->cx, (int)c->cy, EXPLOSION_PLAYER);
                if (c->state == CAPT_PULL) {
                    /* shot your own ship mid-capture: it is lost */
                    s->hud.dead_timer = PLAYER_READY_STEPS + 40;
                    end_beam(s, 0);
                }
                c->state = CAPT_NONE;
                break;
            }
        }
    }

    /* the Boss holding (or reeling in) the captive was destroyed */
    if ((c->state == CAPT_HELD || c->state == CAPT_PULL) && !s->enemies[c->boss].active) {
        const Enemy *boss = &s->enemies[c->boss];
        if (c->state == CAPT_PULL) {
            /* freed before docking: you keep the ship (life back) */
            s->player.lives++;
            c->state = CAPT_NONE;
            s->player.x = (int)c->cx - 8;
            s->player.y = GALAGA_HEIGHT - 36;
            player_clamp_x(&s->player);
            s->hud.dead_timer = 0;
            c->beam_boss = -1;
        } else if (boss->state == ENEMY_STATE_DIVING) {
            c->state = CAPT_RESCUE;      /* fly down and dock: dual fighter */
        } else {
            /* shot in the rack: the captive turns hostile and takes the slot */
            const Flight *bf = &s->flight.f[c->boss];
            flight_spawn_formed(s, ENEMY_TYPE_CAPTIVE, bf->col, bf->row, c->cx, c->cy);
            c->state = CAPT_NONE;
        }
    }

    /* hostile captured fighter: 500 in the rack, 1000 diving (asm paid 50/100) */
    for (int i = 0; i < s->last_hits.count; i++) {
        int ei = s->last_hits.enemy_idx[i];
        if (s->last_hits.type[i] != HIT_PLAYER_BULLET_ENEMY || ei < 0 || ei >= MAX_ENEMIES)
            continue;
        const Enemy *e = &s->enemies[ei];
        if (e->active || e->type != ENEMY_TYPE_CAPTIVE)
            continue;
        score_add_points(s, e->state == ENEMY_STATE_DIVING ? 900 : 450);
    }
}

/* A hit landed on the fighter at hit_x. With a dual fighter only the ship
   that was hit is lost, and the life collision_resolve took is refunded.
   Returns 1 when the hit was absorbed that way. */
int capture_player_hit(GameState *s, int hit_x)
{
    Capture *c = &s->capture;
    if (!c->dual)
        return 0;
    int left = hit_x < s->player.x + 16;
    effects_spawn_explosion_kind(s, s->player.x + (left ? 8 : 24), s->player.y + 8,
                                 EXPLOSION_PLAYER);
    s->player.lives++;
    if (s->phase == GAME_PHASE_GAME_OVER) {
        s->phase = GAME_PHASE_PLAYING;
        s->game_over = false;
    }
    if (left)
        s->player.x += 16;
    s->player.w = 16;
    c->dual = 0;
    return 1;
}

/* Dual fighter fires a pair; up to two pairs in flight (single: asm cap 2). */
void capture_fire(GameState *s)
{
    if (!s->capture.dual) {
        player_try_fire(s);
        return;
    }
    int live = 0;
    for (int i = 0; i < MAX_BULLETS; i++)
        if (s->bullets[i].active && s->bullets[i].from_player)
            live++;
    if (live > 2)
        return;
    int xs[2] = { s->player.x + 7, s->player.x + 16 + 7 };
    int placed = 0;
    for (int i = 0; i < MAX_BULLETS && placed < 2; i++) {
        Bullet *b = &s->bullets[i];
        if (b->active)
            continue;
        b->active = true;
        b->x = xs[placed];
        b->y = s->player.y - 8;
        b->w = 3;
        b->h = 8;
        b->from_player = true;
        placed++;
    }
}
