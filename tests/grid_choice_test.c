// SPDX-License-Identifier: GPL-3.0-only
#include "grid.h"
#include "piano.h"
#include "arrangement.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <string.h>
static Project project;
static Note before[NOTES];static uint8_t selected[NOTES],selection_before[NOTES];
int main(void) {
    assert(grid_choice_layout(GRID_64,1e-8f).lines*1e-8f>=3);
    assert(grid_choice_interval(GRID_64,1e-8f)==.25f);
    for(int choice=0;choice<GRID_CHOICES;choice++)for(int zoom=0;zoom<3;zoom++) {
        float pixels=5*(1<<zoom),q=grid_choice_interval(choice,pixels);
        if(choice==GRID_AUTO) {
            assert(q==grid_interval(pixels));
            TimelineGrid a=grid_choice_layout(choice,pixels),b=timeline_grid_layout(pixels);
            assert(a.lines==b.lines && a.labels==b.labels && a.band_alpha==b.band_alpha);
        } else assert(q==grid_choice_interval(choice,1000));
        if(choice==GRID_NONE)assert(q==0 && grid_choice_layout(choice,pixels).lines==0);
        project_new(&project);memset(selected,0,sizeof selected);
        PianoViewport view={.width=724,.height=480,.span=640/pixels,.zoom=1,.top=72,.grid_choice=choice};
        PianoGesture gesture={.length=2,.before=before,.selected=selected,.selection_before=selection_before};
        float wanted=5.37f;
        PianoPress press={.pointer={60+wanted*pixels,86+12.5f*12},.enabled=1,.left=1};
        assert(piano_press(&gesture,&project,&view,press,0,0)&PIANO_PRESS_CONSUMED);
        assert(gesture.drag && fabsf(gesture.drag->start-snap_floor(wanted,q))<.00001f);
        /* Moving a note continues using the selected quantum at a different zoom. */
        float initial=gesture.drag->start;
        piano_note_drag_update(&gesture,&project,wanted+3.37f,12.5f,q,0,0);
        assert(fabsf(gesture.drag->start-(initial+snap_round(3.37f,q)))<.00002f);
        project_new(&project);
        Arrangement arrangement={.tool=PENCIL,.source_pattern=-1,.snap=q};
        arrangement_press(&arrangement,&project,wanted/STEPS,.5f,0,0,0,0);
        assert(project.clips[0][0]);
        assert(fabsf(project.clip_starts[0][0]*STEPS-snap_floor(wanted+.00001f,q))<.00002f);
    }
    return 0;
}
