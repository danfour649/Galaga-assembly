#include "galaga.h"
#include "synth.h"

#include <stdio.h>
#include <string.h>

static int failures;

static void expect_int(const char *name, int got, int want)
{
    if (got != want) {
        fprintf(stderr, "FAIL %s: got %d want %d\n", name, got, want);
        failures++;
    }
}

static void test_overlap(void)
{
    Rect a = {0, 0, 10, 10};
    Rect hit = {9, 9, 4, 4};
    Rect touch = {10, 0, 4, 4};
    Rect miss = {100, 100, 4, 4};

    expect_int("overlap hit", rect_overlap(&a, &hit), 1);
    expect_int("overlap edge", rect_overlap(&a, &touch), 0);
    expect_int("overlap miss", rect_overlap(&a, &miss), 0);
}

static void test_spawn_and_bullets(void)
{
    GameState state;
    int idx;
    int boss;
    int b0, b1, b2;
    int y0;

    memset(&state, 0, sizeof(state));
    idx = entity_spawn(&state, ENTITY_KIND_ENEMY, ENEMY_TYPE_BEE, 123, 45);
    expect_int("spawn idx", idx >= 0, 1);
    if (idx < 0)
        return;
    expect_int("spawn x", state.enemies[idx].x, 123);
    expect_int("spawn y", state.enemies[idx].y, 45);
    expect_int("bee hp", state.enemies[idx].hp, 1);

    boss = entity_spawn(&state, ENTITY_KIND_ENEMY, ENEMY_TYPE_BOSS, 10, 20);
    expect_int("boss idx", boss >= 0, 1);
    if (boss >= 0)
        expect_int("boss hp", state.enemies[boss].hp, 2);

    b0 = entity_spawn(&state, ENTITY_KIND_BULLET, 1, 77, 200);
    expect_int("bullet0 idx", b0 >= 0, 1);
    if (b0 < 0)
        return;
    expect_int("bullet x", state.bullets[b0].x, 77);
    expect_int("bullet from player", state.bullets[b0].from_player, 1);

    b1 = entity_spawn(&state, ENTITY_KIND_BULLET, 1, 78, 190);
    expect_int("bullet1 idx", b1 >= 0, 1);
    b2 = entity_spawn(&state, ENTITY_KIND_BULLET, 1, 79, 180);
    expect_int("fire cap", b2, -1);

    y0 = state.bullets[b0].y;
    entity_tick_all(&state, 2);
    expect_int("bullet speed", state.bullets[b0].y, y0 - 6);
}

static void test_clamp_and_score(void)
{
    Player player = {0, 0, 16, 16, 0, 0, 0};
    GameState state;

    player.x = -5;
    player_clamp_x(&player);
    expect_int("clamp left", player.x, 0);
    player.x = 300;
    player_clamp_x(&player);
    expect_int("clamp right", player.x, GALAGA_WIDTH - 16);

    memset(&state, 0, sizeof(state));
    player_init(&state);
    game_state_init(&state);
    score_add_points(&state, SCORE_BEE_FORMATION);
    expect_int("score", state.player.score, SCORE_BEE_FORMATION);
}

static void test_player_hits(void)
{
    GameState state;
    int bullet;

    memset(&state, 0, sizeof(state));
    player_init(&state);
    bullet = entity_spawn(&state, ENTITY_KIND_BULLET, 0,
                           state.player.x, state.player.y);
    expect_int("enemy bullet", bullet >= 0, 1);
    collision_resolve(&state, &state.last_hits);
    expect_int("bullet hit lives", state.player.lives, 2);
    if (bullet >= 0)
        expect_int("bullet consumed", state.bullets[bullet].active, 0);

    memset(&state, 0, sizeof(state));
    player_init(&state);
    state.enemies[0].active = true;
    state.enemies[0].x = state.player.x;
    state.enemies[0].y = state.player.y;
    state.enemies[0].w = 14;
    state.enemies[0].h = 14;
    state.enemies[0].hp = 1;
    collision_resolve(&state, &state.last_hits);
    expect_int("body hit lives", state.player.lives, 2);
    expect_int("body consumed", state.enemies[0].active, 0);
}

