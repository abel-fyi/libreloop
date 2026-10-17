// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_SIGNAL_PALETTE_H
#define LIBRELOOP_SIGNAL_PALETTE_H
#include "draw.h"
static const DrawColor signal_colors[]={{25,139,71,255},{47,204,88,255},{170,224,65,255},{242,209,58,255},{247,124,36,255},{231,45,48,255}};
static inline DrawColor signal_color(float t) {
    const float stops[]={0,.44f,.8f,.9f,.96f,1};
    if(t<=0)return signal_colors[0];
    for(int i=0;i<5;i++)if(t<stops[i+1]) {
        float f=(t-stops[i])/(stops[i+1]-stops[i]);DrawColor a=signal_colors[i],b=signal_colors[i+1];
        return (DrawColor){a.r+(b.r-a.r)*f,a.g+(b.g-a.g)*f,a.b+(b.b-a.b)*f,255};
    }
    return signal_colors[5];
}
#endif
