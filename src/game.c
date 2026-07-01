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

static void spawn_demo_formation(GameState *state)
{
    const int cols = 8;
    const int rows = 3;
    const int start_x = 24;
    const int start_y = 40;
    const int gap_x = 22;
    const int gap_y = 20;
    int idx = 0;

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            Enemy *e = &state->enemies[idx++];
            e->active = true;
            e->x = start_x + col * gap_x;
            e->y = start_y + row * gap_y;
            e->w = 14;
            e->h = 14;
            e->type = row;
            e->hp = 1;
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

    spawn_demo_formation(state);
}

static void fire_bullet(GameState *state)
{
    int active_player_bullets = 0;
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (state->bullets[i].active && state->bullets[i].from_player)
            active_player_bullets++;
    }
    if (active_player_bullets >= 2)
        return;

    for (int i = 0; i < MAX_BULLETS; i++) {
        Bullet *b = &state->bullets[i];
        if (!b->active) {
            b->active = true;
            b->from_player = true;
            b->w = 3;
            b->h = 8;
            b->x = state->player.x + state->player.w / 2 - b->w / 2;
            b->y = state->player.y - b->h;
            return;
        }
    }
}

static void move_bullets(GameState *state, float dt)
{
    const int speed = (int)(120.0f * dt);
    for (int i = 0; i < MAX_BULLETS; i++) {
        Bullet *b = &state->bullets[i];
        if (!b->active)
            continue;
        if (b->from_player)
            b->y -= speed;
        else
            b->y += speed;
        if (b->y < -b->h || b->y > GALAGA_HEIGHT)
            b->active = false;
    }
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
                b->active = false;
                e->active = false;
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
            e->active = false;
            state->player.lives--;
            if (state->player.lives <= 0)
                state->game_over = true;
        }
    }
}

static bool all_enemies_dead(const GameState *state)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (state->enemies[i].active)
            return false;
    }
    return true;
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

    if (fire)
        fire_bullet(state);

    move_bullets(state, dt);
    resolve_collisions(state);

    if (all_enemies_dead(state)) {
        state->player.stage++;
        spawn_demo_formation(state);
    }
}
