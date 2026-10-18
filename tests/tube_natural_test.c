// SPDX-License-Identifier: GPL-3.0-only
#include "tube.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#define RATE 48000
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static float output[RATE],plain[RATE],replay[24000][2];
static double magnitude(const float *x,int hz){double re=0,im=0;for(int i=0;i<RATE;i++){double a=6.283185307179586*hz*i/RATE;re+=x[i]*cos(a);im+=x[i]*sin(a);}return 2*hypot(re,im)/RATE;}
static void sine(TubeSettings s,float amplitude,int hz){Tube t;tube_reset(&t);float envelope=0;for(int i=0;i<RATE*2;i++){float x=amplitude*sin(6.283185307179586*hz*i/RATE),frame[2]={x,0};tube_process(&t,s,1,frame);double driven=x*pow(10,s.drive/20)/(1+.12f*envelope);float first=(tanh(driven+.2)-.197375320224904)*1.0405361859192275;float level=fabsf(first),rate=level>envelope?1.f/960:1.f/7200;envelope+=(level-envelope)*rate;float raw=(tanh(first*1.15+.08)-.07982976911113136)*1.0064136649895055;if(i>=RATE){output[i-RATE]=frame[0];plain[i-RATE]=raw;}}}
int main(void){
    TubeSettings s=tube_default();CHECK(!s.natural && tube_valid(s));s.natural=1;CHECK(tube_valid(s));s.natural=.5f;CHECK(!tube_valid(s));s.natural=NAN;CHECK(!tube_valid(s));
    s=(TubeSettings){0,TUBE_TONE_OFF,0,1};sine(s,.001f,1000);double one=magnitude(output,1000);sine(s,.001f,10000);double ten=magnitude(output,10000);sine(s,.001f,19000);double nineteen=magnitude(output,19000);
    s.natural=0;sine(s,.001f,1000);double warm_one=magnitude(output,1000);sine(s,.001f,19000);double warm_top=magnitude(output,19000);
    double natural_db=20*log10(nineteen/one),warm_db=20*log10(warm_top/warm_one);printf("Tube small-signal response relative to 1 kHz: Natural 10 kHz %+.3f dB, 19 kHz %+.3f dB; Warm 19 kHz %+.3f dB.\n",20*log10(ten/one),natural_db,warm_db);
    CHECK(fabs(20*log10(ten/one))<.2 && fabs(natural_db)<.5 && natural_db>warm_db+12);
    s=(TubeSettings){0,1200,0,1};sine(s,.001f,19000);CHECK(magnitude(output,19000)<nineteen*.2);
    s=(TubeSettings){24,TUBE_TONE_OFF,0,1};sine(s,.5f,19000);const int folds[]={1000,9000,10000,20000};double aa=0,raw=0;for(unsigned i=0;i<4;i++){double a=magnitude(output,folds[i]),b=magnitude(plain,folds[i]);aa+=a*a;raw+=b*b;}double fundamental=magnitude(output,19000),raw_fundamental=magnitude(plain,19000);double improvement=10*log10((raw/(raw_fundamental*raw_fundamental))/(aa/(fundamental*fundamental)+1e-30));printf("Natural +24 dB / 19 kHz selected folded-harmonic reduction: %.2f dB versus point-sampled model.\n",improvement);CHECK(improvement>12);
    Tube t;tube_reset(&t);s=tube_default();s.natural=1;
    for(int i=0;i<RATE;i++){float f[2]={0};tube_process(&t,s,1,f);CHECK(f[0]==0 && f[1]==0);}
    tube_reset(&t);for(int i=0;i<4800;i++){float x=.2f*sinf(i*.1f),f[2]={x,-x};tube_process(&t,s,0,f);CHECK(f[0]==x && f[1]==-x);}
    tube_reset(&t);for(int i=0;i<RATE;i++){float f[2]={.3f*sinf(i*.1f),0};tube_process(&t,s,1,f);CHECK(isfinite(f[0]) && f[1]==0);}
    for(int block=64;block<=512;block*=8){tube_reset(&t);s=tube_default();float last=0,jump=0;for(int start=0;start<24000;start+=block)for(int i=start;i<start+block && i<24000;i++){if(i==4096)s.natural=1;if(i==8192)s.natural=0;if(i==12288)s.natural=1;if(i==16384)s.tone=1200;if(i==20000)s.tone=TUBE_TONE_OFF;float f[2]={.1f*sinf(i*.03f),.1f*sinf(i*.03f)};tube_process(&t,s,1,f);CHECK(isfinite(f[0]) && f[0]==f[1]);jump=fmaxf(jump,fabsf(f[0]-last));last=f[0];if(block==64)memcpy(replay[i],f,sizeof f);else CHECK(!memcmp(replay[i],f,sizeof f));}CHECK(jump<.1f);}
    s=(TubeSettings){36,TUBE_TONE_OFF,12,1};tube_reset(&t);for(int i=0;i<RATE;i++){float f[2]={i<24000?8:-8,.0000001f};tube_process(&t,s,1,f);CHECK(isfinite(f[0]) && isfinite(f[1]) && fabsf(f[0])<12);}
    tube_reset(&t);for(int i=0;i<1000;i++){float f[2]={0};tube_process(&t,s,1,f);CHECK(f[0]==0 && f[1]==0);}
    printf("Tube Natural: measured bandwidth, alias reduction, default Warm, silence, dry bypass, isolation, switching and replay passed (%zu bytes).\n",sizeof(Tube));return 0;
}
