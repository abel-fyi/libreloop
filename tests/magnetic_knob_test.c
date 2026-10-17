// SPDX-License-Identifier: GPL-3.0-only
#include "control.h"
#include <math.h>
#include <stdio.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
static Project project;
int main(void) {
    ControlWheel wheel={0};
    CHECK(control_snap_steps(1.02f)==1 && control_snap_steps(1.05f)==1.05f);
    CHECK(control_snap_steps(.497f)==.5f && control_snap_steps(.337f)==1.f/3);
    project_new(&project);project.effect_type[0][0]=EFFECT_DELAY;
    float *value=&project.delay[0][0].steps;*value=1;ControlDrag drag={0};
    control_capture(&drag,&project,value,.125f,16,0);CHECK(drag.magnetic);
    control_drag_update_fine(&drag,&project,-.1f,1,0,0);CHECK(*value==1 && drag.raw>1);
    for(int i=0;i<20;i++)control_drag_update_fine(&drag,&project,-.1f,1,0,0);
    CHECK(*value>1.035f); /* Unsnapped raw motion escapes a detent without sticking. */
    *value=1;control_capture(&drag,&project,value,.125f,16,0);
    control_drag_update_fine(&drag,&project,-.1f,1,0,1);CHECK(*value>1 && *value<1.001f);
    float fine=*value;control_drag_update_fine(&drag,&project,0,1,0,1);CHECK(*value==fine);
    control_drag_update_fine(&drag,&project,-100000,1,0,0);CHECK(*value==16);
    control_drag_update_fine(&drag,&project,100000,1,0,1);CHECK(*value==.125f);
    *value=1;control_knob_wheel_fine(&wheel,&project,value,.125f,16,CONTROL_MAGNETIC,1,0);CHECK(*value>1.035f);
    *value=1;control_knob_wheel_fine(&wheel,&project,value,.125f,16,CONTROL_MAGNETIC,1,1);CHECK(*value>1 && *value<1.01f);
    wheel=(ControlWheel){0};*value=1;
    for(int i=0;i<20;i++)control_knob_wheel_fine(&wheel,&project,value,.125f,16,CONTROL_MAGNETIC,.1f,0);
    CHECK(*value>1.035f); /* Precise trackpad deltas also escape the magnet. */
    project.delay[0][0].feedback=.35f;control_capture(&drag,&project,&project.delay[0][0].feedback,0,1,0);CHECK(!drag.magnetic);
    float *tone=&project.tube[0][0].tone;project.effect_type[0][0]=EFFECT_TUBE;*tone=19999;
    control_capture(&drag,&project,tone,1200,TUBE_TONE_OFF,0);CHECK(drag.magnetic==2);
    control_drag_update_fine(&drag,&project,-1,1,0,0);CHECK(*tone==TUBE_TONE_OFF);
    for(int i=0;i<5;i++)control_drag_update_fine(&drag,&project,1,1,0,0);
    CHECK(*tone<=TUBE_TONE_MAX);
    wheel=(ControlWheel){0};*tone=TUBE_TONE_OFF;
    for(int i=0;i<20;i++)control_knob_wheel_fine(&wheel,&project,tone,1200,TUBE_TONE_OFF,CONTROL_LOGARITHMIC,-.1f,0);
    CHECK(*tone<TUBE_TONE_MAX);
    puts("Magnetic knobs: rhythmic detents, raw drag escape, Ctrl fine bypass, wheel and bounds passed.");return 0;
}
