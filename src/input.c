#include "galaga.h"

#include <SDL.h>

void input_poll(bool *move_left, bool *move_right, bool *fire, bool *quit)
{
    *move_left = false;
    *move_right = false;
    *fire = false;
    *quit = false;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT)
            *quit = true;
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
            *quit = true;
    }

    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A])
        *move_left = true;
    if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D])
        *move_right = true;
    if (keys[SDL_SCANCODE_SPACE])
        *fire = true;
}
