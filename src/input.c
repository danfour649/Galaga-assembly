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

/* Drag-to-move: while a finger is down on the playfield the page reports
   how far it has moved since touching down, as a fraction of the canvas
   width in 1/1000ths. The fighter follows that offset from where it was. */
static int drag_active, drag_milli, drag_anchor;
EMSCRIPTEN_KEEPALIVE void galaga_set_drag(int active, int dx_milli)
{
    drag_active = active ? 1 : 0;
    drag_milli = dx_milli;
    if (active == 2)              /* touch down: re-anchor on the next poll */
        drag_anchor = -1;
}

void input_apply_drag(const GameState *s, bool *move_left, bool *move_right)
{
    if (!drag_active)
        return;
    int cx = s->player.x + s->player.w / 2;
    if (drag_anchor < 0)
        drag_anchor = cx;
    int target = drag_anchor + drag_milli * GALAGA_WIDTH / 1000;
    if (target < cx - 1)
        *move_left = true;
    else if (target > cx + 1)
        *move_right = true;
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
        /* M toggles sound (the web page handles M itself, with its button) */
        if (event.type == SDL_KEYDOWN && !event.key.repeat &&
            event.key.keysym.sym == SDLK_m)
            audio_set_mute(!audio_muted());
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
