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
static WavePeak blend(WavePeak a,WavePeak b,float t) {
    return (WavePeak){a.low+(b.low-a.low)*t,a.high+(b.high-a.high)*t};
}
static WavePeak envelope_bin(const Waveform *wave,Sample sample,unsigned base,unsigned frames,unsigned size,unsigned bin) {
    unsigned start=base+(uint64_t)bin*size,end=fmin((uint64_t)start+size,(uint64_t)base+frames);
    if(wave->tree && size>=BLOCK && size/BLOCK<=wave->leaves && base%size==0 &&
       (end-start==size || end==sample.frames)) {
        unsigned level=wave->leaves/(size/BLOCK);
        if(start/size<level) return wave->tree[level+start/size];
    }
    return waveform_range(wave,sample,start,end);
}
static WavePeak envelope_level(const Waveform *wave,Sample sample,unsigned start,unsigned frames,double position,unsigned size) {
    double at=fmax(0,fmin((frames-1)/size,position/size-.5));
    unsigned first=floor(at),last=fmin(first+1,(frames-1)/size);
    return blend(envelope_bin(wave,sample,start,frames,size,first),envelope_bin(wave,sample,start,frames,size,last),at-first);
}
WavePeak waveform_envelope_region(const Waveform *wave,Sample sample,unsigned start,unsigned frames,double position,double width) {
    if(start>=sample.frames || !frames || !isfinite(position) || !isfinite(width)) return (WavePeak){0};
    frames=fmin(frames,sample.frames-start);
    /* Source-aligned power-of-two bins stay stable during pans; blend levels at zoom boundaries. */
    double level=log2(fmax(1,fmin(width,frames)));
    unsigned size=(unsigned)exp2(floor(level));
    return blend(envelope_level(wave,sample,start,frames,position,size),envelope_level(wave,sample,start,frames,position,size*2),level-floor(level));
}
WavePeak waveform_envelope(const Waveform *wave,Sample sample,double position,double width) {
    return waveform_envelope_region(wave,sample,0,sample.frames,position,width);
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
