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
    setup((Sample){pcm,1024,2}); float out[128]; callback(NULL,out,NULL,64);
    CHECK(!memcmp(out,pcm,sizeof out)); /* Device output is transparent at unity. */
    /* A UI lock may postpone a volume edit, but never silence or rewind playback. */
    ready=1; ma_atomic_device_state_set(&device.state,ma_device_state_started);
    audio_key(127,0,60,1); CHECK(audio_key_position(127,0)==0);
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
    puts("Transparent output, lock contention, mixing headroom, final clipping, sample lifetime and ordered controls passed.");
    return 0;
}
