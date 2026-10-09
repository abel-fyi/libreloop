// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include "preset.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
int main(void) {
    DevicePreset p={0};
    void *settings[]={&p.sampler,&p.fm,&p.chorus,&p.eq};
    for(unsigned id=DEVICE_SAMPLER;id<=DEVICE_EQ;id++) {
        const DeviceDescriptor *d=device_descriptor(id); CHECK(d && d->name && d->settings_size);
        d->defaults(settings[id-1]); CHECK(d->valid(settings[id-1]) && !d->valid(NULL));
    }
    CHECK(!device_descriptor(0) && !device_descriptor(99) && !instrument_descriptor(99) && !effect_descriptor(EFFECT_EMPTY));
    CHECK(!strcmp(instrument_descriptor(INSTRUMENT_FM)->name,"FM Synth"));
    CHECK(device_has_parameter(effect_descriptor(EFFECT_EQ),PARAM_EFFECT_MIX));
    CHECK(!device_has_parameter(effect_descriptor(EFFECT_EQ),PARAM_CHORUS_RATE));
    CHECK(device_has_parameter(instrument_descriptor(INSTRUMENT_FM),PARAM_DX7_LAST));
    CHECK(!device_has_parameter(instrument_descriptor(INSTRUMENT_SAMPLER),PARAM_FM_RATIO));
    CHECK(device_parameter(effect_descriptor(EFFECT_EQ),&p.eq,PARAM_EQ_FIRST+7)==&p.eq.bands[2].gain);
    CHECK(device_parameter(effect_descriptor(EFFECT_CHORUS),&p.chorus,PARAM_CHORUS_RATE)==&p.chorus.rate);
    CHECK(!device_parameter(effect_descriptor(EFFECT_CHORUS),&p.chorus,PARAM_EQ_FIRST));
    CHECK(!device_parameter(NULL,NULL,PARAM_FM_RATIO));
    p.fm=fm_epiano(); p.sampler=(Sampler){.time=1,.length=1};
    InstrumentSettings controls={&p.sampler,&p.fm}; InstrumentVoice voice;
    FMVoice reference; fm_note_on_velocity(&reference,220,p.fm,.73f);
    instrument_start(&voice,INSTRUMENT_FM,controls,(InstrumentNote){.frequency=220,.velocity=.73f});
    Sample empty={0}; float stereo[2],expected[2];
    for(int n=0;n<5000;n++) {
        if(n==1000) { instrument_release(&voice,INSTRUMENT_FM,controls); fm_note_off(&reference,p.fm); }
        instrument_process(&voice,INSTRUMENT_FM,controls,empty,1.13,1,1,stereo);
        fm_sample_stereo(&reference,&p.fm,1.13,expected);
        CHECK(!memcmp(stereo,expected,sizeof stereo) && instrument_active(&voice,INSTRUMENT_FM,empty)==fm_active(&reference));
    }
    float pcm[2048]; for(int i=0;i<2048;i++) pcm[i]=sinf(i*.1f)*.1f;
    Sample sample={pcm,2048,1}; SamplerVoice sampler;
    sampler_voice_reset(&sampler,4.25,1.1); instrument_start(&voice,INSTRUMENT_SAMPLER,controls,(InstrumentNote){.position=4.25,.speed=1.1});
    for(int n=0;n<500;n++) {
        instrument_process(&voice,INSTRUMENT_SAMPLER,controls,sample,.75,1.137,1,stereo);
        sampler_voice_sample(&sampler,sample,1.1*.75*1.137,1,0,expected);
        CHECK(!memcmp(stereo,expected,sizeof stereo));
    }
    p.fm.depth=NAN; CHECK(!device_descriptor(DEVICE_FM)->valid(&p.fm));
    puts("Shared device defaults, validation, parameter ownership and instrument DSP equivalence passed."); return 0;
}
