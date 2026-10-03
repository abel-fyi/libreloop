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
    p.trim=.001f; CHECK(sample_process(padded,p,&out) && out.frames==7 && out.data[0]==0); free(out.data);
    p.trim=.4f; CHECK(sample_process(padded,p,&out) && out.frames==6); free(out.data);
    p.trim=1; CHECK(sample_process(padded,p,&out) && out.frames==4 && out.data[3]==.1f); free(out.data);
    p.trim=NAN; CHECK(!sample_process(padded,p,&out));
    p=(Sampler){.time=1,.length=1,.trim=.1f,.flags=SAMPLE_REVERSE};
    CHECK(sample_process(padded,p,&out) && out.frames==7 && out.data[0]==.0001f); free(out.data);
    CHECK(tail[8]==0 && tail[1]==.8f); /* reversible: original audio is intact */
    unsigned n=RATE; float *tone=malloc(n*sizeof *tone); CHECK(tone);
    for(unsigned i=0;i<n;i++) tone[i]=.4f*sinf(2*3.14159265359f*440*i/RATE);
    input=(Sample){tone,n};
    p=(Sampler){.time=2,.length=1,.stretch=1}; CHECK(sample_process(input,p,&out) && out.frames==n*2);
    CHECK(fabsf(frequency(out)-440)<6); free(out.data);
    p.stretch=0; CHECK(sample_process(input,p,&out) && out.frames==n*2); CHECK(fabsf(frequency(out)-220)<4); free(out.data);
    p.time=1; p.pitch=12; CHECK(sample_process(input,p,&out) && out.frames==n); CHECK(fabsf(frequency(out)-880)<10); free(out.data);
    p.pitch=-12; CHECK(sample_process(input,p,&out) && out.frames==n); CHECK(fabsf(frequency(out)-220)<5); free(out.data);
    p.time=.5f; p.pitch=0; p.stretch=1; CHECK(sample_process(input,p,&out) && out.frames==n/2); CHECK(fabsf(frequency(out)-440)<10); free(out.data);
    free(tone);
    Project project,loaded; project_default(&project); project.sampler[0]=(Sampler){.pitch=7,.time=2,.start=.1f,.length=.6f,.trim=.15f,.flags=7,.stretch=1};
    CHECK(project_save("sampler.hbt",&project) && project_load("sampler.hbt",&loaded)); CHECK(project_equal(&project,&loaded)); remove("sampler.hbt");
    puts("Sampler transforms, trims, silence, pitch/duration independence and persistence passed."); return 0;
}
