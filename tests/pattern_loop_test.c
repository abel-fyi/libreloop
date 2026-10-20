// SPDX-License-Identifier: GPL-3.0-only
#include "playback_plan.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
static Project project;
static Player player;
static PlaybackPlan plan;
static Sample samples[CHANNELS];
static float out[4],peaks[INSERTS+1][2];
int main(void) {
    project_default(&project); project.bpm=120; project.channel_count=2;
    memset(project.notes,0,sizeof project.notes);
    memset(project.clips,0,sizeof project.clips);
    project.pattern_steps[0]=48; /* Capacity retained after editing/recording. */
    CHECK(pattern_playback_steps(&project,0)==16);
    project.notes[0][0][0]=(Note){.start=0,.length=1,.pitch=60,.velocity=100};
    project.notes[0][0][1]=(Note){.start=15,.length=0,.pitch=60,.velocity=100};
    CHECK(pattern_playback_steps(&project,0)==16);
    const uint64_t step_frames=RATE/8; /* 120 BPM, four steps per beat. */
    player_reset(&player); player.frame=16*step_frames-1;
    render_mixer(&player,NULL,&project,samples,out,2,1,peaks);
    CHECK(player.frame==1); /* Wrap at the occupied bar, not editing capacity. */
    project.notes[0][1][0]=(Note){.start=12,.length=4,.pitch=60,.velocity=100};
    CHECK(pattern_playback_steps(&project,0)==16); /* Exact boundary: no extra bar. */
    project.notes[0][1][0].length=5; project.mute[1]=1;
    CHECK(pattern_playback_steps(&project,0)==32); /* Include lengths and muted channels. */
    player_reset(&player); player.frame=32*step_frames-1;
    playback_plan_prepare(&plan,&project,0,NULL,1); player.plan=&plan;
    render_mixer(&player,NULL,&project,samples,out,2,1,peaks);
    CHECK(player.frame==1);
    project.notes[0][1][0].velocity=0;
    CHECK(pattern_playback_steps(&project,0)==16); /* Deletion removes the empty tail. */
    player_reset(&player); player.start_step=4; player.frame=16*step_frames;
    render_mixer(&player,NULL,&project,samples,out,1,1,peaks);
    CHECK(player.frame==4*step_frames+1);
    player_reset(&player); player.start_step=32; player.frame=32*step_frames;
    render_mixer(&player,NULL,&project,samples,out,1,1,peaks);
    CHECK(player.frame==1); /* A marker beyond the shortened loop returns to zero. */
    player_reset(&player); player.loop_start=8; player.loop_end=40; player.frame=40*step_frames;
    render_mixer(&player,NULL,&project,samples,out,1,1,peaks);
    CHECK(player.frame==8*step_frames+1); /* Explicit loops retain their endpoints. */
    project.clips[0][0]=1; project.clip_starts[0][0]=0; project.clip_steps[0][0]=48;
    CHECK(clip_length(&project,0,0)==48);
    player_reset(&player); player.song=1; player.frame=16*step_frames;
    render_mixer(&player,NULL,&project,samples,out,1,1,peaks);
    CHECK(player.frame==16*step_frames+1); /* Song still follows the arranged clip. */
    puts("Pattern content/bar loops, note lengths, deletion, markers and Song boundaries passed.");
    return 0;
}
