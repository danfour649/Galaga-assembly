#include "galaga.h"

#include <stdlib.h>
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
    if (state->hud.demo)
        return;                       /* demo scores are never kept */
    if (phase_before != GAME_PHASE_PLAYING || state->phase != GAME_PHASE_GAME_OVER)
        return;
    /* Save while player.score is still greater than the stored high score.
       Updating high_score first makes high_score_save return immediately. */
    high_score_save(state);
    if (state->player.score > state->high_score)
        state->high_score = state->player.score;
}

/* Back on the title: clear the playfield so no rack, shots or bursts
   linger behind the attract screen. */
static void clear_field(GameState *state)
{
    memset(state->enemies, 0, sizeof(state->enemies));
    memset(state->bullets, 0, sizeof(state->bullets));
    memset(state->explosions, 0, sizeof(state->explosions));
    memset(&state->challenge, 0, sizeof(state->challenge));
    capture_reset(state, true);
    state->hud.dead_timer = 0;
    state->hud.banner = BANNER_NONE;
    state->hud.banner_timer = 0;
}

static int player_bullets_live(const GameState *state)
{
    int n = 0;
    for (int i = 0; i < MAX_BULLETS; i++)
        if (state->bullets[i].active && state->bullets[i].from_player)
            n++;
    return n;
}

static void hud_begin_stage(GameState *state, bool new_game)
{
    Hud *h = &state->hud;
    if (h->dead_timer > 0 || state->player.y > GALAGA_HEIGHT) {
        /* died (or was captured) as the stage ended: back on the line */
        state->player.x = GALAGA_WIDTH / 2 - state->player.w / 2;
        state->player.y = GALAGA_HEIGHT - 36;
    }
    h->dead_timer = 0;
    if (new_game) {
        h->shots = 0;
        h->hits = 0;
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
        int hit_x = -1;
        if (hits->type[i] == HIT_ENEMY_BODY_PLAYER) {
            int ei = hits->enemy_idx[i];
            if (ei >= 0 && ei < MAX_ENEMIES) {
                const Enemy *e = &state->enemies[ei];
                effects_spawn_explosion(state, e->x + e->w / 2, e->y + e->h / 2);
                hit_x = e->x + e->w / 2;
            }
        } else if (hits->type[i] == HIT_ENEMY_BULLET_PLAYER) {
            int bi = hits->bullet_idx[i];
            if (bi >= 0 && bi < MAX_BULLETS)
                hit_x = state->bullets[bi].x + 1;
        } else {
            continue;
        }
        /* dual fighter: lose one of the two ships, not a life */
        if (!died && capture_player_hit(state, hit_x >= 0 ? hit_x : px))
            continue;
        died = true;
    }
    if (!died)
        return;
    effects_spawn_explosion_kind(state, px + state->player.w / 2,
                                 py + state->player.h / 2, EXPLOSION_PLAYER);
    state->hud.dead_timer = PLAYER_DEAD_STEPS;
    state->player.y = GALAGA_HEIGHT + 1000;   /* out of every hitbox */
}

/* --- attract-mode demo ---------------------------------------------------- */

static void demo_start(GameState *state)
{
    Hud *hud = &state->hud;
    hud->demo = 1;
    hud->demo_t = 0;
    hud->demo_beamed = 0;
    hud->demo_high = state->high_score;
}

/* Back to the title: the demo never touches the high score. */
static void demo_end(GameState *state)
{
    Hud *hud = &state->hud;
    state->high_score = hud->demo_high;
    state->phase = GAME_PHASE_TITLE;
    state->game_over = false;
    state->frame = 0;
    player_init(state);
    clear_field(state);
    hud->demo = 0;
    hud->title_t = 0;
    hud->over_t = 0;
}

static void demo_check_end(GameState *state)
{
    Hud *hud = &state->hud;
    if (hud->demo && (++hud->demo_t >= DEMO_STEPS || state->phase == GAME_PHASE_GAME_OVER))
        demo_end(state);
}

/* The demo pilot: dodge shots, line up under an enemy and fire; once,
   park under a Boss's beam so the capture is shown. */
