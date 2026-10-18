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
    if(s.model!=0 && s.model!=1)return 0;
    if(s.model) {
        for(int i=0;i<3;i++)if(!isfinite(s.osc_wave[i]) || s.osc_wave[i]<0 || s.osc_wave[i]>3 ||
            !isfinite(s.osc_tune[i]) || fabsf(s.osc_tune[i])>24 || !isfinite(s.osc_fine[i]) || fabsf(s.osc_fine[i])>100 ||
            !isfinite(s.osc_level[i]) || s.osc_level[i]<0 || s.osc_level[i]>1)return 0;
        if(!isfinite(s.drive) || s.drive<0 || s.drive>24 || !isfinite(s.drift) || s.drift<0 || s.drift>1 ||
            !isfinite(s.keytrack) || s.keytrack<0 || s.keytrack>1)return 0;
    }
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
static float analog_legacy_sample(AnalogVoice *v,AnalogSettings s,double hz,float lfo,float env) {
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


AnalogSettings analog_modern_default(void) {
    AnalogSettings s=analog_default();s.model=1;s.chorus=0;s.cutoff=1600;s.resonance=.18f;s.filter_env=2.5f;
    s.osc_wave[0]=s.osc_wave[1]=0;s.osc_wave[2]=2;
    s.osc_level[0]=.65f;s.osc_level[1]=.35f;s.osc_level[2]=.25f;
    s.osc_tune[2]=-12;s.osc_fine[1]=7;s.osc_fine[2]=-4;
    s.drive=6;s.drift=.15f;s.keytrack=.5f;return s;
}
/* Original implementation of a nonlinear trapezoidal ladder. Four simultaneous
   stage equations include the resonance feedback in the current sub-sample.
   Three bounded Newton steps solve the cyclic bidiagonal Jacobian. No delayed
   resonance approximation, heap work, unbounded convergence loop or locks. */
static float transistor(float x,float *derivative) {
    if(x<=-3 || x>=3){*derivative=0;return x<0?-1:1;}
    float square=x*x,den=27+9*square;
    *derivative=9*(square-9)*(square-9)/(den*den);
    return x*(27+square)/den;
}
static float ladder(AnalogVoice *v,float input,float g,float resonance) {
    float y[4];memcpy(y,v->ladder_y,sizeof y);
    for(int iteration=0;iteration<3;iteration++) {
        float t[4],d[4],a[4],b[4],f[4],prop[4]={1},offset[4]={0},din;
        float first=transistor(input-resonance*y[3],&din);
        for(int i=0;i<4;i++)t[i]=transistor(y[i],&d[i]);
        for(int i=0;i<4;i++) {
            a[i]=1+g*d[i];b[i]=i?-g*d[i-1]:0;
            f[i]=y[i]-v->filter[i]-g*((i?t[i-1]:first)-t[i]);
            if(i){prop[i]=-b[i]*prop[i-1]/a[i];offset[i]=(-f[i]-b[i]*offset[i-1])/a[i];}
        }
        float c=g*resonance*din,delta=(-f[0]-c*offset[3])/(a[0]+c*prop[3]);
        for(int i=0;i<4;i++)y[i]+=prop[i]*delta+offset[i];
    }
    for(int i=0;i<4;i++){v->filter[i]=2*y[i]-v->filter[i];v->ladder_y[i]=y[i];if(fabsf(v->filter[i])<1e-20f)v->filter[i]=0;}
    return y[3];
}
/* Two 19-tap half-band decimators: 192 -> 96 -> 48 kHz. Coefficients are
   normalized Hamming-windowed sinc, generated from sin(pi*m/2)/(pi*m). */
static float decimate(AnalogVoice *v,int stage,float input,int consume) {
    static const float h[19]={0.002821689321f,0,-0.008508280148f,0,0.02921221974f,0,-0.08147627915f,0,0.308633819f,0.4986336624f,0.308633819f,0,-0.08147627915f,0,0.02921221974f,0,-0.008508280148f,0,0.002821689321f};
    unsigned at=v->fir_at[stage];v->fir[stage][at]=input;float sum=0;
    if(consume)for(unsigned i=0;i<19;i++){sum+=h[i]*v->fir[stage][(at+19-i)%19];}
    v->fir_at[stage]=(at+1)%19;return sum;
}
static unsigned random_next(AnalogVoice *v){unsigned x=v->random;x^=x<<13;x^=x>>17;x^=x<<5;return v->random=x;}
static float analog_modern_sample(AnalogVoice *v,AnalogSettings s,double hz,float lfo,float env) {
    if(!v->ready){
        v->current=s;v->ready=1;for(int i=0;i<3;i++)v->phase[i]=(random_next(v)>>8)/16777216.;
        /* A deterministic thermal-scale perturbation lets the unstable ladder
           equilibrium start self-oscillation even with its mixer muted. */
        v->filter[0]=1e-8f;
    }
    const float slew=1.f/(RATE*.02f);
#define MODERN_SLEW(field) v->current.field+=(s.field-v->current.field)*slew
    for(int i=0;i<3;i++){MODERN_SLEW(osc_wave[i]);MODERN_SLEW(osc_tune[i]);MODERN_SLEW(osc_fine[i]);MODERN_SLEW(osc_level[i]);}
    MODERN_SLEW(pulse);MODERN_SLEW(pwm);MODERN_SLEW(noise);MODERN_SLEW(cutoff);MODERN_SLEW(resonance);
    MODERN_SLEW(filter_env);MODERN_SLEW(drive);MODERN_SLEW(drift);MODERN_SLEW(keytrack);
#undef MODERN_SLEW
    s=v->current;
    if(!v->tick)for(int i=0;i<3;i++)v->drift_target[i]=((random_next(v)>>8)/8388608.f-1)*8;
    v->tick=(v->tick+1)%64;
    double step[3];for(int i=0;i<3;i++) {
        v->drift_value[i]+=(v->drift_target[i]-v->drift_value[i])/(RATE*.3f);
        step[i]=fmin(.45,hz*exp2(s.osc_tune[i]/12+(s.osc_fine[i]+v->drift_value[i]*s.drift)/1200)/(RATE*4));
    }
    float cutoff=fmaxf(20,fminf(20000,s.cutoff*exp2f(s.filter_env*env)*powf(fmaxf(1,hz)/261.625565f,s.keytrack)));
    /* Coefficients follow both envelopes and performance pitch every sample. */
    float g=tanf(3.14159265358979323846f*cutoff/(RATE*4)),drive=exp2f(s.drive/6.020599913f);
    float pulse=fmaxf(.05f,fminf(.95f,s.pulse+s.pwm*lfo)),out=0;
    for(int sub=0;sub<4;sub++) {
        float x=0;for(int i=0;i<3;i++){x+=s.osc_level[i]*morph(v->phase[i],step[i],s.osc_wave[i],pulse);v->phase[i]+=step[i];if(v->phase[i]>=1)v->phase[i]-=1;}
        x+=s.noise*((random_next(v)>>8)/8388608.f-1);
        float y=ladder(v,x*drive,g,s.resonance*(4.2f/.95f));
        /* Write every oversample; consume each decimator at half its input rate. */
        float half=decimate(v,0,y,sub&1);
        if(sub&1){float quarter=decimate(v,1,half,sub==3);if(sub==3)out=quarter;}
    }
    float output=out-v->dc_input+.9993457f*v->dc_output;v->dc_input=out;v->dc_output=output;
    return output;
}
float analog_sample(AnalogVoice *v,AnalogSettings s,double hz,float lfo,float env) {
    return s.model?analog_modern_sample(v,s,hz,lfo,env):analog_legacy_sample(v,s,hz,lfo,env);
}
