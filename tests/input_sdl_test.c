// SPDX-License-Identifier: GPL-3.0-only
#include "input_sdl.h"
#include <stdio.h>
#define CHECK(x) do { if(!(x)){fprintf(stderr,"SDL input line%d: %s\n",__LINE__,#x);return 1;} } while(0)
static void key(InputSDL *adapter,SDL_Scancode code,int down,int repeat,SDL_Keymod mods) {
    SDL_Event event={0};event.type=down?SDL_EVENT_KEY_DOWN:SDL_EVENT_KEY_UP;
    event.key.scancode=code;event.key.repeat=repeat;event.key.mod=mods;
    /* Localized symbol differs deliberately: shortcuts are physical. */
    event.key.key=SDLK_Q;input_sdl_event(adapter,&event);
}
int main(void) {
    AppInput input;input_init(&input);InputSDL adapter;input_sdl_init(&adapter,&input,0);
    for(int i=0;i<26;i++)CHECK(input_sdl_key(SDL_SCANCODE_A+i)=='A'+i);
    for(int i=0;i<9;i++)CHECK(input_sdl_key(SDL_SCANCODE_1+i)=='1'+i);
    CHECK(input_sdl_key(SDL_SCANCODE_0)=='0');CHECK(input_sdl_key(SDL_SCANCODE_UNKNOWN)==-1);
    CHECK(input_sdl_key(SDL_SCANCODE_KP_ENTER)==APP_KEY_KP_ENTER);
    CHECK(input_sdl_key(SDL_SCANCODE_LEFTBRACKET)=='[');CHECK(input_sdl_key(SDL_SCANCODE_RIGHTBRACKET)==']');
    CHECK(input_sdl_key(SDL_SCANCODE_LGUI)==APP_KEY_LEFT_SUPER);CHECK(input_sdl_key(SDL_SCANCODE_RCTRL)==APP_KEY_RIGHT_CONTROL);
    input_begin(&input,100);key(&adapter,SDL_SCANCODE_A,1,0,0);
    CHECK(input_key_pressed(&input,'A'));CHECK(!input_key_down(&input,'Q'));CHECK(input_next_key(&input)=='A');
    input_begin(&input,200);key(&adapter,SDL_SCANCODE_A,1,1,0);
    CHECK(input_key_repeat(&input,'A'));CHECK(input_key_down(&input,'A'));CHECK(!input_key_pressed(&input,'A'));CHECK(input_next_key(&input)==0);
    SDL_Event event={0};event.type=SDL_EVENT_TEXT_INPUT;event.text.text="A\xc3\xa9\xce\xbb";
    CHECK(input_sdl_event(&adapter,&event));CHECK(input_next_char(&input)=='A');CHECK(input_next_char(&input)==0xe9);CHECK(input_next_char(&input)==0x3bb);CHECK(input_next_char(&input)==0);
    event.text.text="abcdefghijklmnopqrst";input_sdl_event(&adapter,&event);
    for(int i=0;i<16;i++)CHECK(input_next_char(&input)=='a'+i);CHECK(input_next_char(&input)==0);
    event=(SDL_Event){0};event.type=SDL_EVENT_MOUSE_BUTTON_DOWN;event.button.button=SDL_BUTTON_RIGHT;
    input_sdl_event(&adapter,&event);CHECK(input_button_pressed(&input,1));CHECK(!input_button_down(&input,2));
    event.button.button=SDL_BUTTON_MIDDLE;input_sdl_event(&adapter,&event);CHECK(input_button_pressed(&input,2));
    event=(SDL_Event){0};event.type=SDL_EVENT_MOUSE_MOTION;event.motion.x=12.125f;event.motion.y=-7.25f;
    input_sdl_event(&adapter,&event);CHECK(input.pointer.x==12.125f && input.pointer.y==-7.25f);
    event=(SDL_Event){0};event.type=SDL_EVENT_MOUSE_WHEEL;event.wheel.x=8;event.wheel.y=1;input_sdl_event(&adapter,&event);
    event.wheel.x=.25f;event.wheel.y=-.5f;event.wheel.direction=SDL_MOUSEWHEEL_FLIPPED;input_sdl_event(&adapter,&event);
    CHECK(scroll_input_scalar(&input.scroll)==-.5f);CHECK(input.scroll.wheel.x==.25f);
    key(&adapter,SDL_SCANCODE_CAPSLOCK,1,0,SDL_KMOD_CAPS);key(&adapter,SDL_SCANCODE_CAPSLOCK,0,0,SDL_KMOD_CAPS);
    CHECK(input_key_down(&input,APP_KEY_CAPS_LOCK));
    input_begin(&input,300);event=(SDL_Event){0};event.type=SDL_EVENT_WINDOW_FOCUS_LOST;input_sdl_event(&adapter,&event);
    CHECK(input_key_released(&input,'A'));CHECK(input_button_released(&input,1));CHECK(input_button_released(&input,2));
    CHECK(input_key_down(&input,APP_KEY_CAPS_LOCK));CHECK(!input.focused);
    event.type=SDL_EVENT_WINDOW_FOCUS_GAINED;input_sdl_event(&adapter,&event);CHECK(input.focused);
    input_begin(&input,400);input_sdl_init(&adapter,&input,1);
    InputEvent native={.kind=INPUT_NATIVE_SCROLL,.data.scroll={4.125,8.25,1}};input_event(&input,&native);
    event=(SDL_Event){0};event.type=SDL_EVENT_MOUSE_WHEEL;event.wheel.x=99;event.wheel.y=99;input_sdl_event(&adapter,&event);
    CHECK(input.scroll.native.x==4.125f && input.scroll.native.y==8.25f);CHECK(input.scroll.wheel.y==(float)(8.25*.1));
    puts("SDL physical keys, repeats, UTF8 queue, buttons, focus releases and wheel boundary passed");return 0;
}
