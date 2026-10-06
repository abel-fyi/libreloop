// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
static const ParameterDescriptor descriptors[]={
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
    {PARAM_CHORUS_RATE,"Chorus rate",.05f,5,.8f,PARAMETER_CONTINUOUS},
    {PARAM_CHORUS_DEPTH,"Chorus depth",0,8,3,PARAMETER_CONTINUOUS},
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
};
const ParameterDescriptor *parameter_descriptor(unsigned id) {
    for(size_t i=0;i<sizeof descriptors/sizeof *descriptors;i++) if(descriptors[i].id==id) return &descriptors[i];
    return NULL;
}
static const float *parameter_pointer(const Project *p,ParameterTarget t,float *lo,float *hi) {
    const ParameterDescriptor *info=parameter_descriptor(t.parameter);
    *lo=info?info->low:0; *hi=info?info->high:1;
    if(t.parameter>=PARAM_EQ_FIRST && t.parameter<=PARAM_EQ_LAST) {
        if(t.owner>(unsigned)p->insert_count || t.slot>=EFFECT_SLOTS || p->effect_type[t.owner][t.slot]!=EFFECT_EQ) return NULL;
        const EQBand *b=&p->eq[t.owner][t.slot].bands[(t.parameter-PARAM_EQ_FIRST)/3];
        return (t.parameter-PARAM_EQ_FIRST)%3==0?&b->frequency:(t.parameter-PARAM_EQ_FIRST)%3==1?&b->gain:&b->q;
    }
    if(t.parameter>=PARAM_FM_RATIO && t.parameter<=PARAM_FM_LAST) {
        if(t.owner>=(unsigned)p->channel_count || t.slot || p->instrument[t.owner]!=INSTRUMENT_FM) return NULL;
        return fm_parameter_pointer(&p->fm[t.owner],t.parameter);
    }

    if(t.parameter>=PARAM_CHORUS_RATE && t.parameter<=PARAM_EFFECT_MIX) {
        if(t.owner>(unsigned)p->insert_count || t.slot>=EFFECT_SLOTS || (p->effect_type[t.owner][t.slot]==EFFECT_EMPTY || (t.parameter!=PARAM_EFFECT_MIX && p->effect_type[t.owner][t.slot]!=EFFECT_CHORUS))) return NULL;
        if(t.parameter==PARAM_CHORUS_RATE) return &p->chorus[t.owner][t.slot].rate;
        if(t.parameter==PARAM_CHORUS_DEPTH) return &p->chorus[t.owner][t.slot].depth;
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
int parameter_from_pointer(const Project *p,const void *ptr,ParameterTarget *t) {
    for(unsigned id=1;id<=PARAM_PITCH_RANGE;id++) for(unsigned owner=0;owner<(id<=PARAM_CHANNEL_PITCH || id==PARAM_CHANNEL_MUTE || id==PARAM_PITCH_RANGE?CHANNELS:id<=PARAM_INSERT_WIDTH || id==PARAM_INSERT_MUTE?INSERTS:1);owner++) {
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
    return 0;
}
