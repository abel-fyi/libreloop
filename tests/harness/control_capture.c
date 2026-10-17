// SPDX-License-Identifier: GPL-3.0-only
#include "control.h"
#include <stdio.h>
static Project project;static ControlDrag state;static FILE *out;
static void capture(float *value,float low,float high,int fader){control_capture(&state,&project,value,low,high,fader);}
static void drag(float delta,float scale,float y){control_drag_update(&state,&project,delta,scale,y);}
static void wheel(float *value,float low,float high,int style,float delta){control_knob_wheel(&project,value,low,high,style,delta);}
static void snapshot(const char *name){fprintf(out,"%s %.9g %.9g %.9g %.9g %d %d %d %d %.9g %.9g %.9g %.9g %.9g %.9g\n",name,*state.value,state.raw,state.low,state.high,state.fader,state.reverse,state.integer,state.logarithmic,project.volume[0],project.bpm,project.pitch_range[0],project.master_width,project.fm[0].analog.cutoff,project.fm[0].dx7.value[18]);}
#define PROJECT project
#define CAPTURE capture
#define DRAG drag
#define WHEEL wheel
#define ALIAS control_fixed_alias
#define SNAP snapshot
#define REVERSE() (state.reverse=1)
#define FADER(b,r) do{state.bottom=b;state.range=r;}while(0)
#define BOUNDS(l,h) do{state.low=l;state.high=h;}while(0)

static void run(void){
    CAPTURE(&PROJECT.volume[0],0,1.25f,0);DRAG(-80,1,0);SNAP("volume-up");DRAG(80,2,0);SNAP("volume-scaled");
    CAPTURE(&PROJECT.bpm,30,300,0);DRAG(-20,1,0);SNAP("tempo-quarter");
    CAPTURE(&PROJECT.pitch_range[0],1,48,0);for(int i=0;i<7;i++)DRAG(-.5f,1,0);SNAP("integer-remainder");WHEEL(&PROJECT.pitch_range[0],1,48,0,-.5f);SNAP("negative-half-wheel");
    PROJECT.instrument[0]=INSTRUMENT_FM;PROJECT.fm[0].engine=2;PROJECT.fm[0].analog.cutoff=5000;
    CAPTURE(&PROJECT.fm[0].analog.cutoff,20,20000,0);DRAG(-32,2,0);SNAP("logarithmic-drag");WHEEL(&PROJECT.fm[0].analog.cutoff,20,20000,6,-1.25f);SNAP("logarithmic-wheel");
    CAPTURE(&PROJECT.master_width,0,2,0);REVERSE();DRAG(40,1,0);SNAP("width-reversed");WHEEL(&PROJECT.master_width,0,2,3,1.5f);SNAP("width-wheel");
    PROJECT.insert_volume[0]=1;CAPTURE(&PROJECT.insert_volume[0],0,2,1);FADER(400,200);DRAG(0,1,250);SNAP("fader-unity");DRAG(0,1,200);SNAP("fader-boost");DRAG(0,1,450);SNAP("fader-silence");
    PROJECT.fm[0].engine=1;PROJECT.fm[0].dx7.value[18]=31;
    ALIAS(&PROJECT.fm[0].dx7.value[18],7,1,0,0);SNAP("alias-hover-only");ALIAS(&PROJECT.fm[0].dx7.value[18],7,1,1,0);
    CAPTURE(&PROJECT.fm[0].dx7.value[18],0,3,0);BOUNDS(0,3);DRAG(80,1,0);SNAP("alias-capture");WHEEL(&PROJECT.fm[0].dx7.value[18],0,3,7,-.5f);SNAP("alias-integer-wheel");
}

int main(void){project_new(&project);out=fopen("controls.txt","w");if(!out)return 1;run();fclose(out);return 0;}
