// SPDX-License-Identifier: GPL-3.0-only
#include "analog.h"
#include "sample.h"
#include <math.h>
#include <string.h>
AnalogSettings analog_default(void) { return (AnalogSettings){0,8,.65f,.2f,0,.5f,.15f,5000,.2f,1,.35f}; }
int analog_valid(AnalogSettings s) {
    const float values[]={s.wave,s.detune,s.mix,s.sub,s.noise,s.pulse,s.pwm,s.cutoff,s.resonance,s.filter_env,s.chorus};
    const float low[]={0,0,0,0,0,.05f,0,20,0,-6,0},high[]={3,50,1,1,1,.95f,.45f,20000,.95f,6,1};
    for(int i=0;i<11;i++) if(!isfinite(values[i]) || values[i]<low[i] || values[i]>high[i]) return 0;
    return s.wave==roundf(s.wave);
}
void analog_note_on(AnalogVoice *v,double hz) { memset(v,0,sizeof *v); v->random=0x9e3779b9u^(unsigned)(hz*1000); }
static float blep(double phase,double step) {
    if(phase<step) { double t=phase/step; return t+t-t*t-1; }
    if(phase>1-step) { double t=(phase-1)/step; return t*t+t+t+1; }
    return 0;
}
static float oscillator(double phase,double step,int wave,float pulse) {
    if(wave==0) return 2*phase-1-blep(phase,step);
    if(wave==1) { double offset=phase-pulse; if(offset<0) offset+=1; return (phase<pulse?1:-1)+blep(phase,step)-blep(offset,step); }
    if(wave==2) return 1-4*fabs(phase-.5);
    return sin(6.283185307179586*phase);
}
static float morph(double phase,double step,float wave,float pulse) {
    int low=(int)floorf(wave),high=(int)ceilf(wave); float mix=wave-low;
    float a=oscillator(phase,step,low,pulse);
    return low==high?a:a+(oscillator(phase,step,high,pulse)-a)*mix;
}
static float saturate(float x) { x=fmaxf(-3,fminf(3,x)); return x*(27+x*x)/(27+9*x*x); }
float analog_sample(AnalogVoice *v,AnalogSettings s,double hz,float lfo,float env) {
    if(!v->ready) { v->current=s; v->ready=1; }
    const float slew=1.f/(RATE*.02f);
#define SLEW(field) v->current.field+=(s.field-v->current.field)*slew
    SLEW(wave); SLEW(detune); SLEW(mix); SLEW(sub); SLEW(noise); SLEW(pulse); SLEW(pwm); SLEW(cutoff); SLEW(resonance); SLEW(filter_env); SLEW(chorus);
#undef SLEW
    s=v->current;
    double ratio=exp2(s.detune/2400.),step[3]={fmin(.45,hz/ratio/RATE),fmin(.45,hz*ratio/RATE),fmin(.45,hz*.5/RATE)};
    float pulse=fmaxf(.05f,fminf(.95f,s.pulse+s.pwm*lfo));
    float x=morph(v->phase[0],step[0],s.wave,pulse)+s.mix*morph(v->phase[1],step[1],s.wave,pulse);
    x+=s.sub*oscillator(v->phase[2],step[2],1,.5f);
    v->random^=v->random<<13; v->random^=v->random>>17; v->random^=v->random<<5;
    x+=s.noise*((v->random>>8)/8388608.f-1); x/=1+s.mix+s.sub+s.noise;
    for(int i=0;i<3;i++) { v->phase[i]+=step[i]; if(v->phase[i]>=1) v->phase[i]-=1; }
    if(!v->tick) {
        float cutoff=fmaxf(20,fminf(20000,s.cutoff*exp2f(s.filter_env*env)));
        if(!v->cutoff) v->cutoff=cutoff;
        v->cutoff+=(cutoff-v->cutoff)*(1-expf(-16.f/(RATE*.02f)));
        float g=tanf(3.14159265358979323846f*v->cutoff/RATE); v->g=g/(1+g);
    }
    v->tick=(v->tick+1)%16;
    float g=v->g,g2=g*g,k=s.resonance*4,feedback=(1-g)*(g*g2*v->filter[0]+g2*v->filter[1]+g*v->filter[2]+v->filter[3]);
    float y=saturate((x-k*feedback)/(1+k*g2*g2));
    for(int i=0;i<4;i++) { float a=g*(y-v->filter[i]); y=a+v->filter[i]; v->filter[i]=y+a; if(fabsf(v->filter[i])<1e-20f) v->filter[i]=0; }
    y*=1+s.resonance*1.4f;
    float output=y-v->dc_input+.9993457f*v->dc_output; v->dc_input=y; v->dc_output=output;
    return output;
}
