// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include <math.h>
static const ParameterDescriptor descriptors[]={
    {PARAM_PHASER_RATE,"Phaser rate (Hz)",.01f,10,.35f,PARAMETER_LOGARITHMIC},
    {PARAM_PHASER_CENTER,"Phaser center (Hz)",80,8000,900,PARAMETER_LOGARITHMIC},
    {PARAM_PHASER_DEPTH,"Phaser sweep (octaves)",0,4,2,PARAMETER_CONTINUOUS},
    {PARAM_PHASER_FEEDBACK,"Phaser feedback",-.9f,.9f,.35f,PARAMETER_CONTINUOUS},
    {PARAM_PHASER_STEREO,"Phaser stereo LFO spread",0,1,.5f,PARAMETER_CONTINUOUS},
    {PARAM_PHASER_STAGES,"Phaser stages",2,12,6,PARAMETER_INTEGER},
    {PARAM_FLANGER_RATE,"Flanger rate (Hz)",.01f,10,.25f,PARAMETER_LOGARITHMIC},
    {PARAM_FLANGER_DELAY,"Flanger delay (ms)",.2f,12,2.5f,PARAMETER_LOGARITHMIC},
    {PARAM_FLANGER_DEPTH,"Flanger sweep depth",0,1,.8f,PARAMETER_CONTINUOUS},
    {PARAM_FLANGER_FEEDBACK,"Flanger feedback",-.95f,.95f,.55f,PARAMETER_CONTINUOUS},
    {PARAM_FLANGER_STEREO,"Flanger stereo LFO spread",0,1,.25f,PARAMETER_CONTINUOUS},
    {PARAM_FLANGER_INVERT,"Flanger invert wet",0,1,0,PARAMETER_TOGGLE},

    {PARAM_DYNAMICS_FIRST+0,"Low crossover (Hz)",40,1000,200,PARAMETER_LOGARITHMIC},
    {PARAM_DYNAMICS_FIRST+1,"High crossover (Hz)",1000,16000,2500,PARAMETER_LOGARITHMIC},
    {PARAM_DYNAMICS_FIRST+2,"Low cut (Hz / Off)",0,80,0,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+3,"Band mix",0,1,1,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+4,"Low Pre gain (dB)",-24,24,0,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+5,"Low Post gain (dB)",-24,24,0,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+6,"Low Attack (ms)",0,200,10,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+7,"Low Release (ms)",0,2000,150,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+8,"Low Release 2 (ms)",0,2000,0,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+9,"Low Sustain (ms)",0,1000,10,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+10,"Low Stereo width",0,2,1,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+11,"Low Mode",0,3,0,PARAMETER_INTEGER},
    {PARAM_DYNAMICS_FIRST+12,"Low Solo",0,1,0,PARAMETER_TOGGLE},
    {PARAM_DYNAMICS_FIRST+13,"Low RMS",0,1,0,PARAMETER_TOGGLE},
    {PARAM_DYNAMICS_FIRST+14,"Low Saturation ceiling (dB)",-24,12,0,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+15,"Low Saturation",0,1,0,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+16,"Mid Pre gain (dB)",-24,24,0,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+17,"Mid Post gain (dB)",-24,24,0,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+18,"Mid Attack (ms)",0,200,10,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+19,"Mid Release (ms)",0,2000,150,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+20,"Mid Release 2 (ms)",0,2000,0,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+21,"Mid Sustain (ms)",0,1000,10,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+22,"Mid Stereo width",0,2,1,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+23,"Mid Mode",0,3,0,PARAMETER_INTEGER},
    {PARAM_DYNAMICS_FIRST+24,"Mid Solo",0,1,0,PARAMETER_TOGGLE},
    {PARAM_DYNAMICS_FIRST+25,"Mid RMS",0,1,0,PARAMETER_TOGGLE},
    {PARAM_DYNAMICS_FIRST+26,"Mid Saturation ceiling (dB)",-24,12,0,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+27,"Mid Saturation",0,1,0,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+28,"High Pre gain (dB)",-24,24,0,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+29,"High Post gain (dB)",-24,24,0,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+30,"High Attack (ms)",0,200,10,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+31,"High Release (ms)",0,2000,150,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+32,"High Release 2 (ms)",0,2000,0,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+33,"High Sustain (ms)",0,1000,10,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+34,"High Stereo width",0,2,1,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+35,"High Mode",0,3,0,PARAMETER_INTEGER},
    {PARAM_DYNAMICS_FIRST+36,"High Solo",0,1,0,PARAMETER_TOGGLE},
    {PARAM_DYNAMICS_FIRST+37,"High RMS",0,1,0,PARAMETER_TOGGLE},
    {PARAM_DYNAMICS_FIRST+38,"High Saturation ceiling (dB)",-24,12,0,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+39,"High Saturation",0,1,0,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+40,"Master Pre gain (dB)",-24,24,0,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+41,"Master Post gain (dB)",-24,24,0,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+42,"Master Attack (ms)",0,200,10,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+43,"Master Release (ms)",0,2000,150,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+44,"Master Release 2 (ms)",0,2000,0,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+45,"Master Sustain (ms)",0,1000,10,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+46,"Master Stereo width",0,2,1,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+47,"Master Mode",0,3,0,PARAMETER_INTEGER},
    {PARAM_DYNAMICS_FIRST+48,"Master Solo",0,1,0,PARAMETER_TOGGLE},
    {PARAM_DYNAMICS_FIRST+49,"Master RMS",0,1,0,PARAMETER_TOGGLE},
    {PARAM_DYNAMICS_FIRST+50,"Master Saturation ceiling (dB)",-24,12,0,PARAMETER_CONTINUOUS},
    {PARAM_DYNAMICS_FIRST+51,"Master Saturation",0,1,0,PARAMETER_CONTINUOUS},

    {PARAM_REVERB_SIZE,"Reverb size",0,1,.55f,PARAMETER_CONTINUOUS},
    {PARAM_REVERB_DECAY,"Reverb decay (s)",.15f,20,2.4f,PARAMETER_LOGARITHMIC},
    {PARAM_REVERB_DAMPING,"Reverb damping (Hz)",500,20000,18000,PARAMETER_LOGARITHMIC},
    {PARAM_REVERB_PREDELAY,"Reverb pre-delay (ms)",0,250,18,PARAMETER_CONTINUOUS},
    {PARAM_REVERB_WIDTH,"Reverb width",0,2,1,PARAMETER_CONTINUOUS},
    {PARAM_REVERB_MOD,"Reverb modulation",0,1,.3f,PARAMETER_CONTINUOUS},
    {PARAM_REVERB_GATE,"Reverb gate hold (ms / Off)",0,1000,0,PARAMETER_CONTINUOUS},
    {PARAM_TUBE_DRIVE,"Tube drive (dB)",0,36,12,PARAMETER_CONTINUOUS},
    {PARAM_TUBE_TONE,"Tube tone (Hz / None)",1200,TUBE_TONE_OFF,TUBE_TONE_OFF,PARAMETER_LOGARITHMIC},
    {PARAM_TUBE_OUTPUT,"Tube output (dB)",-24,12,-3,PARAMETER_CONTINUOUS},
    {PARAM_TUBE_NATURAL,"Tube voicing (Warm / Natural)",0,1,0,PARAMETER_TOGGLE},
    {PARAM_DELAY_STEPS,"Delay time (steps)",.125f,16,3,PARAMETER_LOGARITHMIC},
    {PARAM_DELAY_INPUT_PAN,"Delay input pan",-1,1,0,PARAMETER_CONTINUOUS},
    {PARAM_DELAY_INPUT_GAIN,"Delay input volume",0,2,.35f,PARAMETER_CONTINUOUS},
    {PARAM_DELAY_OFFSET,"Delay stereo offset",-1,1,0,PARAMETER_CONTINUOUS},
    {PARAM_DELAY_DRY,"Delay dry volume",0,2,1,PARAMETER_CONTINUOUS},
    {PARAM_DELAY_TIME,"Delay time (ms)",1,4000,375,PARAMETER_LOGARITHMIC},
    {PARAM_DELAY_FEEDBACK,"Delay feedback",0,1,.35f,PARAMETER_CONTINUOUS},
    {PARAM_DELAY_CUTOFF,"Delay tone (Hz)",100,20000,8000,PARAMETER_LOGARITHMIC},
    {PARAM_DELAY_STEREO,"Delay ping-pong",0,1,0,PARAMETER_CONTINUOUS},
    {PARAM_DELAY_OUTPUT,"Delay output",0,2,1,PARAMETER_CONTINUOUS},
    {PARAM_EQ_FIRST+0,"EQ 1 frequency",20,20000,60,PARAMETER_LOGARITHMIC},
    {PARAM_EQ_FIRST+1,"EQ 1 gain",-18,18,0,PARAMETER_CONTINUOUS},
    {PARAM_EQ_FIRST+2,"EQ 1 Q",0.2,10,0.707,PARAMETER_CONTINUOUS},
    {PARAM_EQ_FIRST+3,"EQ 2 frequency",20,20000,150,PARAMETER_LOGARITHMIC},
    {PARAM_EQ_FIRST+4,"EQ 2 gain",-18,18,0,PARAMETER_CONTINUOUS},
    {PARAM_EQ_FIRST+5,"EQ 2 Q",0.2,10,1,PARAMETER_CONTINUOUS},
    {PARAM_EQ_FIRST+6,"EQ 3 frequency",20,20000,400,PARAMETER_LOGARITHMIC},
    {PARAM_EQ_FIRST+7,"EQ 3 gain",-18,18,0,PARAMETER_CONTINUOUS},
    {PARAM_EQ_FIRST+8,"EQ 3 Q",0.2,10,1,PARAMETER_CONTINUOUS},
    {PARAM_EQ_FIRST+9,"EQ 4 frequency",20,20000,1000,PARAMETER_LOGARITHMIC},
    {PARAM_EQ_FIRST+10,"EQ 4 gain",-18,18,0,PARAMETER_CONTINUOUS},
    {PARAM_EQ_FIRST+11,"EQ 4 Q",0.2,10,1,PARAMETER_CONTINUOUS},
    {PARAM_EQ_FIRST+12,"EQ 5 frequency",20,20000,2500,PARAMETER_LOGARITHMIC},
    {PARAM_EQ_FIRST+13,"EQ 5 gain",-18,18,0,PARAMETER_CONTINUOUS},
    {PARAM_EQ_FIRST+14,"EQ 5 Q",0.2,10,1,PARAMETER_CONTINUOUS},
    {PARAM_EQ_FIRST+15,"EQ 6 frequency",20,20000,6000,PARAMETER_LOGARITHMIC},
    {PARAM_EQ_FIRST+16,"EQ 6 gain",-18,18,0,PARAMETER_CONTINUOUS},
    {PARAM_EQ_FIRST+17,"EQ 6 Q",0.2,10,1,PARAMETER_CONTINUOUS},
    {PARAM_EQ_FIRST+18,"EQ 7 frequency",20,20000,12000,PARAMETER_LOGARITHMIC},
    {PARAM_EQ_FIRST+19,"EQ 7 gain",-18,18,0,PARAMETER_CONTINUOUS},
    {PARAM_EQ_FIRST+20,"EQ 7 Q",0.2,10,0.707,PARAMETER_CONTINUOUS},
    {PARAM_FM_RATIO,"Body harmonic ratio",1,8,1,PARAMETER_INTEGER},
    {PARAM_FM_DEPTH,"Body FM amount",0,8,2.6f,PARAMETER_CONTINUOUS},
    {PARAM_FM_ATTACK,"Sound attack",.001f,2,.0015f,PARAMETER_CONTINUOUS},
    {PARAM_FM_DECAY,"Sound decay",.01f,3,2.5f,PARAMETER_CONTINUOUS},
    {PARAM_FM_SUSTAIN,"Sound sustain",0,1,.08f,PARAMETER_CONTINUOUS},
    {PARAM_FM_RELEASE,"Sound release",.01f,3,.45f,PARAMETER_CONTINUOUS},
    {PARAM_FM_MOD_DECAY,"Body decay",.01f,5,1.2f,PARAMETER_CONTINUOUS},
    {PARAM_FM_MOD_SUSTAIN,"Body sustain",0,1,.12f,PARAMETER_CONTINUOUS},
    {PARAM_FM_VELOCITY,"Velocity brightness",0,1,.8f,PARAMETER_CONTINUOUS},
    {PARAM_FM_LFO_RATE,"Motion rate",.1f,12,4.8f,PARAMETER_CONTINUOUS},
    {PARAM_FM_VIBRATO,"FM vibrato",0,100,0,PARAMETER_CONTINUOUS},
    {PARAM_FM_TREMOLO,"FM tremolo",0,1,0,PARAMETER_CONTINUOUS},
    {PARAM_FM_CARRIER_RATIO,"Carrier pitch ratio",.125f,32,1,PARAMETER_LOGARITHMIC},
    {PARAM_FM_CARRIER_DETUNE,"Carrier fine tuning (cents)",-100,100,0,PARAMETER_CONTINUOUS},
    {PARAM_FM_BODY_DETUNE,"Body fine tuning (cents)",-100,100,0,PARAMETER_CONTINUOUS},
    {PARAM_FM_MOD_ATTACK,"Body attack",0,2,.001f,PARAMETER_CONTINUOUS},
    {PARAM_FM_MOD_RELEASE,"Body release",0,5,.4f,PARAMETER_CONTINUOUS},
    {PARAM_FM_ATTACK_RATIO,"Attack pitch ratio",.125f,32,14,PARAMETER_LOGARITHMIC},
    {PARAM_FM_ATTACK_DETUNE,"Attack fine tuning (cents)",-100,100,0,PARAMETER_CONTINUOUS},
    {PARAM_FM_ATTACK_DEPTH,"Attack FM amount",0,12,4.5f,PARAMETER_CONTINUOUS},
    {PARAM_FM_ATTACK_ATTACK,"Strike attack",.001f,2,.001f,PARAMETER_LOGARITHMIC},
    {PARAM_FM_ATTACK_DECAY,"Strike decay",.005f,5,.09f,PARAMETER_LOGARITHMIC},
    {PARAM_FM_ATTACK_SUSTAIN,"Strike sustain",0,1,0,PARAMETER_CONTINUOUS},
    {PARAM_FM_ATTACK_RELEASE,"Strike release",.01f,5,.07f,PARAMETER_LOGARITHMIC},
    {PARAM_FM_ROUTING,"FM routing",0,1,0,PARAMETER_INTEGER},
    {PARAM_FM_LFO_SHAPE,"LFO shape",0,3,0,PARAMETER_INTEGER},
    {PARAM_FM_LFO_FADE,"LFO fade in",0,5,.3f,PARAMETER_CONTINUOUS},
    {PARAM_FM_BODY_PITCH,"Body pitch (semitones)",-48,48,0,PARAMETER_CONTINUOUS},
    {PARAM_FM_ENGINE,"Synth engine",0,2,1,PARAMETER_INTEGER},
    {PARAM_FM_ANALOG_WAVE,"Oscillator waveform",0,3,0,PARAMETER_INTEGER},
    {PARAM_FM_ANALOG_DETUNE,"Oscillator detune",0,50,8,PARAMETER_CONTINUOUS},
    {PARAM_FM_ANALOG_MIX,"Second oscillator",0,1,.65f,PARAMETER_CONTINUOUS},
    {PARAM_FM_ANALOG_SUB,"Sub oscillator",0,1,.2f,PARAMETER_CONTINUOUS},
    {PARAM_FM_ANALOG_NOISE,"Noise",0,1,0,PARAMETER_CONTINUOUS},
    {PARAM_FM_ANALOG_PULSE,"Pulse width",.05f,.95f,.5f,PARAMETER_CONTINUOUS},
    {PARAM_FM_ANALOG_PWM,"Pulse modulation",0,.45f,.15f,PARAMETER_CONTINUOUS},
    {PARAM_FM_FILTER_CUTOFF,"Filter cutoff",20,20000,5000,PARAMETER_LOGARITHMIC},
    {PARAM_FM_FILTER_RESONANCE,"Filter resonance",0,.95f,.2f,PARAMETER_CONTINUOUS},
    {PARAM_FM_FILTER_ENV,"Filter envelope octaves",-6,6,1,PARAMETER_CONTINUOUS},
    {PARAM_FM_ANALOG_CHORUS,"Synth chorus",0,1,.35f,PARAMETER_CONTINUOUS},
    {PARAM_CHORUS_RATE,"Chorus rate",.05f,5,.513f,PARAMETER_CONTINUOUS},
    {PARAM_CHORUS_DEPTH,"Chorus depth",0,8,1.85f,PARAMETER_CONTINUOUS},
    {PARAM_EFFECT_MIX,"Effect mix",0,1,.5f,PARAMETER_CONTINUOUS},
    {PARAM_CHANNEL_VOLUME,"Volume",0,VOLUME_KNOB_MAX,1,PARAMETER_CONTINUOUS},
    {PARAM_CHANNEL_PAN,"Pan",-1,1,0,PARAMETER_CONTINUOUS},
    {PARAM_CHANNEL_PITCH,"Pitch",-1,1,0,PARAMETER_CONTINUOUS},
    {PARAM_INSERT_VOLUME,"Volume",0,MIXER_GAIN_MAX,1,PARAMETER_CONTINUOUS},
    {PARAM_INSERT_PAN,"Pan",-1,1,0,PARAMETER_CONTINUOUS},
    {PARAM_INSERT_WIDTH,"Stereo width",0,2,1,PARAMETER_CONTINUOUS},
    {PARAM_MASTER_VOLUME,"Master volume",0,MIXER_GAIN_MAX,1,PARAMETER_CONTINUOUS},
    {PARAM_MASTER_WIDTH,"Master stereo width",0,2,1,PARAMETER_CONTINUOUS},
    {PARAM_MASTER_PITCH,"Master pitch",-12,12,0,PARAMETER_CONTINUOUS},
    {PARAM_CHANNEL_MUTE,"Mute",0,1,0,PARAMETER_TOGGLE},
    {PARAM_INSERT_MUTE,"Mute",0,1,0,PARAMETER_TOGGLE},
    {PARAM_MASTER_MUTE,"Master mute",0,1,0,PARAMETER_TOGGLE},
    {PARAM_SWING,"Swing",0,1,0,PARAMETER_CONTINUOUS},
    {PARAM_PITCH_RANGE,"Pitch range",1,48,2,PARAMETER_INTEGER},
    {PARAM_MASTER_PAN,"Master pan",-1,1,0,PARAMETER_CONTINUOUS},
};
const ParameterDescriptor *parameter_descriptor(unsigned id) {
    if(id>=PARAM_DX7_FIRST && id<=PARAM_DX7_LAST) return dx7_parameter_descriptor(id-PARAM_DX7_FIRST);
    for(size_t i=0;i<sizeof descriptors/sizeof *descriptors;i++) if(descriptors[i].id==id) return &descriptors[i];
    return NULL;
}
static const float *parameter_pointer(const Project *p,ParameterTarget t,float *lo,float *hi) {
    const ParameterDescriptor *info=parameter_descriptor(t.parameter);
    *lo=info?info->low:0; *hi=info?info->high:1;
    if(t.parameter>=PARAM_PHASER_FIRST && t.parameter<=PARAM_PHASER_LAST) {
        if(t.owner>(unsigned)p->insert_count || t.slot>=EFFECT_SLOTS || p->effect_type[t.owner][t.slot]!=EFFECT_PHASER)return NULL;
        return phaser_parameter(&p->phaser[t.owner][t.slot],t.parameter-PARAM_PHASER_FIRST);
    }
    if(t.parameter>=PARAM_FLANGER_FIRST && t.parameter<=PARAM_FLANGER_LAST) {
        if(t.owner>(unsigned)p->insert_count || t.slot>=EFFECT_SLOTS || p->effect_type[t.owner][t.slot]!=EFFECT_FLANGER)return NULL;
        return flanger_parameter(&p->flanger[t.owner][t.slot],t.parameter-PARAM_FLANGER_FIRST);
    }
    if(t.parameter>=PARAM_DYNAMICS_FIRST && t.parameter<=PARAM_DYNAMICS_LAST) {
        if(t.owner>(unsigned)p->insert_count || t.slot>=EFFECT_SLOTS || p->effect_type[t.owner][t.slot]!=EFFECT_DYNAMICS)return NULL;
        return dynamics_parameter(&p->dynamics[t.owner][t.slot],t.parameter-PARAM_DYNAMICS_FIRST);
    }
    if(t.parameter>=PARAM_REVERB_FIRST && t.parameter<=PARAM_REVERB_LAST) {
        if(t.owner>(unsigned)p->insert_count || t.slot>=EFFECT_SLOTS || p->effect_type[t.owner][t.slot]!=EFFECT_REVERB) return NULL;
        return device_parameter(effect_descriptor(EFFECT_REVERB),&p->reverb[t.owner][t.slot],t.parameter);
    }
    if(t.parameter>=PARAM_TUBE_FIRST && t.parameter<=PARAM_TUBE_LAST) {
        if(t.owner>(unsigned)p->insert_count || t.slot>=EFFECT_SLOTS || p->effect_type[t.owner][t.slot]!=EFFECT_TUBE) return NULL;
        return device_parameter(effect_descriptor(EFFECT_TUBE),&p->tube[t.owner][t.slot],t.parameter);
    }
    if(t.parameter>=PARAM_DELAY_FIRST && t.parameter<=PARAM_DELAY_LAST) {
        if(t.owner>(unsigned)p->insert_count || t.slot>=EFFECT_SLOTS || p->effect_type[t.owner][t.slot]!=EFFECT_DELAY) return NULL;
        return device_parameter(effect_descriptor(EFFECT_DELAY),&p->delay[t.owner][t.slot],t.parameter);
    }
    if(t.parameter>=PARAM_EQ_FIRST && t.parameter<=PARAM_EQ_LAST) {
        if(t.owner>(unsigned)p->insert_count || t.slot>=EFFECT_SLOTS || p->effect_type[t.owner][t.slot]!=EFFECT_EQ) return NULL;
        return device_parameter(effect_descriptor(EFFECT_EQ),&p->eq[t.owner][t.slot],t.parameter);
    }
    if(t.parameter>=PARAM_DX7_FIRST && t.parameter<=PARAM_DX7_LAST) {
        if(t.owner>=(unsigned)p->channel_count || t.slot || p->instrument[t.owner]!=INSTRUMENT_FM || p->fm[t.owner].engine!=1) return NULL;
        return device_parameter(instrument_descriptor(p->instrument[t.owner]),&p->fm[t.owner],t.parameter);
    }
    if(t.parameter<PARAM_DX7_FIRST && device_has_parameter(instrument_descriptor(INSTRUMENT_FM),t.parameter)) {
        if(t.parameter==PARAM_FM_ENGINE) return NULL;
        if(t.owner>=(unsigned)p->channel_count || t.slot || p->instrument[t.owner]!=INSTRUMENT_FM) return NULL;
        return device_parameter(instrument_descriptor(p->instrument[t.owner]),&p->fm[t.owner],t.parameter);
    }

    if(t.parameter>=PARAM_CHORUS_RATE && t.parameter<=PARAM_EFFECT_MIX) {
        if(t.owner>(unsigned)p->insert_count || t.slot>=EFFECT_SLOTS || !device_has_parameter(effect_descriptor(p->effect_type[t.owner][t.slot]),t.parameter)) return NULL;
        if(t.parameter!=PARAM_EFFECT_MIX) return device_parameter(effect_descriptor(p->effect_type[t.owner][t.slot]),&p->chorus[t.owner][t.slot],t.parameter);
        return &p->effect_mix[t.owner][t.slot];
    }
    switch(t.parameter) {
    case PARAM_CHANNEL_VOLUME: return t.owner<(unsigned)p->channel_count?&p->volume[t.owner]:NULL;
    case PARAM_CHANNEL_PAN: return t.owner<(unsigned)p->channel_count?&p->pan[t.owner]:NULL;
    case PARAM_CHANNEL_PITCH: return t.owner<(unsigned)p->channel_count?&p->channel_pitch[t.owner]:NULL;
    case PARAM_INSERT_VOLUME: return t.owner<(unsigned)p->insert_count?&p->insert_volume[t.owner]:NULL;
    case PARAM_INSERT_PAN: return t.owner<(unsigned)p->insert_count?&p->insert_pan[t.owner]:NULL;
    case PARAM_INSERT_WIDTH: return t.owner<(unsigned)p->insert_count?&p->insert_width[t.owner]:NULL;
    case PARAM_MASTER_VOLUME: return &p->master;
    case PARAM_MASTER_PAN: return t.owner==0?&p->master_pan:NULL;
    case PARAM_MASTER_WIDTH: return &p->master_width;
    case PARAM_MASTER_PITCH: return &p->master_pitch;
    case PARAM_SWING: return &p->swing;
    case PARAM_PITCH_RANGE: return t.owner<(unsigned)p->channel_count?&p->pitch_range[t.owner]:NULL;
    default: return NULL;
    }
}
int parameter_info(const Project *p,ParameterTarget t,float *v,float *lo,float *hi) {
    const float *ptr=parameter_pointer(p,t,lo,hi);
    if(ptr) { *v=*ptr; return 1; }
    *lo=0; *hi=1;
    if(t.parameter==PARAM_CHANNEL_MUTE && t.owner<(unsigned)p->channel_count) *v=!!(p->mute[t.owner]&1);
    else if(t.parameter==PARAM_INSERT_MUTE && t.owner<(unsigned)p->insert_count) *v=!!(p->insert_mute[t.owner]&1);
    else if(t.parameter==PARAM_MASTER_MUTE) *v=!!p->master_mute;
    else return 0;
    return 1;
}
int parameter_write(Project *p,ParameterTarget t,float normalized) {
    float v,lo,hi;if(!isfinite(normalized) || !parameter_info(p,t,&v,&lo,&hi))return 0;
    normalized=fmaxf(0,fminf(1,normalized));
    float *ptr=(float *)parameter_pointer(p,t,&lo,&hi);
    if(ptr) {
        float value=lo+normalized*(hi-lo);const ParameterDescriptor *d=parameter_descriptor(t.parameter);
        if(d && (d->kind==PARAMETER_INTEGER || d->kind==PARAMETER_TOGGLE))value=roundf(value);
        *ptr=value;
    } else {
        unsigned char *state=t.parameter==PARAM_CHANNEL_MUTE?&p->mute[t.owner]:t.parameter==PARAM_INSERT_MUTE?&p->insert_mute[t.owner]:&p->master_mute;
        *state=(*state&~1u)|(normalized>=.5f);
    }return 1;
}
int parameter_from_pointer(const Project *p,const void *ptr,ParameterTarget *t) {
    {uintptr_t a=(uintptr_t)ptr,base=(uintptr_t)&p->flanger[0][0];if(a>=base && a<base+sizeof p->flanger){unsigned index=(unsigned)((a-base)/sizeof(FlangerSettings)),owner=index/EFFECT_SLOTS,slot=index%EFFECT_SLOTS;if(owner>(unsigned)p->insert_count || p->effect_type[owner][slot]!=EFFECT_FLANGER)return 0;for(unsigned n=0;n<6;n++)if(flanger_parameter(&p->flanger[owner][slot],n)==ptr){*t=(ParameterTarget){PARAM_FLANGER_FIRST+n,owner,slot};return 1;}return 0;}}

    {uintptr_t a=(uintptr_t)ptr,base=(uintptr_t)&p->phaser[0][0];if(a>=base && a<base+sizeof p->phaser){unsigned index=(unsigned)((a-base)/sizeof(PhaserSettings)),owner=index/EFFECT_SLOTS,slot=index%EFFECT_SLOTS;if(owner>(unsigned)p->insert_count || p->effect_type[owner][slot]!=EFFECT_PHASER)return 0;for(unsigned n=0;n<6;n++)if(phaser_parameter(&p->phaser[owner][slot],n)==ptr){*t=(ParameterTarget){PARAM_PHASER_FIRST+n,owner,slot};return 1;}return 0;}}

    /* The new large settings array has a concrete address mapping; avoid a
       full bus/slot/parameter search for every knob value drawn each frame. */
    uintptr_t address=(uintptr_t)ptr,start=(uintptr_t)&p->dynamics[0][0];
    if(address>=start && address<start+sizeof p->dynamics){
        unsigned index=(unsigned)((address-start)/sizeof(DynamicsSettings)),owner=index/EFFECT_SLOTS,slot=index%EFFECT_SLOTS;
        if(owner>(unsigned)p->insert_count || p->effect_type[owner][slot]!=EFFECT_DYNAMICS)return 0;
        for(unsigned n=0;n<DYNAMICS_PARAMETERS;n++)if(dynamics_parameter(&p->dynamics[owner][slot],n)==ptr){*t=(ParameterTarget){PARAM_DYNAMICS_FIRST+n,owner,slot};return 1;}
        return 0;
    }

    for(unsigned c=0;c<(unsigned)p->channel_count;c++) if(p->instrument[c]==INSTRUMENT_FM && p->fm[c].engine==1) {
        uintptr_t address=(uintptr_t)ptr,start=(uintptr_t)p->fm[c].dx7.value,end=start+sizeof p->fm[c].dx7.value;
        if(address>=start && address<end && (address-start)%sizeof(float)==0) { *t=(ParameterTarget){PARAM_DX7_FIRST+(address-start)/sizeof(float),c,0}; return 1; }
    }
    for(unsigned id=1;id<=PARAM_MASTER_PAN;id++) for(unsigned owner=0;owner<(id<=PARAM_CHANNEL_PITCH || id==PARAM_CHANNEL_MUTE || id==PARAM_PITCH_RANGE?CHANNELS:id<=PARAM_INSERT_WIDTH || id==PARAM_INSERT_MUTE?INSERTS:1);owner++) {
        ParameterTarget candidate={id,owner,0}; float lo,hi;
        const void *address=parameter_pointer(p,candidate,&lo,&hi);
        if(id==PARAM_CHANNEL_MUTE) address=&p->mute[owner];
        if(id==PARAM_INSERT_MUTE) address=&p->insert_mute[owner];
        if(id==PARAM_MASTER_MUTE) address=&p->master_mute;
        if(address==ptr) { *t=candidate; return 1; }
    }
    for(unsigned id=PARAM_FM_RATIO;id<=PARAM_FM_LAST;id++) for(unsigned owner=0;owner<(unsigned)p->channel_count;owner++) {
        ParameterTarget candidate={id,owner,0}; float lo,hi;
        if(parameter_pointer(p,candidate,&lo,&hi)==ptr) { *t=candidate; return 1; }
    }
    for(unsigned id=PARAM_CHORUS_RATE;id<=PARAM_EFFECT_MIX;id++) for(unsigned owner=0;owner<=(unsigned)p->insert_count;owner++) for(unsigned slot=0;slot<EFFECT_SLOTS;slot++) {
        ParameterTarget candidate={id,owner,slot}; float lo,hi;
        if(parameter_pointer(p,candidate,&lo,&hi)==ptr) { *t=candidate; return 1; }
    }
    for(unsigned id=PARAM_EQ_FIRST;id<=PARAM_EQ_LAST;id++) for(unsigned owner=0;owner<=(unsigned)p->insert_count;owner++) for(unsigned slot=0;slot<EFFECT_SLOTS;slot++) {
        ParameterTarget candidate={id,owner,slot}; float lo,hi;
        if(parameter_pointer(p,candidate,&lo,&hi)==ptr) { *t=candidate; return 1; }
    }
    for(unsigned id=PARAM_DELAY_FIRST;id<=PARAM_DELAY_LAST;id++) for(unsigned owner=0;owner<=(unsigned)p->insert_count;owner++) for(unsigned slot=0;slot<EFFECT_SLOTS;slot++) {
        ParameterTarget candidate={id,owner,slot}; float lo,hi;
        if(parameter_pointer(p,candidate,&lo,&hi)==ptr) { *t=candidate; return 1; }
    }
    for(unsigned id=PARAM_TUBE_FIRST;id<=PARAM_TUBE_LAST;id++) for(unsigned owner=0;owner<=(unsigned)p->insert_count;owner++) for(unsigned slot=0;slot<EFFECT_SLOTS;slot++) {
        ParameterTarget candidate={id,owner,slot}; float lo,hi;
        if(parameter_pointer(p,candidate,&lo,&hi)==ptr) { *t=candidate; return 1; }
    }
    for(unsigned id=PARAM_REVERB_FIRST;id<=PARAM_REVERB_LAST;id++) for(unsigned owner=0;owner<=(unsigned)p->insert_count;owner++) for(unsigned slot=0;slot<EFFECT_SLOTS;slot++) {
        ParameterTarget candidate={id,owner,slot}; float lo,hi;
        if(parameter_pointer(p,candidate,&lo,&hi)==ptr) { *t=candidate; return 1; }
    }
    return 0;
}
