// SPDX-License-Identifier: GPL-3.0-only
#ifndef AUDIO_H
#define AUDIO_H
#include "engine.h"
int audio_start(const Project *p, const Sample s[CHANNELS]);
/* UI requests apply at a buffer boundary; the callback never waits for the UI. */
void audio_update(const Project *p, int playing, int song, int pattern, int reset, float output_volume,float start_step,float loop_start,float loop_end);
/* Replacements/Stop acknowledge before returning so retired PCM can be freed. */
void audio_sample(int channel, Sample sample);
void audio_channels(const Project *p,const Sample samples[CHANNELS]);
void audio_preview(Sample sample);
int audio_stop(void); /* Stops all playback; returns whether anything was playing. */
double audio_preview_position(Sample sample);
void audio_note(int channel,Note note);
void audio_key(int slot,int channel,int pitch,int down);
double audio_key_position(int slot,int channel);
uint64_t audio_position(void);
double audio_visual_position(void);
void audio_meters(float peaks[INSERTS+1][2]);
/* UI drains bounded stereo taps; FFT work stays outside the callback. */
void audio_spectrum_bus(int bus);
unsigned audio_spectrum_read(int selected,float *stereo,unsigned capacity);
int audio_active(void);
void audio_channel_activity(uint8_t active[CHANNELS],uint8_t triggered[CHANNELS]);
void audio_pattern_activity(int pattern,uint8_t notes[CHANNELS][NOTES]);
void audio_track_activity(uint8_t active[LANES],uint8_t triggered[LANES]);
int audio_devices(int capture,char names[][128],int capacity);
/* One post-fader stereo take per bus. Start/end run on the UI thread. */
int audio_record_start(const Project *p,const int buses[],int count,float start_step,float output_volume,char error[256]);
void audio_record_end(void);
unsigned audio_record_read(int take,float *stereo,unsigned frames);
int audio_record_failed(void);
void audio_metronome(int enabled);
void audio_close(void);
int sample_load(const char *path, Sample *s);
#endif
