// SPDX-License-Identifier: GPL-3.0-only
#ifndef MIDI_SEQUENCE_H
#define MIDI_SEQUENCE_H
#include <stdio.h>
static int midi_sequence_count(int mode){return mode==1?40:12;}
static MidiEvent midi_sequence_event(int mode,int frame) {
    MidiEvent e={0,0,0,1+frame*.08};
    if(mode==0 || mode==3 || mode==6 || mode==7) {
        static const unsigned char events[][3]={{0x90,60,100},{0x90,60,75},{0xb0,64,127},{0x80,60,0},{0x91,67,90},{0x81,67,0},{0x90,62,105},{0xb0,64,0},{0x90,62,0},{0x90,66,70},{0xb0,123,0},{0x90,71,110}};
        e.status=events[frame][0];e.data1=events[frame][1];e.data2=events[frame][2];
    } else if(mode==1){e.status=frame==39?0xb0:0x90;e.data1=frame==39?120:40+frame;e.data2=frame==39?0:40+frame;}
    else if(mode==2 || mode==4){e.status=frame%4==3?0xe0:0xb0;e.data1=e.status==0xe0?127:10+frame%3;e.data2=frame%2?127:frame*7;}
    return e;
}
static void midi_sequence_project(Project *p) {
    project_new(p);p->channel_count=3;p->insert_count=1;
    p->instrument[0]=p->instrument[1]=p->instrument[2]=INSTRUMENT_FM;
    midi_bind(p,0,10,(ParameterTarget){PARAM_MASTER_VOLUME,0,0});
    midi_bind(p,0,11,(ParameterTarget){PARAM_INSERT_VOLUME,1,0});
    midi_bind(p,0,12,(ParameterTarget){PARAM_FM_FILTER_CUTOFF,0,0});
}
static void midi_sequence_values(FILE *file,const Project *p) {
    float values[]={p->bpm,p->master,p->insert_volume[1],p->channel_pitch[0],p->channel_pitch[1],p->channel_pitch[2]};
    fwrite(values,sizeof values,1,file);
}
#endif
