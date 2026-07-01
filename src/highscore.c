#include "galaga.h"

#include <stdio.h>


void high_score_load(GameState *gs) {
    FILE *f = fopen(HIGH_SCORE_FILE, "r");
    if (!f) {
        gs->high_score = 0;
        return;
    }
    unsigned int v = 0;
    if (fscanf(f, "%u", &v) == 1) {
        gs->high_score = (int)v;
    }
    fclose(f);
}

void high_score_save(const GameState *gs)
{
    if (gs->player.score <= gs->high_score)
        return;
    FILE *f = fopen(HIGH_SCORE_FILE, "w");
    if (!f)
        return;
    fprintf(f, "%d\n", gs->player.score);
    fclose(f);
}
