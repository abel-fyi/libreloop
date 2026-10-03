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
    CHECK(waveform_build(&wave,(Sample){data,3}));
    CHECK(waveform_range(&wave,(Sample){data,3},0,3).high==fmaxf(data[1],data[2]));
    CHECK(waveform_build(&wave,(Sample){0}) && !wave.tree && !wave.leaves);
    CHECK(waveform_range(&wave,(Sample){0},0,1).low==0);
    CHECK(waveform_range(&wave,s,256,257).low==-1); /* allocation-free fallback */
    puts("Waveform cache preserves exact peaks at coarse, fine and single-sample zoom levels."); return 0;
}
