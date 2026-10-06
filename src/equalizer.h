// SPDX-License-Identifier: GPL-3.0-only
#ifndef EQUALIZER_H
#define EQUALIZER_H
#define EQ_BANDS 7
enum { EQ_BELL, EQ_LOW_SHELF, EQ_HIGH_SHELF, EQ_LOW_CUT, EQ_HIGH_CUT, EQ_OFF, EQ_SHAPES };
typedef struct { float frequency,gain,q; unsigned shape; } EQBand;
typedef struct { EQBand bands[EQ_BANDS]; } EQSettings;
typedef struct { double b[3],a[2],z[2][2]; } EQFilter;
typedef struct {
    EQFilter filters[EQ_BANDS],next[EQ_BANDS];
    unsigned shape[EQ_BANDS],next_shape[EQ_BANDS];
    float transition[EQ_BANDS];
    EQSettings current,target;
    float wet;
    unsigned tick;
    int ready;
} Equalizer;
EQSettings equalizer_default(void);
EQSettings equalizer_legacy(void);
const char *equalizer_shape_name(unsigned shape);
int equalizer_valid(EQSettings settings);
void equalizer_reset(Equalizer *eq);
/* Seven stereo biquads at 48 kHz; fixed storage, no callback allocation. */
void equalizer_process(Equalizer *eq,EQSettings settings,float wet,float stereo[2]);
float equalizer_response(EQSettings settings,float frequency);
#endif
