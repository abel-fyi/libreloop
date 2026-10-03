// SPDX-License-Identifier: GPL-3.0-only
#ifndef WAVEFORM_H
#define WAVEFORM_H
#include "engine.h"
typedef struct { float low,high; } WavePeak;
typedef struct { WavePeak *tree; unsigned leaves; } Waveform;
int waveform_build(Waveform *wave,Sample sample);
WavePeak waveform_range(const Waveform *wave,Sample sample,unsigned start,unsigned end);
#endif
