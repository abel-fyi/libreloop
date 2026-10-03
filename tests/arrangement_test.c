// SPDX-License-Identifier: GPL-3.0-only
#include "project_check.h"
#include "arrangement.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed: %d: %s\n",__LINE__,#x); return 1; } } while(0)
int main(void) {
    Project p; project_default(&p); memset(p.clips,0,sizeof p.clips);
    Arrangement a={.source_pattern=-1,.source_steps=STEPS,.snap=16};
    arrangement_press(&a,&p,1.2,.4,0,0,0,0); arrangement_drag(&a,&p,5.2,1.4); arrangement_release(&a);
    CHECK(p.clips[1][5]==1 && !p.clips[0][1]);
    CHECK(a.selected[1][5]);
    int count=0; for(int l=0;l<LANES;l++) for(int b=0;b<BARS;b++) count+=p.clips[l][b]!=0; CHECK(count==1);
    /* Clicked copy becomes the paint source, including its independently resized length. */
    p.pattern_steps[0]=32;
    arrangement_press(&a,&p,5.9,1.4,0,1,0,0); arrangement_drag(&a,&p,7,1.4); arrangement_release(&a);
    CHECK(clip_length(&p,1,5)==32 && p.pattern_steps[0]==32);
    a.tool=BRUSH; arrangement_press(&a,&p,1.2,2.4,0,0,0,0); arrangement_drag(&a,&p,7.2,2.4); arrangement_release(&a);
    CHECK(p.clips[2][1]==1 && p.clips[2][3]==1 && p.clips[2][5]==1 && p.clips[2][7]==1 && !p.clips[2][2]);
    CHECK(clip_length(&p,2,3)==32);
    CHECK(a.selected[2][1] && a.selected[2][3] && a.selected[2][5] && a.selected[2][7] && !a.selected[1][5]);
    /* Resuming inside a clip anchors painting to its start, even across skipped frames. */
    Arrangement resumed={.tool=BRUSH,.source_pattern=-1,.snap=1};
    arrangement_press(&resumed,&p,3.8f,2.4f,0,0,0,0);
    arrangement_drag(&resumed,&p,11.4f,2.4f); arrangement_release(&resumed);
    CHECK(arrangement_hit(&p,2,9.01f)>=0 && arrangement_hit(&p,2,11.01f)>=0);
    int continued=arrangement_hit(&p,2,9.01f);
    CHECK(p.clip_starts[2][continued]==9 && clip_length(&p,2,continued)==32);
    CHECK(resumed.selected[2][continued] && resumed.selected[2][3] && !resumed.selected[2][1]);
    /* Undo these extra fixtures before the existing group-move assertions. */
    for(int b=0;b<CLIPS;b++) if(p.clips[2][b] && p.clip_starts[2][b]>=9) p.clips[2][b]=0;
    /* Rectangle selection and a group move preserve lengths and spacing. */
    a.tool=SELECT; arrangement_press(&a,&p,.2,1.9,0,0,0,0); arrangement_drag(&a,&p,5,2.9); arrangement_release(&a);
    CHECK(a.selected[2][1] && a.selected[2][3] && !a.selected[2][5]);
    arrangement_press(&a,&p,1.2,2.4,0,0,0,0); arrangement_drag(&a,&p,2.2,3.4); arrangement_release(&a);
    CHECK(p.clips[3][2] && p.clips[3][4] && !p.clips[2][1] && !p.clips[2][3]);
    CHECK(a.selected[3][2] && a.selected[3][4] && clip_length(&p,3,4)==32);
    /* Overshooting the beginning clamps the group, keeping its spacing. */
    arrangement_press(&a,&p,2.2,3.4,0,0,0,0); arrangement_drag(&a,&p,-15.2,3.4); arrangement_release(&a);
    CHECK(p.clips[3][0] && p.clips[3][2] && !p.clips[3][4]);
    CHECK(p.clip_starts[3][0]==0 && p.clip_starts[3][2]==2 && a.selected[3][0] && a.selected[3][2]);
    arrangement_press(&a,&p,.2,3.4,0,0,0,0); arrangement_drag(&a,&p,2.2,3.4); arrangement_release(&a);
    CHECK(p.clips[3][2] && p.clips[3][4]);
    /* Occupied destinations still leave the group intact. */
    arrangement_press(&a,&p,2.2,3.4,0,0,0,0); arrangement_drag(&a,&p,5.2,2.4); arrangement_release(&a);
    CHECK(p.clips[3][2] && p.clips[3][4] && p.clips[2][5]);
    /* Header dragging selects tracks; erase finds the clip beneath its extended body. */
    arrangement_press(&a,&p,-.5,1.4,0,0,0,0); arrangement_drag(&a,&p,-.5,3.4); arrangement_release(&a);
    CHECK(a.selected[1][5] && a.selected[2][5] && a.selected[3][4]);
    arrangement_press(&a,&p,6.5,1.4,1,0,0,0); arrangement_release(&a); CHECK(!p.clips[1][5]);
    CHECK(song_steps(&p)==9*STEPS);
    /* Shift works with every tool, toggles clicks, and adds rectangles. */
    memset(a.selected,0,sizeof a.selected);
    for(int tool=PENCIL;tool<=SELECT;tool++) {
        a.tool=tool;
        arrangement_press(&a,&p,2.2,3.4,0,0,0,1); arrangement_release(&a); CHECK(a.selected[3][2]);
        arrangement_press(&a,&p,2.2,3.4,0,0,0,1); arrangement_release(&a); CHECK(!a.selected[3][2]);
    }
    arrangement_press(&a,&p,4.2,3.4,0,0,0,1); arrangement_release(&a);
    arrangement_press(&a,&p,.2,2.9,0,0,0,1); arrangement_drag(&a,&p,3,3.9); arrangement_release(&a);
    CHECK(a.selected[3][2] && a.selected[3][4]);
    /* Cropping copies never changes the shared editor length or deletes notes. */
    Project shortened; project_default(&shortened); Arrangement sized={.source_pattern=-1,.source_steps=STEPS};
    arrangement_press(&sized,&shortened,0,.4,0,0,0,0); arrangement_release(&sized);
    shortened.pattern_steps[0]=64;
    arrangement_press(&sized,&shortened,.9,.4,0,1,0,0); arrangement_drag(&sized,&shortened,4,.4);
    CHECK(arrangement_edit_steps(&sized,&shortened,0)==64);
    CHECK(note_add(&shortened,0,3,48,60,1));
    arrangement_drag(&sized,&shortened,1,.4); CHECK(arrangement_edit_steps(&sized,&shortened,0)==64 && clip_length(&shortened,0,0)==16);
    arrangement_drag(&sized,&shortened,1.5,.4); CHECK(arrangement_edit_steps(&sized,&shortened,0)==64 && clip_length(&shortened,0,0)==24);
    CHECK(shortened.pattern_steps[0]==64 && note_at(&shortened,0,3,48,60));
    CHECK(arrangement_edit_steps(&sized,&shortened,1)==16);
    arrangement_drag(&sized,&shortened,100,.4); CHECK(clip_length(&shortened,0,0)==64);
    arrangement_release(&sized);
    Project fractional; project_default(&fractional);
    Arrangement precise={.source_pattern=-1,.source_steps=STEPS,.snap=.5f};
    arrangement_press(&precise,&fractional,.1f,.4f,0,0,0,0); arrangement_release(&precise);
    CHECK(fractional.clips[0][0] && fractional.clip_starts[0][0]==.09375f);
    arrangement_press(&precise,&fractional,1.08f,.4f,0,1,0,0); arrangement_drag(&precise,&fractional,.25f,.4f); arrangement_release(&precise);
    CHECK(clip_length(&fractional,0,0)==2.5f);
    arrangement_press(&precise,&fractional,.4f,.4f,0,0,0,0); arrangement_release(&precise);
    CHECK(fractional.clips[0][1] && fractional.clip_starts[0][1]==.375f);
    arrangement_press(&precise,&fractional,.4f,.4f,0,0,0,0); arrangement_drag(&precise,&fractional,1.4f,1.4f); arrangement_release(&precise);
    CHECK(fractional.clips[1][1] && fractional.clip_starts[1][1]==1.375f && precise.selected[1][1]);
    precise.snap=4.f/3;
    arrangement_press(&precise,&fractional,2.1f,2.4f,0,0,0,0); arrangement_release(&precise);
    CHECK(fabsf(fractional.clip_starts[2][2]-25.f/12)<.00001f);
    /* Non-grid clip lengths still tile exactly in both directions. */
    Arrangement tiled={.tool=BRUSH,.source_pattern=-1,.snap=4};
    int seed=arrangement_place(&fractional,4,2.125f,0,10);
    CHECK(seed>=0);
    arrangement_press(&tiled,&fractional,2.6f,4.4f,0,0,0,0);
    arrangement_drag(&tiled,&fractional,4.1f,4.4f);
    arrangement_drag(&tiled,&fractional,.9f,4.4f); arrangement_release(&tiled);
    int after=arrangement_hit(&fractional,4,2.76f),before=arrangement_hit(&fractional,4,1.51f);
    CHECK(after>=0 && before>=0 && fractional.clip_starts[4][after]==2.75f && fractional.clip_starts[4][before]==1.5f);
    CHECK(clip_length(&fractional,4,after)==10 && fractional.clip_starts[4][seed]==2.125f);
    a.zoom=1; a.view_start=0; arrangement_zoom(&a,2,.5f);
    float zoomed=BARS/a.zoom;
    CHECK(fabsf(zoomed-16/(1.08f*1.08f))<.001f && fabsf(a.view_start+zoomed*.5f-8)<.001f);
    arrangement_zoom(&a,-2,.5f); CHECK(fabsf(a.zoom-1)<.001f && fabsf(a.view_start)<.001f);
    arrangement_zoom(&a,-20,.5f); CHECK(BARS/a.zoom>70 && BARS/a.zoom<80);
    float old_span=BARS/a.zoom; arrangement_zoom(&a,-1,.5f);
    CHECK(fabsf(BARS/a.zoom/old_span-1.08f)<.0001f);
    arrangement_zoom(&a,21,.5f); CHECK(fabsf(a.zoom-1)<.001f);
    float span=1,start=3; timeline_zoom(&span,&start,-1,.25f,1); CHECK(span>1 && span<1.2f);
    timeline_zoom(&span,&start,1,.25f,1); CHECK(fabsf(span-1)<.0001f && fabsf(start-3)<.0001f);
    span=1000; start=500; timeline_zoom(&span,&start,-1,.25f,1);
    CHECK(fabsf(span-1080)<.001f && fabsf(start-480)<.001f);
    CHECK(timeline_thumb(900,1,1000000)==24 && timeline_thumb(900,100,100)==900);
    Project long_song; project_default(&long_song); Arrangement unlimited={.source_pattern=-1,.source_steps=STEPS,.snap=1};
    arrangement_press(&unlimited,&long_song,5000.25f,.4f,0,0,0,0); arrangement_release(&unlimited);
    int far=arrangement_hit(&long_song,0,5000.5f); CHECK(far>=0 && long_song.clip_starts[0][far]==5000.25f);
    long_song.pattern_steps[0]=100*STEPS;
    arrangement_press(&unlimited,&long_song,5001.2f,.4f,0,1,0,0); arrangement_drag(&unlimited,&long_song,5100.25f,.4f); arrangement_release(&unlimited);
    CHECK(clip_length(&long_song,0,far)==100*STEPS && long_song.pattern_steps[0]==100*STEPS);
    CHECK(note_add(&long_song,0,3,99*STEPS,60,2));
    CHECK(song_steps(&long_song)==5100.25f*STEPS);
    CHECK(project_save("long-song.hbt",&long_song)); Project loaded; CHECK(project_load("long-song.hbt",&loaded)); CHECK(project_equal(&long_song,&loaded)); remove("long-song.hbt");
    /* Temporary selection starts over occupied clips without moving or painting. */
    for(int tool=PENCIL;tool<=BRUSH;tool++) {
        precise.tool=tool; precise.source_pattern=2;
        arrangement_select_press(&precise,1.4f,1.4f,0);
        arrangement_drag(&precise,&fractional,1.8f,1.8f); arrangement_release(&precise);
        CHECK(precise.selected[1][1] && !precise.selected[0][0]);
        CHECK(precise.tool==tool && precise.source_pattern==2);
        CHECK(fractional.clip_starts[1][1]==1.375f && fractional.clips[1][1]==1);
        arrangement_select_press(&precise,.1f,.1f,1);
        arrangement_drag(&precise,&fractional,.2f,.8f); arrangement_release(&precise);
        CHECK(precise.selected[1][1] && precise.selected[0][0]);
    }
    puts("Pencil, brush source and spacing, resize, selection, group movement and erase passed."); return 0;
}
