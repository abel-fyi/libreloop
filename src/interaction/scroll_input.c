// SPDX-License-Identifier: GPL-3.0-only
#include "scroll_input.h"
#include <math.h>
void scroll_input_begin(ScrollInput *input) { *input=(ScrollInput){0}; }
void scroll_input_wheel(ScrollInput *input,double x,double y) {
    /* raylib5.5 replaces, rather than sums, ordinary wheel events per poll. */
    input->wheel.x=(float)x; input->wheel.y=(float)y;
}
void scroll_input_native(ScrollInput *input,double x,double y,int precise) {
    if(precise) {
        /* Same compound float accumulation as original macos_input.m. */
        input->native.x+=x; input->native.y+=y; input->native.precise=1;
        x*=.1; y*=.1; /* Original GLFW Cocoa scalar-panel path. */
    }
    if(fabs(x)>0 || fabs(y)>0) scroll_input_wheel(input,x,y);
}
void scroll_input_magnify(ScrollInput *input,double magnification) {
    float factor=1+magnification;
    if(isfinite(factor) && factor>0) input->native.zoom+=logf(factor)/logf(1.08f);
}
NavigationInput scroll_input_navigation(const ScrollInput *input) {
    NavigationInput navigation=input->wheel;
    if(input->native.precise) navigation=input->native;
    else navigation.zoom=input->native.zoom;
    return navigation;
}
float scroll_input_scalar(const ScrollInput *input) {
    return fabsf(input->wheel.x)>fabsf(input->wheel.y)?input->wheel.x:input->wheel.y;
}
