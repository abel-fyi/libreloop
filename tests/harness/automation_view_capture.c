// SPDX-License-Identifier: GPL-3.0-only
#include "automation_view.h"
#include <stdio.h>
static Project project;static AutomationView view;static FILE *out;static int playing,song;
static void update(int p,int s,double step){playing=p;song=s;automation_view_update(&view,&project,p,s,step);}
static void snapshot(int mode,int time){fprintf(out,"%d %d %.9g %.9g",mode,time,automation_displayed_value(&view,&project,playing,song,&project.volume[0],project.volume[0]),automation_displayed_value(&view,&project,playing,song,&project.pitch_range[0],project.pitch_range[0]));for(int a=0;a<project.automation_count;a++)fprintf(out," %d %.9g",view.visible[a],view.shown[a]);fprintf(out,"\n");}
#define PROJECT project
#define UPDATE update
#define SNAP snapshot

static void setup(Project *p){project_new(p);int a=automation_create(p,(ParameterTarget){PARAM_CHANNEL_VOLUME,0,0},"Volume",8),b=automation_create(p,(ParameterTarget){PARAM_CHANNEL_VOLUME,0,0},"Duplicate target",16),c=automation_create(p,(ParameterTarget){PARAM_PITCH_RANGE,0,0},"Range",8);p->automations[a].points[0].value=.1f;p->automations[a].points[1].value=.8f;p->automations[b].points[0].value=.3f;p->automations[b].points[1].value=.6f;p->automations[c].points[0].value=.02f;p->automations[c].points[1].value=.8f;p->clips[0][0]=AUTOMATION_SOURCE+a+1;p->clip_steps[0][0]=8;p->clip_starts[0][0]=0;p->clips[1][0]=AUTOMATION_SOURCE+b+1;p->clip_steps[1][0]=16;p->clip_starts[1][0]=.25f;p->clip_offsets[1][0]=2;p->clips[2][0]=AUTOMATION_SOURCE+c+1;p->clip_steps[2][0]=8;p->clip_starts[2][0]=.5f;}
static void run(void){const double times[]={0,3.999,4,7.999,8,12,16,24};for(int mode=0;mode<4;mode++){PROJECT.lane_mute[1]=mode==1?1:mode==2?2:0;for(int t=0;t<8;t++){UPDATE(mode==3?0:1,1,times[t]);SNAP(mode,t);}}UPDATE(1,0,8);SNAP(4,0);}

int main(void){setup(&project);out=fopen("automation-view.txt","w");if(!out)return 1;run();fclose(out);return 0;}
