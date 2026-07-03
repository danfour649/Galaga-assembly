#include "galaga.h"

#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

EM_JS(int, high_score_load_js, (void), {
    var v = localStorage.getItem('galaga_hi');
    return v ? (parseInt(v, 10) || 0) : 0;
});

EM_JS(void, high_score_save_js, (int score), {
    localStorage.setItem('galaga_hi', score);
});
#endif

void high_score_load(GameState *gs)
{
#ifdef __EMSCRIPTEN__
    gs->high_score = high_score_load_js();
#else
    FILE *f = fopen(HIGH_SCORE_FILE, "r");
    if (!f) {
        gs->high_score = 0;
        return;
    }
    unsigned int v = 0;
    if (fscanf(f, "%u", &v) == 1)
        gs->high_score = (int)v;
    fclose(f);
#endif
}

void high_score_save(const GameState *gs)
{
    if (gs->player.score <= gs->high_score)
        return;
#ifdef __EMSCRIPTEN__
    high_score_save_js(gs->player.score);
#else
    FILE *f = fopen(HIGH_SCORE_FILE, "w");
    if (!f)
        return;
    fprintf(f, "%d\n", gs->player.score);
    fclose(f);
#endif
}
