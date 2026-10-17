// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_PIANO_DRAW_H
#define LIBRELOOP_PIANO_DRAW_H
#include "render.h"
#include "piano.h"
typedef struct {
    const Project *project;
    const PianoViewport *view;
    const PianoGesture *gesture;
    const PianoKeys *keys;
    const PianoVelocity *velocity;
    const PianoScrollbars *bars;
    const uint8_t *keyboard_notes,*playing_keys,*edges;
    int hovered_key,hover_vertical,hover_horizontal;
    float range,playback_start,loop_start,loop_end;
    double playback;
    int marker_drag;
    int tool,hovered_tool,row_zoom_hover,row_zoom_drag;
} PianoDrawing;
/* Read-only drawing: input, hit testing, edits and audition stay in interaction. */
void piano_draw_content(UIRender *render,const PianoDrawing *state);
void piano_draw_toolbar(UIRender *render,const PianoDrawing *state);
#endif