static void demo_pilot(GameState *state, bool *left, bool *right, bool *fire)
{
    Hud *hud = &state->hud;
    const Player *p = &state->player;
    int pcx = p->x + p->w / 2;
    int target = -1, best = 1 << 30, i;

    *left = *right = *fire = false;
    if (hud->dead_timer > 0)
        return;

    /* show the tractor beam once the rack has formed */
    if (!hud->demo_beamed && hud->demo_t >= DEMO_BEAM_AT && capture_can_beam(state)) {
        for (i = 0; i < MAX_ENEMIES; i++) {
            const Enemy *e = &state->enemies[i];
            if (e->active && e->type == ENEMY_TYPE_BOSS && e->state == ENEMY_STATE_FORMATION) {
                flight_start_beam_dive(state, i);
                hud->demo_beamed = 1;
                break;
            }
        }
    }
    if (state->capture.beam_boss >= 0 && state->capture.state == CAPT_NONE) {
        target = (int)state->capture.beam_x;     /* drive into the beam */
    } else {
        for (i = 0; i < MAX_BULLETS; i++) {
            const Bullet *b = &state->bullets[i];
            if (!b->active || b->from_player)
                continue;
            int dx = b->x - pcx;
            if (b->y > p->y - 80 && b->y < p->y + p->h && dx > -14 && dx < 14) {
                /* dodge away from the shot, toward open space */
                bool go_left = dx > 0 || pcx > GALAGA_WIDTH - 24;
                if (pcx < 24) go_left = false;
                *left = go_left;
                *right = !go_left;
                return;
            }
        }
        for (i = 0; i < MAX_ENEMIES; i++) {
            const Enemy *e = &state->enemies[i];
            if (!e->active || e->y > p->y - 40 || e->y < 0)
                continue;
            /* spare the Bosses until one has shown its beam */
            if (!hud->demo_beamed && e->type == ENEMY_TYPE_BOSS)
                continue;
            int d = abs(e->x + e->w / 2 - pcx) + (e->state == ENEMY_STATE_DIVING ? 0 : 20);
            if (d < best) { best = d; target = e->x + e->w / 2; }
        }
    }
    if (target >= 0) {
        if (target < pcx - 2) *left = true;
        else if (target > pcx + 2) *right = true;
        if (abs(target - pcx) < 9 && (hud->ticks % 9) == 0 && state->capture.beam_boss < 0)
            *fire = true;
    }
}

static void sim_step(GameState *state, bool move_left, bool move_right, bool fire)
{
    Hud *hud = &state->hud;

    if (hud->demo) {
        if (move_left || move_right || fire) {
            /* any input ends the demo; fire goes on to start a real game */
            demo_end(state);
            move_left = move_right = false;
        } else {
            demo_pilot(state, &move_left, &move_right, &fire);
        }
    } else if (state->phase == GAME_PHASE_TITLE) {
        if (++hud->title_t >= DEMO_AFTER_STEPS && !fire) {
            demo_start(state);
            fire = true;                      /* "press start" for the demo */
        }
    }

    int phase_before = state->phase;

    hud->ticks++;
    if (state->phase == GAME_PHASE_GAME_OVER) {
        /* hold the results screen, then go back to the title by itself */
        if (++hud->over_t >= GAMEOVER_TITLE_AT) {
            state->phase = GAME_PHASE_TITLE;
            state->game_over = false;
            state->frame = 0;
            player_init(state);
            hud->over_t = 0;
            hud->title_t = 0;
            clear_field(state);
            return;
        }
        if (hud->over_t < GAMEOVER_FIRE_LOCK)
            fire = false;
    } else {
        hud->over_t = 0;
    }
    game_state_tick(state, fire ? 1 : 0);
    if (phase_before != GAME_PHASE_TITLE && state->phase == GAME_PHASE_TITLE) {
        clear_field(state);
        hud->title_t = 0;
    }

    /* game_state_tick (asm or C) spawns the old 5x8 grid when a stage
       starts; replace it with the arcade rack and its fly-in waves. */
    if (phase_before != GAME_PHASE_PLAYING && state->phase == GAME_PHASE_PLAYING) {
        hud_begin_stage(state, phase_before == GAME_PHASE_TITLE);
        capture_reset(state, phase_before == GAME_PHASE_TITLE);
        flight_begin_stage(state);
    }
    if (hud->demo && hud->banner == BANNER_PLAYER1) {
        hud->banner = BANNER_NONE;            /* the demo starts straight in */
        hud->banner_timer = 0;
    }

    if (!game_state_is_playing(state)) {
        effects_tick(state);
        maybe_save_high_score(state, phase_before);
        demo_check_end(state);
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
        if (fire && hud->banner != BANNER_PLAYER1) {
            int before = player_bullets_live(state);
            capture_fire(state);
            int fired = player_bullets_live(state) - before;
            if (fired > 0)
                hud->shots += fired;
        }
    }

    int px = state->player.x, py = state->player.y;
    flight_tick(state);
    capture_tick(state);
    entity_tick_all(state, 2);
    collision_resolve(state, &state->last_hits);
    for (int i = 0; i < state->last_hits.count; i++)
        if (state->last_hits.type[i] == HIT_PLAYER_BULLET_ENEMY)
            hud->hits++;
    capture_on_hits(state);
    flight_on_hits(state);
    effects_process_hits(state);
    handle_player_hits(state, px, py);
    effects_tick(state);

    if (state->phase == GAME_PHASE_PLAYING &&
        !entity_any_active(state, ENTITY_KIND_ENEMY) &&
        flight_stage_finished(state))
        game_state_begin_stage_clear(state);

    maybe_save_high_score(state, phase_before);

    demo_check_end(state);
}

/* Keep a new best score even if the game is closed mid-game (tab hidden,
   page closed, window quit). Demo scores never count. */
void game_save_progress(GameState *state)
{
    if (state->hud.demo || state->phase == GAME_PHASE_TITLE)
        return;
    if (state->player.score > state->high_score) {
        high_score_save(state);
        state->high_score = state->player.score;
    }
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
