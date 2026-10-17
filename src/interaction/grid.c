// SPDX-License-Identifier: GPL-3.0-only
#include "grid.h"
#include <math.h>
static const char *names[]={"Auto","None","1 bar","1/2","1/4","1/8","1/16","1/32","1/64","1/3 beat","1/6 beat","1/3 step","1/6 step","1/3 bar","1/6 bar"};
static const float steps[]={0,0,16,8,4,2,1,.5f,.25f,4.f/3,2.f/3,1.f/3,1.f/6,16.f/3,8.f/3};
int grid_menu_choice(int row) {
    static const int order[]={GRID_AUTO,GRID_NONE,GRID_BAR,GRID_HALF,GRID_HALF_TRIPLET,GRID_QUARTER,GRID_QUARTER_TRIPLET,GRID_EIGHTH,GRID_EIGHTH_TRIPLET,GRID_SIXTEENTH,GRID_SIXTEENTH_TRIPLET,GRID_32,GRID_32_TRIPLET,GRID_64,GRID_64_TRIPLET};
    return row>=0 && row<GRID_CHOICES?order[row]:GRID_AUTO;
}
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
