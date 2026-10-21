// SPDX-License-Identifier: GPL-3.0-only
/* Same original gesture inputs; resulting project/selection/audition must match. */
#include "piano.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static Project project;
static Note before[NOTES];
static uint8_t selected[NOTES],selection_before[NOTES];
static PianoGesture state={.before=before,.selected=selected,.selection_before=selection_before};
static FILE *previews;
static void preview(void *context,int channel,Note n) {
    (void)context; fprintf(previews,"%d %d %d %.9g %.9g\n",channel,n.pitch,n.velocity,n.start,n.length);
}
int main(int argc,char **argv) {
    if(argc!=2)return 1;int mode=atoi(argv[1]);if(mode<0 || mode>4)return 1;
    project_new(&project);project.insert_count=INSERTS; /* Frozen reference project geometry. */project.pattern_steps[0]=17.25f;state.length=1.5f;
    project.notes[0][0][0]=(Note){60,100,0,2};
    project.notes[0][0][1]=(Note){64,80,2.5f,1.25f};
    project.notes[0][0][2]=(Note){60,70,0,1};
    memcpy(before,project.notes[0][0],sizeof before);
    previews=fopen("previews.txt","wb");if(!previews)return 2;
    if(mode==0) {
        state.kind=PIANO_BRUSH;state.from=state.now=(PianoPoint){1.25f,61.25f};
        piano_gesture_update(&state,&project,7.875f,66.75f,.25f,preview,NULL);
        piano_gesture_update(&state,&project,-1.25f,59.75f,.25f,preview,NULL);
        piano_gesture_update(&state,&project,34.125f,128.25f,1,preview,NULL);
    } else if(mode==1) {
        state.kind=PIANO_ERASE;state.from=state.now=(PianoPoint){.5f,59.5f};
        piano_gesture_update(&state,&project,.5f,59.5f,.25f,preview,NULL);piano_gesture_update(&state,&project,.5f,59.5f,.25f,preview,NULL);
        piano_gesture_update(&state,&project,3,63.5f,.25f,preview,NULL);
    } else if(mode==2) {
        selected[0]=selected[1]=1;
        state.drag=&project.notes[0][0][0];state.moving=1;state.grab=(PianoPoint){.75f,3.5f};
        piano_note_drag_update(&state,&project,5.625f,1.25f,.25f,preview,NULL);
        piano_note_drag_update(&state,&project,-3.5f,150,.5f,preview,NULL);
        piano_note_drag_update(&state,&project,30.125f,-200,1,preview,NULL);
    } else if(mode==3) {
        project.notes[0][0][2].length=0;memcpy(before,project.notes[0][0],sizeof before);
        selected[0]=selected[2]=1;
        state.drag=&project.notes[0][0][0];state.grab=(PianoPoint){1.75f,0};
        piano_note_drag_update(&state,&project,-2,0,.25f,preview,NULL);piano_note_drag_update(&state,&project,22.5f,0,.25f,preview,NULL);
    } else {
        selected[1]=1;memcpy(selection_before,selected,NOTES);
        state.kind=PIANO_BOX;state.additive=1;state.from=state.now=(PianoPoint){0,59};
        piano_gesture_update(&state,&project,1,60,.25f,preview,NULL);piano_gesture_update(&state,&project,0,59,.25f,preview,NULL);
    }
    fclose(previews);
    if(!project_save("expected.llp",&project))return 3;
    FILE *selection=fopen("selection.bin","wb"),*state_file=fopen("state.txt","wb");if(!selection || !state_file)return 2;
    fwrite(selected,1,NOTES,selection);fclose(selection);
    fprintf(state_file,"%.9g %.9g %.9g %.9g\n",state.length,state.now.x,state.now.y,project.pattern_steps[0]);fclose(state_file);return 0;
}
