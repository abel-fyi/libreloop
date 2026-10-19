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
    effects_free(effects);effects=0;return 0;
}