static void test_entry_dive_and_stage(void)
{
    GameState state;
    int i;

    memset(&state, 0, sizeof(state));
    player_init(&state);
    game_state_init(&state);
    enemies_spawn_formation(&state);
    expect_int("entry state", state.enemies[0].state, ENEMY_STATE_ENTERING);
    expect_int("entry y", state.enemies[0].y, state.enemies[0].home_y - 24);

    for (i = 0; i < 12; i++)
        enemies_tick_all(&state, 2);
    expect_int("formed state", state.enemies[0].state, ENEMY_STATE_FORMATION);
    expect_int("formed y", state.enemies[0].y, state.enemies[0].home_y);

    for (i = 0; i < MAX_ENEMIES; i++) {
        if (!state.enemies[i].active)
            continue;
        state.enemies[i].state = ENEMY_STATE_FORMATION;
        state.enemies[i].x = state.enemies[i].home_x;
        state.enemies[i].y = state.enemies[i].home_y;
        state.enemies[i].vx = 0;
    }
    state.frame = 54;
    enemies_tick_all(&state, 2);
    expect_int("frame 54 dive", state.enemies[1].state, ENEMY_STATE_DIVING);
    expect_int("frame 54 second dive", state.enemies[8].state, ENEMY_STATE_DIVING);
    expect_int("dive aims +x", state.enemies[1].vx > 0, 1);

    memset(&state, 0, sizeof(state));
    player_init(&state);
    state.player.stage = 1;
    state.player.x = 40;
    state.enemies[0].active = true;
    state.enemies[0].type = ENEMY_TYPE_BEE;
    state.enemies[0].state = ENEMY_STATE_FORMATION;
    state.enemies[0].x = 40;
    state.enemies[0].y = 10;
    state.enemies[0].home_x = 40;
    state.enemies[0].home_y = 10;
    state.enemies[0].w = 14;
    state.enemies[0].h = 14;
    state.enemies[0].hp = 1;
    state.frame = 0;
    enemies_tick_all(&state, 2);
    expect_int("dx 0 aims +x", state.enemies[0].vx, 2);

    memset(&state, 0, sizeof(state));
    player_init(&state);
    game_state_begin_stage_clear(&state);
    for (i = 0; i < 90; i++)
        game_state_tick(&state, 0);
    expect_int("clear holds", state.phase, GAME_PHASE_STAGE_CLEAR);
    expect_int("clear timer", state.clear_timer, 0);
    game_state_tick(&state, 0);
    expect_int("clear advances", state.phase, GAME_PHASE_PLAYING);
    expect_int("next stage", state.player.stage, 2);
}

