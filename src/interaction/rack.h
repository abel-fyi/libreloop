// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_RACK_H
#define LIBRELOOP_RACK_H
#include "engine.h"
typedef struct {
    int pattern,filter,scroll,paint,paint_channel;
    float view,paint_step;
    const uint8_t *melodic;
    float step_width; /* zero retains the reference 16-unit geometry */
} RackState;
float rack_step_width(const RackState *state);
void rack_zoom(float *step_width,float *view,float grid_width,float wheel,float anchor);
int rack_fill_steps(Project *project,int pattern,int channel,int interval,int apply);
int pattern_hits_paint(Project *project,int pattern,int channel,float from,float to,int erase);
int rack_channels(const RackState *state,const Project *project,int *rows);
int rack_channel_at(const RackState *state,const Project *project,int row);
int rack_melodic(const RackState *state,const Project *project,int channel);
void rack_paint(RackState *state,Project *project,float width,float editor_height,float x,float y);
#endif
