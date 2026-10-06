// SPDX-License-Identifier: GPL-3.0-only
#ifndef SAMPLER_H
#define SAMPLER_H
#include "sample.h"
enum { SAMPLE_NORMALIZE=1, SAMPLE_REVERSE=2, SAMPLE_POLARITY=4 };
typedef struct { float pitch,time,start,length,trim; uint8_t flags,stretch; float fit_bpm; /* reference BPM; zero disables tempo fitting */ } Sampler;
/* Per-note DSP state; no project, mixer, UI or file access. Reset specifies
   the source position and note speed, clearing tempo/grain history. */
typedef struct {
    double position,speed,tempo_rate,grains[2];
    unsigned grain_phase;
    int grain_ready;
} SamplerVoice;
void sampler_voice_reset(SamplerVoice *voice,double position,double speed);
/* Realtime: writes stereo PCM and advances source phase; never allocates.
   pitch is the total playback ratio, rate the tempo ratio. */
void sampler_voice_sample(SamplerVoice *voice,Sample sample,double pitch,double rate,int stretch,float stereo[2]);
int sampler_valid(Sampler settings);
int sampler_equal(Sampler a,Sampler b);
int sampler_processing_equal(Sampler a,Sampler b);
/* Non-owning source view: do not release it with sample_free. */
Sample sample_trim(Sample source,float threshold);
/* Offline/worker processing: may allocate. Release result with sample_free. */
int sample_process(Sample source,Sampler settings,Sample *result);
#endif
