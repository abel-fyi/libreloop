// SPDX-License-Identifier: GPL-3.0-only
/* Original ADAA implementation; mathematical provenance and voicing: docs/tube.md. */
#include "tube.h"
#include <math.h>
#include <string.h>
#define TUBE_RATE 48000.f
TubeSettings tube_default(void) {return (TubeSettings){12,6500,-3};}
int tube_valid(TubeSettings s) {
    return isfinite(s.drive) && s.drive>=0 && s.drive<=36 &&
        isfinite(s.tone) && s.tone>=1200 && s.tone<=18000 &&
        isfinite(s.output) && s.output>=-24 && s.output<=12;
}
void tube_reset(Tube *t) {if(t)memset(t,0,sizeof *t);}
static double log_cosh(double x) {
    double a=fabs(x);return a+log1p(exp(-2*a))-.69314718055994530942;
}
static double stage_sample(TubeStage *state,double x,double bias,double zero,double normalize) {
    /* F'(x) = (tanh(x+bias)-tanh(bias)) / (1-tanh(bias)^2).
       Subtracting F(0) also keeps silence exactly zero after reset. */
    if(x==0 && state->x==0){state->integral=0;return 0;}
    double integral=(log_cosh(x+bias)-log_cosh(bias)-x*zero)*normalize;
    double delta=x-state->x;
    double y=fabs(delta)>1e-5?(integral-state->integral)/delta:
        (tanh((x+state->x)*.5+bias)-zero)*normalize;
    state->x=x;state->integral=integral;return y;
}
static float flush(float x) {return fabsf(x)<1e-20f?0:x;}
void tube_process(Tube *t,TubeSettings s,float wet,float stereo[2]) {
    if(!t)return;
    wet=fmaxf(0,fminf(1,wet));
    if(!t->ready) {
        t->drive=s.drive;t->tone=s.tone;t->output=s.output;t->wet=wet;t->ready=1;
        t->gain=powf(10,s.drive/20);t->makeup=powf(10,(s.output-s.drive*.5f)/20);
        t->tone_coefficient=1-expf(-6.283185307179586f*s.tone/TUBE_RATE);
    }
    const float slew=1.f/(TUBE_RATE*.02f);
    t->drive+=(s.drive-t->drive)*slew;t->tone+=(s.tone-t->tone)*slew;
    t->output+=(s.output-t->output)*slew;t->wet+=(wet-t->wet)*slew;
    if(!(t->tick++%32)) {
        t->gain_step=(powf(10,t->drive/20)-t->gain)/32;
        /* Moderate drive compensation; Output remains an independent trim. */
        t->makeup_step=(powf(10,(t->output-t->drive*.5f)/20)-t->makeup)/32;
        t->tone_step=(1-expf(-6.283185307179586f*t->tone/TUBE_RATE)-t->tone_coefficient)/32;
    }
    /* Interpolate cached coefficients; automation never jumps every 32 samples. */
    t->gain+=t->gain_step;t->makeup+=t->makeup_step;t->tone_coefficient+=t->tone_step;
    for(unsigned side=0;side<2;side++) {
        float input=stereo[side];
        t->pre[side]=flush(t->pre[side]+.90521985f*(input-t->pre[side])); /* 18 kHz */
        double driven=t->pre[side]*t->gain/(1+.12f*t->envelope[side]);
        float first=(float)stage_sample(&t->stage[0][side],driven,.2,.197375320224904,1.0405361859192275);
        float level=fabsf(first),rate=level>t->envelope[side]?1.f/960:1.f/7200;
        t->envelope[side]=flush(t->envelope[side]+(level-t->envelope[side])*rate);
        t->inter[side]=flush(t->inter[side]+.79212042f*(first-t->inter[side])); /* 12 kHz */
        float second=(float)stage_sample(&t->stage[1][side],t->inter[side]*1.15,.08,.07982976911113136,1.0064136649895055);
        t->tone_state[side]=flush(t->tone_state[side]+(second-t->tone_state[side])*t->tone_coefficient);
        float blocked=t->tone_state[side]-t->dc_input[side]+.99790780f*t->dc_output[side]; /* 16 Hz */
        t->dc_input[side]=t->tone_state[side];t->dc_output[side]=flush(blocked);
        stereo[side]=input*(1-t->wet)+t->dc_output[side]*t->makeup*t->wet;
    }
}
