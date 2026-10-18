// SPDX-License-Identifier: GPL-3.0-only
#include "studio_layout.h"
#include <math.h>
static float clamp(float n,float low,float high){return fmaxf(low,fminf(high,n));}
void studio_workspace_init(Windows *w,float width,float height,float sidebar) {
    studio_windows_layout(w,width,height,sidebar);
    StudioWorkspace *s=&w->workspace;
    *s=(StudioWorkspace){.enabled=1,.inspector=4,.maximized=-1,.drag=-1,.last_split=-1,
        .editor_width=.5f,.mixer=272.f/1002,.piano=276.f/788,.rack=168,.inspector_width=334};
    s->show[1]=s->show[3]=1;
    for(int i=4;i<WINDOW_CAPACITY;i++){s->floating[i]=1;w->editors[i].visible=0;}
    w->editors[3].minh=160;
    /* Instrument editors are compact floating tools, independent of Mixer size. */
    w->editors[4].minw=440;w->editors[4].minh=420;
    w->editors[4].rect=(Rect){fmaxf(sidebar+12,width-452),84,440,420};
    studio_workspace_layout(w,width,height,sidebar);
    w->focused=1;
}
void studio_workspace_layout(Windows *w,float width,float height,float sidebar) {
    StudioWorkspace *s=&w->workspace;s->width=width;s->height=height;s->sidebar=sidebar;
    float x=sidebar+6,y=60,full=width-x-4,area=height-84;
    if(s->mixer_fx_rows>=0)s->mixer_fx_rows=fmaxf(0,fminf(s->mixer_fx_rows,fminf(3,floorf((area-252-182)/22)-1)));
    float extra=s->mixer_fx_rows<0?0:(1+s->mixer_fx_rows)*22;
    float mixer=s->show[3]?clamp(area*s->mixer,182,fmaxf(182,area-252-extra))+extra:STUDIO_TITLE;
    w->editors[3].minh=182+extra;
    w->editors[3].rect=(Rect){x,height-24-mixer,full,mixer};w->editors[3].visible=s->show[3];
    Rect page={x,y,full,area-mixer-6};
    s->show[0]=0;s->show[1]=!s->show[2];s->maximized=-1;
    for(int i=0;i<=2;i++) {
        w->editors[i].rect=page;w->editors[i].visible=i==2?s->show[2]:i==1?s->show[1]:0;
        w->editors[i].maximized=0;s->floating[i]=0;
    }
    s->floating[3]=0;w->editors[3].maximized=0;
    for(int i=4;i<WINDOW_CAPACITY;i++){s->floating[i]=1;w->editors[i].maximized=0;}
}
static void reflow(Windows *w){StudioWorkspace *s=&w->workspace;studio_workspace_layout(w,s->width,s->height,s->sidebar);}
void studio_workspace_open(Windows *w,int id) {
    StudioWorkspace *s=&w->workspace;if(id==0)id=2;
    s->show[id]=1;
    if(id==1)s->show[2]=0;
    if(id>=4) {
        if(!s->floating[id]) {
            float fw=w->editors[id].minw,fh=w->editors[id].minh;
            w->editors[id].rect=(Rect){fmaxf(s->sidebar+12,s->width-fw-12),82,fw,fh};
        }
        s->floating[id]=1;w->editors[id].visible=1;
    }
    reflow(w);
}
void studio_workspace_hide(Windows *w,int id) {
    /* Pages are switched, never closed or minimized. */
    if(id<=2)return;
    w->workspace.show[id]=0;w->editors[id].visible=0;reflow(w);
}
void studio_workspace_float(Windows *w,int id) {
    if(id<4)return;
    w->workspace.floating[id]=1;
}
void studio_workspace_maximize(Windows *w,int id) {(void)w;(void)id;}
int studio_workspace_split(const Windows *w,float x,float y) {
    int hit=windows_hit(w,x,y);if(hit>=4)return -1;
    Rect r=w->editors[3].rect;
    return w->editors[3].visible && x>=r.x && x<r.x+r.w && y>=r.y-6 && y<r.y?3:-1;
}
void studio_workspace_update(Windows *w,Rect desktop,float x,float y,int pressed,int down,double time) {
    StudioWorkspace *s=&w->workspace;reflow(w);
    if(!down){s->drag=-1;w->grab=-1;}
    if(s->drag==3) {
        float size=s->start_size-(y-s->start_pointer),area=s->height-84;
        s->show[3]=size>=100;s->mixer=clamp((size-(s->mixer_fx_rows<0?0:(1+fminf(3,s->mixer_fx_rows))*22))/area,182/area,.7f);
        w->owner=-1;w->grab=-1;reflow(w);return;
    }
    int split=studio_workspace_split(w,x,y);
    if(split>=0 && pressed) {
        if(s->last_split==split && time-s->last_time<.3 && fabsf(x-s->last_x)<4 && fabsf(y-s->last_y)<4) {
            s->mixer=272.f/1002;s->show[3]=1;s->last_split=-1;reflow(w);
        } else {
            s->drag=3;s->start_pointer=y;s->start_size=w->editors[3].rect.h;
            s->last_split=split;s->last_time=time;s->last_x=x;s->last_y=y;
        }
        w->owner=-1;return;
    }
    for(int i=4;i<windows_count(w);i++) {
        Editor *e=&w->editors[i];Rect *r=&e->rect;
        r->w=fminf(e->minw,desktop.w);
        r->h=fminf(e->minh,desktop.h);
        r->x=clamp(r->x,desktop.x,desktop.x+desktop.w-r->w);r->y=clamp(r->y,desktop.y,desktop.y+desktop.h-r->h);
    }
    if(w->grab>=4) {
        Editor *e=&w->editors[w->grab];w->resize=0;e->rect.x=x-w->dx;e->rect.y=y-w->dy;
    }
    int id=windows_hit(w,x,y);w->owner=id;
    if(pressed && id>=0) {
        windows_focus(w,id);Editor *e=&w->editors[id];Rect r=e->rect;
        if(y<r.y+STUDIO_TITLE) {
            if(id>=4 && x>=r.x+r.w-STUDIO_TITLE)studio_workspace_hide(w,id);
            else if(id>=4){w->grab=id;w->resize=0;w->dx=x-r.x;w->dy=y-r.y;}
            w->owner=-1;
        }
    }
}
