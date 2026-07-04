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
            e->hp = (type == ENEMY_TYPE_BOSS) ? 2 : 1;
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
            b->y -= delta_px * 3;
        else
            b->y += delta_px + delta_px / 2;
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
    if (state->player.lives <= 0) {
        state->game_over = true;
        state->phase = GAME_PHASE_GAME_OVER;
    }
    return state->player.lives;
}

static int row_to_type(int row)
{
    if (row == 0)
        return ENEMY_TYPE_BOSS;
    if (row >= 3)
        return ENEMY_TYPE_BEE;
    return ENEMY_TYPE_BUTTERFLY;
}

static void setup_entering(Enemy *e)
{
    e->state = ENEMY_STATE_ENTERING;
    e->y -= 24;
}

void enemies_spawn_formation(GameState *state)
{
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 8; col++) {
            int idx = entity_spawn(
                state,
                ENTITY_KIND_ENEMY,
                row_to_type(row),
                16 + col * 24,
                28 + row * 18
            );
            if (idx >= 0)
                setup_entering(&state->enemies[idx]);
        }
    }
}

static void start_dive(GameState *state, Enemy *e)
{
    int dx = state->player.x - e->x;
    if (e->type == ENEMY_TYPE_BUTTERFLY)
        e->vx = (dx >= 0) ? 3 : -3;
    else
        e->vx = (dx >= 0) ? 2 : -2;
    e->state = ENEMY_STATE_DIVING;
    e->fire_cd = 20;
}

