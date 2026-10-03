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
int arrangement_place(Project *p,int l,float x,int pattern,float length) {
    if(l<0 || l>=LANES || pattern<0 || pattern>=PATTERNS+CHANNELS || x<0 || !isfinite(x) || !isfinite(length) || length<=0) return -1;
    for(int i=0;i<CLIPS;i++) if(p->clips[l][i] && x*STEPS<p->clip_starts[l][i]*STEPS+clip_length(p,l,i)-.0001f && p->clip_starts[l][i]*STEPS<x*STEPS+length-.0001f) return -1;
    int b=x<CLIPS?(int)x:0;
    if(p->clips[l][b]) { for(b=0;b<CLIPS && p->clips[l][b];b++); if(b==CLIPS) return -1; }
    p->clips[l][b]=pattern+1; p->clip_steps[l][b]=pattern>=PATTERNS?(fabsf(length-clip_source_steps(p,pattern))<.0001f?0:length*15/p->bpm):length; p->clip_starts[l][b]=x; p->clip_offsets[l][b]=0; return b;
}
static float source_steps(const Arrangement *a,const Project *p) { return a->source_pattern>=PATTERNS?a->source_steps*p->bpm/15:a->source_steps; }
static int place_copy(Project *p,int lane,float start,int source,float length,float offset) {
    int slot=arrangement_place(p,lane,start,source,length);
    if(slot>=0) {
        p->clip_offsets[lane][slot]=offset;
        if(source>=PATTERNS) p->clip_steps[lane][slot]=fabsf(length-(clip_source_steps(p,source)-clip_offset_steps(p,lane,slot)))<.0001f?0:length*15/p->bpm;
    }
    return slot;
}
static int paint(Arrangement *a,Project *p,int l,float x) {
    float length=source_steps(a,p); return place_copy(p,l,x,a->source_pattern,length?length:STEPS,a->source_offset);
}
void arrangement_press(Arrangement *a,Project *p,float x,float y,int right,int edge,int pattern,int additive) {
    int l=(int)floorf(y),hit=arrangement_hit(p,l,x);
    a->additive=additive; memcpy(a->selection_before,a->selected,sizeof a->selected);
    a->x=a->now_x=x; a->y=a->now_y=y; a->lane=l; a->bar=hit;
    a->last_lane=l;
    if(l<0 || l>=LANES) return;
    if(x<0) { if(!additive) memset(a->selected,0,sizeof a->selected); a->gesture=TRACK_SELECT; arrangement_drag(a,p,x,y); return; }
    if(right) { a->gesture=ERASE_CLIPS; arrangement_drag(a,p,x,y); return; }
    if(additive) {
        if(hit>=0) { a->selected[l][hit]^=1; a->source_pattern=p->clips[l][hit]-1; a->source_steps=a->source_pattern>=PATTERNS?clip_length(p,l,hit)*15/p->bpm:clip_length(p,l,hit); a->source_offset=p->clip_offsets[l][hit]; }
        else a->gesture=BOX_SELECT;
        return;
    }
    if(a->source_pattern<PATTERNS && a->source_pattern!=pattern) { a->source_pattern=pattern; a->source_steps=p->pattern_steps[pattern]; a->source_offset=0; }
    if(hit>=0) {
        a->source_pattern=p->clips[l][hit]-1; a->source_steps=a->source_pattern>=PATTERNS?clip_length(p,l,hit)*15/p->bpm:clip_length(p,l,hit);
        a->source_offset=p->clip_offsets[l][hit];
        if(edge) {
            a->edge=edge; a->size_start=p->clip_starts[l][hit]; a->size_length=clip_length(p,l,hit); a->size_offset=clip_offset_steps(p,l,hit); a->size_cap=p->clip_steps[l][hit];
            a->gesture=SIZE_CLIP; memset(a->selected,0,sizeof a->selected); a->selected[l][hit]=1; return;
        }
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
    memcpy(a->before,p->clips,sizeof a->before);
    for(int i=0;i<LANES;i++) for(int j=0;j<CLIPS;j++) a->lengths[i][j]=clip_length(p,i,j);
    memcpy(a->starts,p->clip_starts,sizeof a->starts); memcpy(a->offsets,p->clip_offsets,sizeof a->offsets); memcpy(a->moved,a->selected,sizeof a->moved); a->gesture=MOVE_CLIPS;
}
void arrangement_drag(Arrangement *a,Project *p,float x,float y) {
    int l=clamp((int)floorf(y),0,LANES-1);
    float previous=a->now_x; a->now_x=x; a->now_y=y;
    if(a->gesture==SIZE_CLIP) {
        int lane=a->lane,b=a->bar; float start=a->size_start,q=quantum(a),offset=a->size_offset;
        if(fabsf(x-a->x)<.00001f) {
            p->clip_starts[lane][b]=start; p->clip_steps[lane][b]=a->size_cap;
            p->clip_offsets[lane][b]=a->source_pattern>=PATTERNS?offset*15/p->bpm*channel_speed(p,a->source_pattern-PATTERNS):offset;
            a->source_offset=p->clip_offsets[lane][b]; a->source_steps=a->source_pattern>=PATTERNS?a->size_length*15/p->bpm:a->size_length;
            return;
        }
        float n=fmaxf(q,roundf((x-start)*STEPS/q)*q);
        if(a->edge<0) {
            float delta=roundf((x-a->x)*STEPS/q)*q;
            delta=fmaxf(fmaxf(-offset,-start*STEPS),fminf(a->size_length-fminf(q,a->size_length),delta));
            p->clip_starts[lane][b]=start+delta/STEPS; offset+=delta; n=a->size_length-delta;
            p->clip_offsets[lane][b]=a->source_pattern>=PATTERNS?offset*15/p->bpm*channel_speed(p,a->source_pattern-PATTERNS):offset;
        } else if(a->source_pattern<PATTERNS) n=fminf(fmaxf(q,clip_source_steps(p,a->source_pattern)-offset),n);
        float available=fmaxf(0,clip_source_steps(p,a->source_pattern)-offset);
        p->clip_steps[lane][b]=a->source_pattern>=PATTERNS?(fabsf(n-available)<.0001f?0:n*15/p->bpm):n;
        a->source_offset=p->clip_offsets[lane][b];
        a->source_steps=a->source_pattern>=PATTERNS?n*15/p->bpm:n;
    } else if(a->gesture==MOVE_CLIPS) {
        float dx=snapped(a,x)-snapped(a,a->x); int dy=l-a->lane;
        float first=INFINITY; int top=LANES,bottom=-1;
        for(int i=0;i<LANES;i++) for(int j=0;j<CLIPS;j++) if(a->selected[i][j]) {
            first=fminf(first,a->starts[i][j]); if(i<top) top=i; if(i>bottom) bottom=i;
        }
        if(bottom<0) return;
        dx=fmaxf(-first,dx); dy=clamp(dy,-top,LANES-1-bottom);
        Project next=*p; memcpy(next.clips,a->before,sizeof next.clips); memcpy(next.clip_starts,a->starts,sizeof next.clip_starts); memcpy(next.clip_offsets,a->offsets,sizeof next.clip_offsets);
        uint8_t moved[LANES][CLIPS]={0};
        for(int i=0;i<LANES;i++) for(int j=0;j<CLIPS;j++) if(a->selected[i][j]) { next.clips[i][j]=0; next.clip_steps[i][j]=0; }
        for(int i=0;i<LANES;i++) for(int j=0;j<CLIPS;j++) if(a->selected[i][j]) {
            int slot=place_copy(&next,i+dy,a->starts[i][j]+dx,a->before[i][j]-1,a->lengths[i][j]?a->lengths[i][j]:STEPS,a->offsets[i][j]);
            if(slot<0) return;
            moved[i+dy][slot]=1;
        }
        memcpy(p->clips,next.clips,sizeof p->clips); memcpy(p->clip_steps,next.clip_steps,sizeof p->clip_steps); memcpy(p->clip_starts,next.clip_starts,sizeof p->clip_starts); memcpy(p->clip_offsets,next.clip_offsets,sizeof p->clip_offsets); memcpy(a->moved,moved,sizeof moved);
    } else if(a->gesture==BOX_SELECT || a->gesture==TRACK_SELECT) {
        float left=fminf(a->x,x),right=fmaxf(a->x,x),top=fminf(a->y,y),bottom=fmaxf(a->y,y);
        for(int i=0;i<LANES;i++) for(int j=0;j<CLIPS;j++) a->selected[i][j]=(a->additive && a->selection_before[i][j]) || (p->clips[i][j] && (a->gesture==TRACK_SELECT?i<=bottom:i<bottom) && i+1>top && (a->gesture==TRACK_SELECT || (p->clip_starts[i][j]<right && p->clip_starts[i][j]+clip_length(p,i,j)/STEPS>left)));
    } else if(a->gesture==PAINT_CLIPS || a->gesture==ERASE_CLIPS) {
        if(x<0 || y<0 || y>=LANES) { a->last_lane=-1; return; }
        float from=a->last_lane==l?previous:x,left=fminf(from,x),right=fmaxf(from,x);
        if(a->gesture==ERASE_CLIPS) {
            for(int j=0;j<CLIPS;j++) if(p->clips[l][j] && p->clip_starts[l][j]<=right && p->clip_starts[l][j]+clip_length(p,l,j)/STEPS>left) { p->clips[l][j]=0; p->clip_steps[l][j]=0; a->selected[l][j]=0; }
        } else {
            float stride=ceilf(source_steps(a,p)/quantum(a))*quantum(a)/STEPS,origin=snapped(a,a->x);
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
