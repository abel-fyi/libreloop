// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_DELAY_H
#define LIBRELOOP_DELAY_H
/* Stereo delay at the engine's fixed 48 kHz. The owner creates/frees storage
   outside rendering; reset/process never allocate or clear the delay buffer. */
#include <stddef.h>
#define DELAY_MAX_MS 4000
#define DELAY_DIVISIONS 9
typedef struct {
    float time_ms,feedback,cutoff,stereo,output;
    unsigned sync,division;
} DelaySettings;
typedef struct Delay Delay;
DelaySettings delay_default(void);
int delay_valid(DelaySettings settings);
const char *delay_division_name(unsigned division);
float delay_time_ms(DelaySettings settings,float bpm);
size_t delay_storage_bytes(void);
Delay *delay_create(void);
void delay_free(Delay *delay);
void delay_reset(Delay *delay);
int delay_audible(const Delay *delay);
void delay_process(Delay *delay,DelaySettings settings,float bpm,float wet,float stereo[2]);
#endif