void enemies_tick_all(GameState *state, int delta_px)
{
    uint32_t frame = state->frame++;

    int stage = state->player.stage;
    int dive_interval = 60 - stage * 6;
    if (dive_interval < 24)
        dive_interval = 24;

    if (frame % (uint32_t)dive_interval == 0) {
        int idx = (int)((frame / (uint32_t)dive_interval) % MAX_ENEMIES);
        Enemy *e = &state->enemies[idx];
        if (e->active && e->state == ENEMY_STATE_FORMATION)
            start_dive(state, e);
        idx = (idx + 7) % MAX_ENEMIES;
        e = &state->enemies[idx];
        if (e->active && e->state == ENEMY_STATE_FORMATION)
            start_dive(state, e);
    }

    int fire_cd_base = 30 - (stage >> 1);
    if (fire_cd_base < 12)
        fire_cd_base = 12;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &state->enemies[i];
        if (!e->active)
            continue;

        if (e->state == ENEMY_STATE_ENTERING) {
            e->y += delta_px;
            if (e->y >= e->home_y) {
                e->y = e->home_y;
                e->state = ENEMY_STATE_FORMATION;
            }
            continue;
        }

        if (e->state == ENEMY_STATE_DIVING) {
            e->x += e->vx;
            e->y += delta_px + delta_px / 2;
            if (e->fire_cd > 0) {
                e->fire_cd--;
            } else {
                int bx = e->x + e->w / 2 - 1;
                int by = e->y + e->h;
                entity_spawn(state, ENTITY_KIND_BULLET, 0, bx, by);
                e->fire_cd = fire_cd_base;
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

static void score_add_enemy_kill(GameState *state, int enemy_index)
{
    const Enemy *e = &state->enemies[enemy_index];
    int diving = (e->state == ENEMY_STATE_DIVING);
    int points;

    switch (e->type) {
    case ENEMY_TYPE_BUTTERFLY:
        points = diving ? SCORE_BUTTERFLY_DIVING : SCORE_BUTTERFLY_FORMATION;
        break;
    case ENEMY_TYPE_BOSS:
        points = diving ? SCORE_BOSS_DIVING : SCORE_BOSS_FORMATION;
        break;
    default:
        points = diving ? SCORE_BEE_DIVING : SCORE_BEE_FORMATION;
        break;
    }
    score_add_points(state, points);
}

void score_add_points(GameState *state, int points)
{
    state->player.score += points;
    while (state->next_extra_life > 0 && state->player.score >= state->next_extra_life) {
        state->player.lives++;
        if (state->next_extra_life == 20000)
            state->next_extra_life = 70000;
        else if (state->next_extra_life == 70000)
            state->next_extra_life = 150000;
        else
            state->next_extra_life = 0;
    }
}

static void record_hit(CollisionHits *hits, int type, int bullet_idx, int enemy_idx)
{
    if (hits->count >= MAX_COLLISION_HITS)
        return;
    int i = hits->count++;
    hits->type[i] = type;
    hits->bullet_idx[i] = bullet_idx;
    hits->enemy_idx[i] = enemy_idx;
}

static int boxes_overlap(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2)
{
    Rect a = { x1, y1, w1, h1 };
    Rect b = { x2, y2, w2, h2 };
    return rect_overlap(&a, &b);
}

void collision_resolve(GameState *state, CollisionHits *hits)
{
    hits->count = 0;

    for (int bi = 0; bi < MAX_BULLETS; bi++) {
        Bullet *b = &state->bullets[bi];
        if (!b->active || !b->from_player)
            continue;
        for (int ei = 0; ei < MAX_ENEMIES; ei++) {
            Enemy *e = &state->enemies[ei];
            if (!e->active)
                continue;
            if (boxes_overlap(b->x, b->y, b->w, b->h, e->x, e->y, e->w, e->h)) {
                record_hit(hits, HIT_PLAYER_BULLET_ENEMY, bi, ei);
                if (e->hp > 1) {
                    e->hp--;
                    entity_kill(state, ENTITY_KIND_BULLET, bi);
                } else {
                    score_add_enemy_kill(state, ei);
                    entity_kill(state, ENTITY_KIND_BULLET, bi);
                    entity_kill(state, ENTITY_KIND_ENEMY, ei);
                }
            }
        }
    }

    for (int bi = 0; bi < MAX_BULLETS; bi++) {
        Bullet *b = &state->bullets[bi];
        if (!b->active || b->from_player)
            continue;
        if (boxes_overlap(b->x, b->y, b->w, b->h,
                          state->player.x, state->player.y,
                          state->player.w, state->player.h)) {
            record_hit(hits, HIT_ENEMY_BULLET_PLAYER, bi, -1);
            entity_kill(state, ENTITY_KIND_BULLET, bi);
            player_lose_life(state);
        }
    }

    for (int ei = 0; ei < MAX_ENEMIES; ei++) {
        Enemy *e = &state->enemies[ei];
        if (!e->active)
            continue;
        if (boxes_overlap(e->x, e->y, e->w, e->h,
                          state->player.x, state->player.y,
                          state->player.w, state->player.h)) {
            record_hit(hits, HIT_ENEMY_BODY_PLAYER, -1, ei);
            entity_kill(state, ENTITY_KIND_ENEMY, ei);
            player_lose_life(state);
        }
    }
}

void game_state_init(GameState *state)
{
    state->phase = GAME_PHASE_TITLE;
    state->clear_timer = 0;
    state->next_extra_life = 20000;
    state->game_over = false;
}

int game_state_is_playing(const GameState *state)
{
    return state->phase == GAME_PHASE_PLAYING;
}

void game_state_begin_stage_clear(GameState *state)
{
    state->phase = GAME_PHASE_STAGE_CLEAR;
    state->clear_timer = STAGE_CLEAR_FRAMES;
}

void game_state_tick(GameState *state, int fire_pressed)
{
    if (state->phase == GAME_PHASE_TITLE) {
        if (fire_pressed) {
            state->phase = GAME_PHASE_PLAYING;
            state->game_over = false;
            player_init(state);
            enemies_spawn_formation(state);
        }
        return;
    }

    if (state->phase == GAME_PHASE_STAGE_CLEAR) {
        if (state->clear_timer > 0)
            state->clear_timer--;
        else {
            state->player.stage++;
            state->phase = GAME_PHASE_PLAYING;
            enemies_spawn_formation(state);
        }
        return;
    }

    if (state->phase == GAME_PHASE_GAME_OVER) {
        if (fire_pressed) {
            state->phase = GAME_PHASE_TITLE;
            state->game_over = false;
            state->frame = 0;
            player_init(state);
        }
    }
}

#endif
