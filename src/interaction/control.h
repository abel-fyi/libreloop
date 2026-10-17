// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_CONTROL_H
#define LIBRELOOP_CONTROL_H
#include "engine.h"
enum { CONTROL_NORMAL,CONTROL_SWING,CONTROL_PAN,CONTROL_WIDTH,CONTROL_CENTER,CONTROL_VOLUME,CONTROL_LOGARITHMIC,CONTROL_FIXED_COARSE,CONTROL_MAGNETIC };
typedef struct {
    float *value,low,high,raw,bottom,range;
    int fader,reverse,integer,logarithmic,magnetic;
} ControlDrag;
typedef struct {float *value,raw,shown;} ControlWheel;
void control_capture(ControlDrag *state,Project *project,float *value,float low,float high,int fader);
const float *control_step_marks(unsigned *count);
float control_snap_steps(float raw);
void control_drag_update_fine(ControlDrag *state,Project *project,float delta_y,float layout_scale,float global_y,int fine);
void control_knob_wheel_fine(ControlWheel *state,Project *project,float *value,float low,float high,int style,float wheel,int fine);
void control_drag_update(ControlDrag *state,Project *project,float delta_y,float layout_scale,float global_y);
void control_knob_wheel(Project *project,float *value,float low,float high,int style,float wheel);
void control_fixed_alias(float *value,int style,int over,int left_pressed,float wheel);
#endif
