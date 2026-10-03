// SPDX-License-Identifier: GPL-3.0-only
#ifndef AUDIO_H
#define AUDIO_H
#include "engine.h"
int audio_start(const Project *p, const Sample s[CHANNELS]);
void audio_update(const Project *p, int playing, int song, int pattern, int reset, float output_volume,float start_step,float loop_start,float loop_end);
void audio_sample(int channel, Sample sample);
void audio_channels(const Project *p,const Sample samples[CHANNELS]);
void audio_preview(Sample sample);
double audio_preview_position(Sample sample);
void audio_note(int channel,Note note);
void audio_key(int slot,int channel,int pitch,int down);
double audio_key_position(int slot,int channel);
uint64_t audio_position(void);
double audio_visual_position(void);
void audio_meters(float peaks[INSERTS+1][2]);
int audio_active(void);
int audio_devices(int capture,char names[][128],int capacity);
void audio_metronome(int enabled);
void audio_close(void);
int sample_load(const char *path, Sample *s);
#endif
