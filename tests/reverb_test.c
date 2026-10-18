// SPDX-License-Identifier: GPL-3.0-only
#include "reverb.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#define RATE 48000
#define FRAMES (RATE*8)
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
static float impulse[FRAMES][2],reference[RATE*2][2];
static double energy(int first,int last) {double sum=0;for(int i=first;i<last;i++)sum+=impulse[i][0]*impulse[i][0]+impulse[i][1]*impulse[i][1];return sum;}
static void render_impulse(Reverb *r,ReverbSettings s) {
    reverb_reset(r);for(int i=0;i<FRAMES;i++){float frame[2]={i==0,i==0};reverb_process(r,s,1,frame);memcpy(impulse[i],frame,sizeof frame);}
}
static double brightness(void) {
    double high=0,total=0;for(int i=RATE/3;i<RATE;i++)for(int side=0;side<2;side++){
        double x=impulse[i][side],delta=x-impulse[i-1][side];high+=delta*delta;total+=x*x;
    }return high/total;
}
int main(void) {
    ReverbSettings s=reverb_default();CHECK(reverb_valid(s));s.decay=NAN;CHECK(!reverb_valid(s));s=reverb_default();s.size=2;CHECK(!reverb_valid(s));
    for(unsigned i=0;i<REVERB_FACTORY_COUNT;i++)CHECK(reverb_valid(reverb_factory(i)) && reverb_factory_mix(i)>0 && reverb_factory_name(i)[0]);
    Reverb *r=reverb_create(),*other=reverb_create();CHECK(r && other && reverb_storage_bytes()<1024*1024);
    s=reverb_default();
    for(int i=0;i<RATE;i++) {float frame[2]={0};reverb_process(r,s,1,frame);if(frame[0]!=0 || frame[1]!=0)fprintf(stderr,"Silence leak at %d: %g %g\n",i,frame[0],frame[1]);CHECK(frame[0]==0 && frame[1]==0);}
    reverb_reset(r);for(int i=0;i<1000;i++){float frame[2]={.3f,-.2f};reverb_process(r,s,0,frame);CHECK(frame[0]==.3f && frame[1]==-.2f);}
    s.predelay=0;s.mod=0;s.damping=20000;render_impulse(r,s);
    double cross=0,left=0,right=0,side_energy=0,peak=0;int dense=0;
    for(int i=0;i<FRAMES;i++) {
        CHECK(isfinite(impulse[i][0]) && isfinite(impulse[i][1]));
        for(int side=0;side<2;side++)peak=fmax(peak,fabs(impulse[i][side]));
        if(i>=RATE/10 && i<RATE/2){cross+=impulse[i][0]*impulse[i][1];left+=impulse[i][0]*impulse[i][0];right+=impulse[i][1]*impulse[i][1];double d=impulse[i][0]-impulse[i][1];side_energy+=d*d;if(fabsf(impulse[i][0])+fabsf(impulse[i][1])>1e-6f)dense++;}
    }
    double correlation=cross/sqrt(left*right),density=dense/(RATE*.4),t60=60/(10*log10(energy(RATE*.3,FRAMES)/energy(RATE*1.3,FRAMES)));
    printf("Reverb IR: correlation %.3f, active-sample density %.1f%% at 100-500 ms, low-loss T60 %.3f s (target %.2f s), peak %.3f.\n",correlation,density*100,t60,s.decay,peak);
    CHECK(peak<2 && fabs(correlation)<.85 && density>.85 && side_energy>(left+right)*.2 && t60>s.decay*.7 && t60<s.decay*1.4);
    CHECK(energy(0,RATE/10)>0 && energy(RATE*4,FRAMES)<energy(RATE,RATE*2)*.001);
    double bright=brightness();s.damping=3000;render_impulse(r,s);double dark=brightness();printf("Reverb late-tail normalized difference energy: open %.3f / damped %.3f.\n",bright,dark);CHECK(bright>dark*1.3);
    s=reverb_factory(1);render_impulse(r,s);CHECK(energy(0,RATE/4)>0 && energy(RATE*.31,FRAMES)==0);
    s=reverb_default();s.predelay=200;reverb_reset(r);
    double after=0;for(int i=0;i<RATE;i++){float frame[2]={i==0,0};reverb_process(r,s,1,frame);if(i<9600)CHECK(frame[0]==0 && frame[1]==0);else after+=frame[0]*frame[0]+frame[1]*frame[1];}CHECK(after>0);
    s.width=0;s.predelay=0;reverb_reset(r);
    for(int i=0;i<RATE;i++){float frame[2]={i==0,0},silent[2]={0};reverb_process(r,s,1,frame);reverb_process(other,s,1,silent);CHECK(frame[0]==frame[1] && silent[0]==0 && silent[1]==0);}
    reverb_reset(r);for(int i=0;i<RATE;i++){float frame[2]={0};reverb_process(r,s,1,frame);CHECK(frame[0]==0 && frame[1]==0);}
    for(int block=64;block<=512;block*=8) {
        reverb_reset(r);s=reverb_default();float previous=0,largest=0;
        for(int start=0;start<RATE*2;start+=block)for(int i=start;i<start+block && i<RATE*2;i++) {
            if(i==4096)s.size=.8f;if(i==16384)s.predelay=75;if(i==32768)s.decay=6;if(i==49152)s.damping=3500;if(i==65536)s.width=.3f;if(i==81920)s.mod=.8f;if(i==90000)s.gate=200;
            float frame[2]={.05f*sinf(i*.03f),.03f*cosf(i*.05f)};reverb_process(r,s,i<50000?1:.3f,frame);
            CHECK(isfinite(frame[0]) && isfinite(frame[1]));largest=fmaxf(largest,fabsf(frame[0]-previous));previous=frame[0];
            if(block==64)memcpy(reference[i],frame,sizeof frame);else CHECK(!memcmp(reference[i],frame,sizeof frame));
        }CHECK(largest<.1f);
    }
    s=(ReverbSettings){0,20,20000,0,2,1,0};reverb_reset(r);
    double first_tail=0,last_tail=0;
    for(int i=0;i<RATE*30;i++){float frame[2]={i<RATE?.1f*sinf(i*.1f):0,i<RATE?.1f*cosf(i*.07f):0};reverb_process(r,s,1,frame);CHECK(isfinite(frame[0]) && isfinite(frame[1]) && fabsf(frame[0])<4 && fabsf(frame[1])<4);double power=frame[0]*frame[0]+frame[1]*frame[1];if(i>=RATE && i<RATE*2)first_tail+=power;if(i>=RATE*29)last_tail+=power;}
    CHECK(last_tail<first_tail*.00001);
    reverb_free(r);reverb_free(other);printf("Reverb: diffusion, width, decay, damping, gate, pre-delay, silence/reset, isolation, stability and replay determinism passed (%zu bytes).\n",reverb_storage_bytes());return 0;
}
