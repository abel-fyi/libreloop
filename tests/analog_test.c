// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include <math.h>
#include <stdio.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed %d: %s\n",__LINE__,#x); return 1; } } while(0)
int main(void) {
    for(int preset=0;preset<7;preset++) CHECK(fm_valid(fm_factory(preset)));
    float difference=0;
    FMSettings s=fm_factory(5); FMVoice voice; fm_note_on_velocity(&voice,220,s,1);
    for(int i=0;i<RATE;i++) {
        float stereo[2]; fm_sample_stereo(&voice,&s,1,stereo);
        CHECK(isfinite(stereo[0]) && isfinite(stereo[1]) && fabsf(stereo[0])<.5f && fabsf(stereo[1])<.5f);
        difference+=fabsf(stereo[0]-stereo[1]);
    }
    CHECK(difference>1); fm_note_off(&voice,s);
    for(int i=0;i<RATE*2;i++) fm_sample(&voice,s,1); CHECK(!fm_active(&voice));
    for(int wave=0;wave<4;wave++) for(int resonance=0;resonance<2;resonance++) {
        s=fm_factory(6); s.sustain=1; s.analog.wave=wave; s.analog.resonance=resonance?.95f:0;
        s.analog.cutoff=20; s.analog.filter_env=6; s.analog.noise=1;
        fm_note_on(&voice,12000,s);
        for(int i=0;i<4000;i++) CHECK(isfinite(fm_sample(&voice,s,16)));
    }
    AnalogVoice raw; AnalogSettings settings=analog_default(); settings.wave=1; settings.pulse=.05f; settings.pwm=settings.filter_env=0; settings.cutoff=20000;
    analog_note_on(&raw,220); double average=0;
    for(int i=0;i<RATE*2;i++) { float x=analog_sample(&raw,settings,220,0,0); if(i>=RATE) average+=x; }
    CHECK(fabs(average/RATE)<.005); /* Pulse DC does not consume output headroom. */
    settings.cutoff=NAN; CHECK(!analog_valid(settings));
    puts("Analog factory patches, stereo chorus, envelopes, filter bounds and DC removal passed."); return 0;
}
