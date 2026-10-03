// SPDX-License-Identifier: GPL-3.0-only
#include "arrangement.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
static Project p,loaded;
int main(void) {
    project_default(&p); memset(p.notes,0,sizeof p.notes);
    p.channel_audio[0]=1; p.audio_seconds[0]=4; p.volume[0]=p.master=1; p.route[0]=0;
    float *pcm=malloc(RATE*4*sizeof *pcm); CHECK(pcm);
    for(unsigned i=0;i<RATE*4;i++) pcm[i]=.1f+.1f*i/RATE;
    Sample samples[CHANNELS]={{pcm,RATE*4}}; Arrangement a={.source_pattern=-1,.snap=1};
    int slot=arrangement_place(&p,0,1,PATTERNS,32); CHECK(slot>=0);
    arrangement_press(&a,&p,1.05f,.4f,0,-1,0,0);
    arrangement_drag(&a,&p,2.05f,.4f);
    CHECK(p.clip_starts[0][slot]==2 && p.clip_offsets[0][slot]==2 && clip_length(&p,0,slot)==16);
    /* Pulling back out restores the source and keeps the right edge fixed. */
    arrangement_drag(&a,&p,1.05f,.4f);
    CHECK(p.clip_starts[0][slot]==1 && p.clip_offsets[0][slot]==0 && clip_length(&p,0,slot)==32);
    arrangement_drag(&a,&p,2.05f,.4f); arrangement_release(&a);
    Player player; player_reset(&player); player.song=1; player.frame=32*6000;
    float out[2]; render(&player,&p,samples,out,1);
    CHECK(fabsf(out[0]-tanhf(pcm[2*RATE]))<.00001f);
    player_reset(&player); player.song=1; player.frame=36*6000; render(&player,&p,samples,out,1);
    CHECK(fabsf(out[0]-tanhf(pcm[2*RATE+24000]))<.00001f);
    a.tool=BRUSH; arrangement_press(&a,&p,2.5f,.4f,0,0,0,0); arrangement_release(&a);
    arrangement_press(&a,&p,5,1.4f,0,0,0,0); arrangement_release(&a);
    int copy=arrangement_hit(&p,1,5.2f); CHECK(copy>=0 && p.clip_offsets[1][copy]==2 && clip_length(&p,1,copy)==16);
    a.tool=PENCIL; arrangement_press(&a,&p,2.5f,.4f,0,0,0,0); arrangement_drag(&a,&p,3.5f,2.4f); arrangement_release(&a);
    int moved=arrangement_hit(&p,2,3.2f); CHECK(moved>=0 && p.clip_offsets[2][moved]==2 && clip_length(&p,2,moved)==16);
    CHECK(project_save("trimmed-clips.hbt",&p) && project_load("trimmed-clips.hbt",&loaded));
    CHECK(memcmp(&p,&loaded,sizeof p)==0); remove("trimmed-clips.hbt");
    Arrangement quick={.source_pattern=-1,.snap=1};
    arrangement_press(&quick,&loaded,3.4f,2.4f,0,0,0,0);
    arrangement_drag(&quick,&loaded,2.4f,2.4f); /* last valid intermediate position */
    arrangement_drag(&quick,&loaded,-100,2.4f); arrangement_release(&quick);
    int beginning=arrangement_hit(&loaded,2,.1f);
    CHECK(beginning>=0 && loaded.clip_starts[2][beginning]==0 && loaded.clip_offsets[2][beginning]==2 && clip_length(&loaded,2,beginning)==16);
    arrangement_press(&a,&p,3.9f,2.4f,0,1,0,0); arrangement_drag(&a,&p,5,2.4f); arrangement_release(&a);
    CHECK(clip_length(&p,2,moved)==32 && p.clip_offsets[2][moved]==2);
    a.tool=BRUSH; arrangement_press(&a,&p,3.5f,2.4f,0,0,0,0); arrangement_release(&a);
    arrangement_press(&a,&p,8,4.4f,0,0,0,0); arrangement_release(&a);
    int extended=arrangement_hit(&p,4,8.2f);
    CHECK(extended>=0 && clip_length(&p,4,extended)==32 && p.clip_offsets[4][extended]==2);
    p.clip_offsets[2][moved]=NAN; CHECK(!project_save("invalid-trim.hbt",&p));
    /* Pattern previews/playback use the retained source offset too. */
    project_default(&p); memset(p.notes,0,sizeof p.notes); p.pattern_steps[0]=64; p.volume[0]=p.master=1; p.route[0]=0;
    Note *note=note_add(&p,0,0,20,60,1); CHECK(note); note->velocity=127; slot=arrangement_place(&p,0,1,0,32);
    a=(Arrangement){.source_pattern=-1,.snap=1}; arrangement_press(&a,&p,1.05f,.4f,0,-1,0,0); arrangement_drag(&a,&p,2.05f,.4f); arrangement_release(&a);
    CHECK(p.clip_offsets[0][slot]==16 && p.clip_starts[0][slot]==2 && clip_length(&p,0,slot)==16);
    player_reset(&player); player.song=1; player.frame=36*6000; render(&player,&p,samples,out,1);
    CHECK(fabsf(out[0]-tanhf(pcm[0]))<.00001f);
    note->velocity=0; note=note_add(&p,0,0,12,60,8); CHECK(note); note->velocity=127;
    player_reset(&player); player.song=1; player.frame=32*6000; render(&player,&p,samples,out,1);
    CHECK(fabsf(out[0]-tanhf(pcm[24000]))<.00001f); /* held note crosses the cropped beginning */
    arrangement_press(&a,&p,2.05f,.4f,0,-1,0,0); arrangement_drag(&a,&p,-100,.4f); arrangement_release(&a);
    CHECK(p.clip_starts[0][slot]==1 && p.clip_offsets[0][slot]==0 && clip_length(&p,0,slot)==32);
    free(pcm); puts("Left trimming, restoration, audio seeking, copying/moving, pattern playback and persistence passed."); return 0;
}
