#include "galaga.h"

#include <SDL.h>
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

static GameState g_state;
static Uint64    g_prev;
static double    g_freq;

static void main_loop(void)
{
    bool move_left, move_right, fire, quit;
    input_poll(&move_left, &move_right, &fire, &quit);
    if (quit) {
        emscripten_cancel_main_loop();
        render_shutdown();
        return;
    }

    Uint64 now = SDL_GetPerformanceCounter();
    float dt = (float)((now - g_prev) / g_freq);
    g_prev = now;
    if (dt > 0.05f)
        dt = 0.05f;

    game_tick(&g_state, dt, move_left, move_right, fire);
    render_frame(&g_state);
}
#endif

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

#ifdef __EMSCRIPTEN__
    game_init(&g_state);
    render_init();

    g_prev = SDL_GetPerformanceCounter();
    g_freq = (double)SDL_GetPerformanceFrequency();
    emscripten_set_main_loop(main_loop, 0, 1);
    return 0;
#else
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
#endif
}
