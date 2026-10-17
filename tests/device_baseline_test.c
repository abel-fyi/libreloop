// SPDX-License-Identifier: GPL-3.0-only
#include "sampler.h" /* The DSP API is usable without project or UI types. */
#include "engine.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
static Project project;
int main(void) {
    float pcm[]={1,-1,.5f,-.5f,.25f,-.25f,0,0};
    Sample sample={.data=pcm,.frames=4,.channels=2};
    SamplerVoice a,b; float output[2];
    sampler_voice_reset(&a,0,1); sampler_voice_reset(&b,1,2);
    sampler_voice_sample(&a,sample,1,1,0,output);
    CHECK(output[0]==1 && output[1]==-1 && a.position==1 && b.position==1);
    sampler_voice_sample(&b,sample,b.speed,1,0,output);
    CHECK(output[0]==.5f && output[1]==-.5f && b.position==3 && a.position==1);
    sampler_voice_sample(&b,sample,b.speed,1,0,output);
    sampler_voice_sample(&b,sample,b.speed,1,0,output);
    CHECK(output[0]==0 && output[1]==0);
    a.grain_ready=1; a.grains[0]=100; a.grain_phase=512;
    sampler_voice_reset(&a,0,.5);
    CHECK(a.position==0 && a.speed==.5 && !a.grain_ready && !a.tempo_rate && !a.grain_phase);
    sampler_voice_sample(&a,sample,a.speed,1,0,output);
    sampler_voice_sample(&a,sample,a.speed,1,0,output);
    CHECK(output[0]==.75f && output[1]==-.75f && a.position==1);
    project_new(&project);
    for(unsigned id=PARAM_CHANNEL_VOLUME;id<=PARAM_PITCH_RANGE;id++) {
        const ParameterDescriptor *d=parameter_descriptor(id); float value,low,high;
        CHECK(d && d->name[0] && d->initial>=d->low && d->initial<=d->high);
        CHECK(parameter_info(&project,(ParameterTarget){id,0,0},&value,&low,&high));
        CHECK(low==d->low && high==d->high && value==d->initial);
    }
    CHECK(!parameter_descriptor(PARAM_PLUGIN));
    float value,low,high; ParameterTarget target;
    CHECK(!parameter_info(&project,(ParameterTarget){PARAM_CHANNEL_VOLUME,CHANNELS,0},&value,&low,&high));
    CHECK(parameter_from_pointer(&project,&project.channel_pitch[0],&target) && target.parameter==PARAM_CHANNEL_PITCH);
    CHECK(parameter_from_pointer(&project,&project.pitch_range[0],&target) && target.parameter==PARAM_PITCH_RANGE);
    CHECK(parameter_descriptor(target.parameter)->kind==PARAMETER_INTEGER);
    puts("Independent sampler voices, stereo interpolation, reset and shared parameter contracts passed.");
    return 0;
}
