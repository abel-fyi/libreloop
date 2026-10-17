// SPDX-License-Identifier: GPL-3.0-only
#include "rack.h"
#include <stdio.h>
static Project project;static uint8_t melodic[CHANNELS];static RackState state={.paint_channel=-1,.melodic=melodic};static FILE *out;
static void paint(float width,float x,float y){rack_paint(&state,&project,width,168,x,y);}
static void snapshot(const char *name){int rows[CHANNELS],n=rack_channels(&state,&project,rows);fprintf(out,"%s %d %.9g %.9g %d %d",name,state.paint_channel,state.paint_step,project.pattern_steps[0],state.filter,state.scroll);for(int i=0;i<n;i++)fprintf(out," %d",rows[i]);fprintf(out,"\n");}
#define PAINT paint
#define SNAP snapshot
#define SET_TOOL(v) (state.paint=(v))
#define SET_FILTER(v) (state.filter=(v))
#define SET_SCROLL(v) (state.scroll=(v))
#define SET_VIEW(v) (state.view=(v))

static void run(void){PAINT(480,208,40);SNAP("first");PAINT(480,360,40);SNAP("interpolation");PAINT(480,199.999f,40);SNAP("outside");PAINT(480,328,68);SNAP("second-row");SET_TOOL(1);PAINT(480,360,68);SNAP("erase");SET_FILTER(1);PAINT(480,240,40);SNAP("audio-filter");SET_FILTER(2);PAINT(480,240,40);SNAP("non-audio-filter");SET_FILTER(0);SET_SCROLL(2);PAINT(480,240,40);SNAP("melodic-note-block");SET_SCROLL(3);PAINT(480,240,40);SNAP("melodic-flag-block");SET_SCROLL(0);SET_VIEW(16.25f);SET_TOOL(0);PAINT(480,440,40);SNAP("duration-extension");PAINT(480,454,40);SNAP("right-boundary");}

int main(void){project_new(&project);project.channel_count=4;project.channel_audio[1]=1;project.notes[0][2][0]=(Note){61,100,0,0};melodic[3]=1;out=fopen("rack.txt","w");if(!out)return 1;run();fclose(out);return project_save("expected.llp",&project)?0:1;}