static void test_high_score_and_step(void)
{
    GameState state;
    GameState loaded;
    int x;
    int i;

    remove(HIGH_SCORE_FILE);
    memset(&state, 0, sizeof(state));
    state.player.score = 10;
    state.high_score = 10;
    high_score_save(&state);
    memset(&loaded, 0, sizeof(loaded));
    high_score_load(&loaded);
    expect_int("no worse high score", loaded.high_score, 0);

    state.player.score = 250;
    high_score_save(&state);
    memset(&loaded, 0, sizeof(loaded));
    high_score_load(&loaded);
    expect_int("saved high score", loaded.high_score, 250);
    remove(HIGH_SCORE_FILE);

    game_init(&state);
    state.phase = GAME_PHASE_PLAYING;
    state.player.score = 250;
    state.player.lives = 1;
    state.enemies[0].active = true;
    state.enemies[0].state = ENEMY_STATE_ENTERING;
    state.enemies[0].x = state.player.x;
    state.enemies[0].y = state.player.y;
    state.enemies[0].home_x = state.player.x;
    state.enemies[0].home_y = state.player.y;
    state.enemies[0].w = 14;
    state.enemies[0].h = 14;
    state.enemies[0].hp = 1;
    game_tick(&state, 1.0f / 60.0f, false, false, false);
    expect_int("death phase", state.phase, GAME_PHASE_GAME_OVER);
    expect_int("death high score", state.high_score, 250);
    memset(&loaded, 0, sizeof(loaded));
    high_score_load(&loaded);
    expect_int("death file", loaded.high_score, 250);
    remove(HIGH_SCORE_FILE);

    game_init(&state);
    state.phase = GAME_PHASE_PLAYING;
    state.enemies[0].active = true;
    state.enemies[0].state = ENEMY_STATE_FORMATION;
    state.enemies[0].x = 0;
    state.enemies[0].y = 0;
    state.enemies[0].home_x = 0;
    state.enemies[0].home_y = 0;
    state.enemies[0].w = 14;
    state.enemies[0].h = 14;
    state.enemies[0].hp = 1;
    x = state.player.x;
    for (i = 0; i < 16; i++)
        game_tick(&state, 0.001f, false, true, false);
    expect_int("high refresh holds", state.player.x, x);
    game_tick(&state, 0.001f, false, true, false);
    expect_int("60 Hz step", state.player.x, x + 1);
    x = state.player.x;
    game_tick(&state, 1.0f, false, true, false);
    expect_int("step cap", state.player.x, x + 4);
    remove(HIGH_SCORE_FILE);
}

static void test_flight(void)
{
    static GameState state;
    int counts[3] = {0, 0, 0};
    int i, t, formed_at = -1, dive_seen = 0, top_entry = 0, bottom_entry = 0;

    game_init(&state);
    state.flight.rng = 12345;
    game_tick(&state, 1.0f / 60.0f, false, false, true);   /* start */
    expect_int("flight playing", state.phase, GAME_PHASE_PLAYING);
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (state.enemies[i].active)
            counts[state.enemies[i].type]++;
    }
    expect_int("rack bees", counts[ENEMY_TYPE_BEE], 20);
    expect_int("rack butterflies", counts[ENEMY_TYPE_BUTTERFLY], 16);
    expect_int("rack bosses", counts[ENEMY_TYPE_BOSS], 4);
    expect_int("waiting off screen", state.enemies[0].y < 0, 1);

    for (t = 0; t < 1800 && formed_at < 0; t++) {
        state.player.lives = 9;
        state.player.x = 0;   /* park the ship away from the paths */
        game_tick(&state, 1.0f / 60.0f, false, false, false);
        for (i = 0; i < MAX_ENEMIES; i++) {
            if (state.enemies[i].y > 150 && state.enemies[i].y < 240 && state.enemies[i].x > 0)
                bottom_entry = 1;
            if (state.enemies[i].y > -16 && state.enemies[i].y < 10)
                top_entry = 1;
        }
        if (state.flight.formed)
            formed_at = t;
    }
    expect_int("rack formed", formed_at > 0, 1);
    expect_int("entry from top", top_entry, 1);
    expect_int("entry from bottom", bottom_entry, 1);
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (!state.enemies[i].active)
            continue;
        if (state.enemies[i].state != ENEMY_STATE_FORMATION) {
            expect_int("all in formation", state.enemies[i].state, ENEMY_STATE_FORMATION);
            break;
        }
    }
    expect_int("no dive before formed", dive_seen, 0);
    for (t = 0; t < 600 && !dive_seen; t++) {
        state.player.lives = 9;
        game_tick(&state, 1.0f / 60.0f, false, false, false);
        for (i = 0; i < MAX_ENEMIES; i++)
            if (state.enemies[i].active && state.enemies[i].state == ENEMY_STATE_DIVING)
                dive_seen = 1;
    }
    expect_int("dive after formed", dive_seen, 1);

    /* escort bonus: diving Boss shot after both escorts = 1600 total */
    memset(&state, 0, sizeof(state));
    player_init(&state);
    game_state_init(&state);
    state.enemies[0].active = true;
    state.enemies[0].type = ENEMY_TYPE_BOSS;
    state.enemies[0].state = ENEMY_STATE_DIVING;
    state.flight.f[0].mode = 6;            /* FLIGHT_ATTACK */
    state.flight.f[0].escorts_lost = 2;
    state.last_hits.count = 1;
    state.last_hits.type[0] = HIT_PLAYER_BULLET_ENEMY;
    state.last_hits.enemy_idx[0] = 0;
    state.enemies[0].active = false;       /* collision already killed it */
    state.player.score = SCORE_BOSS_DIVING;
    flight_on_hits(&state);
    expect_int("escort bonus", state.player.score, 1600);
}

