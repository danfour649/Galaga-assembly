#include "galaga.h"

#include <SDL.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
/* Set from the page's on-screen touch buttons (web/shell.html). */
static int touch_left, touch_right, touch_fire;
EMSCRIPTEN_KEEPALIVE void galaga_set_touch(int left, int right, int fire)
{
    touch_left = left;
    touch_right = right;
    touch_fire = fire;
}
#endif

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
        if (event.type == SDL_KEYDOWN && !event.key.repeat &&
            (event.key.keysym.sym == SDLK_F11 ||
             (event.key.keysym.sym == SDLK_RETURN &&
              (event.key.keysym.mod & KMOD_ALT))))
            render_toggle_fullscreen();
#ifndef __EMSCRIPTEN__
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
            *quit = true;
#endif
    }

    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A])
        *move_left = true;
    if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D])
        *move_right = true;
    if (keys[SDL_SCANCODE_SPACE])
        *fire = true;
#ifdef __EMSCRIPTEN__
    *move_left  |= touch_left != 0;
    *move_right |= touch_right != 0;
    *fire       |= touch_fire != 0;
#endif
}
