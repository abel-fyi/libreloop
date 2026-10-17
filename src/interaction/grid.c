// SPDX-License-Identifier: GPL-3.0-only
#include "grid.h"
#include <math.h>
static const char *names[]={"Auto","None","1 bar","1/2","1/4","1/8","1/16","1/32","1/64","1/8 triplet","1/16 triplet"};
static const float steps[]={0,0,16,8,4,2,1,.5f,.25f,4.f/3,2.f/3};
const char *grid_choice_name(int choice){return names[choice>=0 && choice<GRID_CHOICES?choice:0];}
float grid_choice_interval(int choice,float pixels){return choice>0 && choice<GRID_CHOICES?steps[choice]:grid_interval(pixels);}
TimelineGrid grid_choice_layout(int choice,float pixels) {
    TimelineGrid result=timeline_grid_layout(pixels);
    if(choice>0 && choice<GRID_CHOICES) {
        if(!isfinite(pixels) || pixels<=0){result.lines=0;return result;}
        result.lines=steps[choice];
        /* Thin lines may be omitted at distant zoom; the chosen snap stays fixed. */
        while(result.lines>0 && result.lines*pixels<3 && result.lines<1e30f)result.lines*=2;
    }
    return result;
}