static void test_extra_lives_and_respawn(void)
{
    static GameState state;
    int i, lives;

    memset(&state, 0, sizeof(state));
    player_init(&state);
    game_state_init(&state);
    lives = state.player.lives;
    score_add_points(&state, 20000);
    expect_int("extra life 20k", state.player.lives, lives + 1);
    score_add_points(&state, 50000);
    expect_int("extra life 70k", state.player.lives, lives + 2);
    score_add_points(&state, 70000);
    expect_int("extra life 140k", state.player.lives, lives + 3);
    score_add_points(&state, 70000);
    expect_int("extra life 210k", state.player.lives, lives + 4);
    expect_int("next extra life", state.next_extra_life, 280000);

    /* banners hold the first wave; a hit hides the fighter, then respawns */
    game_init(&state);
    game_tick(&state, 1.0f / 60.0f, false, false, true);
    expect_int("player1 banner", state.hud.banner, BANNER_PLAYER1);
    for (i = 0; i < BANNER_PLAYER1_STEPS; i++)
        game_tick(&state, 1.0f / 60.0f, false, false, false);
    expect_int("stage banner", state.hud.banner, BANNER_STAGE);
    expect_int("wave waits", state.enemies[0].y < 0, 1);
    for (i = 0; i < BANNER_STAGE_STEPS; i++)
        game_tick(&state, 1.0f / 60.0f, false, false, false);
    expect_int("banner done", state.hud.banner, BANNER_NONE);

    lives = state.player.lives;
    i = entity_spawn(&state, ENTITY_KIND_BULLET, 0, state.player.x + 4, state.player.y + 4);
    expect_int("enemy shot", i >= 0, 1);
    game_tick(&state, 1.0f / 60.0f, false, false, false);
    expect_int("hit costs life", state.player.lives, lives - 1);
    expect_int("dead timer", state.hud.dead_timer > 0, 1);
    expect_int("fighter hidden", state.player.y > GALAGA_HEIGHT, 1);
    for (i = 0; i < PLAYER_DEAD_STEPS; i++)
        game_tick(&state, 1.0f / 60.0f, false, false, false);
    expect_int("respawned", state.player.y, GALAGA_HEIGHT - 36);
    expect_int("respawn timer", state.hud.dead_timer, 0);
}

