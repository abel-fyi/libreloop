// SPDX-License-Identifier: GPL-3.0-only
#ifdef _WIN32
#define MA_NO_ASIO
#define MA_NO_DSOUND
#define MA_NO_WINMM
#endif
#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_ENCODING
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE
#include "miniaudio.h"
#include "audio.h"
#include "playback_plan.h"
#include <stdatomic.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "native_time.h"
/* The callback owns rendering state. The UI only writes this mailbox. */
enum { UPDATE, SAMPLE, CHANNEL_SAMPLES, PREVIEW, NOTE, KEY, STOP };
#define AUDIO_COMMANDS 256
typedef struct {
    int kind,channel,slot,pitch,down,run,song,pattern,reset;
    float output,start,loop_start,loop_end;
    Note note;
    Sample sample,channels[CHANNELS];
    uint64_t serial;
    const ProjectSnapshot *snapshot;
    unsigned snapshot_flags;
} AudioCommand;
static struct {
    ProjectSnapshot snapshot;
    int dirty;
    AudioCommand commands[AUDIO_COMMANDS];
    atomic_uint read,committed;
    unsigned write;
    uint64_t serial;
} pending;
/* UI owns the snapshot cache and reserved slots. A committed command borrows
   a slot until its serial is acknowledged. The callback never writes them. */
static struct { ProjectSnapshot value; uint64_t serial; } snapshots[8];
static unsigned control_depth;
static void control_begin(void) { control_depth++; }
static void control_end(void) {
    if(!--control_depth)
        atomic_store_explicit(&pending.committed,pending.write,memory_order_release);
}
typedef struct { int channel; float gain; struct { double position,speed; } sampler; } VoiceView;
typedef struct {
    Sample preview;
    double preview_position,preview_frame,preview_time,preview_rate;
    unsigned preview_end,preview_remaining;
    VoiceView voices[128];
    uint8_t instrument[CHANNELS];
    uint8_t pattern_notes[PATTERNS][CHANNELS][(NOTES+7)/8];
    unsigned sample_frames[CHANNELS],live_frames,visual_frames;
    double speeds[CHANNELS],live_time,visual_frame,visual_time,visual_start,visual_end;
    uint64_t note_serial;
    int playing,note_active,note_channel,note_pitch;
} AudioView;
/* Three owned buffers: callback writer, UI reader and atomic middle slot.
   Only the middle is exchanged. Neither side accesses the other owned slot. */
