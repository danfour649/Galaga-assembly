#include "galaga.h"

void effects_spawn_explosion(GameState *gs, int x, int y)
{
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        Explosion *e = &gs->explosions[i];
        if (e->active)
            continue;
        e->active = true;
        e->x = x;
        e->y = y;
        e->timer = EXPLOSION_FRAMES;
        return;
    }
}

void effects_process_hits(GameState *gs)
{
    for (int i = 0; i < gs->last_hits.count; i++) {
        int type = gs->last_hits.type[i];
        if (type != HIT_PLAYER_BULLET_ENEMY)
            continue;
        int ei = gs->last_hits.enemy_idx[i];
        if (ei < 0 || ei >= MAX_ENEMIES)
            continue;
        const Enemy *e = &gs->enemies[ei];
        effects_spawn_explosion(gs, e->x + e->w / 2, e->y + e->h / 2);
    }
}

void effects_tick(GameState *gs)
{
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        Explosion *e = &gs->explosions[i];
        if (!e->active)
            continue;
        e->timer--;
        if (e->timer <= 0)
            e->active = false;
    }
}
