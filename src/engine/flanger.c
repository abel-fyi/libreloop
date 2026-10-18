// SPDX-License-Identifier: GPL-3.0-only
#include "flanger.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#define PI 3.14159265358979323846
#define RATE 48000.f
#define FRAMES 2048
struct Flanger {float buffer[FRAMES][2];double phase;float rate,delay,depth,feedback,stereo,polarity,wet,dc_input[2],dc_output[2];unsigned cursor,valid,ready;};
static float clamp(float x,float lo,float hi){return fmaxf(lo,fminf(hi,x));}
static float flush(float x){return fabsf(x)<1e-20f?0:x;}
FlangerSettings flanger_default(void){return (FlangerSettings){.25f,2.5f,.8f,.55f,.25f,0};}
FlangerSettings flanger_factory(unsigned i){switch(i){case 1:return (FlangerSettings){.08f,5,1,.75f,.65f,0};case 2:return (FlangerSettings){.4f,1.2f,.7f,-.6f,0,1};default:return flanger_default();}}
const char *flanger_factory_name(unsigned i){static const char *names[]={"Classic Jet","Slow Sweep","Hollow Flange"};return names[i<3?i:0];}
const float *flanger_parameter(const FlangerSettings *s,unsigned i){switch(i){case 0:return &s->rate;case 1:return &s->delay;case 2:return &s->depth;case 3:return &s->feedback;case 4:return &s->stereo;case 5:return &s->invert;default:return NULL;}}
int flanger_valid(FlangerSettings s){for(unsigned i=0;i<6;i++)if(!isfinite(*flanger_parameter(&s,i)))return 0;return s.rate>=.01f && s.rate<=10 && s.delay>=.2f && s.delay<=12 && s.depth>=0 && s.depth<=1 && s.feedback>=-.95f && s.feedback<=.95f && s.stereo>=0 && s.stereo<=1 && (s.invert==0 || s.invert==1);}
Flanger *flanger_create(void){Flanger *f=malloc(sizeof *f);if(f){volatile unsigned char *p=(volatile unsigned char *)f;for(size_t i=0;i<sizeof *f;i+=4096)p[i]=0;flanger_reset(f);}return f;}
void flanger_free(Flanger *f){free(f);}
void flanger_reset(Flanger *f){if(f)memset((char *)f+sizeof f->buffer,0,sizeof *f-sizeof f->buffer);}
int flanger_audible(const Flanger *f){return f && f->ready && f->wet>.00001f;}
size_t flanger_storage_bytes(void){return sizeof(Flanger);}
static float sample(const Flanger *f,unsigned index,int side){unsigned age=(f->cursor+FRAMES-index)%FRAMES;return !age || age>f->valid?0:f->buffer[index][side];}
static float read_delay(const Flanger *f,double delay,int side){double at=f->cursor-delay;if(at<0)at+=FRAMES;unsigned i=(unsigned)at;float t=at-i,a=sample(f,(i+FRAMES-1)%FRAMES,side),b=sample(f,i,side),c=sample(f,(i+1)%FRAMES,side),d=sample(f,(i+2)%FRAMES,side);return -a*t*(t-1)*(t-2)/6+b*(t+1)*(t-1)*(t-2)/2-c*(t+1)*t*(t-2)/2+d*(t+1)*t*(t-1)/6;}
void flanger_process(Flanger *f,FlangerSettings s,float wet,float stereo[2]){
    if(!f)return;
    if(!f->ready){f->rate=s.rate;f->delay=s.delay;f->depth=s.depth;f->feedback=s.feedback;f->stereo=s.stereo;f->polarity=s.invert?-1:1;f->wet=clamp(wet,0,1);f->ready=1;}
    const float slew=1.f/960;
    f->rate+=(s.rate-f->rate)*slew;f->delay+=(s.delay-f->delay)*slew;f->depth+=(s.depth-f->depth)*slew;f->feedback+=(s.feedback-f->feedback)*slew;f->stereo+=(s.stereo-f->stereo)*slew;f->polarity+=((s.invert?-1:1)-f->polarity)*slew;f->wet+=(clamp(wet,0,1)-f->wet)*slew;
    for(int c=0;c<2;c++){
        float input=stereo[c];double ms=f->delay+f->depth*(f->delay-.05f)*cos(f->phase+(c?1:-1)*f->stereo*PI*.5);
        float delayed=read_delay(f,ms*48,c);
        float feedback=flush(delayed-f->dc_input[c]+.99738543f*f->dc_output[c]);f->dc_input[c]=delayed;f->dc_output[c]=feedback;
        f->buffer[f->cursor][c]=flush(input*(1-fabsf(f->feedback))+feedback*f->feedback);
        stereo[c]=input*(1-f->wet)+delayed*f->polarity*f->wet;
    }
    f->cursor=(f->cursor+1)%FRAMES;if(f->valid<FRAMES)f->valid++;
    f->phase+=2*PI*f->rate/RATE;if(f->phase>=2*PI)f->phase-=2*PI;
}
