// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
/* Stable built-in parameter IDs. Plugin processors will register parameter IDs
   with the same normalized point contract; unresolved IDs survive save/load. */
static const float *parameter_pointer(const Project *p,ParameterTarget t,float *lo,float *hi) {
    *lo=0; *hi=1;
    switch(t.parameter) {
    case PARAM_CHANNEL_VOLUME: *hi=VOLUME_KNOB_MAX; return t.owner<(unsigned)p->channel_count?&p->volume[t.owner]:NULL;
    case PARAM_CHANNEL_PAN: *lo=-1; return t.owner<(unsigned)p->channel_count?&p->pan[t.owner]:NULL;
    case PARAM_CHANNEL_PITCH: *lo=-1; return t.owner<(unsigned)p->channel_count?&p->channel_pitch[t.owner]:NULL;
    case PARAM_INSERT_VOLUME: *hi=MIXER_GAIN_MAX; return t.owner<(unsigned)p->insert_count?&p->insert_volume[t.owner]:NULL;
    case PARAM_INSERT_PAN: *lo=-1; return t.owner<(unsigned)p->insert_count?&p->insert_pan[t.owner]:NULL;
    case PARAM_INSERT_WIDTH: *hi=2; return t.owner<(unsigned)p->insert_count?&p->insert_width[t.owner]:NULL;
    case PARAM_MASTER_VOLUME: *hi=MIXER_GAIN_MAX; return &p->master;
    case PARAM_MASTER_WIDTH: *hi=2; return &p->master_width;
    case PARAM_MASTER_PITCH: *lo=-12; *hi=12; return &p->master_pitch;
    case PARAM_SWING: return &p->swing;
    case PARAM_PITCH_RANGE: *lo=1; *hi=48; return t.owner<(unsigned)p->channel_count?&p->pitch_range[t.owner]:NULL;
    default: return NULL;
    }
}
int parameter_info(const Project *p,ParameterTarget t,float *v,float *lo,float *hi) {
    const float *ptr=parameter_pointer(p,t,lo,hi);
    if(ptr) { *v=*ptr; return 1; }
    *lo=0; *hi=1;
    if(t.parameter==PARAM_CHANNEL_MUTE && t.owner<(unsigned)p->channel_count) *v=!!(p->mute[t.owner]&1);
    else if(t.parameter==PARAM_INSERT_MUTE && t.owner<(unsigned)p->insert_count) *v=!!(p->insert_mute[t.owner]&1);
    else if(t.parameter==PARAM_MASTER_MUTE) *v=!!p->master_mute;
    else return 0;
    return 1;
}
int parameter_from_pointer(const Project *p,const void *ptr,ParameterTarget *t) {
    for(unsigned id=1;id<=PARAM_PITCH_RANGE;id++) for(unsigned owner=0;owner<(id<=PARAM_CHANNEL_PITCH || id==PARAM_CHANNEL_MUTE || id==PARAM_PITCH_RANGE?CHANNELS:id<=PARAM_INSERT_WIDTH || id==PARAM_INSERT_MUTE?INSERTS:1);owner++) {
        ParameterTarget candidate={id,owner,0}; float lo,hi;
        const void *address=parameter_pointer(p,candidate,&lo,&hi);
        if(id==PARAM_CHANNEL_MUTE) address=&p->mute[owner];
        if(id==PARAM_INSERT_MUTE) address=&p->insert_mute[owner];
        if(id==PARAM_MASTER_MUTE) address=&p->master_mute;
        if(address==ptr) { *t=candidate; return 1; }
    }
    return 0;
}
float automation_value(const Automation *a,float step) {
    if(!a->count) return 0;
    if(step<a->points[0].step) return a->points[0].value;
    for(int i=1;i<a->count;i++) if(step<a->points[i].step) {
        AutomationPoint x=a->points[i-1],y=a->points[i];
        return x.value+(y.value-x.value)*(step-x.step)/(y.step-x.step);
    }
    return a->points[a->count-1].value;
}
int automation_evaluate(const Project *p,ParameterTarget target,float step,float *normalized) {
    int active=0,solo=solo_any(p->lane_mute,LANES); float held_end=-INFINITY;
    for(int l=0;l<LANES;l++) if(!(p->lane_mute[l]&1) && (!solo || (p->lane_mute[l]&2))) for(int b=0;b<CLIPS;b++) {
        int a=p->clips[l][b]-AUTOMATION_SOURCE-1;
        if(a<0 || a>=p->automation_count) continue;
        const Automation *curve=&p->automations[a];
        if(curve->target.parameter!=target.parameter || curve->target.owner!=target.owner || curve->target.slot!=target.slot) continue;
        float start=p->clip_starts[l][b]*STEPS;
        float end=start+clip_length(p,l,b);
        if(step>=start && step<end) { *normalized=automation_value(curve,step-start+p->clip_offsets[l][b]); active=2; }
        else if(step>=end && active!=2 && end>=held_end) { *normalized=automation_value(curve,end-start+p->clip_offsets[l][b]); held_end=end; active=1; }
    }
    return active;
}
int automation_point(Automation *a,float step,float value) {
    if(!isfinite(step) || !isfinite(value) || step<0 || step>a->steps || value<0 || value>1) return -1;
    int i=0; while(i<a->count && a->points[i].step<step) i++;
    if(i<a->count && fabsf(a->points[i].step-step)<.00001f) { a->points[i].value=value; return i; }
    if(a->count>=AUTOMATION_POINTS) return -1;
    memmove(&a->points[i+1],&a->points[i],(a->count-i)*sizeof a->points[0]); a->count++;
    a->points[i]=(AutomationPoint){step,value}; return i;
}
int automation_move_point(Project *p,int index,int point,float step,float value) {
    if(index<0 || index>=p->automation_count || !isfinite(step) || fabsf(step)>1e14f || !isfinite(value) || value<0 || value>1) return -1;
    Automation *a=&p->automations[index];
    if(point<0 || point>=a->count) return -1;
    if(point>0) step=fmaxf(a->points[point-1].step,step);
    if(point<a->count-1) step=fminf(a->points[point+1].step,step);
    if(step<0) {
        float shift=-step;
        for(int n=0;n<a->count;n++) a->points[n].step+=shift;
        a->steps+=shift;
        /* Preserve other copies' original source positions when prepending time. */
        for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) if(p->clips[l][b]==AUTOMATION_SOURCE+index+1) p->clip_offsets[l][b]+=shift;
        step=0;
    }
    a->points[point]=(AutomationPoint){step,value}; a->steps=fmaxf(a->steps,step);
    return point;
}
int automation_create(Project *p,ParameterTarget target,const char *name,float steps) {
    float v,lo,hi;
    if(p->automation_count<0 || p->automation_count>=AUTOMATIONS || !isfinite(steps) || steps<=0 || !parameter_info(p,target,&v,&lo,&hi)) return -1;
    int index=p->automation_count++; Automation *a=&p->automations[index]; memset(a,0,sizeof *a);
    a->target=target; a->steps=steps; a->color=pattern_palette[index%PATTERNS];
    for(int color=0;color<PATTERNS;color++) {
        int used=0; for(int i=0;i<index;i++) if(p->automations[i].color==pattern_palette[color]) used=1;
        if(!used) { a->color=pattern_palette[color]; break; }
    }
    snprintf(a->name,sizeof a->name,"%s",name); float value=fmaxf(0,fminf(1,(v-lo)/(hi-lo)));
    automation_point(a,0,value); automation_point(a,steps,value); return index;
}
int automation_delete(Project *p,int index) {
    if(index<0 || index>=p->automation_count) return 0;
    for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) {
        int s=p->clips[l][b]-1;
        if(s==AUTOMATION_SOURCE+index) { p->clips[l][b]=0; p->clip_steps[l][b]=0; }
        else if(s>AUTOMATION_SOURCE+index) p->clips[l][b]--;
    }
    memmove(&p->automations[index],&p->automations[index+1],(--p->automation_count-index)*sizeof p->automations[0]);
    memset(&p->automations[p->automation_count],0,sizeof p->automations[0]); return 1;
}
int automation_valid(const Project *p) {
    if(p->automation_count<0 || p->automation_count>AUTOMATIONS) return 0;
    for(int i=0;i<p->automation_count;i++) {
        const Automation *a=&p->automations[i]; float v,lo,hi;
        if((a->target.parameter<PARAM_PLUGIN && a->target.slot) || (((a->target.parameter>=PARAM_MASTER_VOLUME && a->target.parameter<=PARAM_MASTER_PITCH) || a->target.parameter==PARAM_MASTER_MUTE || a->target.parameter==PARAM_SWING) && a->target.owner) || !a->target.parameter || (!parameter_info(p,a->target,&v,&lo,&hi) && a->target.parameter<PARAM_PLUGIN) || a->target.slot>65535 || a->target.owner>65535 || a->color>0xffffff || !a->name[0] || strnlen(a->name,sizeof a->name)==sizeof a->name || strchr(a->name,'\n') || strchr(a->name,'\r') || !isfinite(a->steps) || a->steps<=0 || a->steps>1e15f || a->count<1 || a->count>AUTOMATION_POINTS) return 0;
        for(int n=0;n<a->count;n++) if(!isfinite(a->points[n].step) || a->points[n].step<0 || a->points[n].step>a->steps || (n && a->points[n].step<a->points[n-1].step) || !isfinite(a->points[n].value) || a->points[n].value<0 || a->points[n].value>1) return 0;
    }
    return 1;
}
