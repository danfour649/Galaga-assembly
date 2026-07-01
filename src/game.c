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
        if (!b->active)
            continue;

        if (b->from_player) {
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
        } else {
            Rect br = bullet_rect(b);
            if (rect_overlap(&pr, &br)) {
                entity_kill(state, ENTITY_KIND_BULLET, bi);
                player_lose_life(state);
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
            player_lose_life(state);
        }
    }
}

void game_init(GameState *state)
{
    memset(state, 0, sizeof(*state));
    state->running = true;
    player_init(state);
    enemies_spawn_formation(state);
}

void game_tick(GameState *state, float dt, bool move_left, bool move_right, bool fire)
{
    if (state->game_over)
        return;

    const int speed = (int)(100.0f * dt);
    player_tick(state, speed, move_left ? 1 : 0, move_right ? 1 : 0);

    if (fire)
        player_try_fire(state);

    enemies_tick_all(state, (int)(120.0f * dt));
    entity_tick_all(state, (int)(120.0f * dt));
    resolve_collisions(state);

    if (!entity_any_active(state, ENTITY_KIND_ENEMY)) {
        state->player.stage++;
        enemies_spawn_formation(state);
    }
}
