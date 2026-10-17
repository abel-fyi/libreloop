// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_CONTROLS_DRAW_H
#define LIBRELOOP_CONTROLS_DRAW_H
#include "render.h"
#include "control.h"
typedef struct { int x,y,style,hovered,dragging;float low,high,initial,shown; } KnobDrawing;
void knob_draw(UIRender *render,const KnobDrawing *state);
#endif
