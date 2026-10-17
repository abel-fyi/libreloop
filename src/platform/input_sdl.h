// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_INPUT_SDL_H
#define LIBRELOOP_INPUT_SDL_H
/* SDL types are restricted to this platform-private boundary. */
#include "input.h"
#include <SDL3/SDL.h>
typedef struct {
    AppInput *input;
    unsigned char held_keys[APP_INPUT_KEYS],held_buttons[APP_INPUT_BUTTONS];
    int native_scroll;
} InputSDL;
void input_sdl_init(InputSDL *adapter,AppInput *input,int native_scroll);
int input_sdl_key(SDL_Scancode scancode);
int input_sdl_event(InputSDL *adapter,const SDL_Event *event);
#endif