static void test_capture_and_dual(void)
{
    static GameState s;
    int i, b, lives, t;

    game_init(&s);
    game_tick(&s, 1.0f / 60.0f, false, false, true);
    for (i = 0; i < BANNER_PLAYER1_STEPS + BANNER_STAGE_STEPS + 2; i++)
        game_tick(&s, 1.0f / 60.0f, false, false, false);
    for (i = 0; i < MAX_ENEMIES; i++)
        s.enemies[i].active = false;
    b = flight_spawn_formed(&s, ENEMY_TYPE_BOSS, 4, 0, 112.0f, 40.0f);
    expect_int("capture boss", b >= 0, 1);
    s.flight.dive_timer = 1000000;
    for (i = 0; i < 120; i++)
        game_tick(&s, 1.0f / 60.0f, false, false, false);

    lives = s.player.lives;
    flight_start_beam_dive(&s, b);
    expect_int("beam boss", s.capture.beam_boss, b);
    for (t = 0; t < 900 && s.capture.state != CAPT_HELD; t++) {
        s.flight.dive_timer = 1000000;
        game_tick(&s, 1.0f / 60.0f, false, false, false);
    }
    expect_int("captured", s.capture.state, CAPT_HELD);
    expect_int("capture costs life", s.player.lives, lives - 1);
    expect_int("beam over", s.capture.beam_boss, -1);

    /* shoot the Boss on its way home (still DIVING): captive is freed */
    for (t = 0; t < 40; t++)
        game_tick(&s, 1.0f / 60.0f, false, false, false);
    expect_int("boss returning", s.enemies[b].state, ENEMY_STATE_DIVING);
    s.enemies[b].hp = 1;
    i = entity_spawn(&s, ENTITY_KIND_BULLET, 1, s.enemies[b].x + 5, s.enemies[b].y + 4);
    expect_int("rescue shot", i >= 0, 1);
    game_tick(&s, 1.0f / 60.0f, false, false, false);
    expect_int("boss down", s.enemies[b].active, 0);
    expect_int("rescue", s.capture.state, CAPT_RESCUE);
    for (t = 0; t < 900 && !s.capture.dual; t++)
        game_tick(&s, 1.0f / 60.0f, false, false, false);
    expect_int("dual fighter", s.capture.dual, 1);
    expect_int("dual width", s.player.w, 32);

    /* dual fires a pair */
    for (i = 0; i < MAX_BULLETS; i++)
        s.bullets[i].active = false;
    capture_fire(&s);
    t = 0;
    for (i = 0; i < MAX_BULLETS; i++)
        t += s.bullets[i].active && s.bullets[i].from_player;
    expect_int("dual pair", t, 2);

    /* a hit on the left ship costs that ship, not a life */
    lives = s.player.lives;
    {
        int x0 = s.player.x;
        for (i = 0; i < MAX_BULLETS; i++)
            s.bullets[i].active = false;
        entity_spawn(&s, ENTITY_KIND_BULLET, 0, s.player.x + 2, s.player.y + 2);
        game_tick(&s, 1.0f / 60.0f, false, false, false);
        expect_int("dual hit keeps life", s.player.lives, lives);
        expect_int("dual hit single", s.capture.dual, 0);
        expect_int("dual hit width", s.player.w, 16);
        expect_int("right ship remains", s.player.x, x0 + 16);
    }
}

