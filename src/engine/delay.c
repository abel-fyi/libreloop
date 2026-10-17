// SPDX-License-Identifier: GPL-3.0-only
#include "delay.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#define DELAY_RATE 48000
#define DELAY_FRAMES (DELAY_RATE*4+2)
struct Delay {
    float pcm[DELAY_FRAMES][2];
    unsigned cursor,valid,tick;
    float feedback,cutoff,stereo,output,wet,filter[2],coefficient;
    double old_time,new_time,transition;
    int ready;
};
static const float beats[]={.125f,1.f/6,.25f,1.f/3,.5f,.75f,1,1.5f,2};
static const char *names[]={"1/32","1/16 T","1/16","1/8 T","1/8","1/8 D","1/4","1/4 D","1/2"};
DelaySettings delay_default(void) {return (DelaySettings){375,.35f,8000,0,1,1,5};}
int delay_valid(DelaySettings s) {
    return isfinite(s.time_ms) && s.time_ms>=1 && s.time_ms<=DELAY_MAX_MS &&
        isfinite(s.feedback) && s.feedback>=0 && s.feedback<=.95f &&
        isfinite(s.cutoff) && s.cutoff>=100 && s.cutoff<=20000 &&
        isfinite(s.stereo) && s.stereo>=0 && s.stereo<=1 &&
        isfinite(s.output) && s.output>=0 && s.output<=2 && s.sync<=1 && s.division<DELAY_DIVISIONS;
}
const char *delay_division_name(unsigned division) {return division<DELAY_DIVISIONS?names[division]:"?";}
float delay_time_ms(DelaySettings s,float bpm) {
    if(!s.sync)return fmaxf(1,fminf(DELAY_MAX_MS,s.time_ms));
    if(!isfinite(bpm) || bpm<=0)bpm=120;
    return fmaxf(1,fminf(DELAY_MAX_MS,60000.f/bpm*beats[s.division<DELAY_DIVISIONS?s.division:4]));
}
size_t delay_storage_bytes(void) {return sizeof(Delay);}
Delay *delay_create(void) {
    Delay *delay=malloc(sizeof *delay);
    if(delay) {
        /* Touch storage on its owning/control thread, before audio publication. */
        volatile unsigned char *bytes=(volatile unsigned char *)delay;
        for(size_t i=0;i<sizeof *delay;i+=4096)bytes[i]=0;
        delay_reset(delay);
    }
    return delay;
}
void delay_free(Delay *delay) {free(delay);}
void delay_reset(Delay *delay) {
    if(delay)memset((char *)delay+sizeof delay->pcm,0,sizeof *delay-sizeof delay->pcm);
}
static float read_tap(const Delay *d,double frames,unsigned side) {
    if(frames>d->valid)return 0;
    double position=d->cursor-frames;
    if(position<0)position+=DELAY_FRAMES;
    unsigned first=(unsigned)position,next=first+1==DELAY_FRAMES?0:first+1;
    float a=d->pcm[first][side];
    /* next may be the unwritten cursor at a fresh reset for a one-sample tap. */
    float b=next==d->cursor?0:d->pcm[next][side];
    return a+(b-a)*(float)(position-first);
}
void delay_process(Delay *d,DelaySettings s,float bpm,float wet,float stereo[2]) {
    if(!d)return;
    double time=delay_time_ms(s,bpm)*(DELAY_RATE/1000.0);
    float target_wet=fmaxf(0,fminf(1,wet));
    if(!d->ready) {
        d->feedback=s.feedback;d->cutoff=s.cutoff;d->stereo=s.stereo;d->output=s.output;d->wet=target_wet;
        d->old_time=d->new_time=time;d->transition=1;d->ready=1;
    }
    const float slew=1.f/(DELAY_RATE*.02f);
    d->feedback+=(s.feedback-d->feedback)*slew;d->cutoff+=(s.cutoff-d->cutoff)*slew;
    d->stereo+=(s.stereo-d->stereo)*slew;d->output+=(s.output-d->output)*slew;d->wet+=(target_wet-d->wet)*slew;
    if(!(d->tick++%32))d->coefficient=d->cutoff>=19999?1:1-expf(-6.283185307179586f*d->cutoff/DELAY_RATE);
    /* Crossfade fixed taps instead of continuously changing read speed. */
    if(d->transition>=1 && fabs(time-d->new_time)>=.01) {
        d->old_time=d->new_time;d->new_time=time;d->transition=0;
    }
    float delayed[2];
    for(unsigned side=0;side<2;side++) {
        float current=read_tap(d,d->new_time,side);
        float previous=d->transition<1?read_tap(d,d->old_time,side):current;
        float sample=previous+(current-previous)*(float)d->transition;
        d->filter[side]+=(sample-d->filter[side])*d->coefficient;
        if(fabsf(d->filter[side])<1e-20f)d->filter[side]=0;
        delayed[side]=d->filter[side];
    }
    for(unsigned side=0;side<2;side++) {
        float feedback=delayed[side]+(delayed[1-side]-delayed[side])*d->stereo;
        d->pcm[d->cursor][side]=stereo[side]+feedback*d->feedback;
        stereo[side]=stereo[side]*(1-d->wet)+delayed[side]*d->wet*d->output;
    }
    if(++d->cursor==DELAY_FRAMES)d->cursor=0;
    if(d->valid<DELAY_FRAMES)d->valid++;
    if(d->transition<1)d->transition=fmin(1,d->transition+slew);
}

int delay_audible(const Delay *d) {return d && d->ready && d->wet>.00001f;}
