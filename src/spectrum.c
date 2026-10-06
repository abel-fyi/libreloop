// SPDX-License-Identifier: GPL-3.0-only
#include "spectrum.h"
#include <math.h>
#include <string.h>
#define PI 3.14159265358979323846
void spectrum_reset(Spectrum *s) { memset(s,0,sizeof *s); for(int i=0;i<SPECTRUM_BINS;i++) s->db[i]=-90; }
float spectrum_frequency(float x) { return 20*powf(1000,x); }
static void transform(Spectrum *s) {
    float power[SPECTRUM_FFT/2+1]={0};
    for(int side=0;side<2;side++) {
        float re[SPECTRUM_FFT],im[SPECTRUM_FFT]={0};
        for(unsigned i=0;i<SPECTRUM_FFT;i++) re[i]=s->history[(s->cursor+i)%SPECTRUM_FFT][side]*(.5-.5*cos(2*PI*i/SPECTRUM_FFT));
        for(unsigned i=1,j=0;i<SPECTRUM_FFT;i++) {
            unsigned bit=SPECTRUM_FFT>>1;
            for(;j&bit;bit>>=1) j^=bit;
            j^=bit;
            if(i<j) { float t=re[i]; re[i]=re[j]; re[j]=t; }
        }
        for(unsigned n=2;n<=SPECTRUM_FFT;n*=2) {
            double angle=-2*PI/n;
            for(unsigned start=0;start<SPECTRUM_FFT;start+=n) {
                double wr=1,wi=0,cr=cos(angle),ci=sin(angle);
                for(unsigned j=0;j<n/2;j++) {
                    unsigned a=start+j,b=a+n/2;
                    float tr=wr*re[b]-wi*im[b],ti=wr*im[b]+wi*re[b];
                    re[b]=re[a]-tr; im[b]=im[a]-ti; re[a]+=tr; im[a]+=ti;
                    double next=wr*cr-wi*ci; wi=wr*ci+wi*cr; wr=next;
                }
            }
        }
        for(int i=0;i<=SPECTRUM_FFT/2;i++) power[i]+=(re[i]*re[i]+im[i]*im[i])*.5f;
    }
    for(int b=0;b<SPECTRUM_BINS;b++) {
        float lo=spectrum_frequency((float)b/SPECTRUM_BINS)*SPECTRUM_FFT/48000;
        float hi=spectrum_frequency((float)(b+1)/SPECTRUM_BINS)*SPECTRUM_FFT/48000;
        float p=0;
        if(hi-lo<1) {
            float at=(lo+hi)*.5f; int k=(int)at;
            p=power[k]+(power[k+1]-power[k])*(at-k);
        } else for(int k=(int)floorf(lo);k<=(int)ceilf(hi);k++) p=fmaxf(p,power[k]);
        float db=fmaxf(-90,10*log10f(fmaxf(1e-20f,p*16/(SPECTRUM_FFT*(float)SPECTRUM_FFT))));
        s->db[b]+= (db-s->db[b])*(db>s->db[b]?.75f:.18f);
    }
}
void spectrum_push(Spectrum *s,const float *stereo,unsigned frames) {
    for(unsigned i=0;i<frames;i++) {
        s->history[s->cursor][0]=stereo[2*i]; s->history[s->cursor][1]=stereo[2*i+1];
        s->cursor=(s->cursor+1)%SPECTRUM_FFT;
        if(++s->pending==1024) { s->pending=0; transform(s); }
    }
}
