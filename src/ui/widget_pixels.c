// SPDX-License-Identifier: GPL-3.0-only
/* Original ui_widgets.c circles_init/arcs_init/cables_init formulas. */
#include "widget_pixels.h"
#include <stdlib.h>
#include <math.h>
#define PI 3.14159265358979323846f
#define KNOB_RADIUS 10
static WidgetPixels image(int width,int height,DrawColor color) {
    DrawColor *pixels=malloc((size_t)width*height*sizeof *pixels);if(!pixels)return (WidgetPixels){0};
    for(int i=0;i<width*height;i++)pixels[i]=color;
    return (WidgetPixels){width,height,pixels};
}
WidgetPixels widget_circle_pixels(void) {
    WidgetPixels out=image(64,64,(DrawColor){255,255,255,255});if(!out.pixels)return out;
    DrawColor *pixels=out.pixels;
    for(int y=0;y<64;y++)for(int x=0;x<64;x++) {
        float dx=x+.5f-32,dy=y+.5f-32;
        pixels[y*64+x].a=255*fmaxf(0,fminf(1,(32-sqrtf(dx*dx+dy*dy))/3));
    }
    return out;
}
WidgetPixels widget_arc_pixels(void) {
    WidgetPixels out=image(1536,288,(DrawColor){0});if(!out.pixels)return out;DrawColor *pixels=out.pixels;
    for(int bank=0;bank<3;bank++)for(int level=0;level<=128;level++)for(int y=0;y<32;y++)for(int x=0;x<32;x++) {
        float dx=x+.5f-16,dy=y+.5f-16,distance=sqrtf(dx*dx+dy*dy);
        float origin=bank==0?2.4f:bank==1?PI/2:-PI/2;
        float angle=atan2f(dy,dx)-origin;if(angle<0)angle+=2*PI;
        float end=level/128.f*(bank==0?4.6f:2*PI),start=bank==1?PI:0;
        float alpha=fmaxf(0,fminf(1,fminf(16-distance,distance-(16-48.f/KNOB_RADIUS))/1.4f));
        if(bank!=2 || level!=128)alpha*=fmaxf(0,fminf(1,fminf(angle-fminf(start,end),fmaxf(start,end)-angle)*distance));
        pixels[(level/16*32+y)*1536+bank*512+level%16*32+x]=(DrawColor){255,255,255,255*alpha};
    }
    return out;
}
WidgetPixels widget_cable_pixels(void) {
    WidgetPixels out=image(1,16,(DrawColor){255,255,255,255});if(!out.pixels)return out;
    for(int y=0;y<16;y++)out.pixels[y].a=255*fminf(1,(8-fabsf(y+.5f-8))/4);return out;
}
void widget_pixels_free(WidgetPixels image){free(image.pixels);}
