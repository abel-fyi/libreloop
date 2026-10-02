// SPDX-License-Identifier: GPL-3.0-only
#include "windows.h"
#include <math.h>
static int inside(Rect r,float x,float y) { return x>=r.x && y>=r.y && x<r.x+r.w && y<r.y+r.h; }
void windows_init(Windows *w,float width,float height) {
    *w=(Windows){.editors={
        {{200,200,470,208},{0},438,192,1,0,0},
        {{180,42,width-184,height-66},{0},440,240,1,0,0},
        {{320,128,760,430},{0},440,392,0,0,0},
        {{188,height-264,fminf(720,width-196),240},{0},460,220,1,0,0},
        {{(width-560)/2,(height-380)/2,560,380},{0},520,340,0,0,0}},
        .order={1,2,3,4,0},.owner=-1,.grab=-1};
}
int windows_hit(const Windows *w,float x,float y) {
    for(int i=EDITORS-1;i>=0;i--) { int id=w->order[i]; if(w->editors[id].visible && inside(w->editors[id].rect,x,y)) return id; }
    return -1;
}
void windows_focus(Windows *w,int id) {
    int i=0; while(i<EDITORS && w->order[i]!=id) i++;
    for(;i<EDITORS-1;i++) w->order[i]=w->order[i+1];
    w->order[EDITORS-1]=id; w->editors[id].visible=1;
    for(int pass=0;pass<EDITORS;pass++) for(int j=0;j<EDITORS-1;j++)
        if(w->editors[w->order[j]].pinned && !w->editors[w->order[j+1]].pinned) {
            int swap=w->order[j]; w->order[j]=w->order[j+1]; w->order[j+1]=swap;
        }
}
void windows_pin(Windows *w,int id) { w->editors[id].pinned^=1; windows_focus(w,id); }
static Rect constrain(Rect r,Rect d,float minw,float minh) {
    r.w=fminf(d.w,fmaxf(minw,r.w)); r.h=fminf(d.h,fmaxf(minh,r.h));
    r.x=fmaxf(d.x,fminf(d.x+d.w-r.w,r.x)); r.y=fmaxf(d.y,fminf(d.y+d.h-r.h,r.y)); return r;
}
void windows_update(Windows *w,Rect d,float x,float y,int pressed,int down) {
    for(int i=0;i<EDITORS;i++) { Editor *e=&w->editors[i]; e->rect=e->maximized?d:constrain(e->rect,d,e->minw,e->minh); }
    if(!down) w->grab=-1;
    if(w->grab>=0) {
        Editor *e=&w->editors[w->grab];
        if(w->resize) { e->rect.w=x-e->rect.x+w->dx; e->rect.h=y-e->rect.y+w->dy; }
        else { e->rect.x=x-w->dx; e->rect.y=y-w->dy; }
        e->rect=constrain(e->rect,d,e->minw,e->minh);
    }
    int id=inside(d,x,y)?windows_hit(w,x,y):-1;
    if(pressed && id>=0) {
        windows_focus(w,id); Editor *e=&w->editors[id]; Rect r=e->rect;
        if(y<r.y+TITLE && x>=r.x+r.w-22) e->visible=0;
        else if(y<r.y+TITLE && x>=r.x+r.w-44) {
            if(e->maximized) { e->rect=e->restore; e->maximized=0; }
            else { e->restore=r; e->rect=d; e->maximized=1; }
        } else if(!e->maximized && x>r.x+r.w-12 && y>r.y+r.h-12) {
            w->grab=id; w->resize=1; w->dx=r.x+r.w-x; w->dy=r.y+r.h-y;
        } else if(!e->maximized && y<r.y+TITLE) {
            w->grab=id; w->resize=0; w->dx=x-r.x; w->dy=y-r.y;
        }
        if(y<r.y+TITLE) id=-1;
    }
    w->owner=w->grab>=0?-1:id;
}
