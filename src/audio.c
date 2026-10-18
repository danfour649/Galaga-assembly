#include "galaga.h"
#include "synth.h"

#include <SDL.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

/* SDL audio output for src/synth.c. On the web the device is opened only
   from galaga_audio_unlock(), which the page calls inside a user gesture
   (tap / key / click), so the AudioContext starts under autoplay rules. */

static SDL_AudioDeviceID dev;
static SoundWatch watch;
static int want_mute;

static void SDLCALL audio_cb(void *ud, Uint8 *stream, int len)
{
    (void)ud;
    synth_render((int16_t *)stream, len / (int)sizeof(int16_t));
}

static void audio_open(void)
{
    if (dev)
        return;
    if (!SDL_WasInit(SDL_INIT_AUDIO) && SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
        return;                       /* no audio: play silently */
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = 48000;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 1024;
    want.callback = audio_cb;
    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if (!dev)
        return;
    synth_init(have.freq);
    synth_set_mute(want_mute);
    SDL_PauseAudioDevice(dev, 0);
}

void audio_init(void)
{
#ifndef __EMSCRIPTEN__
    audio_open();
#endif
}

void audio_update(const GameState *state)
{
    /* The watcher runs even before the device opens (web: first gesture),
       so haptic events work too; with no device there is no audio thread
       and synth calls are harmless. */
    if (dev) SDL_LockAudioDevice(dev);
    sound_watch_update(&watch, state);
    if (dev) SDL_UnlockAudioDevice(dev);
}

uint32_t audio_haptic_events(void) { return watch.haptic; }

void audio_set_mute(int mute)
{
    want_mute = mute ? 1 : 0;
    if (dev) SDL_LockAudioDevice(dev);
    synth_set_mute(want_mute);
    if (dev) SDL_UnlockAudioDevice(dev);
}

int audio_muted(void) { return want_mute; }

void audio_shutdown(void)
{
    if (dev) {
        SDL_CloseAudioDevice(dev);
        dev = 0;
    }
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE void galaga_audio_unlock(void)
{
    audio_open();
    /* SDL's port keeps its context on Module.SDL2; make sure it runs */
    EM_ASM({
        var c = Module['SDL2'] && Module['SDL2'].audioContext;
        if (c && c.state !== 'running' && c.resume) c.resume();
    });
}

EMSCRIPTEN_KEEPALIVE void galaga_set_mute(int mute) { audio_set_mute(mute); }

EMSCRIPTEN_KEEPALIVE int galaga_audio_state(void)
{
    /* 0 = not opened, 1 = opened (for the page / tests) */
    return dev ? 1 : 0;
}
#endif
