// SPDX-License-Identifier: GPL-3.0-only
#include "piano_frame.h"
#include <stdlib.h>
#include <string.h>
#include "piano_sequence.h"
static Project project;
static Note before[NOTES],velocity_before[NOTES];
static uint8_t selected[NOTES],selection_before[NOTES];
static FILE *previews,*audition;
static void preview(void *ctx,int c,Note n){(void)ctx;fprintf(previews,"%d %d %d %.9g %.9g\n",c,n.pitch,n.velocity,n.start,n.length);}
static void key_preview(void *ctx,int slot,int c,int pitch,int down){(void)ctx;fprintf(audition,"%d %d %d %d\n",slot,c,pitch,down);}
int main(int argc,char **argv) {
    if(argc!=2)return 1;int mode=atoi(argv[1]);if(mode<0 || mode>11)return 1;
    project_new(&project);project.pattern_steps[0]=17.25f;
    project.notes[0][0][0]=(Note){60,100,4,2};project.notes[0][0][1]=(Note){60,70,4,1};project.notes[0][0][2]=(Note){64,80,8,1.25f};
    PianoGesture g={.length=1.5f,.before=before,.selected=selected,.selection_before=selection_before};
    PianoViewport v={484,580,32,0,1,72};PianoScrollbars bars={0};PianoKeys keys={.pitch=-1};PianoVelocity velocity={.ramp=-1,.before=velocity_before};
    FILE *trace=fopen("frames.bin","wb");previews=fopen("previews.txt","w");audition=fopen("audition.txt","w");
    if(!trace || !previews || !audition)return 2;
    for(int frame=0;frame<8;frame++) {
        SequenceInput s=sequence_input(mode,frame);PianoPoint local={s.x,s.y},origin={320,128};
        PianoFrame f={{s.x+origin.x,s.y+origin.y},origin,s.modal,s.focused,s.visible,s.left,s.right,s.held_left,s.held_right};
        piano_frame_advance(&g,&keys,&velocity,&bars,&project,&v,f,preview,key_preview,NULL);
        int enabled=!s.modal;PianoPress press={local,enabled,s.left,s.right,s.shift,s.shortcut,s.tool};
        if(piano_frame_press(&g,&keys,&velocity,&bars,&project,&v,64,origin,press,preview,key_preview,NULL))enabled=0;
        int integers[14]={enabled,g.kind,g.additive,g.moving,g.melodic,g.drag?(int)(g.drag-project.notes[0][0]):-1,v.top,bars.vertical,bars.horizontal,keys.pitch,keys.channel,keys.drag,velocity.ramp,velocity.pattern};
        float floats[20]={g.length,g.from.x,g.from.y,g.now.x,g.now.y,g.grab.x,g.grab.y,v.pan,bars.y,bars.pitch_start,bars.x,bars.pan_start,bars.range_per_pixel,velocity.from.x,velocity.from.y,velocity.now.x,velocity.now.y,project.pattern_steps[0],0,0};
        sequence_dump(trace,integers,floats,project.notes[0][0],before,velocity_before,selected,selection_before);
    }
    fclose(trace);fclose(previews);fclose(audition);return project_save("expected.llp",&project)?0:2;
}
