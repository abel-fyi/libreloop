// SPDX-License-Identifier: GPL-3.0-only
#include "spectrum.h"
#include <math.h>
#include <stdio.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed %d: %s\n",__LINE__,#x); return 1; } } while(0)
int main(void) {
    Spectrum s; spectrum_reset(&s); float pcm[1024*2];
    for(int n=0;n<24;n++) {
        for(int i=0;i<1024;i++) { pcm[2*i]=.5f*sinf(6.2831853f*1000*(n*1024+i)/48000); pcm[2*i+1]=-pcm[2*i]; }
        spectrum_push(&s,pcm,1024);
    }
    int peak=0; for(int i=1;i<SPECTRUM_BINS;i++) if(s.db[i]>s.db[peak]) peak=i;
    float frequency=spectrum_frequency((peak+.5f)/SPECTRUM_BINS);
    CHECK(frequency>900 && frequency<1100 && s.db[peak]>-9 && s.db[peak]<-4);
    for(int i=0;i<2048;i++) pcm[i]=0;
    for(int n=0;n<100;n++) spectrum_push(&s,pcm,1024);
    for(int i=0;i<SPECTRUM_BINS;i++) CHECK(isfinite(s.db[i]) && s.db[i]<-85);
    CHECK(fabsf(spectrum_frequency(0)-20)<.01 && fabsf(spectrum_frequency(1)-20000)<.01);
    puts("Log spectrum, calibrated sine peak, stereo phase immunity and silence decay passed."); return 0;
}
