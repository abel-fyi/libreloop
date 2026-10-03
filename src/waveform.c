// SPDX-License-Identifier: GPL-3.0-only
#include "waveform.h"
#include <stdlib.h>
#include <math.h>
#define BLOCK 256
static WavePeak merge(WavePeak a,WavePeak b) { return (WavePeak){fminf(a.low,b.low),fmaxf(a.high,b.high)}; }
static WavePeak scan(Sample s,unsigned start,unsigned end) {
    WavePeak peak={0};
    for(unsigned i=start;i<end;i++) for(unsigned side=0;side<sample_channels(s);side++) {
        float x=sample_at(s,i,side);
        if(x<peak.low) peak.low=x;
        if(x>peak.high) peak.high=x;
    }
    return peak;
}
int waveform_build(Waveform *wave,Sample sample) {
    free(wave->tree); *wave=(Waveform){0};
    if(!sample.frames) return 1;
    unsigned blocks=(sample.frames-1)/BLOCK+1,leaves=1;
    while(leaves<blocks) leaves*=2;
    WavePeak *tree=calloc((size_t)leaves*2,sizeof *tree); if(!tree) return 0;
    for(unsigned i=0;i<blocks;i++) {
        unsigned end=(i+1)*BLOCK; if(end>sample.frames) end=sample.frames;
        tree[leaves+i]=scan(sample,i*BLOCK,end);
    }
    for(unsigned i=leaves-1;i;i--) tree[i]=merge(tree[i*2],tree[i*2+1]);
    *wave=(Waveform){tree,leaves}; return 1;
}
WavePeak waveform_range(const Waveform *wave,Sample sample,unsigned start,unsigned end) {
    if(end>sample.frames) end=sample.frames;
    if(start>=end) return (WavePeak){0};
    if(!wave->tree) return scan(sample,start,end);
    /* Exact partial blocks at the edges; cached coarse levels in between. */
    unsigned first=(start+BLOCK-1)/BLOCK,last=end/BLOCK;
    if(first>=last) return scan(sample,start,end);
    WavePeak peak=merge(scan(sample,start,first*BLOCK),scan(sample,last*BLOCK,end));
    for(unsigned a=first+wave->leaves,b=last+wave->leaves;a<b;a/=2,b/=2) {
        if(a&1) peak=merge(peak,wave->tree[a++]);
        if(b&1) peak=merge(peak,wave->tree[--b]);
    }
    return peak;
}
int waveform_append(Waveform *wave,Sample sample,unsigned previous_frames) {
    unsigned blocks=sample.frames?(sample.frames-1)/BLOCK+1:0;
    if(!wave->tree || blocks>wave->leaves) return waveform_build(wave,sample);
    for(unsigned i=previous_frames/BLOCK;i<blocks;i++) {
        unsigned end=(i+1)*BLOCK; if(end>sample.frames) end=sample.frames;
        unsigned at=wave->leaves+i; wave->tree[at]=scan(sample,i*BLOCK,end);
        for(at/=2;at;at/=2) wave->tree[at]=merge(wave->tree[at*2],wave->tree[at*2+1]);
    }
    return 1;
}
