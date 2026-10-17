// SPDX-License-Identifier: GPL-3.0-only
#include "grid.h"
#include "piano_frame.h"
void piano_frame_advance(PianoGesture *g,PianoKeys *keys,PianoVelocity *velocity,
    PianoScrollbars *bars,Project *project,PianoViewport *v,PianoFrame f,
    PianoPreview preview,PianoKeyPreview key_preview,void *context) {
    if(!f.left_held || f.left_pressed || f.modal)g->drag=NULL;
    PianoPoint local={f.pointer.x-f.origin.x,f.pointer.y-f.origin.y};
    if(keys->drag)piano_key_update(keys,v,local,f.modal || !f.left_held ||
        f.left_pressed || !f.focused || !f.visible,key_preview,context);
    piano_scrollbar_update(v,bars,f.pointer,f.left_held,f.left_pressed);
    float cw=(v->width-84)/v->span,rh=(v->height-180)/piano_rows(v);
    if(g->kind) {
        int right=g->kind==PIANO_ERASE;
        if(f.modal || f.left_pressed || f.right_pressed)g->kind=PIANO_IDLE;
        else {
            piano_gesture_update(g,project,v->pan+(local.x-60)/cw,
                v->top-(local.y-86)/rh,grid_choice_interval(v->grid_choice,cw),preview,context);
            if(!(right?f.right_held:f.left_held))g->kind=PIANO_IDLE;
        }
    }
    if(g->drag)piano_note_drag_update(g,project,v->pan+(local.x-60)/cw,
        (local.y-86)/rh,grid_choice_interval(v->grid_choice,cw),preview,context);
    piano_velocity_update(velocity,project,g->pattern,g->channel,v,local,
        f.modal,f.left_pressed || f.right_pressed,
        velocity->ramp?f.right_held:f.left_held);
}
int piano_frame_press(PianoGesture *g,PianoKeys *keys,PianoVelocity *velocity,
    PianoScrollbars *bars,Project *p,PianoViewport *v,float range,
    PianoPoint origin,PianoPress press,PianoPreview preview,
    PianoKeyPreview key_preview,void *context) {
    if(piano_key_press(keys,v,g->channel,press,key_preview,context))return 1;
    int result=piano_press(g,p,v,press,preview,context);
    if(result&PIANO_PRESS_CONSUMED)return 1;
    if(piano_velocity_press(velocity,p,g->pattern,g->channel,v,press))return 1;
    return piano_scrollbar_press(v,bars,range,origin,press);
}
