#include "galaga.h"

#include <string.h>

void game_init(GameState *state)
{
    memset(state, 0, sizeof(*state));
    state->running = true;
    high_score_load(state);
    game_state_init(state);
    player_init(state);
}

void game_tick(GameState *state, float dt, bool move_left, bool move_right, bool fire)
{
    int phase_before = state->phase;
    game_state_tick(state, fire ? 1 : 0);

    if (phase_before == GAME_PHASE_PLAYING && state->phase == GAME_PHASE_GAME_OVER) {
        if (state->player.score > state->high_score)
            state->high_score = state->player.score;
        high_score_save(state);
    }

    if (!game_state_is_playing(state)) {
        effects_tick(state);
        return;
    }

    const int speed = (int)(100.0f * dt);
    player_tick(state, speed, move_left ? 1 : 0, move_right ? 1 : 0);

    if (fire)
        player_try_fire(state);

    enemies_tick_all(state, (int)(120.0f * dt));
    entity_tick_all(state, (int)(120.0f * dt));
    collision_resolve(state, &state->last_hits);
    effects_process_hits(state);
    effects_tick(state);

    if (!entity_any_active(state, ENTITY_KIND_ENEMY))
        game_state_begin_stage_clear(state);
}
