// SPDX-License-Identifier: GPL-3.0-only
#include "fm_synth.h"
#include "sample.h"
#include <math.h>
enum { OFF, ATTACK, DECAY, SUSTAIN, RELEASE };
FMSettings fm_default(void) { return (FMSettings){2,2,.005f,.3f,.65f,.2f}; }
int fm_valid(FMSettings s) {
    return isfinite(s.ratio) && s.ratio>=1 && s.ratio<=8 && s.ratio==roundf(s.ratio) &&
        isfinite(s.depth) && s.depth>=0 && s.depth<=8 && isfinite(s.attack) && s.attack>=.001f && s.attack<=2 &&
        isfinite(s.decay) && s.decay>=.01f && s.decay<=3 && isfinite(s.sustain) && s.sustain>=0 && s.sustain<=1 &&
        isfinite(s.release) && s.release>=.01f && s.release<=3;
}
void fm_note_on(FMVoice *v,double frequency,FMSettings s) {
    *v=(FMVoice){.frequency=frequency,.current_frequency=frequency,.ratio=s.ratio,.depth=s.depth,.stage=ATTACK};
}
void fm_note_off(FMVoice *v,FMSettings s) {
    if(v->stage && v->stage!=RELEASE) { v->release_step=v->envelope/(s.release*RATE); v->stage=RELEASE; }
}
int fm_active(const FMVoice *v) { return v->stage!=OFF; }
float fm_sample(FMVoice *v,FMSettings s,double pitch) {
    if(!v->stage) return 0;
    const float slew=1.f/(RATE*.02f);
    v->ratio+=(roundf(s.ratio)-v->ratio)*slew; v->depth+=(s.depth-v->depth)*slew;
    double target=fmax(1,fmin(RATE*.45,v->frequency*pitch));
    v->current_frequency+=(target-v->current_frequency)*slew;
    if(v->stage==ATTACK) { v->envelope+=1/(s.attack*RATE); if(v->envelope>=1) { v->envelope=1; v->stage=DECAY; } }
    else if(v->stage==DECAY) { v->envelope-=fmaxf(0,1-s.sustain)/(s.decay*RATE); if(v->envelope<=s.sustain) { v->envelope=s.sustain; v->stage=SUSTAIN; } }
    else if(v->stage==SUSTAIN) v->envelope+=(s.sustain-v->envelope)*slew;
    else { v->envelope-=v->release_step; if(v->envelope<=.000001f) { v->envelope=0; v->stage=OFF; return 0; } }
    /* Four substeps reduce aliasing. Reduce modulation at high notes where
       sidebands would exceed the output band; this is not an alias-free claim. */
    double frequency=v->current_frequency,ratio=fmin(v->ratio,RATE*.45/frequency);
    double depth=fmin(v->depth,fmax(0,(RATE*.45/frequency-1)/ratio));
    const double turn=6.283185307179586; float sum=0;
    for(int i=0;i<4;i++) {
        sum+=sinf(v->carrier+depth*sinf(v->modulator));
        v->carrier+=turn*frequency/(RATE*4); v->modulator+=turn*frequency*ratio/(RATE*4);
        if(v->carrier>=turn) v->carrier-=turn;
        if(v->modulator>=turn) v->modulator-=turn;
    }
    return sum*.05f*v->envelope; /* .2 peak per voice, before velocity/channel gain. */
}
