#include "galaga.h"

#include <SDL.h>
#include <stdio.h>

int main(void)
{
    GameState state;
    game_init(&state);

    render_init();

    Uint64 prev = SDL_GetPerformanceCounter();
    const double freq = (double)SDL_GetPerformanceFrequency();

    while (state.running) {
        bool move_left, move_right, fire, quit;
        input_poll(&move_left, &move_right, &fire, &quit);
        if (quit)
            break;

        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)((now - prev) / freq);
        prev = now;
        if (dt > 0.05f)
            dt = 0.05f;

        game_tick(&state, dt, move_left, move_right, fire);
        render_frame(&state);
    }

    render_shutdown();
    return 0;
}
