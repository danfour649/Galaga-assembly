#include "galaga.h"

#include <string.h>
#include <time.h>

/* One sim step is the old 60 Hz truncation: player 1 px, world 2 px. */
static float sim_accum;

void game_init(GameState *state)
{
    memset(state, 0, sizeof(*state));
    state->running = true;
    sim_accum = 0.0f;
    state->flight.rng = (uint32_t)time(NULL) * 2654435761u | 1u;
    high_score_load(state);
    game_state_init(state);
    player_init(state);
}

static void maybe_save_high_score(GameState *state, int phase_before)
{
    if (phase_before != GAME_PHASE_PLAYING || state->phase != GAME_PHASE_GAME_OVER)
        return;
    /* Save while player.score is still greater than the stored high score.
       Updating high_score first makes high_score_save return immediately. */
    high_score_save(state);
    if (state->player.score > state->high_score)
        state->high_score = state->player.score;
}

static void hud_begin_stage(GameState *state, bool new_game)
{
    Hud *h = &state->hud;
    h->dead_timer = 0;
    if (new_game) {
        h->banner = BANNER_PLAYER1;
        h->banner_timer = BANNER_PLAYER1_STEPS;
    } else {
        h->banner = BANNER_STAGE;
        h->banner_timer = BANNER_STAGE_STEPS;
    }
}

static void hud_tick_banner(Hud *h)
{
    if (h->banner_timer <= 0)
        return;
    if (--h->banner_timer == 0) {
        if (h->banner == BANNER_PLAYER1) {
            h->banner = BANNER_STAGE;
            h->banner_timer = BANNER_STAGE_STEPS;
        } else {
            h->banner = BANNER_NONE;
        }
    }
}

/* Fighter hit: burst, hide it off the playfield, respawn after a pause. */
static void handle_player_hits(GameState *state, int px, int py)
{
    const CollisionHits *hits = &state->last_hits;
    bool died = false;
    for (int i = 0; i < hits->count; i++) {
        if (hits->type[i] == HIT_ENEMY_BODY_PLAYER) {
            int ei = hits->enemy_idx[i];
            if (ei >= 0 && ei < MAX_ENEMIES) {
                const Enemy *e = &state->enemies[ei];
                effects_spawn_explosion(state, e->x + e->w / 2, e->y + e->h / 2);
            }
            died = true;
        } else if (hits->type[i] == HIT_ENEMY_BULLET_PLAYER) {
            died = true;
        }
    }
    if (!died)
        return;
    effects_spawn_explosion_kind(state, px + state->player.w / 2,
                                 py + state->player.h / 2, EXPLOSION_PLAYER);
    state->hud.dead_timer = PLAYER_DEAD_STEPS;
    state->player.y = GALAGA_HEIGHT + 1000;   /* out of every hitbox */
}

static void sim_step(GameState *state, bool move_left, bool move_right, bool fire)
{
    int phase_before = state->phase;
    Hud *hud = &state->hud;

    hud->ticks++;
    game_state_tick(state, fire ? 1 : 0);

    /* game_state_tick (asm or C) spawns the old 5x8 grid when a stage
       starts; replace it with the arcade rack and its fly-in waves. */
    if (phase_before != GAME_PHASE_PLAYING && state->phase == GAME_PHASE_PLAYING) {
        hud_begin_stage(state, phase_before == GAME_PHASE_TITLE);
        flight_begin_stage(state);
    }

    if (!game_state_is_playing(state)) {
        effects_tick(state);
        maybe_save_high_score(state, phase_before);
        return;
    }

    hud_tick_banner(hud);

    if (hud->dead_timer > 0) {
        if (--hud->dead_timer == 0) {
            state->player.x = GALAGA_WIDTH / 2 - state->player.w / 2;
            state->player.y = GALAGA_HEIGHT - 36;
        }
    } else {
        player_tick(state, 1, move_left ? 1 : 0, move_right ? 1 : 0);
        if (fire && hud->banner != BANNER_PLAYER1)
            player_try_fire(state);
    }

    int px = state->player.x, py = state->player.y;
    flight_tick(state);
    entity_tick_all(state, 2);
    collision_resolve(state, &state->last_hits);
    flight_on_hits(state);
    effects_process_hits(state);
    handle_player_hits(state, px, py);
    effects_tick(state);

    if (state->phase == GAME_PHASE_PLAYING &&
        !entity_any_active(state, ENTITY_KIND_ENEMY))
        game_state_begin_stage_clear(state);

    maybe_save_high_score(state, phase_before);
}

void game_tick(GameState *state, float dt, bool move_left, bool move_right, bool fire)
{
    const float step = 1.0f / 60.0f;
    int steps = 0;

    sim_accum += dt;
    if (sim_accum > step * 4.0f)
        sim_accum = step * 4.0f;

    while (sim_accum >= step && steps < 4) {
        int phase_before = state->phase;
        sim_accum -= step;
        steps++;
        sim_step(state, move_left, move_right, fire);
        /* Death is applied after game_state_tick. Stop so a held fire
           button cannot skip the game-over screen in the same frame. */
        if (phase_before == GAME_PHASE_PLAYING &&
            state->phase == GAME_PHASE_GAME_OVER)
            break;
    }
}
