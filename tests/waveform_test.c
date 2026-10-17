// SPDX-License-Identifier: GPL-3.0-only
#include "waveform.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
int main(void) {
    float data[4099];
    for(unsigned i=0;i<4099;i++) data[i]=sinf(i*.31f)*.5f;
    data[256]=-1; data[2048]=1;
    Sample s={data,4099}; Waveform wave={0}; CHECK(waveform_build(&wave,s));
    /* Compare cached ranges against the original samples at every scale. */
    for(unsigned start=0;start<s.frames;start+=37) for(unsigned size=1;size<8192;size*=2) {
        unsigned end=start+size; if(end>s.frames) end=s.frames;
        WavePeak expected={0},actual=waveform_range(&wave,s,start,end);
        for(unsigned i=start;i<end;i++) { expected.low=fminf(expected.low,data[i]); expected.high=fmaxf(expected.high,data[i]); }
        CHECK(actual.low==expected.low && actual.high==expected.high);
    }
    CHECK(waveform_range(&wave,s,256,257).low==-1);
    CHECK(waveform_range(&wave,s,2048,2049).high==1);
    CHECK(waveform_range(&wave,s,4100,5000).high==0);
    /* Display bins interpolate continuously during fractional pans and across zoom levels. */
    for(unsigned size=1;size<=8192;size*=2) {
        WavePeak before=waveform_envelope(&wave,s,2047.999,size),after=waveform_envelope(&wave,s,2048.001,size);
        CHECK(fabsf(before.low-after.low)<.005f && fabsf(before.high-after.high)<.005f);
        before=waveform_envelope(&wave,s,2048,size*.999999);
        after=waveform_envelope(&wave,s,2048,size*1.000001);
        CHECK(fabsf(before.low-after.low)<.0001f && fabsf(before.high-after.high)<.0001f);
        CHECK(after.low>=-1 && after.high<=1 && after.low<=0 && after.high>=0);
        Waveform uncached={0}; WavePeak fallback=waveform_envelope(&uncached,s,2048,size*1.000001);
        CHECK(fallback.low==after.low && fallback.high==after.high);
    }
    CHECK(waveform_envelope(&wave,(Sample){0},0,1).high==0);
    CHECK(waveform_envelope(&wave,s,NAN,1).high==0);
    float stereo[]={.8f,-.9f,.8f,-.9f}; Waveform empty={0};
    WavePeak both=waveform_envelope(&empty,(Sample){stereo,2,2},.5,1);
    CHECK(both.high==.8f && both.low==-.9f);
    float cropped[]={1,.2f,.2f,-1}; Sample crop={cropped,4};
    for(unsigned width=1;width<=8192;width*=2) {
        WavePeak region=waveform_envelope_region(&empty,crop,1,2,0,width);
        CHECK(region.high==.2f && region.low==0); /* Excludes trimmed-away peaks. */
        region=waveform_envelope_region(&wave,s,37,2000,1024,width);
        WavePeak fallback=waveform_envelope_region(&empty,s,37,2000,1024,width);
        CHECK(region.low==fallback.low && region.high==fallback.high);
    }
    CHECK(waveform_build(&wave,(Sample){data,3}));
    CHECK(waveform_range(&wave,(Sample){data,3},0,3).high==fmaxf(data[1],data[2]));
    CHECK(waveform_build(&wave,(Sample){0}) && !wave.tree && !wave.leaves);
    CHECK(waveform_range(&wave,(Sample){0},0,1).low==0);
    CHECK(waveform_range(&wave,s,256,257).low==-1); /* allocation-free fallback */
    /* Live recording extends partial blocks and grows the cache without losing peaks. */
    unsigned previous=0;
    for(unsigned n=1;n<4099;n+=113) {
        Sample growing={data,n}; CHECK(waveform_append(&wave,growing,previous)); previous=n;
        for(unsigned start=0;start<n;start+=251) {
            WavePeak expected={0},actual=waveform_range(&wave,growing,start,n);
            for(unsigned i=start;i<n;i++) { expected.low=fminf(expected.low,data[i]); expected.high=fmaxf(expected.high,data[i]); }
            CHECK(actual.low==expected.low && actual.high==expected.high);
        }
    }
    free(wave.tree);
    puts("Waveform cache preserves exact peaks at coarse, fine and single-sample zoom levels."); return 0;
}
