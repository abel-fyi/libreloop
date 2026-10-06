// SPDX-License-Identifier: GPL-3.0-only
#ifndef CHORUS_H
#define CHORUS_H
#define CHORUS_DELAY_FRAMES 1024
/* Saved controls: rate in Hz, depth in milliseconds. */
typedef struct { float rate,depth; } ChorusSettings;
typedef struct {
    float delay[CHORUS_DELAY_FRAMES][2];
    double phase;
    float rate,depth,wet;
    unsigned cursor;
    int ready;
} Chorus;
ChorusSettings chorus_default(void);
int chorus_valid(ChorusSettings settings);
void chorus_reset(Chorus *chorus);
/* Fixed 48 kHz stereo processing, no allocation. Zero reported latency for the
   direct signal; delayed wet signal is the effect, not compensation latency. */
void chorus_process(Chorus *chorus,ChorusSettings settings,float wet,float stereo[2]);
#endif