static void test_challenging_stage(void)
{
    static GameState state;
    int i, enemy_shots = 0, saw_result = 0, hits_seen = 0;

    expect_int("stage 3 is challenging", flight_is_challenge_stage(3), 1);
    expect_int("stage 7 is challenging", flight_is_challenge_stage(7), 1);
    expect_int("stage 4 is normal", flight_is_challenge_stage(4), 0);

    game_init(&state);
    state.flight.rng = 12345;
    game_tick(&state, 1.0f / 60.0f, false, false, true);
    /* jump to the end of stage 2 */
    state.player.stage = 2;
    for (i = 0; i < MAX_ENEMIES; i++)
        state.enemies[i].active = false;
    for (i = 0; i < 400 && state.player.stage != 3; i++)
        game_tick(&state, 1.0f / 60.0f, false, false, false);
    expect_int("reached stage 3", state.player.stage, 3);
    expect_int("challenge active", state.challenge.active, 1);
    expect_int("40 challengers", state.challenge.group_left[0] + state.challenge.group_left[1] +
               state.challenge.group_left[2] + state.challenge.group_left[3] +
               state.challenge.group_left[4], 40);

    /* let every group fly through untouched; nobody fires */
    int score0 = state.player.score;
    for (i = 0; i < 3000 && state.player.stage == 3; i++) {
        state.player.lives = 5;           /* ignore fly-through collisions */
        game_tick(&state, 1.0f / 60.0f, false, false, false);
        for (int b = 0; b < MAX_BULLETS; b++)
            if (state.bullets[b].active && !state.bullets[b].from_player)
                enemy_shots++;
        if (state.challenge.result == 1)
            saw_result = 1;
    }
    expect_int("challengers never fire", enemy_shots, 0);
    expect_int("results shown", saw_result, 1);
    expect_int("no hits, no bonus", state.player.score - score0, 0);
    expect_int("advanced to stage 4", state.player.stage, 4);
    for (i = 0; i < 5 && !state.challenge.active; i++)
        game_tick(&state, 1.0f / 60.0f, false, false, false);
    expect_int("stage 4 is a normal stage", state.challenge.active, 0);

    /* perfect: shoot every challenger -> 100 each + group bonuses + 10000 */
    game_init(&state);
    state.flight.rng = 777;
    game_tick(&state, 1.0f / 60.0f, false, false, true);
    state.player.stage = 2;
    for (i = 0; i < MAX_ENEMIES; i++)
        state.enemies[i].active = false;
    for (i = 0; i < 400 && state.player.stage != 3; i++)
        game_tick(&state, 1.0f / 60.0f, false, false, false);
    score0 = state.player.score;
    for (i = 0; i < 3000 && state.player.stage == 3; i++) {
        state.player.lives = 5;
        /* a player bullet on every visible challenger */
        for (int e = 0; e < MAX_ENEMIES; e++) {
            const Enemy *en = &state.enemies[e];
            if (en->active && en->x > 0 && en->x < GALAGA_WIDTH - 16 &&
                en->y > 20 && en->y < GALAGA_HEIGHT - 40)
                entity_spawn(&state, ENTITY_KIND_BULLET, 1, en->x + 6, en->y + 4);
            for (int b = 0; b < MAX_BULLETS; b++)
                if (state.bullets[b].active && state.bullets[b].from_player &&
                    state.bullets[b].y < 0)
                    state.bullets[b].active = false;
        }
        game_tick(&state, 1.0f / 60.0f, false, false, false);
        if (state.challenge.active && state.challenge.hits > hits_seen)
            hits_seen = state.challenge.hits;
    }
    expect_int("perfect hits", hits_seen, 40);
    expect_int("perfect score", state.player.score - score0,
               40 * CHALLENGE_HIT_POINTS + 5 * 1000 + CHALLENGE_PERFECT_BONUS);
}

static int peak(const int16_t *b, int n)
{
    int m = 0;
    for (int i = 0; i < n; i++) {
        int v = b[i] < 0 ? -b[i] : b[i];
        if (v > m) m = v;
    }
    return m;
}

static void test_synth_and_sound_watch(void)
{
    static GameState state;
    static SoundWatch w;
    static int16_t buf[4800];
    int i;

    synth_init(48000);
    synth_set_mute(0);
    synth_render(buf, 4800);
    expect_int("silence when idle", peak(buf, 4800), 0);
    for (i = 0; i < SND_COUNT; i++) {
        synth_init(48000);
        synth_play(i);
        synth_render(buf, 4800);
        if (peak(buf, 4800) < 1000)
            expect_int("every sound is audible", i, -1);
    }
    /* a shot ends after ~70 ms */
    synth_init(48000);
    synth_play(SND_SHOT);
    synth_render(buf, 4800);
    synth_render(buf, 4800);
    expect_int("shot finished", synth_active_voices(), 0);
    /* muted: nothing plays */
    synth_init(48000);
    synth_set_mute(1);
    synth_play(SND_START);
    synth_render(buf, 4800);
    expect_int("muted is silent", peak(buf, 4800), 0);
    synth_set_mute(0);

    /* watcher: start jingle on PLAYER 1, a shot sound when firing */
    synth_init(48000);
    memset(&w, 0, sizeof(w));
    game_init(&state);
    sound_watch_update(&w, &state);
    game_tick(&state, 1.0f / 60.0f, false, false, true);
    sound_watch_update(&w, &state);
    expect_int("start jingle", (w.played >> SND_START) & 1, 1);
    for (i = 0; i < BANNER_PLAYER1_STEPS + 2; i++)
        game_tick(&state, 1.0f / 60.0f, false, false, false);
    sound_watch_update(&w, &state);
    expect_int("no stage chime after PLAYER 1", (w.played >> SND_STAGE) & 1, 0);
    for (i = 0; i < BANNER_STAGE_STEPS; i++)
        game_tick(&state, 1.0f / 60.0f, false, false, false);
    sound_watch_update(&w, &state);
    game_tick(&state, 1.0f / 60.0f, false, false, true);
    sound_watch_update(&w, &state);
    expect_int("shot sound", (w.played >> SND_SHOT) & 1, 1);
    game_tick(&state, 1.0f / 60.0f, false, false, false);
    sound_watch_update(&w, &state);
    expect_int("shot sound once", (w.played >> SND_SHOT) & 1, 0);
}

