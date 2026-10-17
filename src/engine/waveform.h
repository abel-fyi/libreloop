// SPDX-License-Identifier: GPL-3.0-only
#ifndef WAVEFORM_H
#define WAVEFORM_H
#include "engine.h"
typedef struct { float low,high; } WavePeak;
typedef struct { WavePeak *tree; unsigned leaves; } Waveform;
int waveform_build(Waveform *wave,Sample sample);
int waveform_append(Waveform *wave,Sample sample,unsigned previous_frames);
WavePeak waveform_range(const Waveform *wave,Sample sample,unsigned start,unsigned end);
/* Smooth display envelope; exact analysis peaks remain available through range. */
WavePeak waveform_envelope(const Waveform *wave,Sample sample,double position,double width);
WavePeak waveform_envelope_region(const Waveform *wave,Sample sample,unsigned start,unsigned frames,double position,double width);
#endif
