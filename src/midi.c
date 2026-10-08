// SPDX-License-Identifier: GPL-3.0-only
#include "midi.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
void midi_parse(MidiParser *p,const unsigned char *data,unsigned count,double time,MidiEmit emit,void *context) {
    for(unsigned i=0;i<count;i++) {
        unsigned char b=data[i]; if(b>=0xf8) continue;
        if(b&0x80) {
            p->count=0;
            if(b>=0xf0) { p->status=0; p->sysex=b==0xf0; continue; }
            p->status=b; p->sysex=0; continue;
        }
        if(p->sysex || !p->status) continue;
        p->data[p->count++]=b;
        unsigned needed=(p->status&0xf0)==0xc0 || (p->status&0xf0)==0xd0?1:2;
        if(p->count==needed) { emit(context,(MidiEvent){p->status,p->data[0],needed==2?p->data[1]:0,time}); p->count=0; }
    }
}
int midi_bind(Project *p,unsigned channel,unsigned controller,ParameterTarget target) {
    float v,lo,hi;if(channel>15 || controller>119 || !parameter_info(p,target,&v,&lo,&hi))return 0;
    int i=0;for(;i<p->midi_binding_count;i++)if(p->midi_bindings[i].channel==channel && p->midi_bindings[i].controller==controller)break;
    if(i>=MIDI_BINDINGS)return 0;if(i==p->midi_binding_count)p->midi_binding_count++;
    p->midi_bindings[i]=(MidiBinding){channel,controller,target};return 1;
}
void midi_unbind(Project *p,ParameterTarget target) {
    for(int i=p->midi_binding_count-1;i>=0;i--){ParameterTarget x=p->midi_bindings[i].target;
        if(x.parameter==target.parameter && x.owner==target.owner && x.slot==target.slot){memmove(&p->midi_bindings[i],&p->midi_bindings[i+1],(--p->midi_binding_count-i)*sizeof p->midi_bindings[0]);memset(&p->midi_bindings[p->midi_binding_count],0,sizeof p->midi_bindings[0]);}}
}
static float position(const MidiTake *t,double time) { return fmax(0,(time-t->time)*t->bpm/15); }
static int free_lane(const Project *p) {
    for(int l=0;l<LANES;l++) { int used=0;for(int j=0;j<CLIPS;j++)used|=p->clips[l][j]!=0;if(!used)return l; } return -1;
}
int midi_take_begin(MidiTake *t,Project *p,int channel,int notes,float origin,double time) {
    if(channel<0 || channel>=p->channel_count || !isfinite(origin) || origin<0 || !isfinite(time))return 0;
    int lane=notes?free_lane(p):-1;
    if(notes && (p->pattern_count>=PATTERNS || lane<0))return 0;
    memset(t,0,sizeof *t); memset(t->held,0xff,sizeof t->held);
    t->active=1;t->pattern=-1;t->channel=channel;t->lane=lane;t->clip=0;t->origin=origin;t->time=time;t->bpm=p->bpm;
    if(notes) {
        t->lane_mute[0]=p->lane_mute[lane];p->lane_mute[lane]=1;
        t->pattern=p->pattern_count++;
        memcpy(t->previous_notes,p->notes[t->pattern],sizeof t->previous_notes);
        memcpy(t->previous_name,p->pattern_names[t->pattern],sizeof t->previous_name);
        t->previous_steps=p->pattern_steps[t->pattern];t->previous_start=p->clip_starts[lane][0];
        t->previous_cap=p->clip_steps[lane][0];t->previous_offset=p->clip_offsets[lane][0];
        p->pattern_steps[t->pattern]=STEPS;
        memset(p->notes[t->pattern],0,sizeof p->notes[t->pattern]);
        snprintf(p->pattern_names[t->pattern],PATTERN_NAME,"MIDI take %d",t->pattern+1);
        p->clips[lane][0]=t->pattern+1;p->clip_starts[lane][0]=origin/STEPS;p->clip_steps[lane][0]=.01f;p->clip_offsets[lane][0]=0;
    }
    return 1;
}
int midi_take_note(MidiTake *t,Project *p,int channel,int pitch,int velocity,double time) {
    if(!t->active || t->pattern<0 || channel<0 || channel>15 || pitch<0 || pitch>127 || !isfinite(time))return 0;
    float step=position(t,time);int *slot=&t->held[channel][pitch];
    if(*slot>=0) { Note *n=&p->notes[t->pattern][t->channel][*slot];n->length=fmaxf(.01f,step-n->start);*slot=-1; }
    if(velocity>0) {
        p->pattern_steps[t->pattern]=fmaxf(p->pattern_steps[t->pattern],ceilf((step+.01f)/STEPS)*STEPS);
        Note *n=note_add(p,t->pattern,t->channel,step,pitch,.01f);
        if(!n) {t->failed=1;return 0;}n->velocity=fminf(127,velocity);*slot=n-p->notes[t->pattern][t->channel];
    }
    return 1;
}
int midi_take_control(MidiTake *t,Project *p,ParameterTarget target,float value,double time) {
    float original,lo,hi;if(!t->active || !parameter_info(p,target,&original,&lo,&hi) || !isfinite(time) || !isfinite(value))return 0;
    int entry=0;for(;entry<t->count;entry++) {ParameterTarget x=p->automations[t->curves[entry]].target;if(x.parameter==target.parameter && x.owner==target.owner && x.slot==target.slot)break;}
    if(entry==t->count) {
        int lane=free_lane(p);if(lane<0 || t->count>=AUTOMATIONS){t->failed=1;return 0;}
        int a=automation_create(p,target,parameter_descriptor(target.parameter)->name,STEPS);if(a<0){t->failed=1;return 0;}
        t->lane_mute[entry+1]=p->lane_mute[lane];p->lane_mute[lane]=1;
        t->curves[entry]=a;t->curve_lane[entry]=lane;t->curve_clip[entry]=0;t->count++;
        p->automations[a].count=0;p->clips[lane][0]=AUTOMATION_SOURCE+a+1;p->clip_starts[lane][0]=t->origin/STEPS;p->clip_steps[lane][0]=.01f;p->clip_offsets[lane][0]=0;
        float initial=fmaxf(0,fminf(1,(original-lo)/(hi-lo)));
        automation_point(&p->automations[a],0,initial);
        float first=position(t,time);p->automations[a].steps=fmaxf(STEPS,ceilf((first+.01f)/STEPS)*STEPS);
        if(first>.01f)automation_point(&p->automations[a],first-.01f,initial);
    }
    Automation *a=&p->automations[t->curves[entry]];float step=position(t,time);a->steps=fmaxf(a->steps,ceilf((step+.01f)/STEPS)*STEPS);
    value=fmaxf(0,fminf(1,value));
    if(a->count)step=fmaxf(step,a->points[a->count-1].step);
    if(a->count>=2 && fabsf(a->points[a->count-1].value-value)<.000001f && fabsf(a->points[a->count-2].value-value)<.000001f) {
        a->points[a->count-1].step=step;return 1;
    }
    /* Compact the least significant interior point, keeping both endpoints
       and important bends instead of collapsing a long take into one ramp. */
    if(a->count>=AUTOMATION_POINTS) {
        int remove=1;float smallest=INFINITY;
        for(int i=1;i<a->count-1;i++){AutomationPoint x=a->points[i-1],y=a->points[i],z=a->points[i+1];
            float span=z.step-x.step,estimate=span>0?x.value+(z.value-x.value)*(y.step-x.step)/span:y.value;
            float error=fabsf(estimate-y.value)*span;if(error<smallest){smallest=error;remove=i;}}
        memmove(&a->points[remove],&a->points[remove+1],(--a->count-remove)*sizeof a->points[0]);
    }
    if(automation_point(a,step,value)<0)return 0;
    return 1;
}
void midi_take_update(MidiTake *t,Project *p,double time) {
    if(!t->active || !isfinite(time))return;float step=position(t,time),length=fmaxf(.01f,step);
    if(t->pattern>=0) {
        for(int c=0;c<16;c++)for(int k=0;k<128;k++)if(t->held[c][k]>=0){Note *n=&p->notes[t->pattern][t->channel][t->held[c][k]];n->length=fmaxf(.01f,step-n->start);}
        p->pattern_steps[t->pattern]=fmaxf(p->pattern_steps[t->pattern],ceilf((step+.01f)/STEPS)*STEPS);
        p->clip_steps[t->lane][t->clip]=length;
    }
    for(int i=0;i<t->count;i++)p->clip_steps[t->curve_lane[i]][t->curve_clip[i]]=length;
}
void midi_take_finish(MidiTake *t,Project *p,double time) {
    midi_take_update(t,p,time);t->active=0;
    if(t->pattern>=0)p->lane_mute[t->lane]=t->lane_mute[0];
    for(int i=0;i<t->count;i++)p->lane_mute[t->curve_lane[i]]=t->lane_mute[i+1];
    if(t->pattern>=0){int used=0;for(int i=0;i<NOTES;i++)used|=p->notes[t->pattern][t->channel][i].velocity;if(!used && t->pattern==p->pattern_count-1){p->clips[t->lane][t->clip]=0;p->pattern_count--;
            memcpy(p->notes[t->pattern],t->previous_notes,sizeof t->previous_notes);
            memcpy(p->pattern_names[t->pattern],t->previous_name,sizeof t->previous_name);
            p->pattern_steps[t->pattern]=t->previous_steps;p->clip_starts[t->lane][t->clip]=t->previous_start;
            p->clip_steps[t->lane][t->clip]=t->previous_cap;p->clip_offsets[t->lane][t->clip]=t->previous_offset;}}
    memset(t->held,0xff,sizeof t->held);
}
