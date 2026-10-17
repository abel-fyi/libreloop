// SPDX-License-Identifier: GPL-3.0-only
#include "edit_clipboard.h"
#include <math.h>
#include <string.h>
int clipboard_copy_notes(EditClipboard *c,const Project *p,int pat,int ch,const uint8_t selected[NOTES]) {
    if(pat<0 || pat>=p->pattern_count || ch<0 || ch>=p->channel_count) return 0;
    int count=0; float start=INFINITY;
    for(int i=0;i<NOTES;i++) if(selected[i] && p->notes[pat][ch][i].velocity) { count++; start=fminf(start,p->notes[pat][ch][i].start); }
    if(!count) return 0;
    c->kind=COPY_NOTES; c->count=0;
    for(int i=0;i<NOTES;i++) if(selected[i] && p->notes[pat][ch][i].velocity) { Note n=p->notes[pat][ch][i]; n.start-=start; c->notes[c->count++]=n; }
    return count;
}
int clipboard_copy_clips(EditClipboard *c,const Project *p,const uint8_t selected[LANES][CLIPS]) {
    int count=0,lane=LANES; float start=INFINITY;
    for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) if(selected[l][b] && p->clips[l][b]) { count++; if(l<lane) lane=l; start=fminf(start,p->clip_starts[l][b]); }
    if(!count) return 0;
    c->kind=COPY_CLIPS; c->count=0; c->lane=lane; c->channels=p->channel_count; c->patterns=p->pattern_count; c->automations=p->automation_count;
    for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) if(selected[l][b] && p->clips[l][b])
        c->clips[c->count++]=(CopiedClip){l-lane,p->clips[l][b],p->clip_starts[l][b]-start,p->clip_steps[l][b],p->clip_offsets[l][b]};
    return count;
}
int clipboard_paste_notes(const EditClipboard *c,Project *p,int pat,int ch,float start,uint8_t selected[NOTES]) {
    if(c->kind!=COPY_NOTES || !c->count) return 0;
    if(pat<0 || pat>=p->pattern_count || ch<0 || ch>=p->channel_count || !isfinite(start) || start<0) return -4;
    int free=0; float end=p->pattern_steps[pat];
    for(int i=0;i<NOTES;i++) free+=!p->notes[pat][ch][i].velocity;
    if(free<c->count) return -2;
    for(int i=0;i<c->count;i++) {
        const Note *n=&c->notes[i]; float at=start+n->start;
        if(!isfinite(at) || at>1e12f) return -4;
        float finish=at+(n->length?n->length:1); if(!isfinite(finish) || finish<=at) return -4;
        end=fmaxf(end,finish);
    }
    p->pattern_steps[pat]=ceilf(end/STEPS)*STEPS; memset(selected,0,NOTES);
    for(int i=0,slot=0;i<c->count;i++) {
        while(p->notes[pat][ch][slot].velocity) slot++;
        Note n=c->notes[i]; n.start+=start; p->notes[pat][ch][slot]=n; selected[slot++]=1;
    }
    return c->count;
}
int clipboard_paste_clips(const EditClipboard *c,Project *p,int lane,float bar,uint8_t selected[LANES][CLIPS]) {
    if(c->kind!=COPY_CLIPS || !c->count) return 0;
    if(c->channels!=p->channel_count || c->patterns!=p->pattern_count || c->automations!=p->automation_count) return -1;
    int needed[LANES]={0},free[LANES]={0};
    for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) free[l]+=!p->clips[l][b];
    for(int i=0;i<c->count;i++) {
        CopiedClip n=c->clips[i]; int l=lane+n.lane,source=n.source-1; float at=bar+n.start;
        if(l<0 || l>=LANES || at<0 || !isfinite(at) || at>1e12f) return -4;
        if(source<PATTERNS ? source>=p->pattern_count : source<AUTOMATION_SOURCE ? source-PATTERNS>=p->channel_count || !p->channel_audio[source-PATTERNS] : source-AUTOMATION_SOURCE>=p->automation_count) return -1;
        if(++needed[l]>free[l]) return -2;
        /* Reuse the engine duration mapping, including uncapped audio and source offsets. */
        int empty=0; while(empty<CLIPS && p->clips[l][empty]) empty++;
        uint8_t saved=p->clips[l][empty]; float saved_length=p->clip_steps[l][empty],saved_offset=p->clip_offsets[l][empty],saved_start=p->clip_starts[l][empty];
        p->clips[l][empty]=n.source; p->clip_steps[l][empty]=n.length; p->clip_offsets[l][empty]=n.offset; p->clip_starts[l][empty]=at;
        float length=clip_length(p,l,empty)/STEPS;
        p->clips[l][empty]=saved; p->clip_steps[l][empty]=saved_length; p->clip_offsets[l][empty]=saved_offset; p->clip_starts[l][empty]=saved_start;
        for(int b=0;b<CLIPS;b++) if(p->clips[l][b] && at<p->clip_starts[l][b]+clip_length(p,l,b)/STEPS-.00001f && p->clip_starts[l][b]<at+length-.00001f) return -3;
    }
    memset(selected,0,LANES*CLIPS);
    for(int i=0;i<c->count;i++) {
        CopiedClip n=c->clips[i]; int l=lane+n.lane,b=0; while(p->clips[l][b]) b++;
        p->clips[l][b]=n.source; p->clip_starts[l][b]=bar+n.start; p->clip_steps[l][b]=n.length; p->clip_offsets[l][b]=n.offset; selected[l][b]=1;
    }
    return c->count;
}
