#include "galaga.h"

#include <string.h>

int main(void)
{
    GameState state;

    memset(&state, 0, sizeof(state));
    state.phase = GAME_PHASE_TITLE;
    state.player.lives = 3;
    state.player.stage = 1;

    render_init();
    render_frame(&state);
    state.phase = GAME_PHASE_PLAYING;
    render_frame(&state);
    render_shutdown();
    return 0;
}
