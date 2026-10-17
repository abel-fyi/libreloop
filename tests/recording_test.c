// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
static Project project;
static float taps[INSERTS+1][2];
static unsigned tapped[INSERTS+1];
static void input(void *context,float buses[INSERTS+1][2]) {
    (void)context; buses[1][0]+=.8f; buses[1][1]+=.4f;
}
static void output(void *context,int bus,float left,float right) {
    (void)context; taps[bus][0]=left; taps[bus][1]=right; tapped[bus]++;
}
int main(void) {
    project_new(&project); project.insert_count=3;
    project.insert_volume[0]=.5f; project.insert_volume[1]=.5f;
    CHECK(insert_connect(&project,1,2));
    MixerIO io={.input=input,.output=output}; io.active[0]=io.active[1]=io.active[2]=1;
    Player player; player_reset(&player); player.song=1; Sample samples[CHANNELS]={0}; float pcm[128]={0};
    render_mixer_io(&player,NULL,&project,samples,pcm,64,1,NULL,&io);
    CHECK(tapped[0]==64 && tapped[1]==64 && tapped[2]==64);
    CHECK(taps[1][0]==.4f && taps[1][1]==.2f && taps[2][0]==.2f && taps[2][1]==.1f);
    CHECK(pcm[0]==.2f && pcm[1]==.1f);
    project.insert_width[0]=0; memset(pcm,0,sizeof pcm);
    render_mixer_io(&player,NULL,&project,samples,pcm,64,0,NULL,&io);
    CHECK(fabsf(taps[1][0]-.3f)<.00001f && taps[1][0]==taps[1][1]);
    CHECK(fabsf(pcm[0]-.15f)<.00001f && pcm[0]==pcm[1]);
    project.insert_mute[0]=1;
    render_mixer_io(&player,NULL,&project,samples,pcm,64,1,NULL,&io); CHECK(taps[1][0]==0 && taps[0][0]==0);
    project.insert_mute[0]=0; project.insert_mute[2]=2;
    render_mixer_io(&player,NULL,&project,samples,pcm,64,1,NULL,&io); CHECK(taps[1][0]==0);
    project.insert_mute[2]=0; project.insert_mute[1]=2;
    render_mixer_io(&player,NULL,&project,samples,pcm,64,1,NULL,&io); CHECK(taps[1][0]>.29f);
    player.frame=RATE*20; uint64_t before=player.frame;
    render_mixer_io(&player,NULL,&project,samples,pcm,64,1,NULL,&io); CHECK(player.frame==before+64);
    CHECK(insert_connect(&project,1,255));
    render_mixer_io(&player,NULL,&project,samples,pcm,64,1,NULL,&io);
    CHECK(taps[0][0]==0); /* A disconnected armed insert still has its own take. */
    project.insert_mute[1]=0;
    render_mixer_io(&player,NULL,&project,samples,pcm,64,1,NULL,&io); CHECK(taps[1][0]>.29f && taps[0][0]==0);
    puts("Recording inputs, post-fader taps, width, mute/solo, routing and continuous transport passed."); return 0;
}
