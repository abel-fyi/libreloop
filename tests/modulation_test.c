// SPDX-License-Identifier: GPL-3.0-only
#include "phaser.h"
#include "flanger.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static float reference[96000][2];
static double tone(int phaser,int wet,float hz){Phaser p;phaser_reset(&p);PhaserSettings ps=phaser_default();ps.depth=ps.feedback=ps.stereo=0;Flanger *f=flanger_create();FlangerSettings fs=flanger_default();fs.delay=10;fs.depth=fs.feedback=fs.stereo=0;double input=0,output=0;for(int i=0;i<48000;i++){float x=.2f*sinf(i*6.28318530718f*hz/48000),v[2]={x,x};if(phaser)phaser_process(&p,ps,wet?1:.5f,v);else flanger_process(f,fs,wet?1:.5f,v);if(i>24000){input+=x*x;output+=v[0]*v[0];}}flanger_free(f);return 10*log10(fmax(output,1e-30)/input);}
int main(void){
    CHECK(phaser_valid(phaser_default()) && flanger_valid(flanger_default()));for(unsigned i=0;i<3;i++){CHECK(phaser_valid(phaser_factory(i)) && flanger_valid(flanger_factory(i)));CHECK(phaser_factory_name(i)[0] && flanger_factory_name(i)[0]);}
    PhaserSettings ps=phaser_default();FlangerSettings fs=flanger_default();ps.rate=NAN;fs.delay=NAN;CHECK(!phaser_valid(ps) && !flanger_valid(fs));ps=phaser_default();ps.stages=3.5f;CHECK(!phaser_valid(ps));fs=flanger_default();fs.invert=2;CHECK(!flanger_valid(fs));
    Phaser p;phaser_reset(&p);Flanger *f=flanger_create();CHECK(f);ps=phaser_default();fs=flanger_default();
    for(int i=0;i<48000;i++){float a[2]={0},b[2]={0};phaser_process(&p,ps,1,a);flanger_process(f,fs,1,b);CHECK(a[0]==0 && a[1]==0 && b[0]==0 && b[1]==0);}
    phaser_reset(&p);flanger_reset(f);for(int i=0;i<1000;i++){float a[2]={.2f,-.3f},b[2]={.2f,-.3f};phaser_process(&p,ps,0,a);flanger_process(f,fs,0,b);CHECK(a[0]==.2f && a[1]==-.3f && b[0]==.2f && b[1]==-.3f);}
    ps.stereo=fs.stereo=0;phaser_reset(&p);flanger_reset(f);for(int i=0;i<48000;i++){float x=.2f*sinf(i*.1f),a[2]={x,x},b[2]={x,x};phaser_process(&p,ps,.5f,a);flanger_process(f,fs,.5f,b);CHECK(a[0]==a[1] && b[0]==b[1]);}
    ps=phaser_default();fs=flanger_default();phaser_reset(&p);flanger_reset(f);double phase_side=0,flange_side=0;
    for(int i=0;i<48000;i++){float x=.2f*sinf(i*.1f),a[2]={x,x},b[2]={x,x};phaser_process(&p,ps,.5f,a);flanger_process(f,fs,.5f,b);phase_side+=(a[0]-a[1])*(a[0]-a[1]);flange_side+=(b[0]-b[1])*(b[0]-b[1]);}CHECK(phase_side>1e-5 && flange_side>1e-5);
    double notch=tone(1,0,900),allpass=tone(1,1,900),comb=tone(0,0,50),pass=tone(0,0,100);printf("Phaser center notch %.2f dB, wet allpass %.3f dB; Flanger 50 Hz notch %.2f dB / 100 Hz pass %.3f dB.\n",notch,allpass,comb,pass);CHECK(notch<-65 && fabs(allpass)<.01 && comb<-65 && fabs(pass)<.01);
    fs=flanger_default();fs.delay=2.5f;fs.depth=fs.feedback=fs.stereo=0;flanger_reset(f);for(int i=0;i<400;i++){float v[2]={i==0,i==0};flanger_process(f,fs,1,v);CHECK(v[0]==(i==120?1.f:0.f) && v[1]==v[0]);}
    fs.invert=1;flanger_reset(f);for(int i=0;i<400;i++){float v[2]={i==0,0};flanger_process(f,fs,1,v);CHECK(v[0]==(i==120?-1.f:0.f) && v[1]==0);}
    /* Rate, sweep, spread, polarity, stages and feedback edits are deterministic
       for 64/512-frame callers; extreme feedback stays finite and decays. */
    for(int effect=0;effect<2;effect++)for(int block=64;block<=512;block*=8){phaser_reset(&p);flanger_reset(f);ps=phaser_default();fs=flanger_default();float previous=0,largest=0;for(int start=0;start<96000;start+=block)for(int i=start;i<start+block && i<96000;i++){if(i==4096){ps.stages=12;fs.invert=1;}if(i==16384){ps.center=5000;fs.delay=10;}if(i==32768){ps.feedback=-.9f;fs.feedback=-.95f;}if(i==49152){ps.stereo=fs.stereo=1;}float v[2]={.1f*sinf(i*.05f),.1f*cosf(i*.07f)};if(effect)flanger_process(f,fs,.5f,v);else phaser_process(&p,ps,.5f,v);CHECK(isfinite(v[0]) && isfinite(v[1]));largest=fmaxf(largest,fabsf(v[0]-previous));previous=v[0];if(block==64)memcpy(reference[i],v,sizeof v);else CHECK(!memcmp(reference[i],v,sizeof v));}CHECK(largest<.15);}
    for(int effect=0;effect<2;effect++)for(int range=0;range<2;range++)for(int sign=-1;sign<=1;sign+=2){ps=(PhaserSettings){10,range?8000:80,4,sign*.9f,1,12};fs=(FlangerSettings){10,range?12:.2f,1,sign*.95f,1,0};phaser_reset(&p);flanger_reset(f);double first=0,last=0;for(int i=0;i<48000*12;i++){float x=i<48000?.2f*sinf(i*.1f):0,v[2]={x,x};if(effect)flanger_process(f,fs,1,v);else phaser_process(&p,ps,1,v);CHECK(isfinite(v[0]) && isfinite(v[1]) && fabsf(v[0])<4 && fabsf(v[1])<4);double energy=v[0]*v[0]+v[1]*v[1];if(i>=48000 && i<96000)first+=energy;if(i>=48000*11)last+=energy;}CHECK(last<first*.001+1e-20);}
    phaser_reset(&p);flanger_reset(f);for(int i=0;i<4800;i++){float a[2]={0},b[2]={0};phaser_process(&p,ps,1,a);flanger_process(f,fs,1,b);CHECK(!a[0] && !a[1] && !b[0] && !b[1]);}
    flanger_free(f);printf("Phaser/Flanger silence, bypass, stereo center, analytic notches, exact delay, inversion, reset, stability and replay passed (%zu / %zu runtime bytes).\n",sizeof(Phaser),flanger_storage_bytes());return 0;
}
