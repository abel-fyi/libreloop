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
    float input_pan,input_gain,dry,swap;
    double old_time[2],new_time[2],transition;
    int ready;
};
static const float beats[]={.125f,1.f/6,.25f,1.f/3,.5f,.75f,1,1.5f,2};
static const char *names[]={"1/32","1/16 T","1/16","1/8 T","1/8","1/8 D","1/4","1/4 D","1/2"};
DelaySettings delay_legacy(void) {return (DelaySettings){.time_ms=375,.feedback=.35f,.cutoff=8000,.output=1,.sync=1,.division=5};}
DelaySettings delay_default(void) {DelaySettings s=delay_legacy();s.input_gain=.35f;s.steps=3;s.dry=1;s.model=1;return s;}
int delay_valid(DelaySettings s) {
    return isfinite(s.time_ms) && s.time_ms>=1 && s.time_ms<=DELAY_MAX_MS &&
        isfinite(s.feedback) && s.feedback>=0 && s.feedback<=1 &&
        isfinite(s.cutoff) && s.cutoff>=100 && s.cutoff<=20000 &&
        isfinite(s.stereo) && s.stereo>=0 && s.stereo<=1 &&
        isfinite(s.output) && s.output>=0 && s.output<=2 && s.sync<=1 && s.division<DELAY_DIVISIONS && s.model<=1 && s.mode<=DELAY_PINGPONG &&
        isfinite(s.input_pan) && s.input_pan>=-1 && s.input_pan<=1 &&
        isfinite(s.input_gain) && s.input_gain>=0 && s.input_gain<=2 &&
        isfinite(s.offset) && s.offset>=-1 && s.offset<=1 &&
        isfinite(s.dry) && s.dry>=0 && s.dry<=2 &&
        isfinite(s.steps) && (!s.model || (s.steps>=.125f && s.steps<=16));
}
const char *delay_division_name(unsigned division) {return division<DELAY_DIVISIONS?names[division]:"?";}
float delay_time_ms(DelaySettings s,float bpm) {
    if(s.model) {if(!isfinite(bpm) || bpm<=0)bpm=120;return fmaxf(1,fminf(DELAY_MAX_MS,15000.f/bpm*s.steps));}
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
    double base=delay_time_ms(s,bpm)*(DELAY_RATE/1000.0);
    double time[2]={base,base};
    /* Offset refers to the audible side, including swapped-output modes. */
    float offset=s.mode==DELAY_NORMAL?s.offset:-s.offset;
    if(s.model)for(unsigned side=0;side<2;side++)time[side]=fmin(DELAY_RATE*4,base*(1+.5f*fmaxf(0,side?offset:-offset)));
    float target_pan=s.model?s.input_pan:0,target_gain=s.model?s.input_gain:1;
    float target_dry=s.model?s.dry:0,target_swap=s.model && s.mode!=DELAY_NORMAL;
    float target_cross=s.model?(s.mode==DELAY_PINGPONG):s.stereo;
    float target_wet=fmaxf(0,fminf(1,wet));
    if(!d->ready) {
        d->feedback=s.feedback;d->cutoff=s.cutoff;d->stereo=target_cross;d->output=s.output;d->wet=target_wet;
        d->input_pan=target_pan;d->input_gain=target_gain;d->dry=target_dry;d->swap=target_swap;
        for(unsigned side=0;side<2;side++)d->old_time[side]=d->new_time[side]=time[side];
        d->transition=1;d->ready=1;
    }
    const float slew=1.f/(DELAY_RATE*.02f);
    d->feedback+=(s.feedback-d->feedback)*slew;d->cutoff+=(s.cutoff-d->cutoff)*slew;
    d->stereo+=(target_cross-d->stereo)*slew;d->output+=(s.output-d->output)*slew;d->wet+=(target_wet-d->wet)*slew;
    d->input_pan+=(target_pan-d->input_pan)*slew;d->input_gain+=(target_gain-d->input_gain)*slew;
    d->dry+=(target_dry-d->dry)*slew;d->swap+=(target_swap-d->swap)*slew;
    if(!(d->tick++%32))d->coefficient=d->cutoff>=19999?1:1-expf(-6.283185307179586f*d->cutoff/DELAY_RATE);
    /* Crossfade fixed taps instead of continuously changing read speed. */
    if(d->transition>=1 && (fabs(time[0]-d->new_time[0])>=.01 || fabs(time[1]-d->new_time[1])>=.01)) {
        for(unsigned side=0;side<2;side++){d->old_time[side]=d->new_time[side];d->new_time[side]=time[side];}
        d->transition=0;
    }
    float delayed[2];
    for(unsigned side=0;side<2;side++) {
        float current=read_tap(d,d->new_time[side],side);
        float previous=d->transition<1?read_tap(d,d->old_time[side],side):current;
        float sample=previous+(current-previous)*(float)d->transition;
        d->filter[side]+=(sample-d->filter[side])*d->coefficient;
        if(fabsf(d->filter[side])<1e-20f)d->filter[side]=0;
        delayed[side]=d->filter[side];
    }
    for(unsigned side=0;side<2;side++) {
        float feedback=delayed[side]+(delayed[1-side]-delayed[side])*d->stereo;
        float input=stereo[side]*d->input_gain*fminf(1,1+(side?d->input_pan:-d->input_pan));
        d->pcm[d->cursor][side]=input+feedback*d->feedback;
        float echo=delayed[side]+(delayed[1-side]-delayed[side])*d->swap;
        stereo[side]=stereo[side]*(1-d->wet+d->wet*d->dry)+echo*d->wet*d->output;
    }
    if(++d->cursor==DELAY_FRAMES)d->cursor=0;
    if(d->valid<DELAY_FRAMES)d->valid++;
    if(d->transition<1)d->transition=fmin(1,d->transition+slew);
}

int delay_audible(const Delay *d) {return d && d->ready && d->wet>.00001f;}
