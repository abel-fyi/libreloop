// SPDX-License-Identifier: GPL-3.0-only
/* Effective pinned raylib5.5 GLFW polling/callback/query behavior. Own POD
   events; no windowing, drawing, audio or native keyboard dependency. */
#include "input.h"
#include <string.h>
void input_init(AppInput *input) { memset(input,0,sizeof *input);input->scale=(InputPoint){1,1}; }
void input_begin(AppInput *input,uint64_t time) {
    input->key_count=0;input->char_count=0;
    memcpy(input->previous_keys,input->keys,sizeof input->keys);
    memset(input->repeat,0,sizeof input->repeat);
    memcpy(input->previous_buttons,input->buttons,sizeof input->buttons);
    input->previous_pointer=input->pointer;scroll_input_begin(&input->scroll);
    input->frame_timestamp_ns=time;
}
void input_event(AppInput *input,const InputEvent *event) {
    switch(event->kind) {
    case INPUT_KEY: {
        int code=event->data.key.code,action=event->data.key.action;
        if(code<0 || code>=APP_INPUT_KEYS)return;
        if(action==APP_RELEASE)input->keys[code]=0;
        else if(action==APP_PRESS)input->keys[code]=1;
        else if(action==APP_REPEAT)input->repeat[code]=1;
        if((code==APP_KEY_CAPS_LOCK && (event->data.key.locks&APP_LOCK_CAPS)) ||
           (code==APP_KEY_NUM_LOCK && (event->data.key.locks&APP_LOCK_NUM)))input->keys[code]=1;
        if(input->key_count<APP_INPUT_QUEUE && action==APP_PRESS)input->key_queue[input->key_count++]=code;
        break;
    }
    case INPUT_TEXT:
        if(input->char_count<APP_INPUT_QUEUE)input->char_queue[input->char_count++]=(int)event->data.codepoint;
        break;
    case INPUT_BUTTON: {
        int b=event->data.button.button;if(b<0 || b>=APP_INPUT_BUTTONS)return;
        if(event->data.button.action==APP_PRESS)input->pending_clicks[b]++;
        input->buttons[b]=(unsigned char)event->data.button.action;break;
    }
    case INPUT_POINTER:input->pointer=(InputPoint){(float)event->data.pointer.x,(float)event->data.pointer.y};break;
    case INPUT_WHEEL:scroll_input_wheel(&input->scroll,event->data.scroll.x,event->data.scroll.y);break;
    case INPUT_NATIVE_SCROLL:scroll_input_native(&input->scroll,event->data.scroll.x,event->data.scroll.y,event->data.scroll.precise);break;
    case INPUT_MAGNIFY:scroll_input_magnify(&input->scroll,event->data.magnification);break;
    case INPUT_FOCUS:input->focused=event->data.focused;break;
    }
}
void input_clicks_begin(AppInput *input) {
    for(int b=0;b<APP_INPUT_BUTTONS;b++) {
        input->frame_clicks[b]=input->pending_clicks[b]>0;
        if(input->frame_clicks[b])input->pending_clicks[b]--;
    }
}
int input_click(const AppInput *input,int button) { return button>=0 && button<APP_INPUT_BUTTONS && input->frame_clicks[button]; }
int input_key_pressed(const AppInput *i,int k) { return k>0 && k<APP_INPUT_KEYS && i->previous_keys[k]==0 && i->keys[k]==1; }
int input_key_repeat(const AppInput *i,int k) { return k>0 && k<APP_INPUT_KEYS && i->repeat[k]==1; }
int input_key_down(const AppInput *i,int k) { return k>0 && k<APP_INPUT_KEYS && i->keys[k]==1; }
int input_key_released(const AppInput *i,int k) { return k>0 && k<APP_INPUT_KEYS && i->previous_keys[k]==1 && i->keys[k]==0; }
int input_key_up(const AppInput *i,int k) { return k>0 && k<APP_INPUT_KEYS && i->keys[k]==0; }
static int next(int *queue,int *count) {
    int result=0;if(*count>0){result=queue[0];for(int i=0;i<*count-1;i++)queue[i]=queue[i+1];queue[*count-1]=0;(*count)--;}return result;
}
int input_next_key(AppInput *i) { return next(i->key_queue,&i->key_count); }
int input_next_char(AppInput *i) { return next(i->char_queue,&i->char_count); }
int input_button_pressed(const AppInput *i,int b) { return b>=0 && b<APP_INPUT_BUTTONS && i->buttons[b]==1 && i->previous_buttons[b]==0; }
int input_button_down(const AppInput *i,int b) { return b>=0 && b<APP_INPUT_BUTTONS && i->buttons[b]==1; }
int input_button_released(const AppInput *i,int b) { return b>=0 && b<APP_INPUT_BUTTONS && i->buttons[b]==0 && i->previous_buttons[b]==1; }
int input_button_up(const AppInput *i,int b) { return b>=0 && b<APP_INPUT_BUTTONS && i->buttons[b]==0; }
InputPoint input_pointer(const AppInput *i) { return (InputPoint){(i->pointer.x+i->offset.x)*i->scale.x,(i->pointer.y+i->offset.y)*i->scale.y}; }
InputPoint input_delta(const AppInput *i) { return (InputPoint){i->pointer.x-i->previous_pointer.x,i->pointer.y-i->previous_pointer.y}; }
