// SPDX-License-Identifier: GPL-3.0-only
/* Reference filter/melodic/paint behavior with optional zoomed geometry. */
#include "rack.h"
#include <math.h>
float rack_step_width(const RackState *state) {return state->step_width>0?state->step_width:16;}
void rack_zoom(float *step_width,float *view,float grid_width,float wheel,float anchor) {
    if(grid_width<=0 || !isfinite(wheel))return;
    float width=*step_width>0?*step_width:16,old=grid_width/width,span=old,start=*view;
    timeline_zoom(&span,&start,fmaxf(-128,fminf(128,wheel)),anchor,STEPS);
    span=fmaxf(grid_width/128,fminf(grid_width/4,span));
    *view=fmaxf(0,*view+anchor*(old-span));*step_width=grid_width/span;
}
int rack_channels(const RackState *state,const Project *project,int *rows) {
    int count=0;
    for(int c=0;c<project->channel_count;c++)if(!state->filter || project->channel_audio[c]==(state->filter==1)){if(rows)rows[count]=c;count++;}
    return count;
}
int rack_channel_at(const RackState *state,const Project *project,int row) {
    int rows[CHANNELS],count=rack_channels(state,project,rows);return row>=0 && row<count?rows[row]:-1;
}
int rack_melodic(const RackState *state,const Project *project,int c) {
    if(state->melodic && state->melodic[c])return 1;
    for(int i=0;i<NOTES;i++){Note n=project->notes[state->pattern][c][i];if(n.velocity && (n.pitch!=60 || n.length>0))return 1;}return 0;
}
void rack_paint(RackState *state,Project *project,float width,float height,float x,float y) {
    int visible=fmaxf(1,(height-32-48)/28);
    if(x<200 || x>=width-26 || y<32 || y>=32+visible*28){state->paint_channel=-1;return;}
    int c=rack_channel_at(state,project,state->scroll+(y-32)/28);float step=floorf(state->view+(x-200)/rack_step_width(state));
    if(c<0 || c>=project->channel_count || rack_melodic(state,project,c)){state->paint_channel=-1;return;}
    float from=state->paint_channel==c?state->paint_step:step;
    for(int i=0;i<=fabsf(step-from) && i<NOTES;i++) {
        float start=fminf(from,step)+i;Note *n=note_at(project,state->pattern,c,start,60);
        if(state->paint==1){if(n)n->velocity=0;}
        else if(!n) {
            if(start+1>project->pattern_steps[state->pattern])project->pattern_steps[state->pattern]=ceilf((start+1)/STEPS)*STEPS;
            note_add(project,state->pattern,c,start,60,0);
        }
    }
    state->paint_channel=c;state->paint_step=step;
}

/* Preflight prevents partial fills when the channel has no room. */
int pattern_hits_paint(Project *p,int pattern,int channel,float from,float to,int erase) {
    if(pattern<0 || pattern>=p->pattern_count || channel<0 || channel>=p->channel_count || !isfinite(from) || !isfinite(to) || from<0 || to<0)return -1;
    float low=floorf(fminf(from,to)),high=floorf(fmaxf(from,to));
    if(high-low>=NOTES)return -1;
    int free=0,missing=0,changed=0;
    for(int i=0;i<NOTES;i++)free+=!p->notes[pattern][channel][i].velocity;
    if(!erase)for(int i=0;i<=(int)(high-low);i++)if(!note_at(p,pattern,channel,low+i,60))missing++;
    if(missing>free)return -1;
    if(!erase && high+1>p->pattern_steps[pattern])p->pattern_steps[pattern]=ceilf((high+1)/STEPS)*STEPS;
    for(int i=0;i<=(int)(high-low);i++) {
        float step=low+i;Note *n=note_at(p,pattern,channel,step,60);
        if(erase){if(n){n->velocity=0;changed++;}}
        else if(!n){note_add(p,pattern,channel,step,60,0);changed++;}
    }
    return changed;
}

int rack_fill_steps(Project *p,int pattern,int channel,int interval,int apply) {
    if(pattern<0 || pattern>=p->pattern_count || channel<0 || channel>=p->channel_count || interval<1)return -1;
    int free=0,missing=0;for(int i=0;i<NOTES;i++)free+=!p->notes[pattern][channel][i].velocity;
    for(float step=0;step<p->pattern_steps[pattern];step+=interval) {
        if(!note_at(p,pattern,channel,step,60) && ++missing>free)return -1;
    }
    if(apply)for(float step=0;step<p->pattern_steps[pattern];step+=interval)
        if(!note_at(p,pattern,channel,step,60))note_add(p,pattern,channel,step,60,0);
    return missing;
}
