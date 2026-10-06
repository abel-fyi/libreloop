// SPDX-License-Identifier: GPL-3.0-only
/* Exercise the real callback and mailbox without opening an audio device. */
#include "../src/audio.c"
#include "arrangement.h"
#include <stdio.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); exit(1); } } while(0)
static Project fixture;
static atomic_int running;
static void *audio_thread(void *unused) {
    (void)unused; float out[128];
    while(atomic_load(&running)) {
        callback(NULL,out,NULL,64);
        for(unsigned i=0;i<128;i++) CHECK(isfinite(out[i]) && fabsf(out[i])<=1);
        wait_control();
    }
    return NULL;
}
static void setup(Sample source) {
    ready=0; memset(&pending,0,sizeof pending); atomic_store(&acknowledged,0);
    project_default(&fixture); memset(fixture.notes,0,sizeof fixture.notes); memset(fixture.clips,0,sizeof fixture.clips);
    fixture.channel_count=1; fixture.channel_audio[0]=1; fixture.route[0]=0;
    fixture.audio_seconds[0]=fmaxf(1,source.frames/(float)RATE); fixture.clips[0][0]=PATTERNS+1;
    Sample list[CHANNELS]={source}; audio_channels(&fixture,list);
    audio_update(&fixture,1,1,0,1,1,0,0,0); audio_metronome(0);
}
int main(void) {
    float pcm[2048];
    for(unsigned i=0;i<1024;i++) { pcm[i*2]=.8f*sinf(i*.1f); pcm[i*2+1]=-.6f*cosf(i*.07f); }
    float out[128];
    /* Song end wraps audio and the displayed cursor to zero, not the seek marker. */
    setup((Sample){pcm,1024,2}); audio_update(&fixture,1,1,0,1,1,2,0,0);
    double step_frames=RATE*60.0/fixture.bpm/4;
    CHECK(player.frame==(uint64_t)llround(2*step_frames));
    player.frame=(uint64_t)llround(song_steps(&fixture)*step_frames)-1;
    callback(NULL,out,NULL,64); CHECK(player.frame==63 && view.visual_start==0 && player.start_step==2);
    audio_update(&fixture,1,1,0,1,1,2,2,3);
    player.frame=(uint64_t)llround(3*step_frames)-1;
    callback(NULL,out,NULL,64); CHECK(player.frame==(uint64_t)llround(2*step_frames)+63 && view.visual_start==2*step_frames);
    /* BPM mailbox edits retain PCM and voices, and preserve musical position. */
    static float fitted_pcm[4096*2];
    for(unsigned i=0;i<4096;i++) { fitted_pcm[i*2]=.3f*sinf(i*.04f); fitted_pcm[i*2+1]=-.7f*fitted_pcm[i*2]; }
    for(int mode=0;mode<2;mode++) {
        setup((Sample){fitted_pcm,4096,2});
        fixture.sampler[0].fit_bpm=120; fixture.sampler[0].stretch=mode;
        audio_update(&fixture,1,1,0,0,1,0,0,0); callback(NULL,out,NULL,64);
        for(int edit=0;edit<12;edit++) {
            double beat=player.frame*fixture.bpm/(RATE*15.0),source=player.voices[0].sampler.position;
            float last=out[126]; fixture.bpm=edit%2?60:240;
            audio_update(&fixture,1,1,0,0,1,0,0,0);
            CHECK(!player.audio_resync && samples[0].data==fitted_pcm);
            callback(NULL,out,NULL,64);
            CHECK(!player.channel_trigger[0] && player.voices[0].sampler.position>source);
            CHECK(fabs(player.frame*fixture.bpm/(RATE*15.0)-beat-64*fixture.bpm/(RATE*15.0))<.001);
            CHECK(fabs(view.visual_frame*fixture.bpm/(RATE*15.0)-beat)<.001);
            for(int i=0;i<64;i++) { CHECK(fabsf(out[i*2]-last)<.05f); last=out[i*2]; }
        }
    }
    setup((Sample){pcm,1024,2}); callback(NULL,out,NULL,64);
    CHECK(!memcmp(out,pcm,sizeof out)); /* Device output is transparent at unity. */
    float tapped[8192*2]; unsigned tap_frames=audio_spectrum_read(0,tapped,8192);
    CHECK(tap_frames>=64 && !memcmp(tapped+(tap_frames-64)*2,out,sizeof out));
    audio_spectrum_bus(0); callback(NULL,out,NULL,64);
    tap_frames=audio_spectrum_read(1,tapped,8192); CHECK(tap_frames>=64);
    CHECK(!memcmp(tapped+(tap_frames-64)*2,out,sizeof out));
    audio_spectrum_bus(-1);
    setup((Sample){pcm,1024,2}); callback(NULL,out,NULL,64);
    uint8_t channel_active_ui[CHANNELS],channel_triggered[CHANNELS];
    audio_channel_activity(channel_active_ui,channel_triggered);
    CHECK(channel_active_ui[0] && channel_triggered[0]);
    audio_channel_activity(channel_active_ui,channel_triggered); CHECK(!channel_triggered[0]);
    callback(NULL,out,NULL,64); audio_channel_activity(channel_active_ui,channel_triggered);
    CHECK(channel_active_ui[0] && !channel_triggered[0]);
    /* A complete short note between UI frames still produces a flash. */
    setup((Sample){pcm,4,2}); callback(NULL,out,NULL,64);
    audio_channel_activity(channel_active_ui,channel_triggered); CHECK(channel_triggered[0]);
    callback(NULL,out,NULL,64); audio_channel_activity(channel_active_ui,channel_triggered);
    CHECK(!channel_active_ui[0] && !channel_triggered[0]);
    setup((Sample){pcm,1024,2}); callback(NULL,out,NULL,64);
    audio_channel_activity(channel_active_ui,channel_triggered);
    /* A UI lock may postpone a volume edit, but never silence or rewind playback. */
    ready=1; ma_atomic_device_state_set(&device.state,ma_device_state_started);
    audio_key(127,0,60,1); CHECK(audio_key_position(127,0)==0);
    callback(NULL,out,NULL,64); audio_channel_activity(channel_active_ui,channel_triggered);
    CHECK(channel_active_ui[0] && channel_triggered[0]);
    audio_key(127,0,60,0);
    /* Drain the key events separately before testing exact song samples. */
    pthread_mutex_lock(&mutex); consume_commands(); memset(live.voices,0,sizeof live.voices); pthread_mutex_unlock(&mutex);
    fixture.volume[0]=.5f; audio_update(&fixture,1,1,0,0,1,0,0,0);
    pthread_mutex_lock(&mutex);
    uint64_t before=player.frame; callback(NULL,out,NULL,64);
    CHECK(player.frame==before+64 && !memcmp(out,pcm+before*2,sizeof out));
    CHECK(project.volume[0]==1 && pending.read!=pending.write);
    pthread_mutex_unlock(&mutex);
    callback(NULL,out,NULL,64); CHECK(project.volume[0]==.5f && player.frame==before+128);
    for(unsigned i=0;i<128;i++) CHECK(out[i]==pcm[(before+64)*2+i]*.5f);
    ready=0; ma_atomic_device_state_set(&device.state,ma_device_state_uninitialized);
    /* Attenuation happens after all contributors, preserving float headroom. */
    float loud[]={2,-2,2,-2,2,-2,2,-2}; setup((Sample){loud,4,2});
    audio_update(&fixture,1,1,0,1,.25f,0,0,0); audio_preview((Sample){loud,4,2});
    clicks[1][0]=.1f; clicks[0][0]=.1f; audio_metronome(1);
    callback(NULL,out,NULL,1);
    CHECK(fabsf(out[0]-.875f)<1e-6 && fabsf(out[1]+.825f)<1e-6);
    audio_metronome(0); audio_preview((Sample){0});
    audio_update(&fixture,1,1,0,1,1.25f,0,0,0); callback(NULL,out,NULL,1);
    CHECK(out[0]==1 && out[1]==-1);
    float peaks[INSERTS+1][2]; audio_meters(peaks); CHECK(peaks[0][0]>=2.5f);
    /* Automation continues inside the actual callback while the UI mailbox is locked. */
    float flat[2048]; for(int i=0;i<2048;i++) flat[i]=.2f;
    setup((Sample){flat,1024,2});
    int automation=automation_create(&fixture,(ParameterTarget){PARAM_CHANNEL_VOLUME,0,0},"Volume",16); CHECK(automation==0);
    fixture.automations[0].points[0].value=0; fixture.automations[0].points[1].value=1;
    CHECK(arrangement_place(&fixture,1,0,AUTOMATION_SOURCE,16)>=0);
    audio_update(&fixture,1,1,0,1,1,0,0,0); callback(NULL,out,NULL,64); CHECK(out[0]==0 && out[126]>0);
    float last=out[126]; ready=1; ma_atomic_device_state_set(&device.state,ma_device_state_started);
    pthread_mutex_lock(&mutex); callback(NULL,out,NULL,64); pthread_mutex_unlock(&mutex);
    CHECK(out[0]>last && out[126]>out[0] && project.volume[0]==1);
    ready=0; ma_atomic_device_state_set(&device.state,ma_device_state_uninitialized);
    float bad[]={NAN,INFINITY}; setup((Sample){bad,1,2}); callback(NULL,out,NULL,1);
    CHECK(out[0]==0 && out[1]==0);
    /* Sample replacements and arrangement edits catch up inside an already playing clip. */
    setup((Sample){pcm,1024,2}); callback(NULL,out,NULL,64);
    uint64_t edit_frame=player.frame; int64_t edit_tick=player.last_step;
    audio_sample(0,(Sample){pcm+40,1004,2}); callback(NULL,out,NULL,16);
    CHECK(player.frame==edit_frame+16 && player.last_step==edit_tick);
    CHECK(out[0]==pcm[(edit_frame+20)*2]); /* Resume at the song cursor, not frame zero. */
    fixture.clip_starts[0][0]=.0005f; edit_frame=player.frame;
    audio_update(&fixture,1,1,0,0,1,0,0,0); callback(NULL,out,NULL,64);
    CHECK(player.frame==edit_frame+64 && fabsf(out[0]-pcm[(edit_frame-48+20)*2])<1e-6);
    fixture.clip_starts[0][0]=.5f;
    audio_update(&fixture,1,1,0,0,1,0,0,0); callback(NULL,out,NULL,64); CHECK(out[0]==0);
    fixture.clips[0][0]=0; fixture.clips[2][0]=PATTERNS+1; fixture.clip_starts[2][0]=0;
    edit_frame=player.frame; audio_update(&fixture,1,1,0,0,1,0,0,0); callback(NULL,out,NULL,64);
    CHECK(out[0]==pcm[(edit_frame+20)*2] && player.lane_active[2] && !player.lane_active[0]);
    fixture.clip_offsets[2][0]=20.f/RATE; edit_frame=player.frame;
    audio_update(&fixture,1,1,0,0,1,0,0,0); callback(NULL,out,NULL,64);
    CHECK(fabsf(out[0]-pcm[(edit_frame+40)*2])<1e-6);
    /* Shortening an active clip stops it at the edited end; extending it resumes it. */
    fixture.clip_steps[2][0]=1.f/RATE; audio_update(&fixture,1,1,0,0,1,0,0,0);
    callback(NULL,out,NULL,64); CHECK(out[0]==0);
    fixture.clip_steps[2][0]=0; audio_update(&fixture,1,1,0,0,1,0,0,0); callback(NULL,out,NULL,64); CHECK(out[0]!=0);
    fixture.channel_count=2; Sample unaffected[CHANNELS]={{pcm+40,1004,2},{flat,1024,2}};
    audio_channels(&fixture,unaffected);
    player.voices[127]=(Voice){.channel=1,.sampler={.position=100,.speed=1},.remaining=-1,.gain=.2f,.lane=6,.audio_clip=0}; live.voices[127]=(Voice){.channel=1,.sampler={.position=100,.speed=1},.remaining=-1,.gain=.2f,.lane=-1,.audio_clip=0};
    audio_sample(0,(Sample){pcm+40,900,2}); callback(NULL,out,NULL,1);
    CHECK(player.voices[127].sampler.position==101 && live.voices[127].sampler.position==101);
    CHECK(player.voices[127].gain==.2f && live.voices[127].gain==.2f);
    /* A real producer/consumer exchange acknowledges replacements before free. */
    float *data=malloc(RATE*2*sizeof(float)); CHECK(data);
    for(unsigned i=0;i<RATE*2;i++) data[i]=.1f;
    setup((Sample){data,RATE,2}); ready=1; ma_atomic_device_state_set(&device.state,ma_device_state_started);
    atomic_store(&running,1); pthread_t worker; CHECK(!pthread_create(&worker,NULL,audio_thread,NULL));
    for(int n=0;n<32;n++) {
        float *next=malloc(RATE*2*sizeof(float)); CHECK(next);
        for(unsigned i=0;i<RATE*2;i++) next[i]=n%2?.2f:-.2f;
        audio_sample(0,(Sample){next,RATE,2}); free(data); data=next;
        Sample list[CHANNELS]={{data,RATE,2}}; audio_channels(&fixture,list);
        float *preview_data=malloc(256*sizeof(float)); CHECK(preview_data);
        for(unsigned i=0;i<256;i++) preview_data[i]=.3f;
        audio_preview((Sample){preview_data,128,2});
        audio_preview((Sample){0}); free(preview_data);
    }
    /* Queue pressure must preserve key-up and the final transport reset. */
    for(int i=0;i<1000;i++) audio_key(i%128,0,60,i%2);
    audio_key(7,0,60,1);
    CHECK(audio_stop()); CHECK(!audio_stop());
    CHECK(audio_key_position(7,0)<0);
    audio_update(&fixture,1,1,0,1,1,4,0,0);
    audio_update(&fixture,1,1,0,0,1,0,0,0); /* A later frame must not erase the seek. */
    audio_note(0,(Note){60,100,0,1}); audio_key(7,0,60,1);
    CHECK(audio_stop());
    atomic_store(&running,0); CHECK(!pthread_join(worker,NULL)); ready=0; ma_atomic_device_state_set(&device.state,ma_device_state_uninitialized);
    CHECK(!playing && !preview_remaining);
    CHECK(player.frame>=(uint64_t)(4*RATE*60.0/fixture.bpm/4));
    for(int i=0;i<128;i++) CHECK(!live.voices[i].gain && !player.voices[i].gain);
    /* A stopped device must not fill the queue and freeze editing. */
    CHECK(ma_mutex_init(&device.startStopLock)==MA_SUCCESS);
    ready=1; ma_atomic_device_state_set(&device.state,ma_device_state_stopped);
    for(int i=0;i<1000;i++) audio_update(&fixture,0,1,0,0,1,0,0,0);
    CHECK(pending.read==pending.write);
    audio_preview((Sample){data,RATE,2}); CHECK(audio_stop());
    ready=0; ma_mutex_uninit(&device.startStopLock);
    ma_atomic_device_state_set(&device.state,ma_device_state_uninitialized);
    free(data);
    /* Capture enters unused mixer buses and taps their post-fader routed audio. */
    setup((Sample){0}); memset(fixture.clips,0,sizeof fixture.clips);
    fixture.insert_count=2; fixture.insert_volume[0]=fixture.insert_volume[1]=.5f;
    CHECK(insert_connect(&fixture,1,2));
    audio_update(&fixture,1,1,0,1,1,0,0,0);
    recording.count=3; recording.devices_count=1;
    recording.buses[0]=1; recording.buses[1]=2; recording.buses[2]=0;
    recording.sources[0]=0; recording.sources[1]=recording.sources[2]=-1;
    recording.inputs[0].pcm=calloc(RECORD_RING_FRAMES*2,sizeof(float));
    for(int i=0;i<3;i++) recording.takes[i].pcm=calloc(RECORD_RING_FRAMES*2,sizeof(float));
    recording.io.active[0]=recording.io.active[1]=recording.io.active[2]=1;
    recording.io.input=record_input; recording.io.output=record_output;
    recording.active=1;
    float input[128]; for(int i=0;i<64;i++) { input[i*2]=.6f; input[i*2+1]=-.4f; }
    ma_device capture={0}; capture.pUserData=&recording.inputs[0]; capture_callback(&capture,NULL,input,64);
    callback(NULL,out,NULL,64);
    float recorded[128];
    CHECK(audio_record_read(0,recorded,64)==64);
    for(int i=0;i<64;i++) { CHECK(recorded[i*2]==.3f && recorded[i*2+1]==-.2f); CHECK(out[i*2]==.15f && out[i*2+1]==-.1f); }
    CHECK(audio_record_read(1,recorded,64)==64 && recorded[0]==.15f && recorded[1]==-.1f);
    CHECK(audio_record_read(2,recorded,64)==64 && recorded[0]==.15f && recorded[1]==-.1f);
    CHECK(audio_record_read(0,recorded,64)==0);
    /* One capture stream can feed two armed inserts without consuming it twice. */
    recording.sources[1]=0; capture_callback(&capture,NULL,input,64); callback(NULL,out,NULL,64);
    CHECK(audio_record_read(0,recorded,32)==32 && recorded[0]==.3f);
    CHECK(audio_record_read(0,recorded,64)==32 && recorded[0]==.3f);
    CHECK(audio_record_read(1,recorded,64)==64 && fabsf(recorded[0]-.45f)<.000001f && fabsf(recorded[1]+.3f)<.000001f);
    CHECK(audio_record_read(2,recorded,64)==64 && fabsf(recorded[0]-.45f)<.000001f);
    /* Recording keeps the Song cursor moving beyond the previous end. */
    player.frame=RATE*3; callback(NULL,out,NULL,64); CHECK(player.frame==RATE*3+64);
    /* Ring wraparound preserves order, and overflow is reported instead of overwriting. */
    RecordRing *ring=&recording.inputs[0];
    atomic_store(&ring->read,RECORD_RING_FRAMES-1); atomic_store(&ring->write,RECORD_RING_FRAMES-1);
    CHECK(ring_push(ring,.2f,.4f) && ring_push(ring,.6f,.8f));
    float pair[2]; CHECK(ring_pop(ring,pair) && pair[0]==.2f && pair[1]==.4f);
    CHECK(ring_pop(ring,pair) && pair[0]==.6f && pair[1]==.8f);
    atomic_store(&ring->write,atomic_load(&ring->read)+RECORD_RING_FRAMES);
    CHECK(!ring_push(ring,1,1) && atomic_load(&ring->overflow));
    recording.active=0; CHECK(audio_record_failed()); record_release();
    /* FM uses the real keyboard/note mailbox, without needing sample PCM. */
    ready=0; project_new(&fixture); fixture.instrument[0]=INSTRUMENT_FM; fixture.fm[0].release=.01f;
    audio_stop(); audio_update(&fixture,0,0,0,1,1,0,0,0);
    audio_key(0,0,69,1); callback(NULL,out,NULL,64);
    CHECK(live.voices[0].instrument==INSTRUMENT_FM && fm_active(&live.voices[0].fm));
    CHECK(audio_key_position(0,0)>=0 && out[20]!=0);
    audio_key(0,0,69,0);
    for(int i=0;i<20;i++) callback(NULL,out,NULL,64);
    CHECK(!live.voices[0].gain && audio_key_position(0,0)<0);
    audio_note(0,(Note){69,127,0,.25f}); callback(NULL,out,NULL,64);
    CHECK(live.voices[127].instrument==INSTRUMENT_FM && out[20]!=0);
    for(int i=0;i<40;i++) callback(NULL,out,NULL,64);
    CHECK(!live.voices[127].gain);
    /* Song and Pattern playback expose the sounding source notes to editors. */
    static uint8_t activity[CHANNELS][NOTES];
    project_new(&fixture); fixture.instrument[0]=INSTRUMENT_FM;
    fixture.fm[0].release=.01f; fixture.notes[0][0][0]=(Note){60,100,0,2}; fixture.clips[0][0]=1;
    audio_update(&fixture,1,1,0,1,1,0,0,0); callback(NULL,out,NULL,64);
    audio_pattern_activity(0,activity); CHECK(activity[0][0] && player.voices[0].note_id==1);
    audio_pattern_activity(1,activity); CHECK(!activity[0][0]);
    fixture.lane_mute[0]=1; audio_update(&fixture,1,1,0,0,1,0,0,0); callback(NULL,out,NULL,64);
    audio_pattern_activity(0,activity); CHECK(!activity[0][0]);
    fixture.lane_mute[0]=0; audio_update(&fixture,1,0,0,1,1,0,0,0); callback(NULL,out,NULL,64);
    audio_pattern_activity(0,activity); CHECK(activity[0][0]);
    audio_stop(); audio_pattern_activity(0,activity); CHECK(!activity[0][0]);
    puts("Transparent output, capture, post-fader recording, ring bounds, mixing and ordered controls passed.");
    return 0;
}
