// SPDX-License-Identifier: GPL-3.0-only
#include "sampler.h"
#include <math.h>

void sampler_voice_reset(SamplerVoice *voice,double position,double speed) {
    *voice=(SamplerVoice){.position=position,.speed=speed};
}

/* Two overlapping grains, aligned on both stereo channels. State belongs to
   each voice: tempo edits never allocate, rebuild PCM or restart a voice. */
static float read_sample(Sample sample,double position,unsigned side) {
    if(position<0 || position>=sample.frames) return 0;
    unsigned i=(unsigned)position;
    float a=sample_at(sample,i,side),b=sample_at(sample,i+1,side);
    return a+(b-a)*(position-i);
}
static double match_score(Sample sample,double candidate,double reference,double pitch,double nominal) {
    double dot=0,a=1e-12,b=1e-12;
    for(int i=0;i<256;i+=8) for(unsigned side=0;side<sample_channels(sample);side++) {
        float x=read_sample(sample,candidate+i*pitch,side),y=read_sample(sample,reference+i*pitch,side);
        dot+=x*y; a+=x*x; b+=y*y;
    }
    return dot/sqrt(a*b)-fabs(candidate-nominal)*1e-6;
}
static double align_grain(Sample sample,double nominal,double reference,double pitch) {
    if(fabs(nominal-reference)<.001) return nominal;
    double best=fmax(0,fmin(sample.frames-1,nominal)),score=-2;
    for(int offset=-256;offset<=256;offset+=8) {
        double candidate=nominal+offset;
        if(candidate<0 || candidate>=sample.frames) continue;
        double value=match_score(sample,candidate,reference,pitch,nominal);
        if(value>score) { score=value; best=candidate; }
    }
    double center=best;
    for(int offset=-7;offset<=7;offset++) {
        double candidate=center+offset;
        if(candidate<0 || candidate>=sample.frames) continue;
        double value=match_score(sample,candidate,reference,pitch,nominal);
        if(value>score) { score=value; best=candidate; }
    }
    return best;
}
void sampler_voice_sample(SamplerVoice *voice,Sample sample,double pitch,double rate,int stretch,float stereo[2]) {
    if(!voice->tempo_rate) voice->tempo_rate=rate;
    /* 20 ms rate slew: keep source phase continuous even for a large BPM jump. */
    voice->tempo_rate+=(rate-voice->tempo_rate)*(1.0/(RATE*.02));
    if(stretch) {
        if(!voice->grain_ready) {
            voice->grains[0]=voice->grains[1]=voice->position;
            voice->grain_phase=0; voice->grain_ready=1;
        }
        unsigned phase=voice->grain_phase;
        if(phase==0 && voice->position!=voice->grains[0]) voice->grains[0]=align_grain(sample,voice->position,voice->grains[1],pitch);
        if(phase==512) voice->grains[1]=align_grain(sample,voice->position,voice->grains[0],pitch);
        float weight=phase<512?phase/512.f:(1024-phase)/512.f;
        for(unsigned side=0;side<2;side++) stereo[side]=read_sample(sample,voice->grains[0],side)*weight+read_sample(sample,voice->grains[1],side)*(1-weight);
        voice->grains[0]+=pitch; voice->grains[1]+=pitch;
        voice->grain_phase=(phase+1)%1024;
    } else {
        voice->grain_ready=0;
        for(unsigned side=0;side<2;side++) stereo[side]=read_sample(sample,voice->position,side);
    }
    voice->position+=pitch*voice->tempo_rate;
}
