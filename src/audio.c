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
static double preview_position,preview_speed=1,preview_frame,preview_time,preview_rate=1;
static float preview_gain=.7f;
static unsigned preview_end;
static Player player,live;
static int playing, ready;
static float output_volume=1;
static atomic_uint_fast64_t position;
static _Atomic float meter_peak[INSERTS+1][2];
static atomic_int meter_active,metronome;
static float clicks[2][1200];
static unsigned click_position=1200;
static int click_accent;
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
    float peaks[INSERTS+1][2]={{0}};
    if(playing) {
        visual_frame=player.frame;
        render_mixer(&player,&live,&project,samples,out,frames,1,peaks);
        visual_time=monotonic_time(); visual_frames=frames;
        double step_frames=RATE*60.0/project.bpm/4;
        visual_start=(player.loop_end?player.loop_start:player.start_step)*step_frames;
        float end=player.loop_end?player.loop_end:player.song?song_steps(&project):project.pattern_steps[player.pattern];
        if(visual_start>=end*step_frames) visual_start=0;
        visual_end=end*step_frames;
    }
    else render_mixer(&live,NULL,&project,samples,out,frames,0,peaks);
    live_time=monotonic_time(); live_frames=frames;
    float *buffer=out;
    double pitch_speed=pow(2,project.master_pitch/12.0);
    if(preview_position<preview_end) { preview_frame=preview_position; preview_time=live_time; preview_rate=pitch_speed*preview_speed; }
    for(unsigned i=0;i<frames && preview_position<preview_end;i++,preview_position+=pitch_speed*preview_speed) {
        unsigned n=(unsigned)preview_position;
        float a=preview.data[n],b=n+1<preview.frames?preview.data[n+1]:0;
        float x=(a+(b-a)*(preview_position-n))*preview_gain;
        buffer[i*2]=tanhf(buffer[i*2]+x); buffer[i*2+1]=tanhf(buffer[i*2+1]+x);
    }
    if(playing && atomic_load_explicit(&metronome,memory_order_relaxed)) {
        double beat_frames=RATE*60.0/project.bpm;
        for(unsigned i=0;i<frames;i++) {
            double frame=visual_frame+i;
            if(frame>=visual_end) frame=visual_start+fmod(frame-visual_end,visual_end-visual_start);
            double beat=floor(frame/beat_frames),phase=frame-beat*beat_frames;
            if(phase<1) { click_position=0; click_accent=fmod(beat,4)==0; }
            if(click_position<1200) {
                float x=clicks[click_accent][click_position++];
                buffer[i*2]=fmaxf(-1,fminf(1,buffer[i*2]+x));
                buffer[i*2+1]=fmaxf(-1,fminf(1,buffer[i*2+1]+x));
            }
        }
    } else click_position=1200;
    for(unsigned i=0;i<frames*2;i++) {
        peaks[0][i%2]=fmaxf(peaks[0][i%2],fabsf(buffer[i]));
        buffer[i]*=output_volume;
    }
    int active=0;
    for(int id=0;id<=INSERTS;id++) for(int side=0;side<2;side++) {
        float peak=peaks[id][side],old=atomic_load_explicit(&meter_peak[id][side],memory_order_relaxed);
        active|=peak>.00001f;
        while(peak>old && !atomic_compare_exchange_weak_explicit(&meter_peak[id][side],&old,peak,memory_order_relaxed,memory_order_relaxed)) {}
    }
    atomic_store_explicit(&meter_active,active,memory_order_relaxed);
    atomic_store(&position,player.frame);
    pthread_mutex_unlock(&mutex);
}
int audio_start(const Project *p,const Sample s[CHANNELS]) {
    for(int accent=0;accent<2;accent++) for(int i=0;i<1200;i++)
        clicks[accent][i]=.25f*sinf(6.2831853f*(accent?1800:1200)*i/RATE)*expf(-i/180.f);
    project=*p; memcpy(samples,s,sizeof samples); player_reset(&player);
    ma_device_config config=ma_device_config_init(ma_device_type_playback);
    config.playback.format=ma_format_f32; config.playback.channels=2;
    config.sampleRate=RATE; config.dataCallback=callback;
    if(ma_device_init(NULL,&config,&device)!=MA_SUCCESS) return 0;
    if(ma_device_start(&device)!=MA_SUCCESS) { ma_device_uninit(&device); return 0; }
    ready=1; return 1;
}
int audio_devices(int capture,char names[][128],int capacity) {
    if(!ready || capacity<=0) return 0;
    ma_device_info *outputs,*inputs; ma_uint32 output_count,input_count;
    if(ma_context_get_devices(device.pContext,&outputs,&output_count,&inputs,&input_count)!=MA_SUCCESS) return 0;
    ma_device_info *devices=capture?inputs:outputs; unsigned count=capture?input_count:output_count;
    int n=count<(unsigned)capacity?(int)count:capacity;
    for(int i=0;i<n;i++) snprintf(names[i],128,"%s",devices[i].name);
    return n;
}
void audio_metronome(int enabled) { atomic_store_explicit(&metronome,enabled,memory_order_relaxed); }
void audio_update(const Project *p,int run,int song,int pattern,int reset,float output,float start_step,float loop_start,float loop_end) {
    pthread_mutex_lock(&mutex);
    project=*p; playing=run; output_volume=output;
    if(reset || !run || player.song!=song || player.pattern!=pattern) { click_position=1200; player_reset(&player); player_seek(&player,&project,start_step); atomic_store(&position,player.frame); visual_frame=player.frame; visual_frames=0; }
    player.song=song; player.pattern=pattern;
    player.loop_start=isfinite(loop_start)?fmaxf(0,loop_start):0; player.loop_end=isfinite(loop_end) && loop_end>player.loop_start?loop_end:0;
    if(run && player.loop_end && player.frame>=(uint64_t)llround(player.loop_end*RATE*60.0/project.bpm/4)) {
        player.frame=(uint64_t)llround(player.loop_start*RATE*60.0/project.bpm/4); player.last_step=-1; memset(player.voices,0,sizeof player.voices);
    }
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
    pthread_mutex_lock(&mutex); preview=s; preview_position=preview_frame=0; preview_time=monotonic_time(); preview_speed=1; preview_gain=.7f; preview_end=s.frames; pthread_mutex_unlock(&mutex);
}
double audio_preview_position(Sample sample) {
    pthread_mutex_lock(&mutex);
    double progress=-1;
    if(ready && sample.frames && preview.data==sample.data) {
        double frame=fmax(0,fmin(preview_position,preview_frame+(monotonic_time()-preview_time)*RATE*preview_rate));
        if(frame<preview_end) progress=fmin(1,frame/sample.frames);
    }
    pthread_mutex_unlock(&mutex); return progress;
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
        live.voices[slot]=(Voice){channel,0,pow(2,(pitch-60)/12.0),-1,100/127.f,-1};
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
void audio_meters(float peaks[INSERTS+1][2]) {
    for(int id=0;id<=INSERTS;id++) for(int side=0;side<2;side++)
        peaks[id][side]=atomic_exchange_explicit(&meter_peak[id][side],0,memory_order_relaxed);
}
int audio_active(void) { return atomic_load_explicit(&meter_active,memory_order_relaxed); }
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
