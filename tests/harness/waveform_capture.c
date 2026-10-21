// SPDX-License-Identifier: GPL-3.0-only
#include "waveform_draw.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
static FILE *out;
static void emit(void *unused,const WaveVertex quad[4]){(void)unused;for(int i=0;i<4;i++)fprintf(out,"%.9g %.9g %.9g %.9g\n",quad[i].point.x,quad[i].point.y,quad[i].u,quad[i].v);}
static Waveform *preview_wave;static Sample preview_sample;
static WavePeak preview(void *unused,int channel,double position,double width,unsigned frames){(void)unused;(void)channel;(void)frames;return waveform_envelope(preview_wave,preview_sample,position+7.25,width*1.125);}
static void draw_port(WaveDrawing view,DrawRect area,float left,float right,float layout,float raster){preview_wave=(Waveform *)view.wave;preview_sample=view.sample;wave_drawing_quads(view,area,left,right,layout,raster,preview,NULL,emit,NULL);}
#define VIEW WaveDrawing
#define RECT DrawRect
#define DRAW draw_port

static float samples[4096*2];static Waveform wave;static Project project;
int main(int argc,char **argv){if(argc!=2)return 1;int mode=atoi(argv[1]);if(mode<0 || mode>4)return 1;
    int channels=mode==1?2:1;for(int i=0;i<4096;i++)for(int c=0;c<channels;c++)samples[i*channels+c]=sinf(i*.013f+c*.3f)*(.1f+.9f*cosf(i*.007f)*cosf(i*.007f));
    Sample sample={.data=samples,.frames=4096,.channels=channels};if(!waveform_build(&wave,sample))return 2;project_new(&project);
    AudioTimeline timeline;project.sampler[0].fit_bpm=120;project.clips[0][0]=1;project.clip_starts[0][0]=0;project.clip_steps[0][0]=64;
    project.bpm=147;project.pitch_range[0]=12;project.channel_audio[0]=1;project.audio_seconds[0]=4096/(float)RATE;
    project.clips[0][0]=PATTERNS+1;
    int automation=automation_create(&project,(ParameterTarget){PARAM_CHANNEL_PITCH,0,0},"Pitch",64);
    project.clips[1][0]=AUTOMATION_SOURCE+automation+1;project.clip_steps[1][0]=64;
    project.automations[automation].count=3;project.automations[automation].points[0]=(AutomationPoint){0,0};
    project.automations[automation].points[1]=(AutomationPoint){4,.5f};project.automations[automation].points[2]=(AutomationPoint){16,1};
    audio_timeline_init(&timeline,&project,0);
    VIEW view={.sample=sample,.wave=&wave,.channel=0,.preview=mode==4,.timeline=mode==2?&timeline:NULL,.start=0,.offset=125.25,.frames_per_step=4800,.origin=12.25f,.width=220.5f,.pixels_per_step=25};
    float layout=mode==1?2:1,raster=mode==0?1:mode==1?4:1.25f;
    out=fopen("quads.txt","wb");if(!out)return 2;DRAW(view,((RECT){12.25f,17.5f,220.5f,63.25f}),mode==3?50:15.125f,mode==3?50:131.75f,layout,raster);fclose(out);free(wave.tree);return 0;}
