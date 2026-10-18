// SPDX-License-Identifier: GPL-3.0-only
#include "phaser.h"
#include <math.h>
#include <string.h>
#define PI 3.14159265358979323846
#define RATE 48000.f
static float clamp(float x,float lo,float hi){return fmaxf(lo,fminf(hi,x));}
static float flush(float x){return fabsf(x)<1e-20f?0:x;}
PhaserSettings phaser_default(void){return (PhaserSettings){.35f,900,2,.35f,.5f,6};}
PhaserSettings phaser_factory(unsigned i){switch(i){case 1:return (PhaserSettings){.18f,650,3,.65f,0,8};case 2:return (PhaserSettings){.7f,1400,1.5f,-.45f,1,12};default:return phaser_default();}}
const char *phaser_factory_name(unsigned i){static const char *names[]={"Warm Sweep","Deep Phase","Stereo Motion"};return names[i<3?i:0];}
const float *phaser_parameter(const PhaserSettings *s,unsigned i){switch(i){case 0:return &s->rate;case 1:return &s->center;case 2:return &s->depth;case 3:return &s->feedback;case 4:return &s->stereo;case 5:return &s->stages;default:return NULL;}}
int phaser_valid(PhaserSettings s){for(unsigned i=0;i<6;i++)if(!isfinite(*phaser_parameter(&s,i)))return 0;return s.rate>=.01f && s.rate<=10 && s.center>=80 && s.center<=8000 && s.depth>=0 && s.depth<=4 && s.feedback>=-.9f && s.feedback<=.9f && s.stereo>=0 && s.stereo<=1 && s.stages>=2 && s.stages<=12 && s.stages==floorf(s.stages);}
void phaser_reset(Phaser *p){memset(p,0,sizeof *p);}
void phaser_process(Phaser *p,PhaserSettings s,float wet,float stereo[2]){
    if(!p->ready){p->rate=s.rate;p->center=s.center;p->depth=s.depth;p->feedback=s.feedback;p->stereo=s.stereo;p->stages=s.stages;p->wet=clamp(wet,0,1);p->ready=1;for(int c=0;c<2;c++){float hz=clamp(s.center*exp2f(s.depth*cos((c?1:-1)*s.stereo*PI*.5)),20,18000);float t=tanf(PI*hz/RATE);p->coefficient[c]=(t-1)/(t+1);}}
    const float slew=1.f/960;
    p->rate+=(s.rate-p->rate)*slew;p->center+=(s.center-p->center)*slew;p->depth+=(s.depth-p->depth)*slew;p->feedback+=(s.feedback-p->feedback)*slew;p->stereo+=(s.stereo-p->stereo)*slew;p->stages+=(s.stages-p->stages)*slew;p->wet+=(clamp(wet,0,1)-p->wet)*slew;
    for(int c=0;c<2;c++){
        /* Symmetric LFO offsets start centered. Identical input stays identical
           at zero stereo spread. Allpass feedback has a one-sample loop delay. */
        float hz=clamp(p->center*exp2f(p->depth*cos(p->phase+(c?1:-1)*p->stereo*PI*.5)),20,18000),t=tanf(PI*hz/RATE),target=(t-1)/(t+1);
        p->coefficient[c]+=(target-p->coefficient[c])*(1.f/32);
        float input=stereo[c],x=input*(1-fabsf(p->feedback))+p->feedback*p->last[c],tap[13]={0};tap[0]=x;
        for(int i=0;i<12;i++){float y=p->coefficient[c]*x+p->state[c][i];p->state[c][i]=flush(x-p->coefficient[c]*y);x=y;tap[i+1]=x;}
        float n=clamp(p->stages,2,12);int at=(int)n;float effect=tap[at]+(tap[at<12?at+1:12]-tap[at])*(n-at);
        p->last[c]=flush(effect-p->dc_input[c]+.99738543f*p->dc_output[c]);p->dc_input[c]=effect;p->dc_output[c]=p->last[c];
        stereo[c]=input*(1-p->wet)+effect*p->wet;
    }
    p->phase+=2*PI*p->rate/RATE;if(p->phase>=2*PI)p->phase-=2*PI;
}
