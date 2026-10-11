#include "galaga.h"

#include <string.h>

/* One sim step is the old 60 Hz truncation: player 1 px, world 2 px. */
static float sim_accum;

void game_init(GameState *state)
{
    memset(state, 0, sizeof(*state));
    state->running = true;
    sim_accum = 0.0f;
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

static void sim_step(GameState *state, bool move_left, bool move_right, bool fire)
{
    int phase_before = state->phase;
    game_state_tick(state, fire ? 1 : 0);

    if (!game_state_is_playing(state)) {
        effects_tick(state);
        maybe_save_high_score(state, phase_before);
        return;
    }

    player_tick(state, 1, move_left ? 1 : 0, move_right ? 1 : 0);

    if (fire)
        player_try_fire(state);

    enemies_tick_all(state, 2);
    entity_tick_all(state, 2);
    collision_resolve(state, &state->last_hits);
    effects_process_hits(state);
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
