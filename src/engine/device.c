// SPDX-License-Identifier: GPL-3.0-only
#include "device.h"
#include "sampler.h"
#include "fm_synth.h"
#include "chorus.h"
#include "equalizer.h"
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
static const DeviceDescriptor devices[]={
    {"Sampler",sizeof(Sampler),sampler_defaults,sampler_check,NULL,{{0,0},{0,0}}},
    {"FM Synth",sizeof(FMSettings),fm_defaults,fm_check,fm_parameter,{{PARAM_FM_RATIO,PARAM_FM_LAST},{PARAM_DX7_FIRST,PARAM_DX7_LAST}}},
    {"Chorus",sizeof(ChorusSettings),chorus_defaults,chorus_check,chorus_parameter,{{PARAM_CHORUS_RATE,PARAM_CHORUS_DEPTH},{PARAM_EFFECT_MIX,PARAM_EFFECT_MIX}}},
    {"Equalizer",sizeof(EQSettings),eq_defaults,eq_check,eq_parameter,{{PARAM_EQ_FIRST,PARAM_EQ_LAST},{PARAM_EFFECT_MIX,PARAM_EFFECT_MIX}}},
};
const DeviceDescriptor *device_descriptor(unsigned kind) {
    return kind>=DEVICE_SAMPLER && kind<=DEVICE_EQ?&devices[kind-1]:NULL;
}
const DeviceDescriptor *instrument_descriptor(unsigned type) {
    return type==INSTRUMENT_SAMPLER?device_descriptor(DEVICE_SAMPLER):type==INSTRUMENT_FM?device_descriptor(DEVICE_FM):NULL;
}
const DeviceDescriptor *effect_descriptor(unsigned type) {
    return type==EFFECT_CHORUS?device_descriptor(DEVICE_CHORUS):type==EFFECT_EQ?device_descriptor(DEVICE_EQ):NULL;
}
int device_has_parameter(const DeviceDescriptor *device,unsigned id) {
    if(!device || !id) return 0;
    for(unsigned i=0;i<2;i++) if(id>=device->parameters[i].first && id<=device->parameters[i].last) return 1;
    return 0;
}

const float *device_parameter(const DeviceDescriptor *device,const void *settings,unsigned id) {
    return settings && device_has_parameter(device,id) && device->parameter?device->parameter(settings,id):NULL;
}
