// SPDX-License-Identifier: GPL-3.0-only
#include "navigation.h"
#include "engine.h"
#include <math.h>
#include <stdio.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
int main(void) {
    NavigationMotion m=navigation_motion((NavigationInput){.y=-1},0,0,1);
    CHECK(m.x==0 && m.y==48 && m.zoom==0);
    m=navigation_motion((NavigationInput){.y=-1},1,0,1); CHECK(m.x==48 && m.y==0);
    m=navigation_motion((NavigationInput){.x=-12,.y=-20,.precise=1},0,0,2); CHECK(m.x==6 && m.y==10);
    m=navigation_motion((NavigationInput){.y=1},0,1,1); CHECK(m.x==0 && m.y==0 && m.zoom==1);
    m=navigation_motion((NavigationInput){.y=20,.precise=1},1,1,1); CHECK(m.x==0 && m.y==0 && m.zoom==1);
    m=navigation_motion((NavigationInput){.zoom=2},0,0,1); CHECK(m.zoom==2 && m.x==0 && m.y==0);
    float span=32,start=10,anchor=.3f,point=start+span*anchor;
    timeline_zoom(&span,&start,m.zoom,anchor,1);
    CHECK(span<32 && fabsf(start+span*anchor-point)<1e-5);
    timeline_zoom(&span,&start,-m.zoom,anchor,1); CHECK(fabsf(span-32)<1e-5 && fabsf(start-10)<1e-5);
    puts("Wheel modifiers, precise scrolling, pinch zoom and pointer anchoring passed."); return 0;
}
