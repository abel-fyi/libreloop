// SPDX-License-Identifier: GPL-3.0-only
/* Frozen ui_playlist.c ruler_press and main.c marker update semantics. */
#include "ruler.h"
#include <math.h>
#include <string.h>
void ruler_state_init(RulerState *s){memset(s,0,sizeof *s);s->last_id=-1;s->marker_drag=-1;s->last_click=-1;}
void ruler_press(RulerState *s,int pattern,int id,int button,float at,double time,RulerPoint position) {
    float *range=id==1?s->song_loop:s->pattern_loop[pattern];
    int twice=s->last_id==id && (id==1 || s->last_pattern==pattern) && s->last_button==button &&
        time-s->last_click<.3 && fabsf(position.x-s->last_position.x)<6 && fabsf(position.y-s->last_position.y)<6;
    s->last_id=id;s->last_pattern=pattern;s->last_button=button;s->last_click=time;s->last_position=position;
    s->stop_armed=0;if(s->song!=(id==1)){s->song=id==1;s->reset=1;}
    if(twice){range[0]=range[1]=0;s->marker_drag=-1;s->last_id=-1;return;}
    s->marker_drag=id;s->loop_drag=button==1;
    if(s->loop_drag) {
        s->anchor=range[1]>range[0]?(at<(range[0]+range[1])/2?range[1]:range[0]):at;
        range[0]=fminf(s->anchor,at);range[1]=fmaxf(s->anchor,at);
        if(range[1]>range[0]){if(id==1)s->playlist_start=range[0]/STEPS;else s->piano_start[pattern]=range[0];}
    }else if(id==1)s->playlist_start=at/STEPS;else s->piano_start[pattern]=at;
    if(!s->playing)s->reset=1;
}
void ruler_drag(RulerState *s,int pattern,float at,int cancel,int new_press,int held) {
    if(s->marker_drag<0)return;
    if(cancel || new_press){s->marker_drag=-1;return;}
    int id=s->marker_drag;
    if(s->loop_drag) {
        float *range=id==1?s->song_loop:s->pattern_loop[pattern];
        range[0]=fminf(s->anchor,at);range[1]=fmaxf(s->anchor,at);
        if(range[1]>range[0]){if(id==1)s->playlist_start=range[0]/STEPS;else s->piano_start[pattern]=range[0];}
    }else if(id==1)s->playlist_start=at/STEPS;else s->piano_start[pattern]=at;
    if(!s->playing)s->reset=1;
    if(!held)s->marker_drag=-1;
}
