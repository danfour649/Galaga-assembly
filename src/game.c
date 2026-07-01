#include "galaga.h"

#include <stdlib.h>
#include <string.h>

static Rect enemy_rect(const Enemy *e)
{
    Rect r = { e->x, e->y, e->w, e->h };
    return r;
}

static Rect bullet_rect(const Bullet *b)
{
    Rect r = { b->x, b->y, b->w, b->h };
    return r;
}

static Rect player_rect(const Player *p)
{
    Rect r = { p->x, p->y, p->w, p->h };
    return r;
}

static void resolve_collisions(GameState *state)
{
    Rect pr = player_rect(&state->player);

    for (int bi = 0; bi < MAX_BULLETS; bi++) {
        Bullet *b = &state->bullets[bi];
        if (!b->active || !b->from_player)
            continue;

        Rect br = bullet_rect(b);
        for (int ei = 0; ei < MAX_ENEMIES; ei++) {
            Enemy *e = &state->enemies[ei];
            Rect er;
            if (!e->active)
                continue;

            er = enemy_rect(e);
            if (rect_overlap(&br, &er)) {
                entity_kill(state, ENTITY_KIND_BULLET, bi);
                entity_kill(state, ENTITY_KIND_ENEMY, ei);
                state->player.score += 100;
            }
        }
    }

    for (int ei = 0; ei < MAX_ENEMIES; ei++) {
        Enemy *e = &state->enemies[ei];
        Rect er;
        if (!e->active)
            continue;
        er = enemy_rect(e);
        if (rect_overlap(&pr, &er)) {
            entity_kill(state, ENTITY_KIND_ENEMY, ei);
            state->player.lives--;
            if (state->player.lives <= 0)
                state->game_over = true;
        }
    }
}

void game_init(GameState *state)
{
    memset(state, 0, sizeof(*state));
    state->running = true;

    state->player.x = GALAGA_WIDTH / 2 - 8;
    state->player.y = GALAGA_HEIGHT - 36;
    state->player.w = 16;
    state->player.h = 16;
    state->player.lives = 3;
    state->player.score = 0;
    state->player.stage = 1;

    entities_spawn_demo_formation(state);
}

void game_tick(GameState *state, float dt, bool move_left, bool move_right, bool fire)
{
    if (state->game_over)
        return;

    const int speed = (int)(100.0f * dt);
    if (move_left)
        state->player.x -= speed;
    if (move_right)
        state->player.x += speed;

    if (state->player.x < 0)
        state->player.x = 0;
    if (state->player.x + state->player.w > GALAGA_WIDTH)
        state->player.x = GALAGA_WIDTH - state->player.w;

    if (fire) {
        int bx = state->player.x + state->player.w / 2 - 1;
        int by = state->player.y - 8;
        entity_spawn(state, ENTITY_KIND_BULLET, 1, bx, by);
    }

    entity_tick_all(state, (int)(120.0f * dt));
    resolve_collisions(state);

    if (!entity_any_active(state, ENTITY_KIND_ENEMY)) {
        state->player.stage++;
        entities_spawn_demo_formation(state);
    }
}
