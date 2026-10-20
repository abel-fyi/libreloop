// SPDX-License-Identifier: GPL-3.0-only
/* Actual callback, mailbox and Master rack; no audio device is opened. */
#include "../src/platform/audio_miniaudio.c"
#include <stdio.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
static Project fixture;static float pcm[8192],dry[8192],wet[8192],chunked[8192];
static void setup(int phaser,int note,float master,int mute) {
    ready=0;memset(&pending,0,sizeof pending);memset(snapshots,0,sizeof snapshots);control_depth=0;atomic_store(&acknowledged,0);
    player_reset(&player);player_reset(&live);player.effects=live.effects=effects;effects_reset(effects);
    project_new(&fixture);fixture.volume[0]=1;fixture.master=master;fixture.master_mute=mute;
    if(phaser){fixture.effect_type[0][0]=EFFECT_PHASER;fixture.phaser[0][0]=phaser_default();fixture.effect_mix[0][0]=1;}
    Sample s={.data=pcm,.frames=4096,.channels=2},list[CHANNELS]={s};audio_channels(&fixture,list);audio_update(&fixture,0,1,0,1,1,0,0,0);
    if(note)audio_note(0,(Note){60,127,0,16});else audio_preview(s);
}
int main(void) {
    for(int i=0;i<4096;i++){pcm[2*i]=.2f*sinf(i*.17f);pcm[2*i+1]=.2f*cosf(i*.11f);}
    effects=effects_create(INSERTS+1);CHECK(effects);
    for(int note=0;note<2;note++){
        setup(0,note,1,0);callback(0,dry,0,4096);CHECK(callback_taps==4096);
        setup(1,note,1,0);callback(0,wet,0,4096);CHECK(callback_taps==4096);
        double difference=0;for(int i=0;i<8192;i++){CHECK(isfinite(wet[i]));difference+=fabsf(wet[i]-dry[i]);}CHECK(difference>10);
        setup(1,note,1,0);for(int at=0;at<4096;){int n=at+37<4096?37:4096-at;callback(0,chunked+at*2,0,n);at+=n;}CHECK(!memcmp(wet,chunked,sizeof wet));
        setup(0,note,.5f,0);callback(0,wet,0,4096);for(int i=0;i<8192;i++)CHECK(wet[i]==dry[i]*.5f);
        setup(1,note,1,1);callback(0,wet,0,4096);for(int i=0;i<8192;i++)CHECK(wet[i]==0);
    }
    /* The click uses the same Master path as previews and sequenced audio. */
    clicks_init();
    for(int mode=0;mode<4;mode++) {
        setup(mode==3,0,mode==1?.5f:1,mode==2);
        audio_preview((Sample){0});audio_update(&fixture,1,0,0,1,1,0,0,0);audio_metronome(1);audio_spectrum_bus(0);
        callback(0,mode?wet:dry,0,4096);
        float *out=mode?wet:dry;double energy=0,difference=0;
        for(int i=0;i<8192;i++){energy+=fabsf(out[i]);difference+=fabsf(out[i]-dry[i]);if(mode==1)CHECK(out[i]==dry[i]*.5f);if(mode==2)CHECK(out[i]==0);}
        if(mode!=2)CHECK(energy>1);
        if(mode==3)CHECK(difference>1);
        float tapped[8192];unsigned elapsed;
        CHECK(analyzer_read(1,tapped,4096,&elapsed,INFINITY)==4096);
        CHECK(!memcmp(tapped,out,sizeof tapped));
    }
    setup(1,0,1,0);audio_preview((Sample){0});audio_update(&fixture,1,0,0,1,1,0,0,0);audio_metronome(1);
    for(int at=0;at<4096;){int n=at+37<4096?37:4096-at;callback(0,chunked+at*2,0,n);at+=n;}
    CHECK(!memcmp(wet,chunked,sizeof wet));audio_metronome(0);
    /* Both audio and MIDI recording must retain Pattern looping. Song takes
       still run past their end, and the visual transport uses the same rule. */
    for(int song=0;song<2;song++) {
        setup(0,0,1,0);audio_preview((Sample){0});
        audio_update(&fixture,1,song,0,1,1,15.995f,0,16);
        audio_record_mode(1);recording.active=1;
        callback(0,wet,0,4096);
        CHECK(song?player.frame>96000:player.frame<4096);
        CHECK(song?isinf(visual_end):isfinite(visual_end));
        recording.active=0;audio_record_mode(0);
    }
    /* A stalled reader gets the newest complete window, with no stale backlog. */
    unsigned elapsed;float latest[8192];
    for(unsigned i=0;i<20000;i++)analyzer_push(0,(float)i,-(float)i);
    analyzer_publish(0,0,0);
    CHECK(analyzer_read(0,latest,4096,&elapsed,INFINITY)==4096 && elapsed>=20000);
    for(unsigned i=0;i<4096;i++){CHECK(latest[2*i]==15904+i);CHECK(latest[2*i+1]==-(float)(15904+i));}
    CHECK(!analyzer_read(0,latest,4096,&elapsed,INFINITY) && !elapsed);
    audio_spectrum_bus(1);CHECK(!analyzer_read(1,latest,4096,&elapsed,INFINITY));
    /* A 75 ms callback still yields distinct real FFT windows at 60 Hz.
       The reader retains its slot safely until the next publication. */
    Analyzer *a=&analyzer[0];a->cursor=a->available=a->total=a->writer=a->read_total=a->valid=0;
    a->reader=1;atomic_store(&a->middle,2);
    for(unsigned i=0;i<8192;i++)analyzer_push(0,(float)i,-(float)i);
    analyzer_publish(0,3600,10);
    for(unsigned tick=0;tick<=5;tick++) {
        unsigned end=4592+(tick*800<3600?tick*800:3600);
        CHECK(analyzer_read(0,latest,4096,&elapsed,10+tick/60.)==4096);
        CHECK(elapsed==(tick?end-(4592+(tick-1)*800):4592));
        for(unsigned i=0;i<4096;i++)CHECK(latest[2*i]==end-4096+i && latest[2*i+1]==-(float)(end-4096+i));
    }
    CHECK(!analyzer_read(0,latest,4096,&elapsed,10.09) && !elapsed);
    CHECK(!analyzer_read(0,latest,4096,&elapsed,10.04) && !elapsed);
    for(unsigned i=8192;i<11792;i++)analyzer_push(0,(float)i,-(float)i);
    analyzer_publish(0,3600,10.075);
    CHECK(analyzer_read(0,latest,4096,&elapsed,10+5/60.)==4096 && elapsed==400);
    CHECK(latest[8190]==8591 && latest[8191]==-8591);
    /* Resuming an idle UI after a long session must not wrap its sample clock. */
    a->total+=UINT64_C(1)<<32;analyzer_publish(0,0,0);
    CHECK(analyzer_read(0,latest,4096,&elapsed,INFINITY)==4096 && elapsed==UINT_MAX);
    effects_free(effects);effects=0;return 0;
}
