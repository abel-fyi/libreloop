// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include "arrangement.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
static Project project;
static float pcm[RATE*2],out[RATE*2];
static float frequency(void) {
    unsigned crosses=0;for(unsigned i=RATE/4+1;i<RATE*3/4;i++)if(out[(i-1)*2]<=0 && out[i*2]>0)crosses++;
    return crosses*2.f;
}
int main(void) {
    for(unsigned i=0;i<RATE*2;i++)pcm[i]=.2f*sinf(i*440*6.283185307179586/RATE);
    Sample samples[CHANNELS]={{pcm,RATE*2,1}};
    for(int stretch=0;stretch<2;stretch++)for(int control=0;control<3;control++) {
        project_new(&project);project.master=1;project.sampler[0].stretch=stretch;
        project.pitch_range[0]=12;project.channel_pitch[0]=control==0;
        project.master_pitch=control==1?12:0;
        Player player;player_reset(&player);
        player.voices[0]=(Voice){.channel=0,.sampler={.speed=control==2?2:1},.remaining=-1,.gain=1,.lane=-1};
        memset(out,0,sizeof out);render_live(&player,&project,samples,out,RATE);
        CHECK(player.voices[0].sampler.position==(stretch?RATE:RATE*2));
        CHECK(fabsf(frequency()-880)<10);
    }
    project_new(&project);project.sampler[0].stretch=1;project.channel_audio[0]=1;project.audio_seconds[0]=2;
    CHECK(arrangement_place(&project,0,0,PATTERNS,32)>=0);
    float length=clip_length(&project,0,0),source_steps=clip_source_steps(&project,PATTERNS);
    double cursor=audio_clip_position(&project,0,0,2);
    project.master_pitch=12;project.channel_pitch[0]=1;project.pitch_range[0]=12;
    CHECK(clip_length(&project,0,0)==length && clip_source_steps(&project,PATTERNS)==source_steps);
    CHECK(audio_clip_position(&project,0,0,2)==cursor);
    int a=automation_create(&project,(ParameterTarget){PARAM_MASTER_PITCH,0,0},"Pitch",32);CHECK(a>=0);
    project.clips[1][0]=AUTOMATION_SOURCE+a+1;project.automations[a].points[0].value=0;project.automations[a].points[1].value=1;
    CHECK(clip_length(&project,0,0)==length && audio_clip_position(&project,0,0,2)==cursor);
    project.sampler[0].stretch=0;CHECK(clip_length(&project,0,0)!=length || audio_clip_position(&project,0,0,2)!=cursor);
    puts("Sampler, master and note pitch preserve Stretch duration and shift frequency; Resample speed changes; clip geometry/automation stay fixed in Stretch.");return 0;
}
