// SPDX-License-Identifier: GPL-3.0-only
#ifndef PIANO_SEQUENCE_H
#define PIANO_SEQUENCE_H
#include <stdio.h>
typedef struct {float x,y;int left,right,held_left,held_right,modal,focused,visible,shift,shortcut,tool;} SequenceInput;
static SequenceInput sequence_input(int mode,int frame) {
    SequenceInput s={120,286,0,0,1,0,0,1,1,0,0,0};
    if(mode==1)s.x=133;
    if(mode==2){s.x=220;s.shortcut=1;}
    if(mode==3){s.x=220;s.tool=1;}
    if(mode==4){s.right=frame==0 || frame==4;s.held_left=0;s.held_right=1;}
    if(mode==5){s.x=400;s.y=64;}
    if(mode==6){s.x=472;s.y=200;}
    if(mode==7 || mode==11){s.x=50;s.y=286;}
    if(mode==8 || mode==9){s.x=111;s.y=520;if(mode==9){s.held_left=0;s.held_right=1;s.right=frame==0 || frame==4;}}
    s.x+=frame*11;s.y+=frame*2;
    if(mode!=4 && mode!=9)s.left=frame==0 || frame==4;
    if(frame==2 && mode<5)s.left=1;
    if(frame==3 || frame==7){s.held_left=s.held_right=0;}
    if(frame==5 && mode==10)s.modal=1;
    if(frame==5 && mode==11)s.focused=0;
    if(frame==6 && mode==7)s.visible=0;
    return s;
}
static void sequence_dump(FILE *file,int integers[14],float floats[20],
    const Note *notes,const Note *before,const Note *velocity_before,
    const unsigned char *selected,const unsigned char *selection_before) {
    fwrite(integers,sizeof(int),14,file);fwrite(floats,sizeof(float),20,file);
    fwrite(notes,sizeof(Note),NOTES,file);fwrite(before,sizeof(Note),NOTES,file);
    fwrite(velocity_before,sizeof(Note),NOTES,file);fwrite(selected,1,NOTES,file);
    fwrite(selection_before,1,NOTES,file);
}
#endif
