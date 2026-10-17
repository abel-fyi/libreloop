// SPDX-License-Identifier: GPL-3.0-only
#include "project_check.h"
#include "engine.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
static float frequency(Sample s) {
    unsigned crosses=0,start=s.frames/4,end=s.frames*3/4;
    for(unsigned i=start+1;i<end;i++) if(s.data[i-1]<=0 && s.data[i]>0) crosses++;
    return crosses*(float)RATE/(end-start);
}
int main(void) {
    float data[]={.1f,-.4f,.2f,.8f,-.2f}; Sample input={data,5},out={0};
    Sampler p={.time=1,.length=1};
    CHECK(sample_process(input,p,&out) && out.frames==5 && !memcmp(out.data,data,sizeof data)); free(out.data);
    p.flags=SAMPLE_REVERSE|SAMPLE_POLARITY|SAMPLE_NORMALIZE;
    CHECK(sample_process(input,p,&out) && out.frames==5);
    CHECK(fabsf(out.data[0]-.25f)<1e-6 && out.data[1]==-1 && fabsf(out.data[4]+.125f)<1e-6); free(out.data);
    CHECK(data[0]==.1f && data[3]==.8f); /* Original remains untouched. */
    p=(Sampler){.time=1,.start=.4f,.length=2.f/3}; CHECK(sample_process(input,p,&out) && out.frames==2 && out.data[0]==.2f && out.data[1]==.8f); free(out.data);
    p.start=1; CHECK(sample_process(input,p,&out) && !out.frames && !out.data);
    p.start=NAN; CHECK(!sample_process(input,p,&out));
    float silence[16]={0}; p=(Sampler){.time=1,.length=1,.flags=SAMPLE_NORMALIZE}; CHECK(sample_process((Sample){silence,16},p,&out));
    for(int i=0;i<16;i++) CHECK(out.data[i]==0); free(out.data);
    float tail[]={0,.8f,-.4f,.1f,.01f,.001f,.0001f,0,0}; Sample padded={tail,9};
    p=(Sampler){.time=1,.length=1}; CHECK(sample_process(padded,p,&out) && out.frames==9); free(out.data);
    p.trim=.001f; CHECK(sample_process(padded,p,&out) && out.frames==6 && out.data[0]==.8f); free(out.data);
    p.trim=.4f; CHECK(sample_process(padded,p,&out) && out.frames==5 && out.data[0]==.8f); free(out.data);
    p.trim=1; CHECK(sample_process(padded,p,&out) && out.frames==3 && out.data[2]==.1f); free(out.data);
    p.trim=NAN; CHECK(!sample_process(padded,p,&out));
    p=(Sampler){.time=1,.length=1,.trim=.1f,.flags=SAMPLE_REVERSE};
    CHECK(sample_process(padded,p,&out) && out.frames==6 && out.data[0]==.0001f); free(out.data);
    CHECK(tail[8]==0 && tail[1]==.8f); /* reversible: original audio is intact */
    p=(Sampler){.time=1,.length=1,.trim=1};
    CHECK(sample_process((Sample){silence,16},p,&out) && !out.frames && !out.data);
    CHECK(sample_process((Sample){0},p,&out) && !out.frames && !out.data);
    float edges[]={.001f,0,.8f,0,-.6f,0,.001f};
    CHECK(sample_process((Sample){edges,7},p,&out) && out.frames==3);
    CHECK(out.data[0]==.8f && out.data[1]==0 && out.data[2]==-.6f); free(out.data);
    /* Trim runs inside the manually cropped range, before reverse and stretch. */
    p.start=1.f/7; p.length=5.f/6; p.flags=SAMPLE_REVERSE;
    CHECK(sample_process((Sample){edges,7},p,&out) && out.frames==3 && out.data[0]==-.6f && out.data[2]==.8f); free(out.data);
    p=(Sampler){.time=2,.length=1,.trim=1};
    CHECK(sample_process((Sample){edges,7},p,&out) && out.frames==6); free(out.data);
    unsigned n=RATE; float *tone=malloc(n*sizeof *tone); CHECK(tone);
    for(unsigned i=0;i<n;i++) tone[i]=.4f*sinf(2*3.14159265359f*440*i/RATE);
    input=(Sample){tone,n};
    p=(Sampler){.time=2,.length=1,.stretch=1}; CHECK(sample_process(input,p,&out) && out.frames==n*2);
    CHECK(fabsf(frequency(out)-440)<6); free(out.data);
    p.stretch=0; CHECK(sample_process(input,p,&out) && out.frames==n*2); CHECK(fabsf(frequency(out)-220)<4); free(out.data);
    p.time=1; p.pitch=12; CHECK(sample_process(input,p,&out) && out.frames==n); CHECK(fabsf(frequency(out)-880)<10); free(out.data);
    p.pitch=-12; CHECK(sample_process(input,p,&out) && out.frames==n); CHECK(fabsf(frequency(out)-220)<5); free(out.data);
    p.time=.5f; p.pitch=0; p.stretch=1; CHECK(sample_process(input,p,&out) && out.frames==n/2); CHECK(fabsf(frequency(out)-440)<10); free(out.data);
    Project fitted,loaded_fit; project_default(&fitted); fitted.channel_audio[0]=1;
    fitted.sampler[0]=(Sampler){.time=1,.length=1,.fit_bpm=120};
    fitted.audio_seconds[0]=1; fitted.clips[0][0]=PATTERNS+1;
    fitted.clips[0][1]=PATTERNS+1; fitted.clip_steps[0][1]=.5f; fitted.clip_offsets[0][1]=.25f;
    float full=clip_length(&fitted,0,0),cut=clip_length(&fitted,0,1);
    fitted.bpm=240;
    CHECK(fabsf(clip_length(&fitted,0,0)-full)<.0001f && fabsf(clip_length(&fitted,0,1)-cut)<.0001f);
    CHECK(fitted.clip_offsets[0][1]==.25f && fitted.sampler[0].time==1 && fitted.sampler[0].pitch==0);
    CHECK(sample_process(input,fitted.sampler[0],&out) && out.frames==n);
    CHECK(fabsf(frequency(out)-440)<10); free(out.data);
    fitted.sampler[0].stretch=1;
    CHECK(sample_process(input,fitted.sampler[0],&out) && out.frames==n);
    CHECK(fabsf(frequency(out)-440)<10); free(out.data);
    CHECK(project_save("fit-tempo.llp",&fitted) && project_load("fit-tempo.llp",&loaded_fit));
    CHECK(sampler_equal(fitted.sampler[0],loaded_fit.sampler[0])); remove("fit-tempo.llp");
    fitted.bpm=120;
    CHECK(fitted.clip_offsets[0][1]==.25f && fabsf(clip_length(&fitted,0,0)-full)<.0001f);
    fitted.sampler[0].fit_bpm=0;
    fitted.bpm=240;
    CHECK(fabsf(clip_length(&fitted,0,0)-2*full)<.0001f);
    free(tone);
    Project project,loaded; project_default(&project); project.sampler[0]=(Sampler){.pitch=7,.time=2,.start=.1f,.length=.6f,.trim=.15f,.flags=7,.stretch=1,.fit_bpm=120};
    CHECK(project_save("sampler.llp",&project) && project_load("sampler.llp",&loaded)); CHECK(project_equal(&project,&loaded)); remove("sampler.llp");
    puts("Sampler transforms, trims, silence, pitch/duration independence and persistence passed."); return 0;
}
