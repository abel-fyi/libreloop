// SPDX-License-Identifier: GPL-3.0-only
#include "arrangement.h"
#include <math.h>
#include <string.h>
static int clamp(int x,int lo,int hi) { return x<lo?lo:x>hi?hi:x; }
static float quantum(const Arrangement *a) { return a->snap>0?a->snap:1; }
static float snapped(const Arrangement *a,float x) { return floorf((x*STEPS+.00001f)/quantum(a))*quantum(a)/STEPS; }
float arrangement_edit_steps(const Arrangement *a,const Project *p,int pattern) {
    (void)a; return p->pattern_steps[pattern];
}
int arrangement_hit(const Project *p,int lane,float bar) {
    if(lane<0 || lane>=LANES || bar<0) return -1;
    for(int b=CLIPS-1;b>=0;b--) if(p->clips[lane][b] && bar>=p->clip_starts[lane][b] && bar<p->clip_starts[lane][b]+clip_length(p,lane,b)/STEPS) return b;
    return -1;
}
static int place(Project *p,int l,float x,int pattern,float length) {
    if(l<0 || l>=LANES || x<0 || !isfinite(x) || !isfinite(length)) return -1;
    for(int i=0;i<CLIPS;i++) if(p->clips[l][i] && x*STEPS<p->clip_starts[l][i]*STEPS+clip_length(p,l,i)-.0001f && p->clip_starts[l][i]*STEPS<x*STEPS+length-.0001f) return -1;
    int b=x<CLIPS?(int)x:0;
    if(p->clips[l][b]) { for(b=0;b<CLIPS && p->clips[l][b];b++); if(b==CLIPS) return -1; }
    p->clips[l][b]=pattern+1; p->clip_steps[l][b]=length; p->clip_starts[l][b]=x; return b;
}
static int paint(Arrangement *a,Project *p,int l,float x) { return place(p,l,x,a->source_pattern,a->source_steps?a->source_steps:STEPS); }
void arrangement_press(Arrangement *a,Project *p,float x,float y,int right,int edge,int pattern,int additive) {
    int l=(int)floorf(y),hit=arrangement_hit(p,l,x);
    a->additive=additive; memcpy(a->selection_before,a->selected,sizeof a->selected);
    a->x=a->now_x=x; a->y=a->now_y=y; a->lane=l; a->bar=hit;
    a->last_lane=l;
    if(l<0 || l>=LANES) return;
    if(x<0) { if(!additive) memset(a->selected,0,sizeof a->selected); a->gesture=TRACK_SELECT; arrangement_drag(a,p,x,y); return; }
    if(right) { a->gesture=ERASE_CLIPS; arrangement_drag(a,p,x,y); return; }
    if(additive) {
        if(hit>=0) { a->selected[l][hit]^=1; a->source_pattern=p->clips[l][hit]-1; a->source_steps=clip_length(p,l,hit); }
        else a->gesture=BOX_SELECT;
        return;
    }
    if(a->source_pattern!=pattern) { a->source_pattern=pattern; a->source_steps=p->pattern_steps[pattern]; }
    if(hit>=0) {
        a->source_pattern=p->clips[l][hit]-1; a->source_steps=clip_length(p,l,hit);
        if(edge) { a->gesture=SIZE_CLIP; memset(a->selected,0,sizeof a->selected); a->selected[l][hit]=1; return; }
    }
    if(a->tool==SELECT && hit<0) { memset(a->selected,0,sizeof a->selected); a->gesture=BOX_SELECT; return; }
    int group=0; for(int i=0;i<LANES;i++) for(int j=0;j<CLIPS;j++) group+=a->selected[i][j]!=0;
    if(a->tool==BRUSH && !(hit>=0 && a->selected[l][hit] && group>1)) {
        if(hit>=0) { memset(a->selected,0,sizeof a->selected); a->selected[l][hit]=1; }
        a->gesture=PAINT_CLIPS; if(hit<0) paint(a,p,l,snapped(a,x)); return;
    }
    if(hit<0) {
        hit=paint(a,p,l,snapped(a,x)); if(hit<0) return; a->bar=hit;
        memset(a->selected,0,sizeof a->selected);
    }
    if(!a->selected[l][hit]) { memset(a->selected,0,sizeof a->selected); a->selected[l][hit]=1; }
    memcpy(a->before,p->clips,sizeof a->before); memcpy(a->lengths,p->clip_steps,sizeof a->lengths);
    memcpy(a->starts,p->clip_starts,sizeof a->starts); memcpy(a->moved,a->selected,sizeof a->moved); a->gesture=MOVE_CLIPS;
}
void arrangement_drag(Arrangement *a,Project *p,float x,float y) {
    int l=clamp((int)floorf(y),0,LANES-1);
    float previous=a->now_x; a->now_x=x; a->now_y=y;
    if(a->gesture==SIZE_CLIP) {
        float start=p->clip_starts[a->lane][a->bar],q=quantum(a);
        float n=fminf(p->pattern_steps[a->source_pattern],fmaxf(q,roundf((x-start)*STEPS/q)*q));
        p->clip_steps[a->lane][a->bar]=n; a->source_steps=n;
    } else if(a->gesture==MOVE_CLIPS) {
        float dx=snapped(a,x)-snapped(a,a->x); int dy=l-a->lane;
        Project next=*p; memcpy(next.clips,a->before,sizeof next.clips); memcpy(next.clip_steps,a->lengths,sizeof next.clip_steps); memcpy(next.clip_starts,a->starts,sizeof next.clip_starts);
        uint8_t moved[LANES][CLIPS]={0};
        for(int i=0;i<LANES;i++) for(int j=0;j<CLIPS;j++) if(a->selected[i][j]) { next.clips[i][j]=0; next.clip_steps[i][j]=0; }
        for(int i=0;i<LANES;i++) for(int j=0;j<CLIPS;j++) if(a->selected[i][j]) {
            int slot=place(&next,i+dy,a->starts[i][j]+dx,a->before[i][j]-1,a->lengths[i][j]?a->lengths[i][j]:STEPS);
            if(slot<0) return;
            moved[i+dy][slot]=1;
        }
        memcpy(p->clips,next.clips,sizeof p->clips); memcpy(p->clip_steps,next.clip_steps,sizeof p->clip_steps); memcpy(p->clip_starts,next.clip_starts,sizeof p->clip_starts); memcpy(a->moved,moved,sizeof moved);
    } else if(a->gesture==BOX_SELECT || a->gesture==TRACK_SELECT) {
        float left=fminf(a->x,x),right=fmaxf(a->x,x),top=fminf(a->y,y),bottom=fmaxf(a->y,y);
        for(int i=0;i<LANES;i++) for(int j=0;j<CLIPS;j++) a->selected[i][j]=(a->additive && a->selection_before[i][j]) || (p->clips[i][j] && (a->gesture==TRACK_SELECT?i<=bottom:i<bottom) && i+1>top && (a->gesture==TRACK_SELECT || (p->clip_starts[i][j]<right && p->clip_starts[i][j]+clip_length(p,i,j)/STEPS>left)));
    } else if(a->gesture==PAINT_CLIPS || a->gesture==ERASE_CLIPS) {
        if(x<0 || y<0 || y>=LANES) { a->last_lane=-1; return; }
        float from=a->last_lane==l?previous:x,left=fminf(from,x),right=fmaxf(from,x);
        if(a->gesture==ERASE_CLIPS) {
            for(int j=0;j<CLIPS;j++) if(p->clips[l][j] && p->clip_starts[l][j]<=right && p->clip_starts[l][j]+clip_length(p,l,j)/STEPS>left) { p->clips[l][j]=0; p->clip_steps[l][j]=0; a->selected[l][j]=0; }
        } else {
            float stride=ceilf(a->source_steps/quantum(a))*quantum(a)/STEPS,origin=snapped(a,a->x);
            if(stride<=0) stride=1;
            float first=ceilf((left-origin)/stride-.00001f);
            for(int k=0;k<CLIPS && origin+(first+k)*stride<=right+.00001f;k++) paint(a,p,l,origin+(first+k)*stride);
        }
        a->last_lane=l;
    }
}
void arrangement_release(Arrangement *a) {
    if(a->gesture==MOVE_CLIPS) memcpy(a->selected,a->moved,sizeof a->selected);
    a->gesture=IDLE;
}
void arrangement_zoom(Arrangement *a,float wheel,float cursor) {
    float span=BARS/(a->zoom>0?a->zoom:1);
    timeline_zoom(&span,&a->view_start,wheel,cursor,1); a->zoom=BARS/span;
    a->range=fmaxf(a->range,a->view_start+span*2);
}
