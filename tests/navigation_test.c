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
    CHECK(grid_interval(1)==16 && grid_interval(100)==.25f);
    CHECK(snap_floor(1.137f,0)==1.137f && snap_round(1.137f,0)==1.137f);
    CHECK(snap_floor(1.137f,.25f)==1 && snap_round(1.137f,.25f)==1.25f);
    /* Reference zooms: sixteenths, eighths, beats, half bars, bars, then bands only. */
    const float bar_pixels[]={292,138,64,38,26,15,12,9,5,3,1};
    const float lines[]={1,2,4,8,16,16,16,0,0,0,0};
    const float labels[]={16,16,16,16,32,32,64,64,128,256,512};
    for(int i=0;i<11;i++) {
        TimelineGrid grid=timeline_grid_layout(bar_pixels[i]/STEPS);
        CHECK(grid.lines==lines[i] && grid.labels==labels[i]);
        TimelineGrid near=timeline_grid_layout(bar_pixels[i]/STEPS*(1-1e-7f));
        CHECK(near.lines==grid.lines && near.labels==grid.labels);
        CHECK(grid.band_alpha>=0 && grid.band_alpha<=.09f);
    }
    CHECK(timeline_grid_layout(1.f/STEPS).band_alpha==0);
    CHECK(timeline_grid_layout(5.f/STEPS).band_alpha==.09f);
    CHECK(timeline_grid_layout(0).lines==0 && timeline_grid_layout(INFINITY).lines==0);
    CHECK(grid_interval(256)==.0625f);
    CHECK(grid_interval(1024)==.015625f);
    CHECK(grid_interval(.5f)==timeline_grid_layout(.5f).labels);
    puts("Wheel modifiers, precise scrolling, pinch zoom and pointer anchoring passed."); return 0;
}
