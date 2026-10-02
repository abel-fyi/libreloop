// SPDX-License-Identifier: GPL-3.0-only
#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_ENCODING
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE
#include "miniaudio.h"
#include "audio.h"
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
static ma_device device;
static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
static Project project;
static Sample samples[CHANNELS];
static Sample preview;
static double preview_position,preview_speed=1;
static float preview_gain=.7f;
static unsigned preview_end;
static Player player,live;
static int playing, ready;
static float output_volume=1;
static atomic_uint_fast64_t position;
static double visual_frame,visual_time,visual_end,visual_start;
static unsigned visual_frames,live_frames;
static double live_time;
static double monotonic_time(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t);
    return t.tv_sec+t.tv_nsec/1e9;
}
static void callback(ma_device *d, void *out, const void *in, ma_uint32 frames) {
    (void)d; (void)in;
    memset(out,0,frames*2*sizeof(float));
    /* Never wait for the interface on the real-time thread. */
    if(pthread_mutex_trylock(&mutex)) return;
    if(playing) {
        visual_frame=player.frame;
        render(&player,&project,samples,out,frames);
        visual_time=monotonic_time(); visual_frames=frames;
        double step_frames=RATE*60.0/project.bpm/4;
        visual_start=player.start_step*step_frames;
        float end=player.song?song_steps(&project):project.pattern_steps[player.pattern];
        visual_end=(end>player.start_step?end:player.start_step+STEPS)*step_frames;
    }
    render_live(&live,&project,samples,out,frames);
    live_time=monotonic_time(); live_frames=frames;
    float *buffer=out;
    double pitch_speed=pow(2,project.master_pitch/12.0);
    for(unsigned i=0;i<frames && preview_position<preview_end;i++,preview_position+=pitch_speed*preview_speed) {
        unsigned n=(unsigned)preview_position;
        float a=preview.data[n],b=n+1<preview.frames?preview.data[n+1]:0;
        float x=(a+(b-a)*(preview_position-n))*preview_gain;
        buffer[i*2]=tanhf(buffer[i*2]+x); buffer[i*2+1]=tanhf(buffer[i*2+1]+x);
    }
    for(unsigned i=0;i<frames*2;i++) buffer[i]*=output_volume;
    atomic_store(&position,player.frame);
    pthread_mutex_unlock(&mutex);
}
int audio_start(const Project *p,const Sample s[CHANNELS]) {
    project=*p; memcpy(samples,s,sizeof samples); player_reset(&player);
    ma_device_config config=ma_device_config_init(ma_device_type_playback);
    config.playback.format=ma_format_f32; config.playback.channels=2;
    config.sampleRate=RATE; config.dataCallback=callback;
    if(ma_device_init(NULL,&config,&device)!=MA_SUCCESS) return 0;
    if(ma_device_start(&device)!=MA_SUCCESS) { ma_device_uninit(&device); return 0; }
    ready=1; return 1;
}
void audio_update(const Project *p,int run,int song,int pattern,int reset,float output,float start_step) {
    pthread_mutex_lock(&mutex);
    project=*p; playing=run; output_volume=output;
    if(reset || !run || player.song!=song || player.pattern!=pattern) { player_reset(&player); player_seek(&player,&project,start_step); atomic_store(&position,player.frame); visual_frame=player.frame; visual_frames=0; }
    player.song=song; player.pattern=pattern;
    pthread_mutex_unlock(&mutex);
}
void audio_sample(int c,Sample s) {
    pthread_mutex_lock(&mutex); samples[c]=s;
    preview=(Sample){0}; preview_end=0;
    memset(player.voices,0,sizeof player.voices); memset(live.voices,0,sizeof live.voices);
    pthread_mutex_unlock(&mutex);
}
void audio_channels(const Project *p,const Sample s[CHANNELS]) {
    pthread_mutex_lock(&mutex); project=*p; memcpy(samples,s,sizeof samples);
    preview=(Sample){0}; preview_end=0;
    memset(player.voices,0,sizeof player.voices); memset(live.voices,0,sizeof live.voices); pthread_mutex_unlock(&mutex);
}
void audio_preview(Sample s) {
    pthread_mutex_lock(&mutex); preview=s; preview_position=0; preview_speed=1; preview_gain=.7f; preview_end=s.frames; pthread_mutex_unlock(&mutex);
}
void audio_note(int c,Note n) {
    pthread_mutex_lock(&mutex);
    if(c>=0 && c<project.channel_count) {
        preview=samples[c]; preview_position=0; preview_speed=pow(2,((int)n.pitch-60)/12.0);
        preview_gain=n.velocity/127.f*project.volume[c]*project.master;
        preview_end=fmin(preview.frames,preview_speed*RATE*60.0/project.bpm/4*(n.length?n.length:1));
    }
    pthread_mutex_unlock(&mutex);
}
void audio_key(int slot,int channel,int pitch,int down) {
    if(slot<0 || slot>=128 || pitch<0 || pitch>127) return;
    pthread_mutex_lock(&mutex);
    if(down && channel>=0 && channel<project.channel_count)
        live.voices[slot]=(Voice){channel,0,pow(2,(pitch-60)/12.0),-1,100/127.f};
    else if(live.voices[slot].gain) live.voices[slot].remaining=RATE*.005;
    pthread_mutex_unlock(&mutex);
}
double audio_key_position(int slot,int channel) {
    if(slot<0 || slot>=128) return -1;
    pthread_mutex_lock(&mutex);
    Voice v=live.voices[slot]; double progress=-1;
    if(v.gain && v.channel==channel && samples[channel].frames) {
        double behind=fmax(0,live_frames-(monotonic_time()-live_time)*RATE);
        progress=fmin(1,fmax(0,v.position-behind*v.speed*pow(2,project.master_pitch/12.0))/samples[channel].frames);
    }
    pthread_mutex_unlock(&mutex); return progress;
}
uint64_t audio_position(void) { return atomic_load(&position); }
double audio_visual_position(void) {
    pthread_mutex_lock(&mutex);
    double frame=visual_frame;
    if(playing && visual_frames) {
        /* Animate within the last rendered buffer, never beyond available audio. */
        frame+=fmin(visual_frames,fmax(0,(monotonic_time()-visual_time)*RATE));
        if(frame>=visual_end && visual_end>visual_start)
            frame=visual_start+fmod(frame-visual_start,visual_end-visual_start);
    }
    pthread_mutex_unlock(&mutex);
    return frame;
}
void audio_close(void) { if(ready) ma_device_uninit(&device); }
int sample_load(const char *path,Sample *s) {
    ma_decoder decoder;
    ma_decoder_config config=ma_decoder_config_init(ma_format_f32,1,RATE);
    if(ma_decoder_init_file(path,&config,&decoder)!=MA_SUCCESS) return 0;
    ma_uint64 frames=0,read=0;
    int ok=ma_decoder_get_length_in_pcm_frames(&decoder,&frames)==MA_SUCCESS && frames>0 && frames<=RATE*60;
    float *data=ok?malloc((size_t)frames*sizeof(float)):NULL;
    ok=data && ma_decoder_read_pcm_frames(&decoder,data,frames,&read)==MA_SUCCESS && read==frames;
    ma_decoder_uninit(&decoder);
    if(!ok) { free(data); return 0; }
    *s=(Sample){data,(unsigned)frames}; return 1;
}
