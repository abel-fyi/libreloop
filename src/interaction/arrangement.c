// SPDX-License-Identifier: GPL-3.0-only
#include "arrangement.h"
#include <math.h>
#include <string.h>
static int clamp(int x,int lo,int hi) { return x<lo?lo:x>hi?hi:x; }
static float quantum(const Arrangement *a) { return a->snap>0?a->snap:.01f; }
static float snapped(const Arrangement *a,float x) { return a->snap>0?snap_floor(x*STEPS+.00001f,a->snap)/STEPS:x; }
float arrangement_edit_steps(const Arrangement *a,const Project *p,int pattern) {
    (void)a; return p->pattern_steps[pattern];
}
int arrangement_hit(const Project *p,int lane,float bar) {
    if(lane<0 || lane>=LANES || bar<0) return -1;
    for(int b=CLIPS-1;b>=0;b--) if(p->clips[lane][b] && bar>=p->clip_starts[lane][b] && bar<p->clip_starts[lane][b]+clip_length(p,lane,b)/STEPS) return b;
    return -1;
}
int arrangement_place(Project *p,int l,float x,int pattern,float length) {
    if(l<0 || l>=LANES || pattern<0 || pattern>=SOURCES || x<0 || !isfinite(x) || !isfinite(length) || length<=0) return -1;
    for(int i=0;i<CLIPS;i++) if(p->clips[l][i] && x*STEPS<p->clip_starts[l][i]*STEPS+clip_length(p,l,i)-.0001f && p->clip_starts[l][i]*STEPS<x*STEPS+length-.0001f) return -1;
    int b=x<CLIPS?(int)x:0;
    if(p->clips[l][b]) { for(b=0;b<CLIPS && p->clips[l][b];b++) {} if(b==CLIPS) return -1; }
    p->clips[l][b]=pattern+1; p->clip_steps[l][b]=AUDIO_SOURCE(pattern)?(fabsf(length-clip_source_steps(p,pattern))<.0001f?0:length*15/audio_source_bpm(p,pattern-PATTERNS)):length; p->clip_starts[l][b]=x; p->clip_offsets[l][b]=0; return b;
}
static float source_steps(const Arrangement *a,const Project *p) { return AUDIO_SOURCE(a->source_pattern)?a->source_steps*audio_source_bpm(p,a->source_pattern-PATTERNS)/15:a->source_steps; }
static int place_copy(Project *p,int lane,float start,int source,float length,float offset) {
    int slot=arrangement_place(p,lane,start,source,length);
    if(slot>=0) {
        p->clip_offsets[lane][slot]=offset;
        if(AUDIO_SOURCE(source)) p->clip_steps[lane][slot]=fabsf(length-(clip_source_steps(p,source)-clip_offset_steps(p,lane,slot)))<.0001f?0:length*15/audio_source_bpm(p,source-PATTERNS);
    }
    return slot;
}
static int paint(Arrangement *a,Project *p,int l,float x) {
    float length=source_steps(a,p);
    int slot=place_copy(p,l,x,a->source_pattern,length?length:STEPS,a->source_offset);
    if(slot>=0) a->selected[l][slot]=1;
    return slot;
}
void arrangement_select_press(Arrangement *a,float x,float y,int additive) {
    a->additive=additive; memcpy(a->selection_before,a->selected,sizeof a->selected);
    if(!additive) memset(a->selected,0,sizeof a->selected);
    a->x=a->now_x=x; a->y=a->now_y=y;
    a->gesture=x<0?TRACK_SELECT:BOX_SELECT;
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
        if(hit>=0) { a->selected[l][hit]^=1; a->source_pattern=p->clips[l][hit]-1; a->source_steps=AUDIO_SOURCE(a->source_pattern)?clip_length(p,l,hit)*15/audio_source_bpm(p,a->source_pattern-PATTERNS):clip_length(p,l,hit); a->source_offset=p->clip_offsets[l][hit]; }
        else a->gesture=BOX_SELECT;
        return;
    }
    if(a->tool==CUT) {
        if(hit<0) return;
        float split=snapped(a,x),start=p->clip_starts[l][hit],length=clip_length(p,l,hit);
        float left=(split-start)*STEPS,right_length=length-left;
        if(left<=.0001f || right_length<=.0001f) return;
        int slot=0; while(slot<CLIPS && p->clips[l][slot]) slot++;
        if(slot==CLIPS) return;
        int source=p->clips[l][hit]-1;
        float seconds=AUDIO_SOURCE(source)?15/audio_source_bpm(p,source-PATTERNS):1;
        p->clips[l][slot]=p->clips[l][hit]; p->clip_starts[l][slot]=split;
        p->clip_offsets[l][slot]=AUDIO_SOURCE(source)?audio_clip_position(p,l,hit,split*STEPS)/RATE:p->clip_offsets[l][hit]+left;
        p->clip_steps[l][hit]=left*(AUDIO_SOURCE(source)?seconds:1);
        p->clip_steps[l][slot]=right_length*(AUDIO_SOURCE(source)?seconds:1);
        memset(a->selected,0,sizeof a->selected); a->selected[l][hit]=a->selected[l][slot]=1;
        return;
    }
    if(a->tool==STRETCH && (hit<0 || !edge || !AUDIO_SOURCE(p->clips[l][hit]-1))) return;
    if(a->source_pattern<PATTERNS && a->source_pattern!=pattern) { a->source_pattern=pattern; a->source_steps=p->pattern_steps[pattern]; a->source_offset=0; }
    if(hit>=0) {
        a->source_pattern=p->clips[l][hit]-1; a->source_steps=AUDIO_SOURCE(a->source_pattern)?clip_length(p,l,hit)*15/audio_source_bpm(p,a->source_pattern-PATTERNS):clip_length(p,l,hit);
        a->source_offset=p->clip_offsets[l][hit];
        if(edge) {
            a->edge=edge; a->size_start=p->clip_starts[l][hit]; a->size_length=clip_length(p,l,hit); a->size_offset=clip_offset_steps(p,l,hit); a->size_cap=p->clip_steps[l][hit];
            a->gesture=a->tool==STRETCH?STRETCH_CLIP:SIZE_CLIP;
            if(a->gesture==STRETCH_CLIP) {
                int c=a->source_pattern-PATTERNS;
                a->size_time=p->sampler[c].time; a->size_seconds=p->audio_seconds[c];
                memcpy(a->offsets,p->clip_offsets,sizeof a->offsets);
                memcpy(a->lengths,p->clip_steps,sizeof a->lengths);
            }
            if(a->gesture==STRETCH_CLIP || !a->selected[l][hit]) {
                memset(a->selected,0,sizeof a->selected); a->selected[l][hit]=1;
            }
            if(a->gesture==SIZE_CLIP) {
                memcpy(a->before,p->clips,sizeof a->before);
                memcpy(a->starts,p->clip_starts,sizeof a->starts);
                memcpy(a->offsets,p->clip_offsets,sizeof a->offsets);
                memcpy(a->caps,p->clip_steps,sizeof a->caps);
                for(int i=0;i<LANES;i++) for(int j=0;j<CLIPS;j++) a->lengths[i][j]=clip_length(p,i,j);
            }
            return;
        }
    }
    if(a->tool==SELECT && hit<0) { memset(a->selected,0,sizeof a->selected); a->gesture=BOX_SELECT; return; }
    int group=0; for(int i=0;i<LANES;i++) for(int j=0;j<CLIPS;j++) group+=a->selected[i][j]!=0;
    if(a->tool==BRUSH && !(hit>=0 && a->selected[l][hit] && group>1)) {
        a->paint_origin=hit>=0?p->clip_starts[l][hit]:snapped(a,x);
        memset(a->selected,0,sizeof a->selected);
        if(hit>=0) a->selected[l][hit]=1;
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
    if(a->gesture==STRETCH_CLIP) {
        int lane=a->lane,b=a->bar,c=a->source_pattern-PATTERNS;
        float delta=snap_round((x-a->x)*STEPS,a->snap);
        if(a->edge<0) delta=fmaxf(delta,-a->size_start*STEPS);
        float n=fmaxf(.01f,a->size_length+(a->edge<0?-delta:delta));
        float time=fmaxf(.25f,fminf(4,a->size_time*n/a->size_length));
        float ratio=time/a->size_time;
        p->sampler[c].time=time; p->audio_seconds[c]=a->size_seconds*ratio;
        for(int l=0;l<LANES;l++) for(int j=0;j<CLIPS;j++) if(p->clips[l][j]==a->source_pattern+1) {
            p->clip_offsets[l][j]=a->offsets[l][j]*ratio;
            p->clip_steps[l][j]=a->lengths[l][j]*ratio;
        }
        p->clip_starts[lane][b]=a->edge<0?fmaxf(0,a->size_start+(a->size_length-a->size_length*ratio)/STEPS):a->size_start;
    } else if(a->gesture==SIZE_CLIP) {
        float q=quantum(a),delta=a->edge<0?snap_round((x-a->x)*STEPS,a->snap):
            snap_round((x-a->size_start)*STEPS,a->snap)-a->size_length;
        int unchanged=fabsf(x-a->x)<.00001f;
        float low=-INFINITY,high=INFINITY;
        for(int i=0;i<LANES;i++) for(int j=0;j<CLIPS;j++) if(a->selected[i][j] && a->before[i][j]) {
            int source=a->before[i][j]-1;
            float offset=AUDIO_SOURCE(source)?a->offsets[i][j]*audio_source_bpm(p,source-PATTERNS)/15/channel_speed(p,source-PATTERNS):a->offsets[i][j];
            float length=a->lengths[i][j],minimum=fminf(q,length);
            if(a->edge<0) {
                low=fmaxf(low,-a->starts[i][j]*STEPS);
                if(source<AUTOMATION_SOURCE) low=fmaxf(low,-offset);
                high=fminf(high,length-minimum);
            } else {
                low=fmaxf(low,minimum-length);
                if(source<PATTERNS) high=fminf(high,fmaxf(q,clip_source_steps(p,source)-offset)-length);
            }
        }
        delta=unchanged?0:fmaxf(low,fminf(high,delta));
        if(a->edge<0 && delta<0) {
            for(int i=0;i<LANES;i++) for(int j=0;j<CLIPS;j++) if(a->selected[i][j] && a->before[i][j]>AUTOMATION_SOURCE) {
                int source=a->before[i][j]-1;
                float shift=-a->offsets[i][j]-delta;
                if(shift<=0) continue;
                Automation *curve=&p->automations[source-AUTOMATION_SOURCE];
                for(int n=0;n<curve->count;n++) curve->points[n].step+=shift;
                curve->steps+=shift;
                for(int lane=0;lane<LANES;lane++) for(int slot=0;slot<CLIPS;slot++) if(p->clips[lane][slot]==source+1) {
                    p->clip_offsets[lane][slot]+=shift; a->offsets[lane][slot]+=shift;
                }
            }
        }
        for(int i=0;i<LANES;i++) for(int j=0;j<CLIPS;j++) if(a->selected[i][j] && a->before[i][j]) {
            int source=a->before[i][j]-1;
            float conversion=AUDIO_SOURCE(source)?15/audio_source_bpm(p,source-PATTERNS)*channel_speed(p,source-PATTERNS):1;
            float offset=a->offsets[i][j]/conversion,length=a->lengths[i][j]+(a->edge<0?-delta:delta);
            p->clip_starts[i][j]=a->starts[i][j]+(a->edge<0?delta/STEPS:0);
            p->clip_offsets[i][j]=a->offsets[i][j]+(a->edge<0?delta*conversion:0);
            if(unchanged) p->clip_steps[i][j]=a->caps[i][j];
            else {
                if(a->edge<0) offset+=delta;
                float available=fmaxf(0,clip_source_steps(p,source)-offset);
                p->clip_steps[i][j]=AUDIO_SOURCE(source)?(fabsf(length-available)<.0001f?0:length*15/audio_source_bpm(p,source-PATTERNS)):length;
            }
        }
        a->source_offset=p->clip_offsets[a->lane][a->bar];
        float length=clip_length(p,a->lane,a->bar);
        a->source_steps=AUDIO_SOURCE(a->source_pattern)?length*15/audio_source_bpm(p,a->source_pattern-PATTERNS):length;
    } else if(a->gesture==MOVE_CLIPS) {
        float dx=a->snap>0?snapped(a,x)-snapped(a,a->x):x-a->x; int dy=l-a->lane;
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
            float stride=source_steps(a,p)/STEPS,origin=a->paint_origin;
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
