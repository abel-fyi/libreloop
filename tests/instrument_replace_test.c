// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include "history.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
static Project project,before,loaded;
int main(void) {
    project_new(&project); project.volume[0]=.7f; project.pan[0]=-.2f; project.route[0]=1;
    strcpy(project.channel_names[0],"Lead"); strcpy(project.paths[0],"/original/lead.wav");
    project.sampler[0].flags=SAMPLE_REVERSE;
    project.notes[0][0][0]=(Note){60,100,1,4}; project.notes[1][0][0]=(Note){64,90,3,2};
    project.clips[0][0]=1; project.clip_starts[0][0]=2; project.clip_steps[0][0]=8; project.clip_offsets[0][0]=1;
    before=project;
    CHECK(!channel_replace_instrument(&project,-1,INSTRUMENT_FM)); CHECK(!channel_replace_instrument(&project,0,99));
    CHECK(!memcmp(&project,&before,sizeof project));
    float pcm[]={.1f,.2f}; Sample sources[CHANNELS]={{.data=pcm,.frames=2}},retained[CHANNELS]; uint64_t stamps[CHANNELS]={1}; History history={0};
    CHECK(history_capture(&history,&project,sources,stamps));
    CHECK(channel_replace_instrument(&project,0,INSTRUMENT_FM));
    CHECK(project.instrument[0]==INSTRUMENT_FM && !memcmp(project.notes,before.notes,sizeof project.notes));
    CHECK(!memcmp(project.clips,before.clips,sizeof project.clips) && !memcmp(project.clip_starts,before.clip_starts,sizeof project.clip_starts));
    CHECK(!memcmp(project.clip_steps,before.clip_steps,sizeof project.clip_steps) && !memcmp(project.clip_offsets,before.clip_offsets,sizeof project.clip_offsets));
    CHECK(project.route[0]==1 && project.volume[0]==.7f && project.pan[0]==-.2f && !strcmp(project.paths[0],before.paths[0]));
    CHECK(!strcmp(project.channel_names[0],"Lead") && project.sampler[0].flags==SAMPLE_REVERSE);
    CHECK(project_save("instrument-replace.hbt",&project) && project_load("instrument-replace.hbt",&loaded));
    CHECK(loaded.instrument[0]==INSTRUMENT_FM && loaded.notes[0][0][0].velocity==100 && loaded.route[0]==1);
    remove("instrument-replace.hbt");
    CHECK(history_capture(&history,&project,sources,stamps));
    CHECK(history_peek(&history,-1,&loaded,retained) && loaded.instrument[0]==INSTRUMENT_SAMPLER && retained[0].data[1]==.2f);
    history_clear(&history);
    int shared=automation_create(&project,(ParameterTarget){PARAM_CHANNEL_VOLUME,0,0},"Volume",16);
    int fm=automation_create(&project,(ParameterTarget){PARAM_FM_RATIO,0,0},"Ratio",16);
    CHECK(shared>=0 && fm>=0); project.clips[1][0]=AUTOMATION_SOURCE+fm+1;
    CHECK(channel_replace_instrument(&project,0,INSTRUMENT_SAMPLER));
    CHECK(project.automation_count==1 && project.automations[0].target.parameter==PARAM_CHANNEL_VOLUME && !project.clips[1][0]);
    CHECK(project.notes[1][0][0].pitch==64 && project.clips[0][0]==1);
    project.channel_audio[0]=1; before=project;
    CHECK(!channel_replace_instrument(&project,0,INSTRUMENT_FM) && !memcmp(&project,&before,sizeof project));
    puts("Instrument replacement retains notes, clips, routing, samples, persistence and undo; Audio channels stay compatible."); return 0;
}
