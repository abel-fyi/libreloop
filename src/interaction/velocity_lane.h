// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_VELOCITY_LANE_H
#define LIBRELOOP_VELOCITY_LANE_H
#include <math.h>
typedef struct {
    float height,start_height,start_y,start_preferred;
    int dragging,collapsed,start_collapsed,moved;
} VelocityLane;
/* Preserve the original 62-unit lane and .48 velocity scale by default. */
static inline float velocity_lane_height(float requested,float height) {
    return fmaxf(40,fminf(fmaxf(40,height-214),requested>0?requested:62));
}
static inline float velocity_lane_scale(float height) { return .48f*height/62; }
/* Drag through the minimum to collapse; the divider remains available.
   Clicking a collapsed divider restores its remembered expanded size. */
static inline void velocity_lane_update(VelocityLane *s,float viewport,float y,int press,int held,int cancel) {
    if(s->dragging && cancel){s->height=s->start_preferred;s->collapsed=s->start_collapsed;s->dragging=0;return;}
    if(press && !s->dragging){
        s->start_preferred=s->height;s->start_collapsed=s->collapsed;
        s->start_height=s->collapsed?0:velocity_lane_height(s->height,viewport);
        s->start_y=y;s->moved=0;s->dragging=1;
    }
    if(s->dragging){
        if(fabsf(y-s->start_y)>3)s->moved=1;
        if(s->moved){
            float size=s->start_height+s->start_y-y;
            s->collapsed=size<20;
            if(s->collapsed)s->height=s->start_preferred;
            else s->height=velocity_lane_height(fmaxf(40,size),viewport);
        }
        if(!held){
            if(s->start_collapsed && !s->moved)s->collapsed=0;
            s->dragging=0;
        }
    }
}
#endif
