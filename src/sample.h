// SPDX-License-Identifier: GPL-3.0-only
#ifndef SAMPLE_H
#define SAMPLE_H
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#define RATE 48000
#define SAMPLE_MAX_FRAMES (INT_MAX/8) /* Bound processing allocations. */
/* Interleaved PCM; channels 0 retains compatibility with mono initializers. */
typedef struct SampleStorage SampleStorage;
typedef struct { float *data; unsigned frames,channels; SampleStorage *storage; } Sample;
/* Release owned heap/mapped samples with sample_free. Borrowed PCM views must
   not be released; storage=NULL alone does not distinguish heap from borrowed. */
void sample_free(Sample sample);
int sample_clone(Sample source,Sample *result);
/* Map an IEEE float stereo recording without retaining a heap copy of its PCM. */
int sample_map_recording(const char *path,unsigned frames,Sample *result);
int sample_map_wav(const char *path,Sample *result);
static inline unsigned sample_channels(Sample s) { return s.channels?s.channels:1; }
static inline float sample_at(Sample s,unsigned frame,unsigned side) {
    unsigned channels=sample_channels(s);
    return frame<s.frames?s.data[(size_t)frame*channels+(channels==1?0:side)]:0;
}
#endif
