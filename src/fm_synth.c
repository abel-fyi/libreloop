// SPDX-License-Identifier: GPL-3.0-only
#include "fm_synth.h"
#include "sample.h"
#include <math.h>
#include <stddef.h>
enum { OFF, ATTACK, DECAY, SUSTAIN, RELEASE };
FMSettings fm_legacy(void) {
    dx7_init();
    FMSettings s=(FMSettings){.ratio=2,.depth=2,.attack=.005f,.decay=.3f,.sustain=.65f,.release=.2f,
        .mod_decay=.5f,.mod_sustain=1,.lfo_rate=5,.carrier_ratio=1,.attack_ratio=14,
        .attack_attack=.001f,.attack_decay=.08f,.attack_release=.1f};
    s.dx7=dx7_factory(0); s.analog=analog_default(); return s;
}
FMSettings fm_epiano(void) {
    FMSettings s=fm_legacy(); s.ratio=1; s.depth=2.6f; s.attack=.0015f; s.decay=2.5f; s.sustain=.08f; s.release=.45f;
    s.mod_attack=.001f; s.mod_decay=1.2f; s.mod_sustain=.12f; s.mod_release=.4f;
    s.attack_depth=4.5f; s.attack_decay=.09f; s.attack_release=.07f; s.velocity=.8f; s.lfo_rate=4.8f; s.lfo_fade=.3f;
    return s;
}
FMSettings fm_factory(int preset) {
    FMSettings s=fm_epiano();
    if(preset==7 || preset==8) {
        s.engine=1; s.dx7=dx7_factory(0);
        s.dx7.value[16]=76; s.dx7.value[58]=80;
        s.dx7.value[85]=42; s.dx7.value[89]=84; s.dx7.value[100]=93;
        s.dx7.value[106]=53; s.dx7.value[121]=71; s.dx7.value[135]=7;
        if(preset==8) {
            /* User's betterchime body, with the tine cluster anchored near
               5.5 kHz. Its carrier still tracks the played note; the attack
               no longer sweeps octaves or changes decay speed with the key. */
            s.dx7.value[16]=s.dx7.value[58]=50;
            s.dx7.value[97]=0; s.dx7.value[99]=2; s.dx7.value[100]=75;
            s.dx7.value[101]=1; s.dx7.value[102]=3; s.dx7.value[103]=74;
            s.dx7.value[118]=0; s.dx7.value[121]=99;
            s.dx7.value[DX7_NATIVE_PARAMETERS+4]=25;
        }
        return s;
    }
    if(preset<5) { s.engine=1; s.dx7=dx7_factory(preset==4?-1:preset); return s; }
    s.engine=2; s.carrier_ratio=1; s.mod_attack=0; s.mod_release=.3f; s.vibrato=s.tremolo=0; s.lfo_rate=.5f;
    if(preset==5) { s.attack=.06f; s.decay=1; s.sustain=.8f; s.release=.8f; s.mod_decay=1.8f; s.mod_sustain=.2f; s.analog=(AnalogSettings){0,9,.75f,.15f,.01f,.5f,.2f,2400,.18f,1.3f,.45f}; }
    else { s.carrier_ratio=.25f; s.attack=.001f; s.decay=.3f; s.sustain=.35f; s.release=.12f; s.mod_decay=.16f; s.mod_sustain=0; s.analog=(AnalogSettings){0,1,.3f,.6f,0,.5f,0,250,.4f,4,0}; }
    return s;
}
const char *fm_factory_name(int preset) { return preset==8?"Better Chime":preset==7?"Chime EP":preset<5?dx7_factory_name(preset==4?-1:preset):preset==5?"Juno-inspired Pad":"Juno-inspired Bass"; }
FMSettings fm_default(void) { return fm_factory(0); }
const float *fm_parameter_pointer(const FMSettings *s,unsigned id) {
    if(id>=PARAM_DX7_FIRST && id<=PARAM_DX7_LAST) return &s->dx7.value[id-PARAM_DX7_FIRST];
    switch(id) {
#define FIELD(id,field) case id: return &s->field;
    FIELD(PARAM_FM_RATIO,ratio) FIELD(PARAM_FM_DEPTH,depth) FIELD(PARAM_FM_ATTACK,attack)
    FIELD(PARAM_FM_DECAY,decay) FIELD(PARAM_FM_SUSTAIN,sustain) FIELD(PARAM_FM_RELEASE,release)
    FIELD(PARAM_FM_MOD_DECAY,mod_decay) FIELD(PARAM_FM_MOD_SUSTAIN,mod_sustain)
    FIELD(PARAM_FM_VELOCITY,velocity) FIELD(PARAM_FM_LFO_RATE,lfo_rate)
    FIELD(PARAM_FM_VIBRATO,vibrato) FIELD(PARAM_FM_TREMOLO,tremolo)
    FIELD(PARAM_FM_CARRIER_RATIO,carrier_ratio) FIELD(PARAM_FM_CARRIER_DETUNE,carrier_detune)
    FIELD(PARAM_FM_BODY_DETUNE,body_detune) FIELD(PARAM_FM_MOD_ATTACK,mod_attack) FIELD(PARAM_FM_MOD_RELEASE,mod_release)
    FIELD(PARAM_FM_ATTACK_RATIO,attack_ratio) FIELD(PARAM_FM_ATTACK_DETUNE,attack_detune)
    FIELD(PARAM_FM_ATTACK_DEPTH,attack_depth) FIELD(PARAM_FM_ATTACK_ATTACK,attack_attack)
    FIELD(PARAM_FM_ATTACK_DECAY,attack_decay) FIELD(PARAM_FM_ATTACK_SUSTAIN,attack_sustain)
    FIELD(PARAM_FM_ATTACK_RELEASE,attack_release) FIELD(PARAM_FM_ROUTING,routing)
    FIELD(PARAM_FM_LFO_SHAPE,lfo_shape) FIELD(PARAM_FM_LFO_FADE,lfo_fade) FIELD(PARAM_FM_BODY_PITCH,body_pitch)
    FIELD(PARAM_FM_ENGINE,engine) FIELD(PARAM_FM_ANALOG_WAVE,analog.wave)
    FIELD(PARAM_FM_ANALOG_DETUNE,analog.detune) FIELD(PARAM_FM_ANALOG_MIX,analog.mix)
    FIELD(PARAM_FM_ANALOG_SUB,analog.sub) FIELD(PARAM_FM_ANALOG_NOISE,analog.noise)
    FIELD(PARAM_FM_ANALOG_PULSE,analog.pulse) FIELD(PARAM_FM_ANALOG_PWM,analog.pwm)
    FIELD(PARAM_FM_FILTER_CUTOFF,analog.cutoff) FIELD(PARAM_FM_FILTER_RESONANCE,analog.resonance)
    FIELD(PARAM_FM_FILTER_ENV,analog.filter_env) FIELD(PARAM_FM_ANALOG_CHORUS,analog.chorus)
#undef FIELD
    default: return NULL;
    }
}
static int range(float value,float low,float high) { return isfinite(value) && value>=low && value<=high; }
int fm_valid(FMSettings s) {
    return range(s.ratio,1,8) && s.ratio==roundf(s.ratio) && range(s.depth,0,8) && range(s.attack,.001f,2) && range(s.decay,.01f,3) &&
        range(s.sustain,0,1) && range(s.release,.01f,3) && range(s.mod_decay,.01f,5) && range(s.mod_sustain,0,1) &&
        range(s.velocity,0,1) && range(s.lfo_rate,.1f,12) && range(s.vibrato,0,100) && range(s.tremolo,0,1) &&
        range(s.carrier_ratio,.125f,32) && range(s.carrier_detune,-100,100) && range(s.body_detune,-100,100) &&
        range(s.mod_attack,0,2) && range(s.mod_release,0,5) && range(s.attack_ratio,.125f,32) &&
        range(s.attack_detune,-100,100) && range(s.attack_depth,0,12) && range(s.attack_attack,.001f,2) &&
        range(s.attack_decay,.005f,5) && range(s.attack_sustain,0,1) && range(s.attack_release,.01f,5) &&
        range(s.routing,0,1) && s.routing==roundf(s.routing) && range(s.lfo_shape,0,3) && s.lfo_shape==roundf(s.lfo_shape) && range(s.lfo_fade,0,5) && range(s.body_pitch,-48,48) && range(s.engine,0,2) && s.engine==roundf(s.engine) && dx7_valid(s.dx7) && analog_valid(s.analog);
}
float fm_lfo_value(float phase,int shape) {
    phase-=floorf(phase);
    if(shape==1) return 1-4*fabsf(phase-.5f);
    if(shape==2) return 2*phase-1;
    if(shape==3) return phase<.5f?1:-1;
    return sinf(phase*6.283185307179586f);
}
void fm_note_on_velocity(FMVoice *v,double frequency,FMSettings s,float velocity) {
    *v=(FMVoice){.frequency=frequency,.current_frequency=frequency,.ratio=s.ratio,.depth=s.depth,.stage=ATTACK,
        .mod_envelope=s.mod_attack?0:1,.body_stage=s.mod_attack?ATTACK:DECAY,.attack_stage=ATTACK,
        .velocity=fmaxf(0,fminf(1,velocity)),.lfo_rate=s.lfo_rate,.vibrato=s.vibrato,.tremolo=s.tremolo,
        .body_pitch=s.body_pitch,.carrier_ratio=s.carrier_ratio,.attack_ratio=s.attack_ratio,.attack_depth=s.attack_depth,
        .carrier_detune=s.carrier_detune,.body_detune=s.body_detune,.attack_detune=s.attack_detune,.engine=(int)s.engine};
    if(v->engine==1) dx7_note_on(&v->dx7,frequency,velocity,s.dx7);
    else if(v->engine==2) analog_note_on(&v->analog,frequency);
}
void fm_note_on(FMVoice *v,double frequency,FMSettings s) { fm_note_on_velocity(v,frequency,s,1); }
void fm_note_off(FMVoice *v,FMSettings s) {
    if(v->engine==1) { dx7_note_off(&v->dx7); return; }
    if(v->stage && v->stage!=RELEASE) {
        v->release_step=v->envelope/(s.release*RATE); v->stage=RELEASE;
        if(s.mod_release) { v->body_release_step=v->mod_envelope/(s.mod_release*RATE); v->body_stage=RELEASE; }
        v->attack_release_step=v->attack_envelope/(s.attack_release*RATE); v->attack_stage=RELEASE;
    }
}
int fm_active(const FMVoice *v) { return v->stage!=OFF && (v->engine!=1 || dx7_active(&v->dx7)); }
static void mod_envelope(float *value,int *stage,float attack,float decay,float sustain,float release_step) {
    if(*stage==ATTACK) { *value+=1/fmaxf(1,attack*RATE); if(*value>=1) { *value=1; *stage=DECAY; } }
    else if(*stage==DECAY || *stage==SUSTAIN) {
        if(*stage==SUSTAIN && *value==sustain) return;
        *value+=(sustain-*value)*fminf(1,4.6051702f/(decay*RATE));
        if(fabsf(*value-sustain)<.000001f) { *value=sustain; *stage=SUSTAIN; }
    } else if(*stage==RELEASE) { *value=fmaxf(0,*value-release_step); if(!*value) *stage=OFF; }
}
static float render_mono(FMVoice *v,const FMSettings *s,double pitch) {
    if(!v->stage) return 0;
    if(v->engine!=(int)s->engine) { v->stage=OFF; return 0; }
    if(v->engine==1) return dx7_sample(&v->dx7,&s->dx7,pitch)*.75f;
    const float slew=1.f/(RATE*.02f);
#define SLEW(field) v->field+=(s->field-v->field)*slew
    v->ratio+=(roundf(s->ratio)-v->ratio)*slew; SLEW(body_pitch); SLEW(depth); SLEW(carrier_ratio); SLEW(attack_ratio); SLEW(attack_depth);
    SLEW(carrier_detune); SLEW(body_detune); SLEW(attack_detune); SLEW(lfo_rate); SLEW(vibrato); SLEW(tremolo);
#undef SLEW
    mod_envelope(&v->mod_envelope,&v->body_stage,s->mod_attack,s->mod_decay,s->mod_sustain,v->body_release_step);
    mod_envelope(&v->attack_envelope,&v->attack_stage,s->attack_attack,s->attack_decay,s->attack_sustain,v->attack_release_step);
    float fade=s->lfo_fade?fminf(1,v->lfo_age/s->lfo_fade):1;
    float lfo=v->engine==2 || v->vibrato || (v->tremolo && s->lfo_shape)?fm_lfo_value(v->lfo_phase/6.283185307179586f,(int)s->lfo_shape):0;
    double target=fmax(1,fmin(RATE*.45,v->frequency*pitch));
    v->current_frequency+=(target-v->current_frequency)*slew;
    if(v->stage==ATTACK) { v->envelope+=1/(s->attack*RATE); if(v->envelope>=1) { v->envelope=1; v->stage=DECAY; } }
    else if(v->stage==DECAY) { v->envelope-=fmaxf(0,1-s->sustain)/(s->decay*RATE); if(v->envelope<=s->sustain) { v->envelope=s->sustain; v->stage=SUSTAIN; } }
    else if(v->stage==SUSTAIN) v->envelope+=(s->sustain-v->envelope)*slew;
    else { v->envelope-=v->release_step; if(v->envelope<=.000001f) { v->envelope=0; v->stage=OFF; return 0; } }
    double root=v->current_frequency*(v->vibrato && fade?pow(2,lfo*v->vibrato*fade/1200):1),limit=RATE*.45;
    if(v->engine==2) {
        float value=analog_sample(&v->analog,s->analog,root*v->carrier_ratio,lfo,v->mod_envelope*(1-s->velocity*(1-v->velocity)));
        float amp=v->tremolo?1-v->tremolo*fade*(.5f-.5f*lfo):1;
        v->lfo_phase+=6.283185307179586*v->lfo_rate/RATE; if(v->lfo_phase>=6.283185307179586) v->lfo_phase-=6.283185307179586; v->lfo_age+=1.f/RATE;
        return value*.35f*v->envelope*amp;
    }
    double carrier=fmin(limit,root*v->carrier_ratio*(v->carrier_detune?pow(2,v->carrier_detune/1200):1));
    double body=fmin(limit,root*v->ratio*(v->body_pitch || v->body_detune?pow(2,v->body_pitch/12+v->body_detune/1200):1));
    double strike=fmin(limit,root*v->attack_ratio*(v->attack_detune?pow(2,v->attack_detune/1200):1));
    float velocity=1-s->velocity*(1-v->velocity);
    double body_depth=v->depth*v->mod_envelope*velocity,attack_depth=v->attack_depth*v->attack_envelope*velocity;
    /* Four substeps and a shared sideband budget reduce high-note aliasing. */
    double budget=body_depth*body+attack_depth*strike,scale=budget>0?fmin(1,fmax(0,limit-carrier)/budget):1;
    body_depth*=scale; attack_depth*=scale;
    const double turn=6.283185307179586; float sum=0;
    for(int i=0;i<4;i++) {
        double attack_phase=attack_depth?attack_depth*sinf(v->attack_modulator):0;
        double body_phase=body_depth?body_depth*sinf(v->modulator+(s->routing?attack_phase:0)):0;
        sum+=sinf(v->carrier+body_phase+(s->routing?0:attack_phase));
        v->carrier+=turn*carrier/(RATE*4); if(v->carrier>=turn) v->carrier-=turn;
        v->modulator+=turn*body/(RATE*4); if(v->modulator>=turn) v->modulator-=turn;
        v->attack_modulator+=turn*strike/(RATE*4); if(v->attack_modulator>=turn) v->attack_modulator-=turn;
    }
    /* Preserve the original sine tremolo phase for old projects/presets-> */
    float amplitude=v->tremolo && fade?1-v->tremolo*fade*(s->lfo_shape?(.5f-.5f*lfo):(.5f-.5f*cosf(v->lfo_phase))):1;
    v->lfo_phase+=turn*v->lfo_rate/RATE; if(v->lfo_phase>=turn) v->lfo_phase-=turn; v->lfo_age+=1.f/RATE;
    return sum*.05f*v->envelope*amplitude;
}

void fm_sample_stereo(FMVoice *v,const FMSettings *s,double pitch,float stereo[2]) {
    stereo[0]=stereo[1]=render_mono(v,s,pitch);
    if(v->engine==2) chorus_process(&v->analog.chorus,chorus_default(),s->analog.chorus,stereo);
}
float fm_sample(FMVoice *v,FMSettings s,double pitch) { float stereo[2]; fm_sample_stereo(v,&s,pitch,stereo); return (stereo[0]+stereo[1])*.5f; }
