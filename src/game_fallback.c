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
            e->state = ENEMY_STATE_FORMATION;
            e->home_x = x;
            e->home_y = y;
            e->vx = 0;
            e->fire_cd = 0;
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

void player_init(GameState *state)
{
    state->player.x = GALAGA_WIDTH / 2 - 8;
    state->player.y = GALAGA_HEIGHT - 36;
    state->player.w = 16;
    state->player.h = 16;
    state->player.lives = 3;
    state->player.score = 0;
    state->player.stage = 1;
}

void player_clamp_x(Player *player)
{
    if (player->x < 0)
        player->x = 0;
    if (player->x + player->w > GALAGA_WIDTH)
        player->x = GALAGA_WIDTH - player->w;
}

void player_tick(GameState *state, int delta_px, int move_left, int move_right)
{
    if (move_left)
        state->player.x -= delta_px;
    if (move_right)
        state->player.x += delta_px;
    player_clamp_x(&state->player);
}

void player_try_fire(GameState *state)
{
    int bx = state->player.x + state->player.w / 2 - 1;
    int by = state->player.y - 8;
    entity_spawn(state, ENTITY_KIND_BULLET, 1, bx, by);
}

int player_lose_life(GameState *state)
{
    state->player.lives--;
    if (state->player.lives <= 0)
        state->game_over = true;
    return state->player.lives;
}

void enemies_spawn_formation(GameState *state)
{
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 8; col++) {
            entity_spawn(
                state,
                ENTITY_KIND_ENEMY,
                ENEMY_TYPE_BEE,
                16 + col * 24,
                28 + row * 18
            );
        }
    }
}

void enemies_tick_all(GameState *state, int delta_px)
{
    uint32_t frame = state->frame++;

    if (frame % 90 == 0) {
        int idx = (int)((frame / 90) % MAX_ENEMIES);
        Enemy *e = &state->enemies[idx];
        if (e->active && e->state == ENEMY_STATE_FORMATION) {
            int dx = state->player.x - e->x;
            e->state = ENEMY_STATE_DIVING;
            e->vx = (dx > 0) ? 2 : (dx < 0) ? -2 : 0;
            e->fire_cd = 20;
        }
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &state->enemies[i];
        if (!e->active)
            continue;

        if (e->state == ENEMY_STATE_DIVING) {
            e->x += e->vx;
            e->y += delta_px;
            if (e->fire_cd > 0) {
                e->fire_cd--;
            } else {
                int bx = e->x + e->w / 2 - 1;
                int by = e->y + e->h;
                entity_spawn(state, ENTITY_KIND_BULLET, 0, bx, by);
                e->fire_cd = 40;
            }
            if (e->y >= GALAGA_HEIGHT) {
                e->state = ENEMY_STATE_FORMATION;
                e->x = e->home_x;
                e->y = e->home_y;
                e->vx = 0;
            }
        } else {
            e->x = e->home_x + (int)((frame + i) % 4) - 1;
            e->y = e->home_y + (int)(((frame >> 2) + i) % 2);
        }
    }
}

#endif
