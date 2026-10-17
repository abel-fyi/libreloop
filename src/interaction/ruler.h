// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_RULER_H
#define LIBRELOOP_RULER_H
#include "engine.h"
typedef struct { float x,y; } RulerPoint;
typedef struct {
    int last_id,last_pattern,last_button,marker_drag,loop_drag;
    double last_click;
    RulerPoint last_position;
    float anchor,playlist_start,piano_start[PATTERNS],song_loop[2],pattern_loop[PATTERNS][2];
    int stop_armed,song,reset,playing;
} RulerState;
void ruler_state_init(RulerState *state);
void ruler_press(RulerState *state,int pattern,int editor,int button,float step,double time,RulerPoint position);
void ruler_drag(RulerState *state,int pattern,float step,int cancel,int new_press,int held);
#endif