static AudioView view,view_slots[3];
static unsigned view_writer=0,view_reader=1;
static atomic_uint view_middle=2;
static void refresh_view(void) {
    if(atomic_load_explicit(&view_middle,memory_order_acquire)&4) {
        view_reader=atomic_exchange_explicit(&view_middle,view_reader,memory_order_acq_rel)&3;
        view=view_slots[view_reader];
    }
}
static atomic_uint_fast64_t acknowledged;
static atomic_int stopped_active;
static void consume_commands(void);
static void publish_view(void);
static ma_device device;
static Project project;
static PlaybackPlan playback;
static Sample samples[CHANNELS];
static Sample preview;
static SamplerVoice preview_stretch;
static double preview_position,preview_speed=1,preview_note_pitch=1,preview_frame,preview_time,preview_rate=1;
static float preview_gain=.7f;
static unsigned preview_end;
static unsigned preview_remaining;
static int preview_channel=-1,note_preview_channel=-1,note_preview_pitch;
static uint64_t note_preview_serial;
static Player player,live;
static EffectRack *effects;
static int playing, ready;
static atomic_int midi_recording;
void audio_record_mode(int enabled) {atomic_store(&midi_recording,!!enabled);}
static float output_volume=1;
static atomic_uint_fast64_t position;
static _Atomic float meter_peak[INSERTS+1][2];
static atomic_int meter_active,metronome;
static atomic_uchar channel_active[CHANNELS],channel_trigger[CHANNELS];
static atomic_uchar track_active[LANES],track_trigger[LANES];
static float clicks[2][1200];
static unsigned click_position=1200;
static int click_accent;
static double visual_frame,visual_time,visual_end,visual_start;
static unsigned visual_frames,live_frames;
static double live_time;
#define ANALYZER_FRAMES 8192u
typedef struct { float pcm[ANALYZER_FRAMES][2]; atomic_uint read,write; } AnalyzerRing;
static AnalyzerRing analyzer[2];
static atomic_int analyzer_bus;
static int callback_bus;
static unsigned callback_taps;
static void analyzer_push(int stream,float l,float r) {
    AnalyzerRing *ring=&analyzer[stream]; unsigned w=atomic_load_explicit(&ring->write,memory_order_relaxed);
    if(w-atomic_load_explicit(&ring->read,memory_order_acquire)>=ANALYZER_FRAMES) return;
    ring->pcm[w%ANALYZER_FRAMES][0]=isfinite(l)?l:0; ring->pcm[w%ANALYZER_FRAMES][1]=isfinite(r)?r:0;
    atomic_store_explicit(&ring->write,w+1,memory_order_release);
}
void audio_dynamics_monitor(unsigned bus,unsigned slot,DynamicsMonitor *monitor){effects_dynamics_monitor(effects,bus,slot,monitor);}
void audio_spectrum_bus(int bus) { atomic_store_explicit(&analyzer_bus,bus,memory_order_relaxed); }
unsigned audio_spectrum_read(int stream,float *pcm,unsigned capacity) {
    if(stream<0 || stream>1) return 0;
    AnalyzerRing *ring=&analyzer[stream]; unsigned r=atomic_load_explicit(&ring->read,memory_order_relaxed);
    unsigned n=atomic_load_explicit(&ring->write,memory_order_acquire)-r; if(n>capacity) n=capacity;
    for(unsigned i=0;i<n;i++) { pcm[i*2]=ring->pcm[(r+i)%ANALYZER_FRAMES][0]; pcm[i*2+1]=ring->pcm[(r+i)%ANALYZER_FRAMES][1]; }
    atomic_store_explicit(&ring->read,r+n,memory_order_release); return n;
}
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
    if(bus==callback_bus) { analyzer_push(1,left,right); callback_taps++; }
    for(int i=0;i<recording.count;i++) if(recording.buses[i]==bus) ring_push(&recording.takes[i],left,right);
}
static double monotonic_time(void) { return platform_native_time(); }
/* All auditions enter the Master before its effects, fader, pan and meters. */
static void audition_input(void *context,float buses[INSERTS+1][2],double pitch) {
    (void)context;
    if(preview_position>=preview_end || !preview_remaining)return;
    int stretch=preview_channel>=0 && project.sampler[preview_channel].stretch;
    float stereo[2];
    if(stretch){preview_stretch.position=preview_position;sampler_voice_sample(&preview_stretch,preview,pitch*preview_speed,1,1,stereo);for(int side=0;side<2;side++)stereo[side]*=preview_gain;}
    else {
        preview_stretch.grain_ready=0;unsigned n=(unsigned)preview_position;
        for(int side=0;side<2;side++){float a=sample_at(preview,n,side),b=sample_at(preview,n+1,side);stereo[side]=(a+(b-a)*(preview_position-n))*preview_gain;}
    }
    for(int side=0;side<2;side++)buses[0][side]+=stereo[side];
    preview_position+=stretch?1:pitch*preview_speed;preview_remaining--;
}
static void callback(ma_device *d, void *out, const void *in, ma_uint32 frames) {
    (void)d; (void)in;
    memset(out,0,frames*2*sizeof(float));
    /* A busy UI delays edits, never playback. No UI code touches render state. */
    consume_commands();
    callback_taps=0;
    callback_bus=atomic_load_explicit(&analyzer_bus,memory_order_relaxed);
    MixerIO taps=recording.active?recording.io:(MixerIO){0}; taps.output=record_output; taps.monitor_only=!(recording.active || atomic_load(&midi_recording));
    live_time=monotonic_time();live_frames=frames;
    if(preview_channel>=0)preview_speed=preview_note_pitch*channel_speed(&project,preview_channel);
    if(preview_position<preview_end){preview_frame=preview_position;preview_time=live_time;preview_rate=preview_channel>=0 && project.sampler[preview_channel].stretch?1:exp2(project.master_pitch/12.)*preview_speed;}
    taps.audition=audition_input;
    float peaks[INSERTS+1][2]={{0}};
    int arp_mix=player.arp.count || live.arp.count;
    for(int c=0;c<project.channel_count;c++)arp_mix|=project.arp[c].direction!=0;
    for(int i=0;i<128;i++)arp_mix|=player.voices[i].gain && player.voices[i].arp_generated;
    if(playing) {
        visual_frame=llround(player.frame*(player.clock_bpm?player.clock_bpm/project.bpm:1));
        render_mixer_io(&player,&live,&project,samples,out,frames,1,peaks,&taps);
        visual_time=monotonic_time(); visual_frames=frames;
        double step_frames=RATE*60.0/project.bpm/4;
        visual_start=(player.loop_end?player.loop_start:player.song?0:player.start_step)*step_frames;
        float end=player.loop_end?player.loop_end:player.song?playback.song_end:project.pattern_steps[player.pattern];
        if(visual_start>=end*step_frames) visual_start=0;
        visual_end=(recording.active || atomic_load(&midi_recording))?INFINITY:end*step_frames;
    }
    else if(arp_mix)render_mixer_io(&player,&live,&project,samples,out,frames,0,peaks,&taps);
    else render_mixer_io(&live,NULL,&project,samples,out,frames,0,peaks,&taps);
    for(int l=0;l<LANES;l++) {
        atomic_store_explicit(&track_active[l],playing && player.song && player.lane_active[l],memory_order_relaxed);
        if(playing && player.song && player.lane_trigger[l]) atomic_store_explicit(&track_trigger[l],1,memory_order_relaxed);
    }
    for(int c=0;c<CHANNELS;c++) {
        Player *rendered=playing || arp_mix?&player:&live;
        int previewing=preview_channel==c && preview_remaining && preview_position<preview_end && preview_gain>0;
        atomic_store_explicit(&channel_active[c],rendered->channel_active[c] || previewing,memory_order_relaxed);
        if(rendered->channel_trigger[c]) atomic_store_explicit(&channel_trigger[c],1,memory_order_relaxed);
    }
    float *buffer=out;
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
    for(unsigned i=0;i<frames;i++) analyzer_push(0,buffer[i*2],buffer[i*2+1]);
    for(unsigned i=callback_taps;callback_bus>=0 && i<frames;i++) analyzer_push(1,0,0);
    int active=0;
    for(int id=0;id<=INSERTS;id++) for(int side=0;side<2;side++) {
        float peak=peaks[id][side],old=atomic_load_explicit(&meter_peak[id][side],memory_order_relaxed);
        active|=peak>.00001f;
        /* Exactly one producer; the UI can only exchange this value with zero.
           A failed strong CAS therefore observes zero, and one second strong
           CAS suffices. No spurious-failure/unbounded retry on the callback. */
        if(peak>old && !atomic_compare_exchange_strong_explicit(&meter_peak[id][side],&old,peak,memory_order_relaxed,memory_order_relaxed)) {
            if(peak>old) atomic_compare_exchange_strong_explicit(&meter_peak[id][side],&old,peak,memory_order_relaxed,memory_order_relaxed);
        }
    }
    atomic_store_explicit(&meter_active,active,memory_order_relaxed);
    atomic_store(&position,player.frame);
    publish_view();
}
static void publish_view(void) {
    AudioView *target=&view_slots[view_writer];
    memset(target->pattern_notes,0,sizeof target->pattern_notes);
    if(playing) for(int i=0;i<128;i++) {
        const Voice *v=&player.voices[i];
        if(v->gain>0 && v->note_id>0 && v->note_id<=NOTES && v->pattern>=0 && v->pattern<PATTERNS && v->channel>=0 && v->channel<CHANNELS && player.channel_active[v->channel] && (v->lane<0 || (v->lane<LANES && player.lane_active[v->lane])))
            target->pattern_notes[v->pattern][v->channel][(v->note_id-1)/8]|=1u<<((v->note_id-1)%8);
    }
    target->note_serial=note_preview_serial;target->note_channel=note_preview_channel;target->note_pitch=note_preview_pitch;target->note_active=0;
    if(note_preview_channel>=0 && note_preview_channel<project.channel_count) {
        int channel=note_preview_channel;
        if(project.instrument[channel]==INSTRUMENT_FM) {
            const Voice *voice=&live.voices[127];
            target->note_active=voice->channel==channel && voice->instrument==INSTRUMENT_FM && voice->gain>0 && project.volume[channel]>0 && project.master>0 && !project.master_mute;
        } else target->note_active=preview_channel==channel && preview_remaining && preview_position<preview_end && preview_gain>0;
    }
    for(int i=0;i<128;i++)if(live.arp.held[i].active && live.arp.held[i].id==127 && live.arp.held[i].channel==note_preview_channel && project.volume[note_preview_channel]>0 && !project.master_mute)target->note_active=1;
    target->preview=preview; target->preview_position=preview_position;
    target->preview_frame=preview_frame; target->preview_time=preview_time; target->preview_rate=preview_rate;
    target->preview_end=preview_end; target->preview_remaining=preview_remaining;
    for(int i=0;i<128;i++) {
        const Voice *v=&live.voices[i];
        target->voices[i]=(VoiceView){.channel=v->channel,.gain=v->gain,.sampler={v->instrument==INSTRUMENT_FM?0:v->sampler.position,v->instrument==INSTRUMENT_FM?1:v->sampler.speed}};
    }
    for(int i=0;i<128;i++){const ArpHeld *h=&live.arp.held[i];if(h->active && h->id<128)target->voices[h->id]=(VoiceView){.channel=h->channel,.gain=h->velocity,.sampler={0,1}};}
    double pitch_speed=pow(2,project.master_pitch/12.0);
    for(int c=0;c<CHANNELS;c++) {
        target->sample_frames[c]=samples[c].frames; target->instrument[c]=project.instrument[c];
        target->speeds[c]=(project.sampler[c].stretch?1:pitch_speed*channel_speed(&project,c))*(project.sampler[c].fit_bpm?project.bpm/project.sampler[c].fit_bpm:1);
    }
    target->live_frames=live_frames; target->live_time=live_time;
    target->visual_frames=visual_frames; target->visual_frame=visual_frame; target->visual_time=visual_time;
    target->visual_start=visual_start; target->visual_end=visual_end; target->playing=playing;
    view_writer=atomic_exchange_explicit(&view_middle,view_writer|4,memory_order_acq_rel)&3;
}
static void consume_commands(void) {
    unsigned begin=atomic_load_explicit(&pending.read,memory_order_relaxed);
    unsigned finish=atomic_load_explicit(&pending.committed,memory_order_acquire);
    const ProjectSnapshot *snapshot=NULL;
    unsigned flags=0;
    /* Match the original batch semantics: newest committed project applies
       before any note/key/transport commands in this callback batch. */
    for(unsigned i=begin;i!=finish;i++) {
        const AudioCommand *command=&pending.commands[i%AUDIO_COMMANDS];
        if(command->snapshot) { snapshot=command->snapshot; flags|=command->snapshot_flags; }
    }
    if(snapshot && (flags & SNAPSHOT_PROJECT)) {
        const Project *next=&snapshot->project;
        if(memcmp(project.clips,next->clips,sizeof project.clips) ||
            memcmp(project.clip_starts,next->clip_starts,sizeof project.clip_starts) ||
            memcmp(project.clip_steps,next->clip_steps,sizeof project.clip_steps) ||
            memcmp(project.clip_offsets,next->clip_offsets,sizeof project.clip_offsets) ||
            memcmp(project.audio_seconds,next->audio_seconds,sizeof project.audio_seconds)) player.audio_resync=1;
        for(unsigned bus=0;bus<=INSERTS;bus++)if(project.effect_revision[bus]!=next->effect_revision[bus])effects_reset_bus(effects,bus);
        if(project.channel_count!=next->channel_count){
            player_arp_clear(&player,-1);player_arp_clear(&live,-1);
            for(int i=0;i<128;i++){if(player.voices[i].arp_generated)player.voices[i].gain=0;if(live.voices[i].arp_generated)live.voices[i].gain=0;}
        }
        project=*next;
    }
    if(snapshot && (flags & SNAPSHOT_PLAN)) playback=snapshot->plan;
    uint64_t serial=0;
    unsigned read=begin;
    while(read!=finish) {
        const AudioCommand *c=&pending.commands[read++%AUDIO_COMMANDS];
        switch(c->kind) {
        case UPDATE: {
            /* Transport seeks/stops reset sources, but let delay/reverb tails ring. */
            int was_playing=playing,keep_arp=live.arp.count;
            for(int i=0;i<128;i++)keep_arp|=player.voices[i].gain && player.voices[i].arp_generated;
            playing=c->run; output_volume=c->output;
            if(c->reset || (!playing && (was_playing || !keep_arp)) || player.song!=c->song || (!c->song && player.pattern!=c->pattern)) {
                if(was_playing && !playing){player_arp_clear(&live,-1);for(int v=0;v<128;v++)if(live.voices[v].arp_generated)live.voices[v].gain=0;}
                click_position=1200; player_reset(&player); player.effects=effects; player_seek(&player,&project,c->start);
                atomic_store(&position,player.frame); visual_frame=player.frame; visual_frames=0;
            }
            player.song=c->song; player.pattern=c->pattern;
            player.loop_start=isfinite(c->loop_start)?fmaxf(0,c->loop_start):0;
            player.loop_end=isfinite(c->loop_end) && c->loop_end>player.loop_start?c->loop_end:0;
            if(playing && player.loop_end && player.frame*(player.clock_bpm?player.clock_bpm/project.bpm:1)>=(uint64_t)llround(player.loop_end*RATE*60.0/project.bpm/4)) {
                player.frame=(uint64_t)llround(player.loop_start*RATE*60.0/project.bpm/4); player.clock_bpm=project.bpm;
                player.last_step=-1; memset(player.voices,0,sizeof player.voices);
            }
            break;
        }
        case SAMPLE: case CHANNEL_SAMPLES:
            note_preview_channel=-1;
            for(int channel=0;channel<CHANNELS;channel++) {
                Sample next=c->kind==SAMPLE?(channel==c->channel?c->sample:samples[channel]):c->channels[channel];
                if(next.data!=samples[channel].data || next.frames!=samples[channel].frames || next.channels!=samples[channel].channels) {
                    for(int v=0;v<128;v++) {
                        if(player.voices[v].channel==channel) player.voices[v].gain=0;
                        if(live.voices[v].channel==channel) live.voices[v].gain=0;
                    }
                    player_arp_clear(&player,channel);player_arp_clear(&live,channel);
                    samples[channel]=next; player.audio_resync=1;
                }
            }
            preview=(Sample){0}; preview_end=preview_remaining=0; preview_channel=-1;
            break;
        case PREVIEW:
            note_preview_channel=-1;preview_channel=-1;
            preview=c->sample; preview_position=preview_frame=0; preview_time=monotonic_time();
            preview_speed=preview_rate=1; preview_gain=.7f; preview_end=preview.frames; preview_remaining=5*RATE;
            break;
        case NOTE:
            note_preview_channel=-1;
            if(c->channel>=0 && c->channel<project.channel_count) {
                note_preview_channel=c->note.velocity && project.volume[c->channel]>0 && project.master>0 && (project.instrument[c->channel]==INSTRUMENT_FM || samples[c->channel].frames)?c->channel:-1;
                note_preview_pitch=c->note.pitch;note_preview_serial=c->serial;
                if(player_arp_on(&live,&project,c->channel,127,c->note.pitch,c->note.velocity/127.f,(c->note.length?c->note.length:1)*RATE*15/project.bpm,-1,0,0)){atomic_store(&channel_trigger[c->channel],1);break;}
                if(project.instrument[c->channel]==INSTRUMENT_FM) {
                    player_voice_prepare(&live,&project,127);
                    Voice *voice=&live.voices[127]; uint32_t phase[6];
                    int retain=voice->instrument==INSTRUMENT_FM && voice->channel==c->channel && voice->fm.engine==1 && project.fm[c->channel].engine==1 && !project.fm[c->channel].dx7.value[136];
                    if(retain) memcpy(phase,voice->fm.dx7.phase,sizeof phase);
                    *voice=(Voice){.channel=c->channel,.instrument=INSTRUMENT_FM,.remaining=(c->note.length?c->note.length:1)*RATE*15/project.bpm,.gain=c->note.velocity/127.f,.lane=-1,.note_pitch=c->note.pitch,.base_pitch=c->note.pitch,.current_pitch=c->note.pitch};
                    instrument_start(&voice->device,voice->instrument,(InstrumentSettings){&project.sampler[c->channel],&project.fm[c->channel]},(InstrumentNote){.frequency=440*pow(2,(c->note.pitch-69)/12.0),.velocity=c->note.velocity/127.f});
                    if(retain) memcpy(voice->fm.dx7.phase,phase,sizeof phase);
                    player_voice_on(&live,&project,127,c->note.pitch);
                    atomic_store(&channel_trigger[c->channel],1); break;
                }
                if(voice_policy_enabled(&project,c->channel)) {
                    player_voice_prepare(&live,&project,127);
                    Voice *voice=&live.voices[127];
                    *voice=(Voice){.channel=c->channel,.instrument=INSTRUMENT_SAMPLER,.remaining=(c->note.length?c->note.length:1)*RATE*15/project.bpm,.gain=c->note.velocity/127.f,.lane=-1,.note_pitch=c->note.pitch,.base_pitch=c->note.pitch,.current_pitch=c->note.pitch};
                    instrument_start(&voice->device,INSTRUMENT_SAMPLER,(InstrumentSettings){&project.sampler[c->channel],&project.fm[c->channel]},(InstrumentNote){.speed=exp2((c->note.pitch-60)/12.),.velocity=c->note.velocity/127.f});
                    player_voice_on(&live,&project,127,c->note.pitch);atomic_store(&channel_trigger[c->channel],1);break;
                }
                preview=samples[c->channel]; preview_position=preview_frame=0; preview_time=monotonic_time();
                preview_remaining=UINT_MAX;preview_note_pitch=pow(2,((int)c->note.pitch-60)/12.0);preview_speed=preview_note_pitch*channel_speed(&project,c->channel);
                preview_rate=preview_speed*pow(2,project.master_pitch/12.0);
                preview_gain=c->note.velocity/127.f*project.volume[c->channel];
                preview_end=fmin(preview.frames,(project.sampler[c->channel].stretch?1:preview_speed)*RATE*60.0/project.bpm/4*(c->note.length?c->note.length:1));
                sampler_voice_reset(&preview_stretch,0,1);
                preview_channel=c->channel;
                if(preview_end && preview_gain>0) atomic_store(&channel_trigger[c->channel],1);
            }
            break;
        case KEY:
            if(!c->down && player_arp_off(&live,c->slot))break;
            if(c->down && player_arp_on(&live,&project,c->channel,c->slot,c->pitch,c->note.velocity/127.f,-1,-1,0,0)){atomic_store(&channel_trigger[c->channel],1);break;}
            if(c->down && c->channel>=0 && c->channel<project.channel_count) {
                if((samples[c->channel].frames || project.instrument[c->channel]==INSTRUMENT_FM) && project.volume[c->channel]>0 && !(project.mute[c->channel]&1)) atomic_store(&channel_trigger[c->channel],1);
                player_voice_prepare(&live,&project,c->slot);
                uint32_t phase[6]; Voice *old=&live.voices[c->slot];
                int retain=old->instrument==INSTRUMENT_FM && old->channel==c->channel && old->fm.engine==1 && project.fm[c->channel].engine==1 && !project.fm[c->channel].dx7.value[136];
                if(retain) memcpy(phase,old->fm.dx7.phase,sizeof phase);
                live.voices[c->slot]=(Voice){.channel=c->channel,.sampler={.speed=pow(2,(c->pitch-60)/12.0)},.remaining=-1,.gain=c->note.velocity/127.f,.lane=-1,.note_pitch=c->pitch,.base_pitch=c->pitch,.current_pitch=c->pitch};
                if(project.instrument[c->channel]==INSTRUMENT_FM) {
                    live.voices[c->slot].instrument=INSTRUMENT_FM;
                    instrument_start(&live.voices[c->slot].device,INSTRUMENT_FM,(InstrumentSettings){&project.sampler[c->channel],&project.fm[c->channel]},(InstrumentNote){.frequency=440*pow(2,(c->pitch-69)/12.0),.velocity=c->note.velocity/127.f});
                    if(retain) memcpy(live.voices[c->slot].fm.dx7.phase,phase,sizeof phase);
                }
                player_voice_on(&live,&project,c->slot,c->pitch);
            } else if(live.voices[c->slot].gain) {
                player_voice_off(&live,&project,c->slot);
            }
            break;
        case STOP: {
            note_preview_channel=-1;
            int active=playing || live.arp.count || player.arp.count || (preview_remaining && preview_position<preview_end);
            for(int i=0;i<128;i++) active|=live.voices[i].gain!=0;
            atomic_store(&stopped_active,active);
            playing=0; preview=(Sample){0}; preview_end=preview_remaining=0; preview_channel=-1; click_position=1200;
            memset(player.voices,0,sizeof player.voices); memset(live.voices,0,sizeof live.voices);player_arp_clear(&player,-1);player_arp_clear(&live,-1); if(!c->down) effects_reset(effects);
            break;
        }
        }
        serial=c->serial;
    }
    player.plan=live.plan=&playback;
    atomic_store_explicit(&pending.read,read,memory_order_release);
    if(serial) {
        publish_view();
        /* Replacement callers may now release samples no voice can reference. */
        atomic_store_explicit(&acknowledged,serial,memory_order_release);
    }
}
static void wait_control(void) {
    struct timespec pause={0,100000}; nanosleep(&pause,NULL);
}
/* UI only. Device stop completion establishes sole render ownership here. */
static int consume_stopped(void) {
    if(!ready) {
        atomic_store_explicit(&pending.committed,pending.write,memory_order_release);
        consume_commands(); publish_view(); refresh_view(); return 1;
    }
    if(ma_device_get_state(&device)!=ma_device_state_stopped) return 0;
    ma_mutex_lock(&device.startStopLock);
    int stopped=ma_device_get_state(&device)==ma_device_state_stopped;
    if(stopped) {
        atomic_store_explicit(&pending.committed,pending.write,memory_order_release);
        consume_commands(); publish_view(); refresh_view();
    }
    ma_mutex_unlock(&device.startStopLock);
    return stopped;
}
/* Only the sole UI producer may wait for queue/state capacity. */
static uint64_t submit(AudioCommand command) {
    while(pending.write-atomic_load_explicit(&pending.read,memory_order_acquire)==AUDIO_COMMANDS) {
        if(consume_stopped()) break;
        atomic_store_explicit(&pending.committed,pending.write,memory_order_release);
        wait_control();
    }
    command.serial=++pending.serial;
    if(pending.dirty) {
        unsigned slot=0;
        for(;;) {
            uint64_t acknowledged_serial=atomic_load_explicit(&acknowledged,memory_order_acquire);
            for(slot=0;slot<8;slot++) if(snapshots[slot].serial<=acknowledged_serial) break;
            if(slot<8) break;
            if(!consume_stopped()) {
                atomic_store_explicit(&pending.committed,pending.write,memory_order_release);
                wait_control();
            }
        }
        snapshots[slot].value=pending.snapshot;
        snapshots[slot].serial=command.serial;
        command.snapshot=&snapshots[slot].value; command.snapshot_flags=pending.dirty;
        pending.dirty=0;
    }
    pending.commands[pending.write++%AUDIO_COMMANDS]=command;
    consume_stopped();
    return command.serial;
}
static void wait_acknowledged(uint64_t serial) {
    while(atomic_load(&acknowledged)<serial) {
        if(ma_device_get_state(&device)==ma_device_state_stopped) {
            control_begin(); consume_stopped(); control_end();
        } else wait_control();
    }
}
static int realtime_atomics_supported(void) {
    return atomic_is_lock_free(&pending.read) && atomic_is_lock_free(&pending.committed)
        && atomic_is_lock_free(&view_middle) && atomic_is_lock_free(&acknowledged)
        && atomic_is_lock_free(&position) && atomic_is_lock_free(&meter_peak[0][0])
        && atomic_is_lock_free(&meter_active) && atomic_is_lock_free(&channel_active[0])
        && atomic_is_lock_free(&recording.inputs[0].read);
}
int audio_start(const Project *p,const Sample s[CHANNELS]) {
    /* Reject a platform whose atomics could hide a callback-side library lock. */
    if(!realtime_atomics_supported()) return 0;
    for(int accent=0;accent<2;accent++) for(int i=0;i<1200;i++)
        clicks[accent][i]=.25f*sinf(6.2831853f*(accent?1800:1200)*i/RATE)*expf(-i/180.f);
    memset(&pending,0,sizeof pending); memset(&view,0,sizeof view);
    memset(snapshots,0,sizeof snapshots); memset(view_slots,0,sizeof view_slots);
    control_depth=0; view_writer=0; view_reader=1; atomic_store(&view_middle,2);
    atomic_store(&acknowledged,0); atomic_store(&position,0);
    project=*p; memcpy(samples,s,sizeof samples);
    player_reset(&player); player_reset(&live);
    effects=effects_create(INSERTS+1); if(!effects) return 0;
    if(!effects_prepare(effects,&p->effect_type[0][0],(INSERTS+1)*EFFECT_SLOTS)){effects_free(effects);effects=NULL;return 0;}
    project_snapshot_update(&pending.snapshot,p,1,NULL); playback=pending.snapshot.plan;
    player.plan=live.plan=&playback;
    player.effects=live.effects=effects; playing=0;note_preview_channel=-1; preview=(Sample){0}; preview_end=preview_remaining=0; preview_channel=-1;
    visual_frames=live_frames=0; output_volume=1; publish_view();
    ma_device_config config=ma_device_config_init(ma_device_type_playback);
    config.playback.format=ma_format_f32; config.playback.channels=2;
    config.sampleRate=RATE; config.dataCallback=callback;
    if(ma_device_init(NULL,&config,&device)!=MA_SUCCESS) { effects_free(effects); effects=NULL; player.effects=live.effects=NULL; return 0; }
    if(ma_device_start(&device)!=MA_SUCCESS) { ma_device_uninit(&device); effects_free(effects); effects=NULL; player.effects=live.effects=NULL; return 0; }
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
    recording.active=1;
    audio_update(p,1,1,player.pattern,1,output,start_step,0,0);
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
static int effect_memory_failed;
int audio_effect_memory_failed(void) {return effect_memory_failed;}
void audio_update(const Project *p,int run,int song,int pattern,int reset,float output,float start_step,float loop_start,float loop_end) {
    if(effects)effect_memory_failed=!effects_prepare(effects,&p->effect_type[0][0],(INSERTS+1)*EFFECT_SLOTS);
    control_begin();
    pending.dirty|=project_snapshot_update(&pending.snapshot,p,effects!=NULL,recording.active?&recording.io:NULL);
    submit((AudioCommand){.kind=UPDATE,.run=run,.song=song,.pattern=pattern,.reset=reset,
        .output=output,.start=start_step,.loop_start=loop_start,.loop_end=loop_end});
    control_end();
}
void audio_sample(int c,Sample s) {
    if(c<0 || c>=CHANNELS) return;
    control_begin();
    uint64_t serial=submit((AudioCommand){.kind=SAMPLE,.channel=c,.sample=s});
    control_end(); wait_acknowledged(serial);
}
void audio_channels(const Project *p,const Sample s[CHANNELS]) {
    AudioCommand command={.kind=CHANNEL_SAMPLES}; memcpy(command.channels,s,sizeof command.channels);
    if(effects)effect_memory_failed=!effects_prepare(effects,&p->effect_type[0][0],(INSERTS+1)*EFFECT_SLOTS);
    control_begin(); pending.dirty|=project_snapshot_update(&pending.snapshot,p,effects!=NULL,recording.active?&recording.io:NULL);
    uint64_t serial=submit(command);
    control_end(); wait_acknowledged(serial);
}
void audio_preview(Sample s) {
    control_begin();
    uint64_t serial=submit((AudioCommand){.kind=PREVIEW,.sample=s});
    control_end(); wait_acknowledged(serial);
}
void audio_note(int c,Note n) {
    control_begin(); submit((AudioCommand){.kind=NOTE,.channel=c,.note=n}); control_end();
}
int audio_note_activity(int *channel,int *pitch) {
    static uint64_t seen;
    refresh_view();int c=ready && (view.note_active || view.note_serial!=seen)?view.note_channel:-1,p=view.note_pitch;
    seen=view.note_serial;
    /* Reflect queued previews immediately, without reading callback-owned state. */
    for(unsigned i=pending.read;i!=pending.write;i++) {
        const AudioCommand *command=&pending.commands[i%AUDIO_COMMANDS];
        if(command->kind==STOP || command->kind==SAMPLE || command->kind==CHANNEL_SAMPLES || command->kind==PREVIEW)c=-1;
        if(command->kind==NOTE) {
            int next=command->channel;const Project *state=&pending.snapshot.project;
            c=ready && next>=0 && next<state->channel_count && command->note.velocity && state->volume[next]>0 && state->master>0 &&
                (state->instrument[next]==INSTRUMENT_FM || view.sample_frames[next])?next:-1;
            p=command->note.pitch;seen=command->serial;
        }
    }
    if(c<0 || c>=CHANNELS || p<0 || p>=128){if(channel)*channel=-1;if(pitch)*pitch=-1;return 0;}
    if(channel)*channel=c;
    if(pitch)*pitch=p;
    return 1;
}
void audio_key(int slot,int channel,int pitch,int down) { audio_key_velocity(slot,channel,pitch,down?100:0); }
void audio_key_velocity(int slot,int channel,int pitch,int velocity) {
    if(slot<0 || slot>=128 || pitch<0 || pitch>127) return;
    control_begin();
    submit((AudioCommand){.kind=KEY,.slot=slot,.channel=channel,.pitch=pitch,.down=velocity>0,.note={.velocity=velocity>0?(velocity>127?127:velocity):0}});
    control_end();
}
static int stop_audio(int preserve_tails) {
    control_begin(); uint64_t serial=submit((AudioCommand){.kind=STOP,.down=preserve_tails}); control_end();
    wait_acknowledged(serial); return atomic_load(&stopped_active);
}
int audio_stop(void) { return stop_audio(0); }
int audio_transport_stop(void) { return stop_audio(1); }
double audio_preview_position(Sample sample) {
    refresh_view();
    double progress=-1;
    if(ready && sample.frames && view.preview.data==sample.data && view.preview_remaining) {
        double frame=fmax(0,fmin(view.preview_position,view.preview_frame+(monotonic_time()-view.preview_time)*RATE*view.preview_rate));
        if(frame<view.preview_end) progress=fmin(1,frame/sample.frames);
    }
    return progress;
}
double audio_key_position(int slot,int channel) {
    if(slot<0 || slot>=128 || channel<0 || channel>=CHANNELS) return -1;
    refresh_view();
    VoiceView v=view.voices[slot]; double progress=-1;
    int queued=0;
    /* Keep a new key visible immediately so an idle sampler keeps animating. */
    for(unsigned i=pending.read;i!=pending.write;i++) {
        AudioCommand *c=&pending.commands[i%AUDIO_COMMANDS];
        if(c->kind==STOP || c->kind==SAMPLE || c->kind==CHANNEL_SAMPLES) v.gain=0;
        if(c->kind==KEY && c->slot==slot) {
            if(c->down && c->channel>=0 && c->channel<pending.snapshot.project.channel_count) {
                v=(VoiceView){.channel=c->channel,.gain=1}; queued=1;
            }
        }
    }
    if(v.gain && v.channel==channel && view.instrument[channel]==INSTRUMENT_FM) progress=0;
    else if(v.gain && v.channel==channel && view.sample_frames[channel]) {
        double behind=fmax(0,view.live_frames-(monotonic_time()-view.live_time)*RATE);
        progress=queued?0:fmin(1,fmax(0,v.sampler.position-behind*v.sampler.speed*view.speeds[channel])/view.sample_frames[channel]);
    }
    return progress;
}
uint64_t audio_position(void) { return atomic_load(&position); }
void audio_meters(float peaks[INSERTS+1][2]) {
    for(int id=0;id<=INSERTS;id++) for(int side=0;side<2;side++)
        peaks[id][side]=atomic_exchange_explicit(&meter_peak[id][side],0,memory_order_relaxed);
}
void audio_channel_activity(uint8_t active[CHANNELS],uint8_t triggered[CHANNELS]) {
    for(int c=0;c<CHANNELS;c++) {
        active[c]=atomic_load_explicit(&channel_active[c],memory_order_relaxed);
        triggered[c]=atomic_exchange_explicit(&channel_trigger[c],0,memory_order_relaxed);
    }
}
void audio_pattern_activity(int pattern,uint8_t notes[CHANNELS][NOTES]) {
    refresh_view();
    if(pattern>=0 && pattern<PATTERNS)for(int c=0;c<CHANNELS;c++)for(int n=0;n<NOTES;n++)notes[c][n]=(view.pattern_notes[pattern][c][n/8]>>(n%8))&1;
    else memset(notes,0,CHANNELS*NOTES);
    
}
void audio_track_activity(uint8_t active[LANES],uint8_t triggered[LANES]) {
    for(int l=0;l<LANES;l++) {
        active[l]=atomic_load_explicit(&track_active[l],memory_order_relaxed);
        triggered[l]=atomic_exchange_explicit(&track_trigger[l],0,memory_order_relaxed);
    }
}
int audio_active(void) { return atomic_load_explicit(&meter_active,memory_order_relaxed); }
double audio_visual_position(void) {
    refresh_view();
    double frame=view.visual_frame;
    if(view.playing && view.visual_frames) {
        /* Animate within the last rendered buffer, never beyond available audio. */
        frame+=fmin(view.visual_frames,fmax(0,(monotonic_time()-view.visual_time)*RATE));
        if(frame>=view.visual_end && view.visual_end>view.visual_start)
            frame=view.visual_start+fmod(frame-view.visual_start,view.visual_end-view.visual_start);
    }
    return frame;
}
void audio_close(void) {
    audio_record_end(); record_release();
    if(ma_device_get_state(&device)!=ma_device_state_uninitialized) ma_device_uninit(&device);
    ready=0;
    effects_free(effects); effects=NULL; player.effects=live.effects=NULL;
    control_begin(); atomic_store_explicit(&pending.committed,pending.write,memory_order_release); consume_commands(); publish_view(); refresh_view(); control_end();
}
int sample_load(const char *path,Sample *s) {
    if(sample_map_wav(path,s)) return 1;
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
    *s=(Sample){.data=data,.frames=(unsigned)frames,.channels=channels}; return 1;
}
