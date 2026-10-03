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
/* The callback owns rendering state. The UI only writes this mailbox. */
enum { UPDATE, SAMPLE, CHANNEL_SAMPLES, PREVIEW, NOTE, KEY, STOP };
#define AUDIO_COMMANDS 256
typedef struct {
    int kind,channel,slot,pitch,down,run,song,pattern,reset;
    float output,start,loop_start,loop_end;
    Note note;
    Sample sample,channels[CHANNELS];
    uint64_t serial;
} AudioCommand;
static struct {
    Project project;
    int dirty;
    AudioCommand commands[AUDIO_COMMANDS];
    unsigned read,write;
    uint64_t serial;
} pending;
static struct {
    Sample preview;
    double preview_position,preview_frame,preview_time,preview_rate;
    unsigned preview_end,preview_remaining;
    Voice voices[128];
    unsigned sample_frames[CHANNELS],live_frames,visual_frames;
    double speeds[CHANNELS],live_time,visual_frame,visual_time,visual_start,visual_end;
    int playing;
} view;
static atomic_uint_fast64_t acknowledged;
static atomic_int stopped_active;
static void consume_commands(void);
static void publish_view(void);
static ma_device device;
static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
static Project project;
static Sample samples[CHANNELS];
static Sample preview;
static double preview_position,preview_speed=1,preview_frame,preview_time,preview_rate=1;
static float preview_gain=.7f;
static unsigned preview_end;
static unsigned preview_remaining;
static Player player,live;
static int playing, ready;
static float output_volume=1;
static atomic_uint_fast64_t position;
static _Atomic float meter_peak[INSERTS+1][2];
static atomic_int meter_active,metronome;
static atomic_uchar track_active[LANES],track_trigger[LANES];
static float clicks[2][1200];
static unsigned click_position=1200;
static int click_accent;
static double visual_frame,visual_time,visual_end,visual_start;
static unsigned visual_frames,live_frames;
static double live_time;
#define RECORD_RING_FRAMES (RATE*4u)
typedef struct {
    float *pcm;
    atomic_uint_fast64_t read,write;
    atomic_int overflow;
} RecordRing;
static struct {
    ma_device devices[CHANNELS];
    RecordRing inputs[CHANNELS],takes[CHANNELS];
    int sources[CHANNELS],buses[CHANNELS],count,devices_count,active;
    MixerIO io;
} recording;
static int ring_push(RecordRing *ring,float left,float right) {
    uint64_t w=atomic_load_explicit(&ring->write,memory_order_relaxed);
    if(w-atomic_load_explicit(&ring->read,memory_order_acquire)>=RECORD_RING_FRAMES) {
        atomic_store(&ring->overflow,1); return 0;
    }
    unsigned at=w%RECORD_RING_FRAMES;
    ring->pcm[at*2]=isfinite(left)?left:0; ring->pcm[at*2+1]=isfinite(right)?right:0;
    atomic_store_explicit(&ring->write,w+1,memory_order_release); return 1;
}
static int ring_pop(RecordRing *ring,float stereo[2]) {
    uint64_t r=atomic_load_explicit(&ring->read,memory_order_relaxed);
    if(r==atomic_load_explicit(&ring->write,memory_order_acquire)) return 0;
    unsigned at=r%RECORD_RING_FRAMES;
    stereo[0]=ring->pcm[at*2]; stereo[1]=ring->pcm[at*2+1];
    atomic_store_explicit(&ring->read,r+1,memory_order_release); return 1;
}
static void capture_callback(ma_device *d,void *out,const void *in,ma_uint32 frames) {
    (void)out; RecordRing *ring=d->pUserData; const float *pcm=in;
    for(unsigned i=0;i<frames;i++) ring_push(ring,pcm?pcm[i*2]:0,pcm?pcm[i*2+1]:0);
}
static void record_input(void *context,float buses[INSERTS+1][2]) {
    (void)context; float input[CHANNELS][2]={{0}};
    for(int i=0;i<recording.devices_count;i++) ring_pop(&recording.inputs[i],input[i]);
    for(int i=0;i<recording.count;i++) if(recording.sources[i]>=0) {
        int id=recording.buses[i],source=recording.sources[i];
        buses[id][0]+=input[source][0]; buses[id][1]+=input[source][1];
    }
}
static void record_output(void *context,int bus,float left,float right) {
    (void)context;
    for(int i=0;i<recording.count;i++) if(recording.buses[i]==bus) ring_push(&recording.takes[i],left,right);
}
static double monotonic_time(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t);
    return t.tv_sec+t.tv_nsec/1e9;
}
static void callback(ma_device *d, void *out, const void *in, ma_uint32 frames) {
    (void)d; (void)in;
    memset(out,0,frames*2*sizeof(float));
    /* A busy UI delays edits, never playback. No UI code touches render state. */
    if(!pthread_mutex_trylock(&mutex)) {
        consume_commands();
        pthread_mutex_unlock(&mutex);
    }
    float peaks[INSERTS+1][2]={{0}};
    if(playing) {
        visual_frame=player.frame;
        render_mixer_io(&player,&live,&project,samples,out,frames,1,peaks,recording.active?&recording.io:NULL);
        visual_time=monotonic_time(); visual_frames=frames;
        double step_frames=RATE*60.0/project.bpm/4;
        visual_start=(player.loop_end?player.loop_start:player.start_step)*step_frames;
        float end=player.loop_end?player.loop_end:player.song?song_steps(&project):project.pattern_steps[player.pattern];
        if(visual_start>=end*step_frames) visual_start=0;
        visual_end=recording.active?INFINITY:end*step_frames;
    }
    else render_mixer_io(&live,NULL,&project,samples,out,frames,0,peaks,recording.active?&recording.io:NULL);
    for(int l=0;l<LANES;l++) {
        atomic_store_explicit(&track_active[l],playing && player.song && player.lane_active[l],memory_order_relaxed);
        if(playing && player.song && player.lane_trigger[l]) atomic_store_explicit(&track_trigger[l],1,memory_order_relaxed);
    }
    live_time=monotonic_time(); live_frames=frames;
    float *buffer=out;
    double pitch_speed=pow(2,project.master_pitch/12.0);
    if(preview_position<preview_end) { preview_frame=preview_position; preview_time=live_time; preview_rate=pitch_speed*preview_speed; }
    for(unsigned i=0;i<frames && preview_position<preview_end && preview_remaining;i++,preview_position+=pitch_speed*preview_speed,preview_remaining--) {
        unsigned n=(unsigned)preview_position;
        for(unsigned side=0;side<2;side++) {
            float a=sample_at(preview,n,side),b=sample_at(preview,n+1,side);
            float x=(a+(b-a)*(preview_position-n))*preview_gain;
            buffer[i*2+side]+=x;
        }
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
                buffer[i*2]+=x;
                buffer[i*2+1]+=x;
            }
        }
    } else click_position=1200;
    for(unsigned i=0;i<frames*2;i++) {
        peaks[0][i%2]=fmaxf(peaks[0][i%2],fabsf(buffer[i]));
        float x=buffer[i]*output_volume;
        peaks[0][i%2]=fmaxf(peaks[0][i%2],fabsf(x));
        buffer[i]=isfinite(x)?fmaxf(-1,fminf(1,x)):0;
    }
    int active=0;
    for(int id=0;id<=INSERTS;id++) for(int side=0;side<2;side++) {
        float peak=peaks[id][side],old=atomic_load_explicit(&meter_peak[id][side],memory_order_relaxed);
        active|=peak>.00001f;
        while(peak>old && !atomic_compare_exchange_weak_explicit(&meter_peak[id][side],&old,peak,memory_order_relaxed,memory_order_relaxed)) {}
    }
    atomic_store_explicit(&meter_active,active,memory_order_relaxed);
    atomic_store(&position,player.frame);
    if(!pthread_mutex_trylock(&mutex)) {
        publish_view();
        pthread_mutex_unlock(&mutex);
    }
}
static void publish_view(void) {
    view.preview=preview; view.preview_position=preview_position;
    view.preview_frame=preview_frame; view.preview_time=preview_time; view.preview_rate=preview_rate;
    view.preview_end=preview_end; view.preview_remaining=preview_remaining;
    memcpy(view.voices,live.voices,sizeof view.voices);
    double pitch_speed=pow(2,project.master_pitch/12.0);
    for(int c=0;c<CHANNELS;c++) {
        view.sample_frames[c]=samples[c].frames;
        view.speeds[c]=pitch_speed*channel_speed(&project,c);
    }
    view.live_frames=live_frames; view.live_time=live_time;
    view.visual_frames=visual_frames; view.visual_frame=visual_frame; view.visual_time=visual_time;
    view.visual_start=visual_start; view.visual_end=visual_end; view.playing=playing;
}
static void consume_commands(void) {
    if(pending.dirty) { project=pending.project; pending.dirty=0; }
    uint64_t serial=0;
    while(pending.read!=pending.write) {
        AudioCommand *c=&pending.commands[pending.read++%AUDIO_COMMANDS];
        switch(c->kind) {
        case UPDATE:
            playing=c->run; output_volume=c->output;
            if(c->reset || !playing || player.song!=c->song || player.pattern!=c->pattern) {
                click_position=1200; player_reset(&player); player_seek(&player,&project,c->start);
                atomic_store(&position,player.frame); visual_frame=player.frame; visual_frames=0;
            }
            player.song=c->song; player.pattern=c->pattern;
            player.loop_start=isfinite(c->loop_start)?fmaxf(0,c->loop_start):0;
            player.loop_end=isfinite(c->loop_end) && c->loop_end>player.loop_start?c->loop_end:0;
            if(playing && player.loop_end && player.frame>=(uint64_t)llround(player.loop_end*RATE*60.0/project.bpm/4)) {
                player.frame=(uint64_t)llround(player.loop_start*RATE*60.0/project.bpm/4);
                player.last_step=-1; memset(player.voices,0,sizeof player.voices);
            }
            break;
        case SAMPLE: case CHANNEL_SAMPLES:
            if(c->kind==SAMPLE) samples[c->channel]=c->sample;
            else memcpy(samples,c->channels,sizeof samples);
            preview=(Sample){0}; preview_end=preview_remaining=0;
            memset(player.voices,0,sizeof player.voices); memset(live.voices,0,sizeof live.voices);
            break;
        case PREVIEW:
            preview=c->sample; preview_position=preview_frame=0; preview_time=monotonic_time();
            preview_speed=preview_rate=1; preview_gain=.7f; preview_end=preview.frames; preview_remaining=5*RATE;
            break;
        case NOTE:
            if(c->channel>=0 && c->channel<project.channel_count) {
                preview=samples[c->channel]; preview_position=preview_frame=0; preview_time=monotonic_time();
                preview_remaining=UINT_MAX; preview_speed=pow(2,((int)c->note.pitch-60)/12.0)*channel_speed(&project,c->channel);
                preview_rate=preview_speed*pow(2,project.master_pitch/12.0);
                preview_gain=c->note.velocity/127.f*project.volume[c->channel]*project.master;
                preview_end=fmin(preview.frames,preview_speed*RATE*60.0/project.bpm/4*(c->note.length?c->note.length:1));
            }
            break;
        case KEY:
            if(c->down && c->channel>=0 && c->channel<project.channel_count)
                live.voices[c->slot]=(Voice){c->channel,0,pow(2,(c->pitch-60)/12.0),-1,100/127.f,-1};
            else if(live.voices[c->slot].gain) live.voices[c->slot].remaining=RATE*.005;
            break;
        case STOP: {
            int active=playing || (preview_remaining && preview_position<preview_end);
            for(int i=0;i<128;i++) active|=live.voices[i].gain!=0;
            atomic_store(&stopped_active,active);
            playing=0; preview=(Sample){0}; preview_end=preview_remaining=0; click_position=1200;
            memset(player.voices,0,sizeof player.voices); memset(live.voices,0,sizeof live.voices);
            break;
        }
        }
        serial=c->serial;
    }
    if(serial) {
        publish_view();
        /* Replacement callers may now release samples no voice can reference. */
        atomic_store(&acknowledged,serial);
    }
}
static void wait_control(void) {
    struct timespec pause={0,100000}; nanosleep(&pause,NULL);
}
/* UI only, with the mailbox locked. Prevent restart during an offline flush. */
static int consume_stopped(void) {
    if(!ready) { consume_commands(); publish_view(); return 1; }
    if(ma_device_get_state(&device)!=ma_device_state_stopped) return 0;
    ma_mutex_lock(&device.startStopLock);
    int stopped=ma_device_get_state(&device)==ma_device_state_stopped;
    if(stopped) { consume_commands(); publish_view(); }
    ma_mutex_unlock(&device.startStopLock);
    return stopped;
}
/* Called with the mailbox locked; only UI producers may wait for queue space. */
static uint64_t submit(AudioCommand command) {
    while(pending.write-pending.read==AUDIO_COMMANDS) {
        if(consume_stopped()) break;
        pthread_mutex_unlock(&mutex); wait_control(); pthread_mutex_lock(&mutex);
    }
    command.serial=++pending.serial;
    pending.commands[pending.write++%AUDIO_COMMANDS]=command;
    consume_stopped();
    return command.serial;
}
static void wait_acknowledged(uint64_t serial) {
    while(atomic_load(&acknowledged)<serial) {
        if(ma_device_get_state(&device)==ma_device_state_stopped) {
            pthread_mutex_lock(&mutex); consume_stopped(); pthread_mutex_unlock(&mutex);
        } else wait_control();
    }
}
int audio_start(const Project *p,const Sample s[CHANNELS]) {
    for(int accent=0;accent<2;accent++) for(int i=0;i<1200;i++)
        clicks[accent][i]=.25f*sinf(6.2831853f*(accent?1800:1200)*i/RATE)*expf(-i/180.f);
    memset(&pending,0,sizeof pending); memset(&view,0,sizeof view);
    atomic_store(&acknowledged,0); atomic_store(&position,0);
    project=pending.project=*p; memcpy(samples,s,sizeof samples);
    player_reset(&player); player_reset(&live); playing=0; preview=(Sample){0}; preview_end=preview_remaining=0;
    visual_frames=live_frames=0; output_volume=1; publish_view();
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
void audio_record_end(void) {
    if(!recording.active) return;
    ma_device_stop(&device); recording.active=0;
    for(int i=0;i<recording.devices_count;i++) ma_device_uninit(&recording.devices[i]);
    if(ma_device_start(&device)!=MA_SUCCESS) ready=0;
}
static void record_release(void) {
    for(int i=0;i<CHANNELS;i++) { free(recording.inputs[i].pcm); free(recording.takes[i].pcm); }
    memset(&recording,0,sizeof recording);
}
int audio_record_start(const Project *p,const int buses[],int count,float start_step,float output,char error[256]) {
    if(!ready || count<1 || count>CHANNELS || recording.active) { snprintf(error,256,"Audio unavailable or invalid recording tracks."); return 0; }
    ma_device_stop(&device); record_release();
    ma_device_info *outputs,*inputs; ma_uint32 output_count,input_count;
    if(ma_context_get_devices(device.pContext,&outputs,&output_count,&inputs,&input_count)!=MA_SUCCESS) goto failed;
    char names[CHANNELS][128]={{0}};
    for(int i=0;i<count;i++) {
        int bus=buses[i]; if(bus<0 || bus>p->insert_count) goto failed;
        recording.buses[i]=bus; recording.sources[i]=-1; recording.io.active[bus]=1;
        recording.takes[i].pcm=calloc(RECORD_RING_FRAMES*2,sizeof(float));
        if(!recording.takes[i].pcm) goto failed;
        const char *name=p->audio_io[bus][0]; if(!*name) continue;
        int source=0; while(source<recording.devices_count && strcmp(names[source],name)) source++;
        recording.sources[i]=source;
        if(source<recording.devices_count) continue;
        ma_device_id *id=NULL;
        if(strcmp(name,"@default")) {
            for(unsigned n=0;n<input_count;n++) if(!strcmp(inputs[n].name,name)) { id=&inputs[n].id; break; }
            if(!id) { snprintf(error,256,"Input device unavailable: %.120s",name); goto failed; }
        }
        RecordRing *ring=&recording.inputs[source]; ring->pcm=calloc(RECORD_RING_FRAMES*2,sizeof(float));
        if(!ring->pcm) goto failed;
        ma_device_config config=ma_device_config_init(ma_device_type_capture);
        config.capture.pDeviceID=id; config.capture.format=ma_format_f32; config.capture.channels=2;
        config.sampleRate=RATE; config.dataCallback=capture_callback; config.pUserData=ring;
        if(ma_device_init(device.pContext,&config,&recording.devices[source])!=MA_SUCCESS) {
            snprintf(error,256,"Cannot open input %.120s. Check microphone permission and device availability.",name); goto failed;
        }
        recording.devices_count++; snprintf(names[source],128,"%s",name);
    }
    recording.count=count; recording.io.input=record_input; recording.io.output=record_output;
    for(int i=0;i<recording.devices_count;i++) if(ma_device_start(&recording.devices[i])!=MA_SUCCESS) goto failed;
    /* Commit transport while playback is stopped, so its first frame is also recorded. */
    audio_update(p,1,1,player.pattern,1,output,start_step,0,0);
    recording.active=1;
    if(ma_device_start(&device)==MA_SUCCESS) return 1;
    recording.active=0;
failed:
    if(!*error) snprintf(error,256,"Could not start recording: device or memory unavailable.");
    for(int i=0;i<recording.devices_count;i++) ma_device_uninit(&recording.devices[i]);
    record_release(); if(ma_device_start(&device)!=MA_SUCCESS) ready=0; return 0;
}
unsigned audio_record_read(int take,float *stereo,unsigned frames) {
    if(take<0 || take>=recording.count) return 0;
    unsigned n=0; while(n<frames && ring_pop(&recording.takes[take],stereo+n*2)) n++;
    return n;
}
int audio_record_failed(void) {
    if(recording.active) {
        if(ma_device_get_state(&device)!=ma_device_state_started) return 1;
        for(int i=0;i<recording.devices_count;i++) if(ma_device_get_state(&recording.devices[i])!=ma_device_state_started) return 1;
    }
    for(int i=0;i<recording.count;i++) if(atomic_load(&recording.takes[i].overflow)) return 1;
    for(int i=0;i<recording.devices_count;i++) if(atomic_load(&recording.inputs[i].overflow)) return 1;
    return 0;
}
void audio_update(const Project *p,int run,int song,int pattern,int reset,float output,float start_step,float loop_start,float loop_end) {
    pthread_mutex_lock(&mutex);
    pending.project=*p; pending.dirty=1;
    submit((AudioCommand){.kind=UPDATE,.run=run,.song=song,.pattern=pattern,.reset=reset,
        .output=output,.start=start_step,.loop_start=loop_start,.loop_end=loop_end});
    pthread_mutex_unlock(&mutex);
}
void audio_sample(int c,Sample s) {
    if(c<0 || c>=CHANNELS) return;
    pthread_mutex_lock(&mutex);
    uint64_t serial=submit((AudioCommand){.kind=SAMPLE,.channel=c,.sample=s});
    pthread_mutex_unlock(&mutex); wait_acknowledged(serial);
}
void audio_channels(const Project *p,const Sample s[CHANNELS]) {
    AudioCommand command={.kind=CHANNEL_SAMPLES}; memcpy(command.channels,s,sizeof command.channels);
    pthread_mutex_lock(&mutex); pending.project=*p; pending.dirty=1;
    uint64_t serial=submit(command);
    pthread_mutex_unlock(&mutex); wait_acknowledged(serial);
}
void audio_preview(Sample s) {
    pthread_mutex_lock(&mutex);
    uint64_t serial=submit((AudioCommand){.kind=PREVIEW,.sample=s});
    pthread_mutex_unlock(&mutex); wait_acknowledged(serial);
}
void audio_note(int c,Note n) {
    pthread_mutex_lock(&mutex); submit((AudioCommand){.kind=NOTE,.channel=c,.note=n}); pthread_mutex_unlock(&mutex);
}
void audio_key(int slot,int channel,int pitch,int down) {
    if(slot<0 || slot>=128 || pitch<0 || pitch>127) return;
    pthread_mutex_lock(&mutex);
    submit((AudioCommand){.kind=KEY,.slot=slot,.channel=channel,.pitch=pitch,.down=down});
    pthread_mutex_unlock(&mutex);
}
int audio_stop(void) {
    pthread_mutex_lock(&mutex); uint64_t serial=submit((AudioCommand){.kind=STOP}); pthread_mutex_unlock(&mutex);
    wait_acknowledged(serial); return atomic_load(&stopped_active);
}
double audio_preview_position(Sample sample) {
    pthread_mutex_lock(&mutex);
    double progress=-1;
    if(ready && sample.frames && view.preview.data==sample.data && view.preview_remaining) {
        double frame=fmax(0,fmin(view.preview_position,view.preview_frame+(monotonic_time()-view.preview_time)*RATE*view.preview_rate));
        if(frame<view.preview_end) progress=fmin(1,frame/sample.frames);
    }
    pthread_mutex_unlock(&mutex); return progress;
}
double audio_key_position(int slot,int channel) {
    if(slot<0 || slot>=128 || channel<0 || channel>=CHANNELS) return -1;
    pthread_mutex_lock(&mutex);
    Voice v=view.voices[slot]; double progress=-1;
    int queued=0;
    /* Keep a new key visible immediately so an idle sampler keeps animating. */
    for(unsigned i=pending.read;i!=pending.write;i++) {
        AudioCommand *c=&pending.commands[i%AUDIO_COMMANDS];
        if(c->kind==STOP || c->kind==SAMPLE || c->kind==CHANNEL_SAMPLES) v.gain=0;
        if(c->kind==KEY && c->slot==slot) {
            if(c->down && c->channel>=0 && c->channel<pending.project.channel_count) {
                v=(Voice){.channel=c->channel,.gain=1}; queued=1;
            }
        }
    }
    if(v.gain && v.channel==channel && view.sample_frames[channel]) {
        double behind=fmax(0,view.live_frames-(monotonic_time()-view.live_time)*RATE);
        progress=queued?0:fmin(1,fmax(0,v.position-behind*v.speed*view.speeds[channel])/view.sample_frames[channel]);
    }
    pthread_mutex_unlock(&mutex); return progress;
}
uint64_t audio_position(void) { return atomic_load(&position); }
void audio_meters(float peaks[INSERTS+1][2]) {
    for(int id=0;id<=INSERTS;id++) for(int side=0;side<2;side++)
        peaks[id][side]=atomic_exchange_explicit(&meter_peak[id][side],0,memory_order_relaxed);
}
void audio_track_activity(uint8_t active[LANES],uint8_t triggered[LANES]) {
    for(int l=0;l<LANES;l++) {
        active[l]=atomic_load_explicit(&track_active[l],memory_order_relaxed);
        triggered[l]=atomic_exchange_explicit(&track_trigger[l],0,memory_order_relaxed);
    }
}
int audio_active(void) { return atomic_load_explicit(&meter_active,memory_order_relaxed); }
double audio_visual_position(void) {
    pthread_mutex_lock(&mutex);
    double frame=view.visual_frame;
    if(view.playing && view.visual_frames) {
        /* Animate within the last rendered buffer, never beyond available audio. */
        frame+=fmin(view.visual_frames,fmax(0,(monotonic_time()-view.visual_time)*RATE));
        if(frame>=view.visual_end && view.visual_end>view.visual_start)
            frame=view.visual_start+fmod(frame-view.visual_start,view.visual_end-view.visual_start);
    }
    pthread_mutex_unlock(&mutex); return frame;
}
void audio_close(void) {
    audio_record_end(); record_release();
    if(ma_device_get_state(&device)!=ma_device_state_uninitialized) ma_device_uninit(&device);
    ready=0;
    pthread_mutex_lock(&mutex); consume_commands(); publish_view(); pthread_mutex_unlock(&mutex);
}
int sample_load(const char *path,Sample *s) {
    ma_decoder decoder;
    ma_decoder_config config=ma_decoder_config_init(ma_format_f32,0,RATE);
    if(ma_decoder_init_file(path,&config,&decoder)!=MA_SUCCESS) return 0;
    unsigned channels=decoder.outputChannels;
    ma_uint64 frames=0,read=0;
    int ok=ma_decoder_get_length_in_pcm_frames(&decoder,&frames)==MA_SUCCESS && frames>0 && frames<=SAMPLE_MAX_FRAMES && (channels==1 || channels==2) && frames<=SIZE_MAX/(channels*sizeof(float));
    float *data=ok?malloc((size_t)frames*channels*sizeof(float)):NULL;
    ok=data && ma_decoder_read_pcm_frames(&decoder,data,frames,&read)==MA_SUCCESS && read==frames;
    ma_decoder_uninit(&decoder);
    if(!ok) { free(data); return 0; }
    *s=(Sample){data,(unsigned)frames,channels}; return 1;
}
