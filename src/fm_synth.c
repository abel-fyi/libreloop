// SPDX-License-Identifier: GPL-3.0-only
#include "fm_synth.h"
#include "sample.h"
#include <math.h>
enum { OFF, ATTACK, DECAY, SUSTAIN, RELEASE };
FMSettings fm_default(void) { return (FMSettings){2,2,.005f,.3f,.65f,.2f,.5f,1,0,5,0,0}; }
FMSettings fm_epiano(void) { return (FMSettings){1,3.8f,.002f,2.5f,.08f,.45f,.85f,.08f,.8f,4.8f,3,.12f}; }
int fm_valid(FMSettings s) {
    return isfinite(s.ratio) && s.ratio>=1 && s.ratio<=8 && s.ratio==roundf(s.ratio) &&
        isfinite(s.depth) && s.depth>=0 && s.depth<=8 && isfinite(s.attack) && s.attack>=.001f && s.attack<=2 &&
        isfinite(s.decay) && s.decay>=.01f && s.decay<=3 && isfinite(s.sustain) && s.sustain>=0 && s.sustain<=1 &&
        isfinite(s.release) && s.release>=.01f && s.release<=3 &&
        isfinite(s.mod_decay) && s.mod_decay>=.01f && s.mod_decay<=5 && isfinite(s.mod_sustain) && s.mod_sustain>=0 && s.mod_sustain<=1 &&
        isfinite(s.velocity) && s.velocity>=0 && s.velocity<=1 && isfinite(s.lfo_rate) && s.lfo_rate>=.1f && s.lfo_rate<=12 &&
        isfinite(s.vibrato) && s.vibrato>=0 && s.vibrato<=100 && isfinite(s.tremolo) && s.tremolo>=0 && s.tremolo<=1;
}
void fm_note_on_velocity(FMVoice *v,double frequency,FMSettings s,float velocity) {
    *v=(FMVoice){.frequency=frequency,.current_frequency=frequency,.ratio=s.ratio,.depth=s.depth,.stage=ATTACK,
        .mod_envelope=1,.velocity=fmaxf(0,fminf(1,velocity)),.lfo_rate=s.lfo_rate,.vibrato=s.vibrato,.tremolo=s.tremolo};
}
void fm_note_on(FMVoice *v,double frequency,FMSettings s) { fm_note_on_velocity(v,frequency,s,1); }
void fm_note_off(FMVoice *v,FMSettings s) {
    if(v->stage && v->stage!=RELEASE) { v->release_step=v->envelope/(s.release*RATE); v->stage=RELEASE; }
}
int fm_active(const FMVoice *v) { return v->stage!=OFF; }
float fm_sample(FMVoice *v,FMSettings s,double pitch) {
    if(!v->stage) return 0;
    const float slew=1.f/(RATE*.02f);
    v->ratio+=(roundf(s.ratio)-v->ratio)*slew; v->depth+=(s.depth-v->depth)*slew;
    v->lfo_rate+=(s.lfo_rate-v->lfo_rate)*slew; v->vibrato+=(s.vibrato-v->vibrato)*slew; v->tremolo+=(s.tremolo-v->tremolo)*slew;
    v->mod_envelope+=(s.mod_sustain-v->mod_envelope)*fminf(1,4.6051702f/(s.mod_decay*RATE));
    double lfo=v->vibrato?sin(v->lfo_phase):0;
    double target=fmax(1,fmin(RATE*.45,v->frequency*pitch));
    v->current_frequency+=(target-v->current_frequency)*slew;
    if(v->stage==ATTACK) { v->envelope+=1/(s.attack*RATE); if(v->envelope>=1) { v->envelope=1; v->stage=DECAY; } }
    else if(v->stage==DECAY) { v->envelope-=fmaxf(0,1-s.sustain)/(s.decay*RATE); if(v->envelope<=s.sustain) { v->envelope=s.sustain; v->stage=SUSTAIN; } }
    else if(v->stage==SUSTAIN) v->envelope+=(s.sustain-v->envelope)*slew;
    else { v->envelope-=v->release_step; if(v->envelope<=.000001f) { v->envelope=0; v->stage=OFF; return 0; } }
    /* Four substeps reduce aliasing. Reduce modulation at high notes where
       sidebands would exceed the output band; this is not an alias-free claim. */
    double frequency=fmin(RATE*.45,v->current_frequency*(v->vibrato?pow(2,lfo*v->vibrato/1200):1)),ratio=fmin(v->ratio,RATE*.45/frequency);
    double depth=fmin(v->depth*v->mod_envelope*(1-s.velocity*(1-v->velocity)),fmax(0,(RATE*.45/frequency-1)/ratio));
    const double turn=6.283185307179586; float sum=0;
    for(int i=0;i<4;i++) {
        sum+=sinf(v->carrier+depth*sinf(v->modulator));
        v->carrier+=turn*frequency/(RATE*4); v->modulator+=turn*frequency*ratio/(RATE*4);
        if(v->carrier>=turn) v->carrier-=turn;
        if(v->modulator>=turn) v->modulator-=turn;
    }
    float amplitude=v->tremolo?1-v->tremolo*(.5-.5*cos(v->lfo_phase)):1;
    v->lfo_phase+=turn*v->lfo_rate/RATE; if(v->lfo_phase>=turn) v->lfo_phase-=turn;
    return sum*.05f*v->envelope*amplitude; /* .2 peak per voice, before velocity/channel gain. */
}
