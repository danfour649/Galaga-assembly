#include "galaga.h"

#if !GALAGA_USE_ASM

int rect_overlap(const Rect *a, const Rect *b)
{
    int a_right  = a->x + a->w;
    int a_bottom = a->y + a->h;
    int b_right  = b->x + b->w;
    int b_bottom = b->y + b->h;

    if (a->x >= b_right)  return 0;
    if (b->x >= a_right)  return 0;
    if (a->y >= b_bottom) return 0;
    if (b->y >= a_bottom) return 0;
    return 1;
}

int entity_spawn(GameState *state, int kind, int type, int x, int y)
{
    if (kind == ENTITY_KIND_ENEMY) {
        for (int i = 0; i < MAX_ENEMIES; i++) {
            Enemy *e = &state->enemies[i];
            if (e->active)
                continue;
            e->active = true;
            e->x = x;
            e->y = y;
            e->w = 14;
            e->h = 14;
            e->type = type;
            e->hp = 1;
            return i;
        }
        return -1;
    }

    if (kind == ENTITY_KIND_BULLET) {
        if (type == 1) {
            int player_bullets = 0;
            for (int i = 0; i < MAX_BULLETS; i++) {
                Bullet *b = &state->bullets[i];
                if (b->active && b->from_player)
                    player_bullets++;
            }
            if (player_bullets >= 2)
                return -1;
        }
        for (int i = 0; i < MAX_BULLETS; i++) {
            Bullet *b = &state->bullets[i];
            if (b->active)
                continue;
            b->active = true;
            b->x = x;
            b->y = y;
            b->w = 3;
            b->h = 8;
            b->from_player = (type != 0);
            return i;
        }
        return -1;
    }

    return -1;
}

void entity_kill(GameState *state, int kind, int index)
{
    if (kind == ENTITY_KIND_ENEMY && index >= 0 && index < MAX_ENEMIES)
        state->enemies[index].active = false;
    else if (kind == ENTITY_KIND_BULLET && index >= 0 && index < MAX_BULLETS)
        state->bullets[index].active = false;
}

void entity_tick_all(GameState *state, int delta_px)
{
    for (int i = 0; i < MAX_BULLETS; i++) {
        Bullet *b = &state->bullets[i];
        if (!b->active)
            continue;
        if (b->from_player)
            b->y -= delta_px;
        else
            b->y += delta_px;
        if (b->y < -b->h || b->y > GALAGA_HEIGHT)
            b->active = false;
    }
}

int entity_any_active(const GameState *state, int kind)
{
    if (kind == ENTITY_KIND_ENEMY) {
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (state->enemies[i].active)
                return 1;
        }
        return 0;
    }
    if (kind == ENTITY_KIND_BULLET) {
        for (int i = 0; i < MAX_BULLETS; i++) {
            if (state->bullets[i].active)
                return 1;
        }
        return 0;
    }
    return 0;
}

void entities_spawn_demo_formation(GameState *state)
{
    const int cols = 8;
    const int rows = 3;
    const int start_x = 24;
    const int start_y = 40;
    const int gap_x = 22;
    const int gap_y = 20;

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            entity_spawn(
                state,
                ENTITY_KIND_ENEMY,
                row,
                start_x + col * gap_x,
                start_y + row * gap_y
            );
        }
    }
}

#endif
