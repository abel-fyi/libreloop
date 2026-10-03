// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include <math.h>
#include <float.h>
/* Integrate playback speed over straight automation segments. Pitch is linear
   in semitones, so speed is exponential; its integral and inverse are analytic. */
void audio_timeline_init(AudioTimeline *map,const Project *p,int channel) {
    map->project=p; map->channel=channel; map->count=0;
    unsigned relevant=0;
    for(int a=0;a<p->automation_count;a++) {
        ParameterTarget t=p->automations[a].target;
        if(t.parameter==PARAM_MASTER_PITCH || (t.owner==(unsigned)channel && (t.parameter==PARAM_CHANNEL_PITCH || t.parameter==PARAM_PITCH_RANGE))) relevant|=1u<<a;
    }
    if(!relevant) return;
    int solo=solo_any(p->lane_mute,LANES);
    for(int l=0;l<LANES;l++) if(!(p->lane_mute[l]&1) && (!solo || (p->lane_mute[l]&2))) for(int b=0;b<CLIPS;b++) {
        int a=p->clips[l][b]-AUTOMATION_SOURCE-1;
        if(a>=0 && a<p->automation_count && (relevant&(1u<<a))) map->clips[map->count++]=l*CLIPS+b;
    }
}
static double segment(const AudioTimeline *map,double position,double *speed,double *slope) {
    const Project *p=map->project; int c=map->channel;
    double values[3]={p->channel_pitch[c],p->pitch_range[c],p->master_pitch},slopes[3]={0},next=INFINITY,held_end[3]={-INFINITY,-INFINITY,-INFINITY};
    int active[3]={0};
    for(unsigned n=0;n<map->count;n++) {
        unsigned slot=map->clips[n]; int l=slot/CLIPS,b=slot%CLIPS;
        const Automation *a=&p->automations[p->clips[l][b]-AUTOMATION_SOURCE-1];
        double start=p->clip_starts[l][b]*STEPS,end=start+(p->clip_steps[l][b]?p->clip_steps[l][b]:STEPS);
        if(start>position) next=fmin(next,start);
        if(end>position) next=fmin(next,end);
        if(position<start) continue;
        int target=a->target.parameter==PARAM_CHANNEL_PITCH?0:a->target.parameter==PARAM_PITCH_RANGE?1:2;
        if(position>=end) {
            if(!active[target] && end>=held_end[target]) {
                double lo=target==0?-1:target==1?1:-12,range=target==0?2:target==1?47:24;
                values[target]=lo+range*automation_value(a,end-start+p->clip_offsets[l][b]); slopes[target]=0; held_end[target]=end;
            }
            continue;
        }
        active[target]=1;
        double local=position-start+p->clip_offsets[l][b],value=a->points[a->count-1].value,gradient=0;
        if(local<a->points[0].step) { value=a->points[0].value; next=fmin(next,start+a->points[0].step-p->clip_offsets[l][b]); }
        else for(int i=1;i<a->count;i++) if(local<a->points[i].step) {
            AutomationPoint x=a->points[i-1],y=a->points[i];
            gradient=(y.value-x.value)/(y.step-x.step); value=x.value+(local-x.step)*gradient;
            next=fmin(next,start+y.step-p->clip_offsets[l][b]); break;
        }
        double lo=target==0?-1:target==1?1:-12,range=target==0?2:target==1?47:24;
        values[target]=lo+value*range; slopes[target]=gradient*range;
    }
    /* Range is an integer control; split at its next rounding boundary. */
    double range=floor(values[1]+.5+ (slopes[1]<0?-1e-9:slopes[1]>0?1e-9:0));
    if(slopes[1]) {
        double boundary=range+(slopes[1]>0?.5:-.5),delta=(boundary-values[1])/slopes[1];
        if(delta>1e-10) next=fmin(next,position+delta);
    }
    *speed=exp2((values[0]*range+values[2])/12);
    *slope=(slopes[0]*range+slopes[2])*log(2)/12;
    return next;
}
static double integral(double speed,double slope,double duration) {
    return fabs(slope)<1e-12?speed*duration:speed*expm1(slope*duration)/slope;
}
double audio_timeline_source(const AudioTimeline *map,double start,double end) {
    double consumed=0;
    while(start<end) {
        double speed,slope,next=segment(map,start,&speed,&slope),finish=fmin(end,next);
        if(finish<=start) finish=nextafter(start,INFINITY);
        consumed+=integral(speed,slope,finish-start); start=finish;
    }
    return consumed;
}
double audio_timeline_duration(const AudioTimeline *map,double start,double source_steps) {
    double origin=start;
    while(source_steps>0) {
        double speed,slope,next=segment(map,start,&speed,&slope);
        if(!isfinite(next)) return start-origin+source_steps/speed;
        if(next<=start) next=nextafter(start,INFINITY);
        double consumed=integral(speed,slope,next-start);
        if(source_steps<=consumed) return start-origin+(fabs(slope)<1e-12?source_steps/speed:log1p(source_steps*slope/speed)/slope);
        source_steps-=consumed; start=next;
    }
    return start-origin;
}
float audio_clip_steps(const Project *p,int lane,int clip) {
    int c=p->clips[lane][clip]-PATTERNS-1;
    if(p->clip_steps[lane][clip]) return p->clip_steps[lane][clip]*p->bpm/15;
    double remaining=fmax(0,p->audio_seconds[c]-p->clip_offsets[lane][clip])*p->bpm/15;
    AudioTimeline map; audio_timeline_init(&map,p,c);
    return audio_timeline_duration(&map,p->clip_starts[lane][clip]*STEPS,remaining);
}
double audio_clip_position(const Project *p,int lane,int clip,double step) {
    int c=p->clips[lane][clip]-PATTERNS-1;
    double start=p->clip_starts[lane][clip]*STEPS;
    AudioTimeline map; audio_timeline_init(&map,p,c);
    return (p->clip_offsets[lane][clip]+audio_timeline_source(&map,start,fmax(start,step))*15/p->bpm)*RATE;
}
