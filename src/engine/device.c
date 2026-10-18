// SPDX-License-Identifier: GPL-3.0-only
#include "device.h"
#include "sampler.h"
#include "fm_synth.h"
#include "chorus.h"
#include "equalizer.h"
#include "delay.h"
#include "tube.h"
#include "reverb.h"
#include "dynamics.h"
#include "phaser.h"
#include "flanger.h"
static void sampler_defaults(void *s) { *(Sampler *)s=(Sampler){.time=1,.length=1}; }
static void fm_defaults(void *s) { *(FMSettings *)s=fm_default(); }
static void chorus_defaults(void *s) { *(ChorusSettings *)s=chorus_default(); }
static void eq_defaults(void *s) { *(EQSettings *)s=equalizer_default(); }
static int sampler_check(const void *s) { return s && sampler_valid(*(const Sampler *)s); }
static int fm_check(const void *s) { return s && fm_valid(*(const FMSettings *)s); }
static int chorus_check(const void *s) { return s && chorus_valid(*(const ChorusSettings *)s); }
static int eq_check(const void *s) { return s && equalizer_valid(*(const EQSettings *)s); }
static const float *fm_parameter(const void *s,unsigned id) { return fm_parameter_pointer(s,id); }
static const float *chorus_parameter(const void *s,unsigned id) {
    const ChorusSettings *c=s;
    return id==PARAM_CHORUS_RATE?&c->rate:id==PARAM_CHORUS_DEPTH?&c->depth:NULL;
}
static const float *eq_parameter(const void *s,unsigned id) {
    if(id<PARAM_EQ_FIRST || id>PARAM_EQ_LAST) return NULL;
    const EQBand *b=&((const EQSettings *)s)->bands[(id-PARAM_EQ_FIRST)/3];
    return (id-PARAM_EQ_FIRST)%3==0?&b->frequency:(id-PARAM_EQ_FIRST)%3==1?&b->gain:&b->q;
}
static void delay_defaults(void *s) { *(DelaySettings *)s=delay_default(); }
static int delay_check(const void *s) { return s && delay_valid(*(const DelaySettings *)s); }
static const float *delay_parameter(const void *s,unsigned id) {
    const DelaySettings *d=s;
    switch(id) {case PARAM_DELAY_STEPS:return &d->steps;case PARAM_DELAY_INPUT_PAN:return &d->input_pan;case PARAM_DELAY_INPUT_GAIN:return &d->input_gain;case PARAM_DELAY_OFFSET:return &d->offset;case PARAM_DELAY_DRY:return &d->dry;case PARAM_DELAY_TIME:return &d->time_ms;case PARAM_DELAY_FEEDBACK:return &d->feedback;case PARAM_DELAY_CUTOFF:return &d->cutoff;case PARAM_DELAY_STEREO:return &d->stereo;case PARAM_DELAY_OUTPUT:return &d->output;default:return NULL;}
}
static void tube_defaults(void *s) {*(TubeSettings *)s=tube_default();}
static int tube_check(const void *s) {return s && tube_valid(*(const TubeSettings *)s);}
static const float *tube_parameter(const void *s,unsigned id) {const TubeSettings *t=s;return id==PARAM_TUBE_DRIVE?&t->drive:id==PARAM_TUBE_TONE?&t->tone:id==PARAM_TUBE_OUTPUT?&t->output:id==PARAM_TUBE_NATURAL?&t->natural:NULL;}
static void reverb_defaults(void *s) {*(ReverbSettings *)s=reverb_default();}
static int reverb_check(const void *s) {return s && reverb_valid(*(const ReverbSettings *)s);}
static const float *reverb_parameter(const void *s,unsigned id) {
    const ReverbSettings *r=s;switch(id) {case PARAM_REVERB_SIZE:return &r->size;case PARAM_REVERB_DECAY:return &r->decay;case PARAM_REVERB_DAMPING:return &r->damping;case PARAM_REVERB_PREDELAY:return &r->predelay;case PARAM_REVERB_WIDTH:return &r->width;case PARAM_REVERB_MOD:return &r->mod;case PARAM_REVERB_GATE:return &r->gate;default:return NULL;}
}
static void dynamics_defaults(void *s){*(DynamicsSettings *)s=dynamics_default();}
static int dynamics_check(const void *s){return s && dynamics_valid(*(const DynamicsSettings *)s);}
static const float *dynamics_device_parameter(const void *s,unsigned id){return id>=PARAM_DYNAMICS_FIRST && id<=PARAM_DYNAMICS_LAST?dynamics_parameter(s,id-PARAM_DYNAMICS_FIRST):NULL;}
static void phaser_defaults(void *s){*(PhaserSettings *)s=phaser_default();}
static int phaser_check(const void *s){return s && phaser_valid(*(const PhaserSettings *)s);}
static const float *phaser_device_parameter(const void *s,unsigned id){return id>=PARAM_PHASER_FIRST && id<=PARAM_PHASER_LAST?phaser_parameter(s,id-PARAM_PHASER_FIRST):NULL;}
static void flanger_defaults(void *s){*(FlangerSettings *)s=flanger_default();}
static int flanger_check(const void *s){return s && flanger_valid(*(const FlangerSettings *)s);}
static const float *flanger_device_parameter(const void *s,unsigned id){return id>=PARAM_FLANGER_FIRST && id<=PARAM_FLANGER_LAST?flanger_parameter(s,id-PARAM_FLANGER_FIRST):NULL;}
static const DeviceDescriptor devices[]={
    {"Sampler",sizeof(Sampler),sampler_defaults,sampler_check,NULL,{{0,0},{0,0}}},
    {"FM Synth",sizeof(FMSettings),fm_defaults,fm_check,fm_parameter,{{PARAM_FM_RATIO,PARAM_FM_LAST},{PARAM_DX7_FIRST,PARAM_DX7_LAST},{PARAM_VA_FIRST,PARAM_VA_LAST}}},
    {"Chorus",sizeof(ChorusSettings),chorus_defaults,chorus_check,chorus_parameter,{{PARAM_CHORUS_RATE,PARAM_CHORUS_DEPTH},{PARAM_EFFECT_MIX,PARAM_EFFECT_MIX}}},
    {"Equalizer",sizeof(EQSettings),eq_defaults,eq_check,eq_parameter,{{PARAM_EQ_FIRST,PARAM_EQ_LAST},{PARAM_EFFECT_MIX,PARAM_EFFECT_MIX}}},
    {"Delay",sizeof(DelaySettings),delay_defaults,delay_check,delay_parameter,{{PARAM_DELAY_FIRST,PARAM_DELAY_LAST},{PARAM_EFFECT_MIX,PARAM_EFFECT_MIX}}},
    {"Tube",sizeof(TubeSettings),tube_defaults,tube_check,tube_parameter,{{PARAM_TUBE_FIRST,PARAM_TUBE_LAST},{PARAM_EFFECT_MIX,PARAM_EFFECT_MIX}}},
    {"Reverb",sizeof(ReverbSettings),reverb_defaults,reverb_check,reverb_parameter,{{PARAM_REVERB_FIRST,PARAM_REVERB_LAST},{PARAM_EFFECT_MIX,PARAM_EFFECT_MIX}}},
    {"Multiband Compressor",sizeof(DynamicsSettings),dynamics_defaults,dynamics_check,dynamics_device_parameter,{{PARAM_DYNAMICS_FIRST,PARAM_DYNAMICS_LAST},{PARAM_EFFECT_MIX,PARAM_EFFECT_MIX}}},
    {"Phaser",sizeof(PhaserSettings),phaser_defaults,phaser_check,phaser_device_parameter,{{PARAM_PHASER_FIRST,PARAM_PHASER_LAST},{PARAM_EFFECT_MIX,PARAM_EFFECT_MIX}}},
    {"Flanger",sizeof(FlangerSettings),flanger_defaults,flanger_check,flanger_device_parameter,{{PARAM_FLANGER_FIRST,PARAM_FLANGER_LAST},{PARAM_EFFECT_MIX,PARAM_EFFECT_MIX}}},
};
const DeviceDescriptor *device_descriptor(unsigned kind) {
    return kind>=DEVICE_SAMPLER && kind<=DEVICE_FLANGER?&devices[kind-1]:NULL;
}
const DeviceDescriptor *instrument_descriptor(unsigned type) {
    return type==INSTRUMENT_SAMPLER?device_descriptor(DEVICE_SAMPLER):type==INSTRUMENT_FM?device_descriptor(DEVICE_FM):NULL;
}
const DeviceDescriptor *effect_descriptor(unsigned type) {
    return type==EFFECT_CHORUS?device_descriptor(DEVICE_CHORUS):type==EFFECT_EQ?device_descriptor(DEVICE_EQ):type==EFFECT_DELAY?device_descriptor(DEVICE_DELAY):type==EFFECT_TUBE?device_descriptor(DEVICE_TUBE):type==EFFECT_REVERB?device_descriptor(DEVICE_REVERB):type==EFFECT_DYNAMICS?device_descriptor(DEVICE_DYNAMICS):type==EFFECT_PHASER?device_descriptor(DEVICE_PHASER):type==EFFECT_FLANGER?device_descriptor(DEVICE_FLANGER):NULL;
}
int device_has_parameter(const DeviceDescriptor *device,unsigned id) {
    if(!device || !id) return 0;
    for(unsigned i=0;i<3;i++) if(id>=device->parameters[i].first && id<=device->parameters[i].last) return 1;
    return 0;
}

const float *device_parameter(const DeviceDescriptor *device,const void *settings,unsigned id) {
    return settings && device_has_parameter(device,id) && device->parameter?device->parameter(settings,id):NULL;
}
