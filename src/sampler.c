// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* WSOLA: align overlapping grains before crossfading, preserving local pitch. */
static float at(const float *data,unsigned frames,unsigned i) { return i<frames?data[i]:0; }
static float similarity(const float *in,unsigned frames,const float *out,unsigned d,unsigned start,unsigned hop) {
    double dot=0,a=1e-20,b=1e-20;
    unsigned stride=hop/64; if(!stride) stride=1;
    for(unsigned i=0;i<hop;i+=stride) { float x=out[d+i],y=at(in,frames,start+i); dot+=x*y; a+=x*x; b+=y*y; }
    return dot/sqrt(a*b);
}
static float *stretch(const float *in,unsigned frames,unsigned length) {
    float *out=calloc(length,sizeof *out); if(!out) return NULL;
    unsigned window=frames<1024?frames:1024,hop=window/2;
    if(hop<2) { for(unsigned i=0;i<length;i++) out[i]=in[(uint64_t)i*frames/length]; return out; }
    memcpy(out,in,(length<window?length:window)*sizeof *out);
    float ratio=length/(float)frames;
    for(unsigned d=hop;d<length;d+=hop) {
        unsigned overlap=length-d<hop?length-d:hop;
        int nominal=d/ratio,radius=frames<768?frames/2:384;
        int low=nominal-radius,high=nominal+radius; if(low<0) low=0; if(high>=(int)frames) high=frames-1;
        int best=nominal<(int)frames?nominal:(int)frames-1; float score=-2;
        for(int s=low;s<=high;s+=4) {
            float q=similarity(in,frames,out,d,s,overlap)-abs(s-nominal)*1e-6f;
            if(q>score) { score=q; best=s; }
        }
        int center=best;
        for(int s=center-3;s<=center+3;s++) if(s>=low && s<=high) {
            float q=similarity(in,frames,out,d,s,overlap)-abs(s-nominal)*1e-6f;
            if(q>score) { score=q; best=s; }
        }
        for(unsigned i=0;i<window && d+i<length;i++) {
            float x=at(in,frames,best+i);
            if(i<hop) { float w=.5f-.5f*cosf(3.14159265359f*i/hop); out[d+i]=out[d+i]*(1-w)+x*w; }
            else out[d+i]=x;
        }
    }
    return out;
}
int sampler_valid(Sampler s) { return isfinite(s.pitch) && fabsf(s.pitch)<=12 && isfinite(s.time) && s.time>=.25f && s.time<=4 && isfinite(s.start) && s.start>=0 && s.start<=1 && isfinite(s.length) && s.length>=0 && s.length<=1 && s.flags<=7 && s.stretch<=1; }
int sampler_equal(Sampler a,Sampler b) { return a.pitch==b.pitch && a.time==b.time && a.start==b.start && a.length==b.length && a.flags==b.flags && a.stretch==b.stretch; }
int sample_process(Sample source,Sampler settings,Sample *result) {
    if(!sampler_valid(settings) || source.frames>RATE*60 || (source.frames && !source.data)) return 0;
    unsigned offset=llround(source.frames*(double)settings.start);
    source.data=source.data?source.data+offset:NULL;
    source.frames=llround((source.frames-offset)*(double)settings.length);
    if(!source.frames) { *result=(Sample){0}; return 1; }
    float *input=malloc(source.frames*sizeof *input); if(!input) return 0;
    for(unsigned i=0;i<source.frames;i++) input[i]=source.data[(settings.flags&SAMPLE_REVERSE)?source.frames-1-i:i]*((settings.flags&SAMPLE_POLARITY)?-1:1);
    unsigned length=fmax(1,llround(source.frames*(double)settings.time));
    double pitch=pow(2,settings.pitch/12.0)/(settings.stretch?1:settings.time);
    unsigned intermediate=fmax(1,llround(length*pitch));
    float *grains=intermediate==source.frames?input:stretch(input,source.frames,intermediate);
    if(!grains) { free(input); return 0; }
    float *output;
    if(length==intermediate) output=grains;
    else {
        output=malloc(length*sizeof *output);
        if(output) for(unsigned i=0;i<length;i++) {
            double position=i*(double)intermediate/length; unsigned n=position;
            float a=grains[n],b=n+1<intermediate?grains[n+1]:a;
            output[i]=a+(b-a)*(position-n);
        }
        if(grains!=input) free(grains);
    }
    if(output!=input) free(input);
    if(!output) return 0;
    if(settings.flags&SAMPLE_NORMALIZE) {
        float peak=0; for(unsigned i=0;i<length;i++) peak=fmaxf(peak,fabsf(output[i]));
        if(peak>0) for(unsigned i=0;i<length;i++) output[i]/=peak;
    }
    *result=(Sample){output,length}; return 1;
}
