// SPDX-License-Identifier: GPL-3.0-only
#ifndef INSTRUMENT_H
#define INSTRUMENT_H
#include "device.h"
#include "sampler.h"
#include "fm_synth.h"
typedef union { SamplerVoice sampler; FMVoice fm; } InstrumentVoice;
typedef struct { const Sampler *sampler; const FMSettings *fm; } InstrumentSettings;
typedef struct { double frequency,position,speed; float velocity; } InstrumentNote;
void instrument_start(InstrumentVoice *voice,unsigned type,InstrumentSettings settings,InstrumentNote note);
void instrument_release(InstrumentVoice *voice,unsigned type,InstrumentSettings settings);
/* Direct dispatch keeps the inner audio loop free of virtual-call overhead.
   The host supplies processed PCM, tempo ratio and combined playback pitch.
   Sampler gate fades belong to the sequencer; synth releases belong to DSP. */
static inline void instrument_process(InstrumentVoice *voice,unsigned type,InstrumentSettings settings,Sample source,double master_pitch,double channel_pitch,double tempo,float stereo[2]) {
    if(type==INSTRUMENT_FM) fm_sample_stereo(&voice->fm,settings.fm,master_pitch*channel_pitch,stereo);
    else sampler_voice_sample(&voice->sampler,source,voice->sampler.speed*master_pitch*channel_pitch,tempo,settings.sampler->fit_bpm && settings.sampler->stretch,stereo);
}
static inline int instrument_active(const InstrumentVoice *voice,unsigned type,Sample source) {
    return type==INSTRUMENT_FM?fm_active(&voice->fm):voice->sampler.position<source.frames;
}
#endif
