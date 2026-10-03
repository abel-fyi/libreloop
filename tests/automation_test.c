// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include "arrangement.h"
#include "project_check.h"
#include <math.h>
#include <stdlib.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
static Project p,q;
int main(void) {
    project_new(&p); p.insert_count=1;
    ParameterTarget target={PARAM_CHANNEL_VOLUME,0,0},found;
    CHECK(parameter_from_pointer(&p,&p.volume[0],&found) && found.parameter==target.parameter);
    int a=automation_create(&p,target,"Sampler volume",16); CHECK(a==0);
    Automation *curve=&p.automations[a];
    curve->points[0].value=0; curve->points[1].value=1;
    CHECK(fabsf(automation_value(curve,8)-.5f)<1e-6);
    CHECK(automation_value(curve,-1)==0 && automation_value(curve,20)==1);
    CHECK(automation_point(curve,8,.25f)==1 && curve->count==3);
    CHECK(automation_point(curve,8,.5f)==1 && curve->count==3);
    CHECK(automation_point(curve,NAN,0)<0 && automation_point(curve,1,2)<0);
    CHECK(arrangement_place(&p,1,0,AUTOMATION_SOURCE,16)>=0);
    CHECK(clip_length(&p,1,0)==16 && clip_source_steps(&p,AUTOMATION_SOURCE)==16);
    p.bpm=60; CHECK(clip_length(&p,1,0)==16); p.bpm=120;
    CHECK(project_save("automation.hbt",&p) && project_load("automation.hbt",&q)); CHECK(project_equal(&p,&q));
    curve->points[1].step=-1; CHECK(!project_save("bad-automation.hbt",&p)); curve->points[1].step=8;
    /* Known targets keep normalized values; unresolved future plugin targets persist. */
    p.automations[0].target=(ParameterTarget){PARAM_PLUGIN+4,7,2};
    CHECK(project_save("automation.hbt",&p) && project_load("automation.hbt",&q)); CHECK(project_equal(&p,&q));
    p.automations[0].target=target;
    Sample samples[CHANNELS]={0}; samples[0].frames=RATE*2; samples[0].data=malloc(samples[0].frames*sizeof(float)); CHECK(samples[0].data);
    for(unsigned i=0;i<samples[0].frames;i++) samples[0].data[i]=.2f;
    p.channel_audio[0]=1; p.audio_seconds[0]=2; p.master=1; p.volume[0]=1;
    CHECK(arrangement_place(&p,0,0,PATTERNS,16)>=0);
    Player player={0}; player_reset(&player); player.song=1;
    float output[2048];
    render(&player,&p,samples,output,1024); CHECK(output[0]==0 && output[2046]>0);
    player_seek(&player,&p,8); render(&player,&p,samples,output,1024);
    CHECK(fabsf(output[0]-.125f)<.001f); /* normalized .5 maps to gain .625 */
    CHECK(fabsf(output[2]-output[0])<.0001f); /* sample-accurate ramp */
    p.lane_mute[1]=1; player_seek(&player,&p,8); render(&player,&p,samples,output,1024); CHECK(fabsf(output[0]-.2f)<.001f);
    p.lane_mute[1]=0;
    float normalized=-1; CHECK(automation_evaluate(&p,target,8,&normalized) && normalized==.5f);
    CHECK(automation_evaluate(&p,target,16,&normalized) && normalized==1);
    /* Crop offsets are evaluated in source steps, independent of tempo. */
    p.clip_offsets[1][0]=4; p.clip_steps[1][0]=8;
    CHECK(automation_evaluate(&p,target,0,&normalized) && normalized==.25f);
    player_seek(&player,&p,0); render(&player,&p,samples,output,1); CHECK(fabsf(output[0]-.0625f)<1e-6);
    p.clip_offsets[1][0]=0; p.clip_steps[1][0]=16;
    /* Bus automation and switches use the same renderer, including saved mute overrides. */
    curve->target=(ParameterTarget){PARAM_MASTER_VOLUME,0,0};
    player_seek(&player,&p,8); render(&player,&p,samples,output,1); CHECK(fabsf(output[0]-.2f)<1e-6);
    curve->target=(ParameterTarget){PARAM_CHANNEL_PAN,0,0}; curve->points[0].value=curve->points[1].value=curve->points[2].value=1;
    player_seek(&player,&p,0); render(&player,&p,samples,output,1); CHECK(output[0]==0 && fabsf(output[1]-.2f)<1e-6);
    curve->target=(ParameterTarget){PARAM_CHANNEL_MUTE,0,0};
    player_seek(&player,&p,0); render(&player,&p,samples,output,1); CHECK(output[0]==0 && output[1]==0);
    p.mute[0]=1; for(int n=0;n<curve->count;n++) curve->points[n].value=0;
    player_seek(&player,&p,0); render(&player,&p,samples,output,1); CHECK(fabsf(output[0]-.2f)<1e-6); p.mute[0]=0;
    p.route[0]=1; curve->target=(ParameterTarget){PARAM_INSERT_VOLUME,0,0};
    for(int n=0;n<curve->count;n++) curve->points[n].value=.25f;
    player_seek(&player,&p,0); render(&player,&p,samples,output,1); CHECK(fabsf(output[0]-.1f)<1e-6);
    CHECK(export_wav("automation.wav",&p,samples));
    FILE *wav=fopen("automation.wav","rb"); CHECK(wav && !fseek(wav,44,SEEK_SET));
    unsigned low=(unsigned)fgetc(wav),high=(unsigned)fgetc(wav); fclose(wav); CHECK(abs((int16_t)(low|(high<<8))-3277)<=1); remove("automation.wav");
    p.route[0]=0; curve->target=target;
    curve->points[0].value=0; curve->points[1].value=.5f; curve->points[2].value=1;
    /* Pattern mode ignores Song automation; loop playback re-evaluates at its start. */
    player_reset(&player); player.song=1; player.loop_start=4; player.loop_end=8; player_seek(&player,&p,8);
    render(&player,&p,samples,output,1); CHECK(fabsf(output[0]-.0625f)<1e-6);
    CHECK(automation_create(&p,target,"Later volume",16)==1);
    CHECK(arrangement_place(&p,4,0,AUTOMATION_SOURCE+1,16)>=0);
    p.automations[1].points[0].value=p.automations[1].points[1].value=.1f;
    CHECK(automation_evaluate(&p,target,8,&normalized) && fabsf(normalized-.1f)<1e-6);
    player_seek(&player,&p,4); render(&player,&p,samples,output,1); CHECK(fabsf(output[0]-.025f)<1e-6);
    CHECK(automation_delete(&p,1));

    Arrangement edit={.source_pattern=AUTOMATION_SOURCE,.source_steps=16,.snap=1};
    arrangement_press(&edit,&p,.5,1.5,0,0,0,0); arrangement_drag(&edit,&p,2.5,2.5); arrangement_release(&edit);
    CHECK(arrangement_hit(&p,2,2.5)>=0);
    int slot=arrangement_hit(&p,2,2.5); p.clip_offsets[2][slot]=4; p.clip_steps[2][slot]=8;
    CHECK(clip_offset_steps(&p,2,slot)==4 && clip_length(&p,2,slot)==8);
    CHECK(automation_create(&p,(ParameterTarget){PARAM_MASTER_VOLUME,0,0},"Master",16)==1);
    CHECK(arrangement_place(&p,3,0,AUTOMATION_SOURCE+1,16)>=0);
    CHECK(automation_delete(&p,0) && p.automation_count==1 && p.clips[3][0]==AUTOMATION_SOURCE+1 && !p.clips[2][slot]);
    CHECK(channel_delete(&p,0) && p.automation_count==1); /* Master automation survives. */
    CHECK(automation_create(&p,(ParameterTarget){PARAM_CHANNEL_VOLUME,0,0},"Deleted owner",16)<0);
    project_new(&q); q.channel_count=2;
    CHECK(automation_create(&q,(ParameterTarget){PARAM_CHANNEL_VOLUME,1,0},"Second channel",16)==0);
    CHECK(channel_delete(&q,0) && q.automation_count==1 && q.automations[0].target.owner==0);
    CHECK(channel_delete(&q,0) && q.automation_count==0);
    project_new(&q); CHECK(automation_create(&q,(ParameterTarget){PARAM_MASTER_VOLUME,0,0},"Resize",16)==0);
    CHECK(arrangement_place(&q,0,2,AUTOMATION_SOURCE,16)>=0);
    CHECK(arrangement_place(&q,1,2,AUTOMATION_SOURCE,16)>=0);
    int resized=arrangement_hit(&q,0,2),copy=arrangement_hit(&q,1,2);
    Arrangement resize={.source_pattern=AUTOMATION_SOURCE,.source_steps=16,.snap=1};
    arrangement_press(&resize,&q,2.99f,.5f,0,1,0,0); arrangement_drag(&resize,&q,5,.5f); arrangement_release(&resize);
    CHECK(clip_length(&q,0,resized)==48 && q.automations[0].steps==16);
    arrangement_press(&resize,&q,2.99f,.5f,0,1,0,0); arrangement_drag(&resize,&q,2.5f,.5f); arrangement_release(&resize);
    CHECK(clip_length(&q,0,resized)==8);
    arrangement_press(&resize,&q,2,.5f,0,-1,0,0); arrangement_drag(&resize,&q,1,.5f); arrangement_release(&resize);
    CHECK(q.clip_starts[0][resized]==1 && clip_length(&q,0,resized)==24 && q.clip_offsets[1][copy]==16);
    int moved=automation_move_point(&q,0,1,80,.2f);
    CHECK(moved==1 && q.automations[0].points[1].step==80 && q.automations[0].steps==80);
    moved=automation_move_point(&q,0,moved,-8,.4f);
    CHECK(moved==1 && q.automations[0].points[1].step==q.automations[0].points[0].step && q.clip_offsets[1][copy]==16);
    CHECK(automation_value(&q.automations[0],16)==.4f);
    moved=automation_move_point(&q,0,0,-8,.6f);
    CHECK(moved==0 && q.automations[0].points[0].step==0 && q.clip_offsets[1][copy]==24);
    CHECK(automation_valid(&q) && project_save("automation.hbt",&q) && project_load("automation.hbt",&p) && project_equal(&q,&p));
    /* Nodes stop at neighbors. Equal times are a vertical, right-continuous jump. */
    project_new(&q); CHECK(automation_create(&q,(ParameterTarget){PARAM_MASTER_VOLUME,0,0},"Vertical",16)==0);
    q.automations[0].points[0].value=0; q.automations[0].points[1].value=1;
    CHECK(automation_point(&q.automations[0],4,0)==1);
    CHECK(automation_move_point(&q,0,2,-100,1)==2 && q.automations[0].points[2].step==4);
    CHECK(automation_value(&q.automations[0],3.999f)==0 && automation_value(&q.automations[0],4)==1);
    CHECK(automation_move_point(&q,0,1,100,.25f)==1 && q.automations[0].points[1].step==4);
    CHECK(automation_valid(&q) && project_save("automation.hbt",&q) && project_load("automation.hbt",&p) && project_equal(&q,&p));
    CHECK(automation_move_point(&q,0,2,12,.8f)==2 && q.automations[0].points[2].step==12);
    CHECK(q.automations[0].points[1].value==.25f && q.automations[0].count==3);
    /* Completed clips hold every parameter, ordered by completion time, not lane. */
    project_new(&q); q.channel_audio[0]=1; q.audio_seconds[0]=2;
    CHECK(arrangement_place(&q,0,0,PATTERNS,16)>=0);
    CHECK(automation_create(&q,(ParameterTarget){PARAM_MASTER_VOLUME,0,0},"Earlier",4)==0);
    q.automations[0].points[0].value=q.automations[0].points[1].value=.25f;
    CHECK(arrangement_place(&q,4,0,AUTOMATION_SOURCE,4)>=0);
    player_reset(&player); player.song=1; player_seek(&player,&q,6); render(&player,&q,samples,output,1);
    CHECK(fabsf(output[0]-.1f)<1e-6);
    CHECK(automation_create(&q,(ParameterTarget){PARAM_MASTER_VOLUME,0,0},"Later",4)==1);
    q.automations[1].points[0].value=q.automations[1].points[1].value=.75f;
    CHECK(arrangement_place(&q,1,.5f,AUTOMATION_SOURCE+1,4)>=0);
    CHECK(automation_evaluate(&q,(ParameterTarget){PARAM_MASTER_VOLUME,0,0},14,&normalized) && normalized==.75f);
    player_seek(&player,&q,10); render(&player,&q,samples,output,1); CHECK(fabsf(output[0]-.3f)<1e-6);
    player_seek(&player,&q,14); render(&player,&q,samples,output,1); CHECK(fabsf(output[0]-.3f)<1e-6);
    CHECK(automation_create(&q,(ParameterTarget){PARAM_CHANNEL_MUTE,0,0},"Mute",4)==2);
    q.automations[2].points[0].value=q.automations[2].points[1].value=1;
    CHECK(arrangement_place(&q,2,.25f,AUTOMATION_SOURCE+2,4)>=0);
    player_seek(&player,&q,14); render(&player,&q,samples,output,1); CHECK(output[0]==0);
    free(samples[0].data); remove("automation.hbt"); remove("bad-automation.hbt");
    return 0;
}
