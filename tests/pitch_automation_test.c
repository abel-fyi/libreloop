// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include "arrangement.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
static Project p;
static float pcm[RATE*2],out[RATE*10],split[RATE*10];
static void setup(void) {
    project_new(&p); p.pitch_range[0]=12; p.channel_audio[0]=1; p.audio_seconds[0]=2;
    arrangement_place(&p,0,0,PATTERNS,16);
    int a=automation_create(&p,(ParameterTarget){PARAM_CHANNEL_PITCH,0,0},"Pitch",64);
    arrangement_place(&p,1,0,AUTOMATION_SOURCE+a,64);
}
int main(void) {
    for(unsigned i=0;i<RATE*2;i++) pcm[i]=.1f+.2f*i/(RATE*2);
    Sample samples[CHANNELS]={{pcm,RATE*2,1}}; setup();
    p.automations[0].points[0].value=p.automations[0].points[1].value=0;
    CHECK(fabsf(clip_length(&p,0,0)-32)<1e-5); /* -12 semitones doubles duration */
    Player player; player_reset(&player); player.song=1; render(&player,&p,samples,out,RATE*5);
    CHECK(out[RATE*6]>.24f && out[RATE*8]==0); /* still audible after the old two-second edge */
    CHECK(export_wav("pitch-automation.wav",&p,samples)); FILE *f=fopen("pitch-automation.wav","rb"); CHECK(f);
    CHECK(!fseek(f,44+RATE*3*4,SEEK_SET)); int lo=fgetc(f),hi=fgetc(f); CHECK((int16_t)(lo|(hi<<8))>7000); fclose(f); remove("pitch-automation.wav");
    /* Isolate the audio end: a short automation changes pitch during playback, then holds its final value. */
    p.automations[0].steps=8; p.automations[0].points[1].step=8; p.clip_steps[1][0]=8;
    CHECK(fabsf(clip_length(&p,0,0)-32)<1e-5);
    player_reset(&player); player.song=1; render(&player,&p,samples,out,RATE*3);
    CHECK(out[RATE*4]>.19f); CHECK(fabsf(song_steps(&p)-32)<1e-5);
    p.automations[0].points[0].value=p.automations[0].points[1].value=1;
    CHECK(fabsf(clip_length(&p,0,0)-8)<1e-5); /* +12 semitones halves duration */
    setup(); p.automations[0].steps=16; p.automations[0].points[1].step=16; p.clip_steps[1][0]=16;
    p.automations[0].points[0].value=0; p.automations[0].points[1].value=.5f;
    /* A ramp from half speed to unity consumes 16*.5/log(2) source steps;
       the remaining source plays at unity after the automation ends. */
    double expected=16+(16-8/log(2)); CHECK(fabs(clip_length(&p,0,0)-expected)<1e-5);
    player_reset(&player); player.song=1; render(&player,&p,samples,out,RATE*3);
    player_reset(&player); player.song=1; render(&player,&p,samples,split,12345); render(&player,&p,samples,split+24690,RATE*3-12345);
    CHECK(!memcmp(out,split,RATE*6*sizeof(float)));
    double position=audio_clip_position(&p,0,0,12);
    player_reset(&player); player.song=1; player_seek(&player,&p,12); render(&player,&p,samples,split,1);
    CHECK(fabs(split[0]-(.1f+.2f*position/(RATE*2)))<1e-5);
    p.lane_mute[1]=1; CHECK(clip_length(&p,0,0)==16); p.lane_mute[1]=0;
    p.clip_offsets[0][0]=.5f; CHECK(clip_length(&p,0,0)<expected);
    p.clip_steps[0][0]=1; CHECK(clip_length(&p,0,0)==8); /* explicit crop remains a playback boundary */
    p.clip_offsets[0][0]=p.clip_steps[0][0]=0;
    p.automations[0].target.parameter=PARAM_MASTER_PITCH;
    p.automations[0].points[0].value=p.automations[0].points[1].value=0;
    CHECK(fabsf(clip_length(&p,0,0)-32)<1e-5); /* half speed continues after the clip ends */
    /* Automated pitch range uses the same rounded semitone values as rendering. */
    setup(); p.channel_pitch[0]=1; p.automations[0].target.parameter=PARAM_PITCH_RANGE;
    p.automations[0].points[0].value=0; p.automations[0].points[1].value=1;
    AudioTimeline map; audio_timeline_init(&map,&p,0);
    double duration=audio_timeline_duration(&map,0,16),numeric=0,dt=duration/10000;
    for(int i=0;i<10000;i++) {
        double step=(i+.5)*dt,normalized=automation_value(&p.automations[0],step);
        numeric+=exp2(round(1+47*normalized)/12)*dt;
    }
    CHECK(fabs(numeric-16)<.01);
    setup(); Automation *curve=&p.automations[0];
    curve->steps=16; curve->points[0].value=0; curve->points[1].step=16; curve->points[1].value=1; p.clip_steps[1][0]=16;
    CHECK(automation_point(curve,4,0)==1);
    CHECK(automation_move_point(&p,0,2,-10,1)==2 && curve->points[2].step==4);
    CHECK(fabsf(clip_length(&p,0,0)-11)<1e-5); /* half speed for four steps, then double speed */
    audio_timeline_init(&map,&p,0); CHECK(fabs(audio_timeline_source(&map,0,11)-16)<1e-6);
    player_reset(&player); player.song=1; render(&player,&p,samples,out,RATE*2);
    CHECK(isfinite(out[RATE]) && fabsf(out[RATE]-(.1f+.2f*12000/(RATE*2)))<1e-5);
    for(int i=0;i<RATE*4;i++) CHECK(isfinite(out[i]));
    curve->points[1].step=0; curve->points[2].step=0;
    CHECK(automation_value(curve,0)==1 && fabsf(clip_length(&p,0,0)-8)<1e-5);
    return 0;
}
