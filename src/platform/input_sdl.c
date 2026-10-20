// SPDX-License-Identifier: GPL-3.0-only
#include "input_sdl.h"
#include "utf8.h"
#include <string.h>
void input_sdl_init(InputSDL *adapter,AppInput *input) {
    *adapter=(InputSDL){.input=input};
}
int input_sdl_key(SDL_Scancode code) {
    if(code>=SDL_SCANCODE_A && code<=SDL_SCANCODE_Z)return 'A'+code-SDL_SCANCODE_A;
    if(code>=SDL_SCANCODE_1 && code<=SDL_SCANCODE_9)return '1'+code-SDL_SCANCODE_1;
    if(code>=SDL_SCANCODE_F1 && code<=SDL_SCANCODE_F12)return 290+code-SDL_SCANCODE_F1;
    if(code>=SDL_SCANCODE_F13 && code<=SDL_SCANCODE_F24)return 302+code-SDL_SCANCODE_F13;
    if(code>=SDL_SCANCODE_KP_1 && code<=SDL_SCANCODE_KP_9)return 321+code-SDL_SCANCODE_KP_1;
    switch(code) {
    case SDL_SCANCODE_0:return '0';case SDL_SCANCODE_SPACE:return ' ';
    case SDL_SCANCODE_APOSTROPHE:return '\'';case SDL_SCANCODE_COMMA:return ',';
    case SDL_SCANCODE_MINUS:return '-';case SDL_SCANCODE_PERIOD:return '.';
    case SDL_SCANCODE_SLASH:return '/';case SDL_SCANCODE_SEMICOLON:return ';';
    case SDL_SCANCODE_EQUALS:return '=';case SDL_SCANCODE_LEFTBRACKET:return '[';
    case SDL_SCANCODE_BACKSLASH:return '\\';case SDL_SCANCODE_RIGHTBRACKET:return ']';
    case SDL_SCANCODE_GRAVE:return '`';case SDL_SCANCODE_ESCAPE:return APP_KEY_ESCAPE;
    case SDL_SCANCODE_RETURN:return APP_KEY_ENTER;case SDL_SCANCODE_TAB:return APP_KEY_TAB;
    case SDL_SCANCODE_BACKSPACE:return APP_KEY_BACKSPACE;case SDL_SCANCODE_INSERT:return APP_KEY_INSERT;
    case SDL_SCANCODE_DELETE:return APP_KEY_DELETE;case SDL_SCANCODE_RIGHT:return APP_KEY_RIGHT;
    case SDL_SCANCODE_LEFT:return APP_KEY_LEFT;case SDL_SCANCODE_DOWN:return APP_KEY_DOWN;
    case SDL_SCANCODE_UP:return APP_KEY_UP;case SDL_SCANCODE_PAGEUP:return APP_KEY_PAGE_UP;
    case SDL_SCANCODE_PAGEDOWN:return APP_KEY_PAGE_DOWN;case SDL_SCANCODE_HOME:return APP_KEY_HOME;
    case SDL_SCANCODE_END:return APP_KEY_END;case SDL_SCANCODE_CAPSLOCK:return APP_KEY_CAPS_LOCK;
    case SDL_SCANCODE_SCROLLLOCK:return 281;case SDL_SCANCODE_NUMLOCKCLEAR:return APP_KEY_NUM_LOCK;
    case SDL_SCANCODE_PRINTSCREEN:return 283;case SDL_SCANCODE_PAUSE:return 284;
    case SDL_SCANCODE_KP_0:return 320;case SDL_SCANCODE_KP_PERIOD:return 330;
    case SDL_SCANCODE_KP_DIVIDE:return 331;case SDL_SCANCODE_KP_MULTIPLY:return 332;
    case SDL_SCANCODE_KP_MINUS:return 333;case SDL_SCANCODE_KP_PLUS:return 334;
    case SDL_SCANCODE_KP_ENTER:return APP_KEY_KP_ENTER;case SDL_SCANCODE_KP_EQUALS:return 336;
    case SDL_SCANCODE_LSHIFT:return APP_KEY_LEFT_SHIFT;case SDL_SCANCODE_LCTRL:return APP_KEY_LEFT_CONTROL;
    case SDL_SCANCODE_LALT:return APP_KEY_LEFT_ALT;case SDL_SCANCODE_LGUI:return APP_KEY_LEFT_SUPER;
    case SDL_SCANCODE_RSHIFT:return APP_KEY_RIGHT_SHIFT;case SDL_SCANCODE_RCTRL:return APP_KEY_RIGHT_CONTROL;
    case SDL_SCANCODE_RALT:return APP_KEY_RIGHT_ALT;case SDL_SCANCODE_RGUI:return APP_KEY_RIGHT_SUPER;
    case SDL_SCANCODE_APPLICATION:return APP_KEY_KB_MENU;
    default:return -1;
    }
}
static int button(Uint8 code) {
    if(code==SDL_BUTTON_LEFT)return 0;
    if(code==SDL_BUTTON_RIGHT)return 1;
    if(code==SDL_BUTTON_MIDDLE)return 2;
    return code>=4 && code<=8?code-1:-1;
}
int input_sdl_event(InputSDL *adapter,const SDL_Event *native) {
    InputEvent event={.timestamp_ns=native->common.timestamp};
    switch(native->type) {
    case SDL_EVENT_KEY_DOWN:case SDL_EVENT_KEY_UP: {
        int code=input_sdl_key(native->key.scancode);if(code<0)return 1;
        int action=native->type==SDL_EVENT_KEY_UP?APP_RELEASE:native->key.repeat?APP_REPEAT:APP_PRESS;
        if(action==APP_PRESS)adapter->held_keys[code]=1;
        else if(action==APP_RELEASE)adapter->held_keys[code]=0;
        int locks=(native->key.mod&SDL_KMOD_CAPS?APP_LOCK_CAPS:0)|(native->key.mod&SDL_KMOD_NUM?APP_LOCK_NUM:0);
        event.kind=INPUT_KEY;event.data.key.code=code;event.data.key.action=action;event.data.key.locks=locks;break;
    }
    case SDL_EVENT_TEXT_INPUT: {
        const char *text=native->text.text;
        for(int i=0;text && text[i];) {
            int bytes=1;event.kind=INPUT_TEXT;event.data.codepoint=(unsigned int)utf8_next(text+i,&bytes);
            input_event(adapter->input,&event);i+=bytes;
        }
        return 1;
    }
    case SDL_EVENT_MOUSE_MOTION:
        event.kind=INPUT_POINTER;event.data.pointer.x=native->motion.x;event.data.pointer.y=native->motion.y;break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:case SDL_EVENT_MOUSE_BUTTON_UP: {
        int b=button(native->button.button);if(b<0)return 1;
        int action=native->type==SDL_EVENT_MOUSE_BUTTON_DOWN?APP_PRESS:APP_RELEASE;
        adapter->held_buttons[b]=(unsigned char)action;event.kind=INPUT_BUTTON;
        event.data.button.button=b;event.data.button.action=action;break;
    }
    case SDL_EVENT_MOUSE_WHEEL:
        /* SDL supplies normalized fractional wheel units on every backend,
           including the user's natural-scrolling preference. */
        event.kind=INPUT_WHEEL;event.data.scroll.x=native->wheel.x;event.data.scroll.y=native->wheel.y;break;
    case SDL_EVENT_PINCH_UPDATE:
        event.kind=INPUT_MAGNIFY;event.data.magnification=(double)native->pinch.scale-1;break;
    case SDL_EVENT_PINCH_BEGIN:case SDL_EVENT_PINCH_END:
        return 1;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        /* GLFW releases physically held keys/buttons on focus loss. Track the
           raw held state separately from the legacy lock-key query state. */
        for(int k=0;k<APP_INPUT_KEYS;k++)if(adapter->held_keys[k]) {
            InputEvent release={.kind=INPUT_KEY,.timestamp_ns=event.timestamp_ns,.data.key={k,APP_RELEASE,0}};
            input_event(adapter->input,&release);adapter->held_keys[k]=0;
        }
        for(int b=0;b<APP_INPUT_BUTTONS;b++)if(adapter->held_buttons[b]) {
            InputEvent release={.kind=INPUT_BUTTON,.timestamp_ns=event.timestamp_ns,.data.button={b,APP_RELEASE}};
            input_event(adapter->input,&release);adapter->held_buttons[b]=0;
        }
        event.kind=INPUT_FOCUS;event.data.focused=0;break;
    case SDL_EVENT_WINDOW_FOCUS_GAINED:event.kind=INPUT_FOCUS;event.data.focused=1;break;
    default:return 0;
    }
    input_event(adapter->input,&event);return 1;
}
