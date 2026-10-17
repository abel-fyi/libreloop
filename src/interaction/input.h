// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_INPUT_H
#define LIBRELOOP_INPUT_H
#include "scroll_input.h"
#include <stdint.h>
enum { APP_INPUT_KEYS=512,APP_INPUT_BUTTONS=8,APP_INPUT_QUEUE=16 };
/* Physical US-position key identities retained at the platform boundary. */
enum {
    APP_KEY_NULL=0,APP_KEY_SPACE=32,APP_KEY_ESCAPE=256,APP_KEY_ENTER=257,
    APP_KEY_TAB=258,APP_KEY_BACKSPACE=259,APP_KEY_INSERT=260,APP_KEY_DELETE=261,
    APP_KEY_RIGHT=262,APP_KEY_LEFT=263,APP_KEY_DOWN=264,APP_KEY_UP=265,
    APP_KEY_PAGE_UP=266,APP_KEY_PAGE_DOWN=267,APP_KEY_HOME=268,APP_KEY_END=269,
    APP_KEY_CAPS_LOCK=280,APP_KEY_NUM_LOCK=282,APP_KEY_KP_ENTER=335,
    APP_KEY_LEFT_SHIFT=340,APP_KEY_LEFT_CONTROL=341,APP_KEY_LEFT_ALT=342,
    APP_KEY_LEFT_SUPER=343,APP_KEY_RIGHT_SHIFT=344,APP_KEY_RIGHT_CONTROL=345,
    APP_KEY_RIGHT_ALT=346,APP_KEY_RIGHT_SUPER=347,APP_KEY_KB_MENU=348
};
enum { APP_RELEASE=0,APP_PRESS=1,APP_REPEAT=2 };
enum { APP_LOCK_CAPS=1,APP_LOCK_NUM=2 };
typedef struct { float x,y; } InputPoint;
typedef enum { INPUT_KEY,INPUT_TEXT,INPUT_BUTTON,INPUT_POINTER,INPUT_WHEEL,
    INPUT_NATIVE_SCROLL,INPUT_MAGNIFY,INPUT_FOCUS } InputEventKind;
typedef struct {
    InputEventKind kind;
    uint64_t timestamp_ns;
    union {
        struct { int code,action,locks; } key;
        unsigned int codepoint;
        struct { int button,action; } button;
        struct { double x,y; } pointer;
        struct { double x,y; int precise; } scroll;
        double magnification;
        int focused;
    } data;
} InputEvent;
typedef struct {
    unsigned char keys[APP_INPUT_KEYS],previous_keys[APP_INPUT_KEYS],repeat[APP_INPUT_KEYS];
    unsigned char buttons[APP_INPUT_BUTTONS],previous_buttons[APP_INPUT_BUTTONS];
    int pending_clicks[APP_INPUT_BUTTONS],frame_clicks[APP_INPUT_BUTTONS];
    int key_queue[APP_INPUT_QUEUE],char_queue[APP_INPUT_QUEUE],key_count,char_count;
    InputPoint pointer,previous_pointer,offset,scale;
    ScrollInput scroll;
    uint64_t frame_timestamp_ns;
    int focused;
} AppInput;
void input_init(AppInput *input);
/* Begin before polling/waiting, then apply all events for that original frame.
   Frame time controls interaction timers; event timestamps do not replace it. */
void input_begin(AppInput *input,uint64_t frame_timestamp_ns);
void input_event(AppInput *input,const InputEvent *event);
/* Original UI queues every native press, then consumes at most one per button
   per update. This is separate from raylib's coalesced state query. */
void input_clicks_begin(AppInput *input);
int input_click(const AppInput *input,int button);
int input_key_pressed(const AppInput *input,int key);
int input_key_repeat(const AppInput *input,int key);
int input_key_down(const AppInput *input,int key);
int input_key_released(const AppInput *input,int key);
int input_key_up(const AppInput *input,int key);
int input_next_key(AppInput *input);
int input_next_char(AppInput *input);
int input_button_pressed(const AppInput *input,int button);
int input_button_down(const AppInput *input,int button);
int input_button_released(const AppInput *input,int button);
int input_button_up(const AppInput *input,int button);
InputPoint input_pointer(const AppInput *input);
InputPoint input_delta(const AppInput *input);
#endif