static void test_shots_hits_and_results(void)
{
    static GameState state;
    int i, live;

    game_init(&state);
    state.flight.rng = 31337;
    game_tick(&state, 1.0f / 60.0f, false, false, true);
    for (i = 0; i < BANNER_PLAYER1_STEPS + BANNER_STAGE_STEPS + 2; i++)
        game_tick(&state, 1.0f / 60.0f, false, false, false);
    expect_int("counters start at 0", state.hud.shots + state.hud.hits, 0);

    /* park every enemy away; fire 3 single shots into empty space */
    for (i = 0; i < MAX_ENEMIES; i++)
        state.enemies[i].active = false;
    state.enemies[0].active = true;          /* keep the stage alive */
    state.enemies[0].x = 0; state.enemies[0].y = 0;
    state.flight.f[0].mode = 0;
    for (int k = 0; k < 3; k++) {
        game_tick(&state, 1.0f / 60.0f, false, false, true);
        for (i = 0; i < 200; i++) {
            live = 0;
            for (int b = 0; b < MAX_BULLETS; b++)
                live += state.bullets[b].active && state.bullets[b].from_player;
            if (!live) break;
            game_tick(&state, 1.0f / 60.0f, false, false, false);
        }
    }
    expect_int("3 shots fired", state.hud.shots, 3);
    expect_int("no hits", state.hud.hits, 0);

    /* a shot straight into an enemy above the fighter is a hit */
    Enemy *e = &state.enemies[1];
    memset(e, 0, sizeof(*e));
    e->active = true; e->type = ENEMY_TYPE_BEE; e->hp = 1; e->w = 14; e->h = 14;
    e->state = ENEMY_STATE_FORMATION;
    e->x = state.player.x + 1; e->y = state.player.y - 40;
    state.flight.f[1].mode = 0;
    game_tick(&state, 1.0f / 60.0f, false, false, true);
    for (i = 0; i < 30 && state.enemies[1].active; i++)
        game_tick(&state, 1.0f / 60.0f, false, false, false);
    expect_int("4 shots fired", state.hud.shots, 4);
    expect_int("1 hit", state.hud.hits, 1);

    /* game over: fire is ignored until results are up, then auto title */
    state.player.lives = 1;
    player_lose_life(&state);
    expect_int("game over", state.phase, GAME_PHASE_GAME_OVER);
    for (i = 0; i < GAMEOVER_FIRE_LOCK - 5; i++)
        game_tick(&state, 1.0f / 60.0f, false, false, true);
    expect_int("fire locked on results", state.phase, GAME_PHASE_GAME_OVER);
    expect_int("results showing", state.hud.over_t >= GAMEOVER_RESULTS_AT, 1);
    for (i = 0; i < GAMEOVER_TITLE_AT; i++)
        game_tick(&state, 1.0f / 60.0f, false, false, false);
    expect_int("back to title", state.phase, GAME_PHASE_TITLE);
    game_tick(&state, 1.0f / 60.0f, false, false, true);
    expect_int("new game resets shots", state.hud.shots, 0);
    expect_int("new game resets hits", state.hud.hits, 0);
}

int main(void)
{
    test_overlap();
    test_spawn_and_bullets();
    test_clamp_and_score();
    test_player_hits();
    test_entry_dive_and_stage();
    test_high_score_and_step();
    test_flight();
    test_extra_lives_and_respawn();
    test_capture_and_dual();
    test_challenging_stage();
    test_synth_and_sound_watch();
    test_shots_hits_and_results();
    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    printf("sim_test ok\n");
    return 0;
}
