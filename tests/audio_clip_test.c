// SPDX-License-Identifier: GPL-3.0-only
#include "project_check.h"
#include "arrangement.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed: %d: %s\n",__LINE__,#x); return 1; } } while(0)
static Project p,q;
static float pcm[RATE],out[RATE*6],split[RATE*6];
int main(void) {
    project_default(&p); memset(p.notes,0,sizeof p.notes);
    p.channel_audio[0]=1; p.audio_seconds[0]=1; p.volume[0]=1; p.route[0]=0;
    snprintf(p.paths[0],sizeof p.paths[0],"audio.wav");
    for(int i=0;i<RATE;i++) pcm[i]=.1f+.3f*i/RATE;
    Sample samples[CHANNELS]={{pcm,RATE}};
    int slot=arrangement_place(&p,2,.5f,PATTERNS,8); CHECK(slot>=0);
    CHECK(p.clips[2][slot]==PATTERNS+1 && clip_length(&p,2,slot)==8);
    CHECK(p.clip_steps[2][slot]==0);
    p.audio_seconds[0]=.25f; CHECK(clip_length(&p,2,slot)==2);
    p.audio_seconds[0]=1.5f; CHECK(clip_length(&p,2,slot)==12);
    p.audio_seconds[0]=0; CHECK(clip_length(&p,2,slot)==0);
    CHECK(project_save("zero-audio.hbt",&p) && project_load("zero-audio.hbt",&q)); remove("zero-audio.hbt");
    p.audio_seconds[0]=1200;
    CHECK(project_save("long-audio.hbt",&p) && project_load("long-audio.hbt",&q) && q.audio_seconds[0]==1200);
    remove("long-audio.hbt");
    p.audio_seconds[0]=1;
    Player player; player_reset(&player); player.song=1;
    render(&player,&p,samples,out,RATE*3);
    for(int i=0;i<RATE;i++) CHECK(out[i*2]==0);
    CHECK(fabsf(out[RATE*2]-.1f)<.00001f);
    CHECK(out[RATE*4]==0); /* source stops after one second */
    player_reset(&player); player.song=1;
    render(&player,&p,samples,split,12345); render(&player,&p,samples,split+24690,RATE*3-12345);
    CHECK(memcmp(out,split,sizeof out)==0);
    player_reset(&player); player.song=1; player_seek(&player,&p,12);
    render(&player,&p,samples,out,1); CHECK(fabsf(out[0]-.25f)<.00001f && player.lane_trigger[2] && player.lane_active[2]);
    render(&player,&p,samples,out,1); CHECK(!player.lane_trigger[2] && player.lane_active[2]); /* seek into audio */
    /* A catch-up between sequencer ticks must not duplicate or kill pattern voices. */
    player.voices[127]=(Voice){.channel=0,.sampler={.position=200,.speed=1},.remaining=-1,.gain=.2f,.lane=6,.audio_clip=0};
    player.audio_resync=1; uint64_t cursor=player.frame;
    render(&player,&p,samples,out,1);
    CHECK(player.frame==cursor+1 && player.voices[127].sampler.position==201 && player.voices[127].gain==.2f);
    int audio_count=0; for(int v=0;v<128;v++) audio_count+=player.voices[v].gain && player.voices[v].audio_clip;
    CHECK(audio_count==1 && player.lane_active[2]);
    p.lane_mute[2]=1; player_reset(&player); player.song=1; player_seek(&player,&p,12);
    render(&player,&p,samples,out,1); CHECK(out[0]==0 && !player.lane_trigger[2] && !player.lane_active[2]); p.lane_mute[2]=0;
    p.mute[0]=1; player_reset(&player); player.song=1; player_seek(&player,&p,12);
    render(&player,&p,samples,out,1); CHECK(out[0]==0); p.mute[0]=0;
    p.lane_mute[3]=2; player_reset(&player); player.song=1; player_seek(&player,&p,12);
    render(&player,&p,samples,out,1); CHECK(out[0]==0); p.lane_mute[3]=0;
    /* Audio channels remain playable from sequencer notes. */
    CHECK(note_add(&p,0,0,0,60,0)); player_reset(&player);
    render(&player,&p,samples,out,1); CHECK(out[0]>0);
    CHECK(note_add(&p,0,0,.5f,64,1));
    p.clips[6][0]=1; p.clip_steps[6][0]=16; p.clip_starts[6][0]=0;
    player_reset(&player); player.song=1; render(&player,&p,samples,out,1);
    CHECK(player.lane_active[6] && player.lane_trigger[6]);
    render(&player,&p,samples,out,2999); CHECK(player.lane_active[6] && !player.lane_trigger[6]);
    render(&player,&p,samples,out,1); CHECK(player.lane_active[6] && player.lane_trigger[6]);
    p.clips[6][0]=0;
    memset(p.notes,0,sizeof p.notes);
    p.channel_pitch[0]=.5f; p.pitch_range[0]=24;
    CHECK(channel_speed(&p,0)==2 && clip_length(&p,2,slot)==4);
    player_reset(&player); player.song=1; player_seek(&player,&p,10);
    render(&player,&p,samples,out,1); CHECK(fabsf(out[0]-.25f)<.00001f);
    p.channel_pitch[0]=0; p.pitch_range[0]=2;
    /* Crop/move/brush preserve seconds; tempo changes do not resample audio. */
    Arrangement a={.source_pattern=-1,.source_steps=STEPS,.snap=1};
    arrangement_press(&a,&p,.99f,2.4f,0,1,0,0); arrangement_drag(&a,&p,1,2.4f); arrangement_release(&a);
    CHECK(clip_length(&p,2,slot)==8); /* edge at original end */
    arrangement_press(&a,&p,.99f,2.4f,0,1,0,0); arrangement_drag(&a,&p,2,2.4f); arrangement_release(&a);
    CHECK(clip_length(&p,2,slot)==24); /* explicit empty tail is allowed */
    arrangement_press(&a,&p,.99f,2.4f,0,1,0,0); arrangement_drag(&a,&p,.75f,2.4f); arrangement_release(&a);
    CHECK(clip_length(&p,2,slot)==4 && p.clip_steps[2][slot]==.5f);
    arrangement_press(&a,&p,.6f,2.4f,0,0,0,0); arrangement_drag(&a,&p,1.6f,3.4f); arrangement_release(&a);
    int moved=arrangement_hit(&p,3,1.6f); CHECK(moved>=0 && !p.clips[2][slot] && p.clip_steps[3][moved]==.5f);
    a.tool=BRUSH; arrangement_press(&a,&p,3,4.4f,0,0,0,0); arrangement_drag(&a,&p,3.5f,4.4f); arrangement_release(&a);
    CHECK(arrangement_hit(&p,4,3.1f)>=0 && arrangement_hit(&p,4,3.8f)<0);
    p.bpm=60; CHECK(clip_length(&p,3,moved)==2 && clip_source_steps(&p,PATTERNS)==4);
    arrangement_press(&a,&p,10,5.4f,0,0,0,0); arrangement_release(&a);
    int slow=arrangement_hit(&p,5,10.01f); CHECK(slow>=0 && clip_length(&p,5,slow)==2);
    player_reset(&player); player.song=1; player_seek(&player,&p,24);
    render(&player,&p,samples,out,1); CHECK(fabsf(out[0]-.1f)<.00001f);
    /* Looping from inside a clip resumes at its matching sample offset. */
    p.bpm=120; player_reset(&player); player.song=1; player.loop_start=25; player.loop_end=26;
    player.frame=26*6000; render(&player,&p,samples,out,1);
    CHECK(player.frame==25*6000+1 && fabsf(out[0]-pcm[6000])<.00001f);
    CHECK(project_save("audio-clips.hbt",&p) && project_load("audio-clips.hbt",&q));
    CHECK(project_equal(&p,&q));
    /* An explicit cap must survive a source shrinking to the same length, then growing. */
    q.audio_seconds[0]=.5f;
    CHECK(project_save("cropped-audio.hbt",&q) && project_load("cropped-audio.hbt",&q));
    q.audio_seconds[0]=1; CHECK(clip_length(&q,3,moved)==4); remove("cropped-audio.hbt");
    /* Older imported clips gain the new source-following behavior. */
    q=p; q.clip_steps[3][moved]=1; CHECK(project_save("old-audio.hbt",&q));
    FILE *old=fopen("old-audio.hbt","r"),*legacy=fopen("legacy-audio.hbt","w"); CHECK(old && legacy);
    char line[2048]; CHECK(fgets(line,sizeof line,old)); fputs("HOMEBEAT 23\n",legacy);
    while(fgets(line,sizeof line,old)) fputs(line,legacy); fclose(old); fclose(legacy);
    CHECK(project_load("legacy-audio.hbt",&q) && q.clip_steps[3][moved]==0);
    q.audio_seconds[0]=2; CHECK(clip_length(&q,3,moved)==16);
    remove("old-audio.hbt"); remove("legacy-audio.hbt"); q=p;
    CHECK(export_wav("audio-clips.wav",&p,samples));
    FILE *f=fopen("audio-clips.wav","rb"); CHECK(f); CHECK(fseek(f,44+RATE*3*4,SEEK_SET)==0);
    unsigned char bytes[2]; CHECK(fread(bytes,1,2,f)==2); CHECK(bytes[0] || bytes[1]); fclose(f);
    /* Deleting patterns must not change Audio references; deleting channels must. */
    CHECK(pattern_delete(&q,0)); CHECK(q.clips[3][moved]==PATTERNS+1);
    q.channel_audio[1]=1; q.audio_seconds[1]=2;
    int other=arrangement_place(&q,7,0,PATTERNS+1,16); CHECK(other>=0);
    CHECK(channel_delete(&q,0)); CHECK(!q.clips[3][moved] && q.clips[7][other]==PATTERNS+1 && q.audio_seconds[0]==2);
    q.channel_audio[0]=0; CHECK(!project_save("invalid-audio.hbt",&q));
    remove("audio-clips.hbt"); remove("audio-clips.wav");
    puts("Audio clips: playback, seek, crop, move, brush, looping, mute, sequencing, persistence and export passed.");
    return 0;
}
