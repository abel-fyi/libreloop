// SPDX-License-Identifier: GPL-3.0-only
#ifndef SPECTRUM_H
#define SPECTRUM_H
#define SPECTRUM_FFT 4096
#define SPECTRUM_BINS 96
/* UI-owned FFT state. Stereo power avoids cancellation of opposite-phase audio. */
typedef struct {
    float history[SPECTRUM_FFT][2],db[SPECTRUM_BINS],window[SPECTRUM_FFT];
    unsigned cursor,pending;
} Spectrum;
void spectrum_reset(Spectrum *s);
void spectrum_push(Spectrum *s,const float *stereo,unsigned frames);
void spectrum_snapshot(Spectrum *s,const float *stereo,unsigned frames,unsigned elapsed);
float spectrum_frequency(float normalized);
#endif
