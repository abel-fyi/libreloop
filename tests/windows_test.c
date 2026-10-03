// SPDX-License-Identifier: GPL-3.0-only
#include "windows.h"
#include <stdio.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
int main(void) {
    Windows w; Rect d={180,42,1020,609}; windows_init(&w,1200,675);
    Rect start=w.editors[0].rect;
    CHECK(start.x>=392 && w.editors[3].rect.x>=392);
    CHECK(windows_hit(&w,start.x+50,start.y+60)==0); /* Rack occludes Playlist. */
    windows_update(&w,d,1100,270,1,1,0); CHECK(w.owner==1 && w.order[EDITORS-1]==1);
    CHECK(windows_hit(&w,start.x+50,start.y+60)==1); /* Raised Playlist owns the overlap. */
    windows_focus(&w,0); Rect before=w.editors[0].rect;
    windows_update(&w,d,before.x+50,before.y+10,1,1,0); CHECK(w.grab==0 && w.owner==-1);
    windows_update(&w,d,before.x+100,before.y+50,0,1,0); CHECK(w.editors[0].rect.x==before.x+50 && w.editors[0].rect.y==before.y+40);
    windows_update(&w,d,before.x+100,before.y+50,0,0,0); CHECK(w.grab==-1);
    Rect r=w.editors[0].rect;
    windows_update(&w,d,r.x+r.w-3,r.y+r.h-3,1,1,0); CHECK(w.resize && w.grab==0);
    windows_update(&w,d,-100,-100,0,1,0); CHECK(w.editors[0].rect.w==w.editors[0].minw && w.editors[0].rect.h==w.editors[0].minh);
    windows_update(&w,d,0,0,0,0,0); r=w.editors[0].rect;
    windows_update(&w,d,r.x+r.w-30,r.y+10,1,1,0); CHECK(w.editors[0].maximized && w.owner==-1);
    windows_update(&w,d,d.x+d.w-30,d.y+10,1,1,0); CHECK(!w.editors[0].maximized);
    r=w.editors[0].rect; windows_update(&w,d,r.x+r.w-10,r.y+10,1,1,0); CHECK(!w.editors[0].visible);
    windows_focus(&w,0); CHECK(w.editors[0].visible && w.order[EDITORS-1]==0);
    windows_focus(&w,0); CHECK(w.editors[0].visible);
    windows_pin(&w,0); windows_focus(&w,1); CHECK(w.order[EDITORS-1]==0 && windows_hit(&w,w.editors[0].rect.x+20,w.editors[0].rect.y+40)==0);
    windows_pin(&w,0); windows_focus(&w,1); CHECK(w.order[EDITORS-1]==1);
    windows_pin(&w,0); w.editors[0].visible=0; windows_focus(&w,1); CHECK(w.editors[1].visible);
    w.editors[0].pinned=0;
    windows_focus(&w,4); CHECK(w.editors[4].visible && w.order[EDITORS-1]==4);
    windows_focus(&w,4); CHECK(w.editors[4].visible && w.order[EDITORS-1]==4);
    CHECK(windows_hit(&w,50,300)==-1);
    windows_focus(&w,0); r=w.editors[0].rect;
    windows_update(&w,d,r.x+50,r.y+8,1,1,1);
    windows_update(&w,d,r.x+50,r.y+8,0,0,1.05);
    windows_update(&w,d,r.x+50,r.y+8,1,1,1.15); CHECK(w.editors[0].maximized && w.grab==-1);
    windows_update(&w,d,d.x+50,d.y+8,0,0,1.2);
    windows_update(&w,d,d.x+50,d.y+8,1,1,2);
    windows_update(&w,d,d.x+50,d.y+8,0,0,2.05);
    windows_update(&w,d,d.x+50,d.y+8,1,1,2.15); CHECK(!w.editors[0].maximized);
    r=w.editors[0].rect; windows_update(&w,d,r.x+r.w-45,r.y+8,1,1,3); CHECK(!w.editors[0].visible);
    windows_init(&w,1200,675); windows_focus(&w,0); r=w.editors[0].rect;
    windows_update(&w,d,r.x+r.w-72,r.y+9,1,1,4);
    CHECK(w.grab==-1 && w.owner==0 && w.editors[0].visible);
    windows_update(&w,d,r.x+r.w-72,r.y-40,0,1,4.1);
    CHECK(w.editors[0].rect.x==r.x && w.editors[0].rect.y==r.y);
    puts("Stacking, input ownership, dragging, resize limits, maximize and hide/show passed."); return 0;
}
