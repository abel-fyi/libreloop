// SPDX-License-Identifier: GPL-3.0-only
#include "sampler.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* WSOLA: align overlapping grains before crossfading, preserving local pitch. */
static float at(const float *data,unsigned frames,unsigned i,unsigned channels,unsigned side) { return i<frames?data[(size_t)i*channels+side]:0; }
static float similarity(const float *in,unsigned frames,const float *out,unsigned d,unsigned start,unsigned hop,unsigned channels) {
    double dot=0,a=1e-20,b=1e-20;
    unsigned stride=hop/64; if(!stride) stride=1;
    for(unsigned i=0;i<hop;i+=stride) for(unsigned side=0;side<channels;side++) { float x=out[(size_t)(d+i)*channels+side],y=at(in,frames,start+i,channels,side); dot+=x*y; a+=x*x; b+=y*y; }
    return dot/sqrt(a*b);
}
static float *stretch(const float *in,unsigned frames,unsigned length,unsigned channels) {
    float *out=calloc((size_t)length*channels,sizeof *out); if(!out) return NULL;
    unsigned window=frames<1024?frames:1024,hop=window/2;
    if(hop<2) { for(unsigned i=0;i<length;i++) for(unsigned side=0;side<channels;side++) out[(size_t)i*channels+side]=in[((uint64_t)i*frames/length)*channels+side]; return out; }
    memcpy(out,in,(size_t)(length<window?length:window)*channels*sizeof *out);
    float ratio=length/(float)frames;
    for(unsigned d=hop;d<length;d+=hop) {
        unsigned overlap=length-d<hop?length-d:hop;
        int nominal=d/ratio,radius=frames<768?frames/2:384;
        int low=nominal-radius,high=nominal+radius; if(low<0) low=0; if(high>=(int)frames) high=frames-1;
        int best=nominal<(int)frames?nominal:(int)frames-1; float score=-2;
        for(int s=low;s<=high;s+=4) {
            float q=similarity(in,frames,out,d,s,overlap,channels)-abs(s-nominal)*1e-6f;
            if(q>score) { score=q; best=s; }
        }
        int center=best;
        for(int s=center-3;s<=center+3;s++) if(s>=low && s<=high) {
            float q=similarity(in,frames,out,d,s,overlap,channels)-abs(s-nominal)*1e-6f;
            if(q>score) { score=q; best=s; }
        }
        for(unsigned i=0;i<window && d+i<length;i++) for(unsigned side=0;side<channels;side++) {
            float x=at(in,frames,best+i,channels,side); size_t index=(size_t)(d+i)*channels+side;
            if(i<hop) { float w=.5f-.5f*cosf(3.14159265359f*i/hop); out[index]=out[index]*(1-w)+x*w; }
            else out[index]=x;
        }
    }
    return out;
}
int sampler_processing_equal(Sampler a,Sampler b) {
    a.fit_bpm=b.fit_bpm=0;
    return sampler_equal(a,b);
}
int sampler_valid(Sampler s) { return isfinite(s.pitch) && fabsf(s.pitch)<=12 && isfinite(s.time) && s.time>=.25f && s.time<=4 && isfinite(s.start) && s.start>=0 && s.start<=1 && isfinite(s.length) && s.length>=0 && s.length<=1 && isfinite(s.trim) && s.trim>=0 && s.trim<=1 && s.flags<=7 && s.stretch<=1 && isfinite(s.fit_bpm) && (!s.fit_bpm || (s.fit_bpm>=30 && s.fit_bpm<=300)); }
int sampler_equal(Sampler a,Sampler b) { return a.pitch==b.pitch && a.time==b.time && a.start==b.start && a.length==b.length && a.trim==b.trim && a.flags==b.flags && a.stretch==b.stretch && a.fit_bpm==b.fit_bpm; }
static float frame_peak(Sample source,unsigned frame) {
    float peak=0;
    for(unsigned side=0;side<sample_channels(source);side++) peak=fmaxf(peak,fabsf(sample_at(source,frame,side)));
    return peak;
}
Sample sample_trim(Sample source,float trim) {
    if(trim>0) {
        /* Remove quiet edges at -90 to -30 dBFS; preserve a frame audible on either side. */
        float threshold=powf(10.f,-4.5f+3.f*trim);
        unsigned start=0,end=source.frames;
        while(end && frame_peak(source,end-1)<=threshold) end--;
        while(start<end && frame_peak(source,start)<=threshold) start++;
        if(source.data) source.data+=(size_t)start*sample_channels(source);
        source.frames=end-start;
    }
    return source;
}
int sample_process(Sample source,Sampler settings,Sample *result) {
    unsigned channels=sample_channels(source);
    if(channels>2 || !sampler_valid(settings) || source.frames>SAMPLE_MAX_FRAMES || (source.frames && !source.data)) return 0;
    /* Unmodified mapped recordings share read-only storage instead of duplicating long takes. */
    if(source.storage && settings.pitch==0 && settings.time==1 && settings.start==0 &&
       settings.length==1 && settings.trim==0 && !settings.flags) return sample_clone(source,result);
    /* Bound every output/intermediate allocation. */
    if(source.frames*(double)settings.time>INT_MAX/2 || source.frames*(double)settings.time*pow(2,settings.pitch/12.0)>INT_MAX/2) return 0;
    unsigned offset=llround(source.frames*(double)settings.start);
    source.data=source.data?source.data+(size_t)offset*channels:NULL;
    source.frames=llround((source.frames-offset)*(double)settings.length);
    source=sample_trim(source,settings.trim);
    if(!source.frames) { *result=(Sample){0}; return 1; }
    float *input=malloc((size_t)source.frames*channels*sizeof *input); if(!input) return 0;
    for(unsigned i=0;i<source.frames;i++) for(unsigned side=0;side<channels;side++) input[(size_t)i*channels+side]=sample_at(source,(settings.flags&SAMPLE_REVERSE)?source.frames-1-i:i,side)*((settings.flags&SAMPLE_POLARITY)?-1:1);
    unsigned length=fmax(1,llround(source.frames*(double)settings.time));
    double pitch=pow(2,settings.pitch/12.0)/(settings.stretch?1:settings.time);
    unsigned intermediate=fmax(1,llround(length*pitch));
    float *grains=intermediate==source.frames?input:stretch(input,source.frames,intermediate,channels);
    if(!grains) { free(input); return 0; }
    float *output;
    if(length==intermediate) output=grains;
    else {
        output=malloc((size_t)length*channels*sizeof *output);
        if(output) for(unsigned i=0;i<length;i++) for(unsigned side=0;side<channels;side++) {
            double position=i*(double)intermediate/length; unsigned n=position;
            float a=grains[(size_t)n*channels+side],b=n+1<intermediate?grains[(size_t)(n+1)*channels+side]:a;
            output[(size_t)i*channels+side]=a+(b-a)*(position-n);
        }
        if(grains!=input) free(grains);
    }
    if(output!=input) free(input);
    if(!output) return 0;
    if(settings.flags&SAMPLE_NORMALIZE) {
        float peak=0; for(unsigned i=0;i<(size_t)length*channels;i++) peak=fmaxf(peak,fabsf(output[i]));
        if(peak>0) for(unsigned i=0;i<(size_t)length*channels;i++) output[i]/=peak;
    }
    *result=(Sample){.data=output,.frames=length,.channels=channels}; return 1;
}
