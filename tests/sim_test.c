#include "galaga.h"

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

int main(void)
{
    test_overlap();
    test_spawn_and_bullets();
    test_clamp_and_score();
    test_player_hits();
    test_entry_dive_and_stage();
    test_high_score_and_step();
    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    printf("sim_test ok\n");
    return 0;
}
