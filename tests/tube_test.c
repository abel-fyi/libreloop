// SPDX-License-Identifier: GPL-3.0-only
#include "tube.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
#define RATE 48000
#define N 48000
static float processed[N],pointwise[N],reference[20000][2];
static double magnitude(const float *samples,double hz) {
    double re=0,im=0;
    for(int i=0;i<N;i++){double a=6.283185307179586*hz*i/RATE;re+=samples[i]*cos(a);im+=samples[i]*sin(a);}
    return 2*hypot(re,im)/N;
}
/* Identical static signal path with point-sampled nonlinearities, to quantify
   the production antiderivative processing rather than comparing different FX. */
static float point_sample(Tube *t,TubeSettings s,float input) {
    float gain=powf(10,s.drive/20),makeup=powf(10,(s.output-s.drive*.5f)/20);
    t->pre[0]+=.90521985f*(input-t->pre[0]);
    double x=t->pre[0]*gain/(1+.12f*t->envelope[0]);
    float first=(tanh(x+.2)-.197375320224904)*1.0405361859192275;
    float level=fabsf(first),rate=level>t->envelope[0]?1.f/960:1.f/7200;
    t->envelope[0]+=(level-t->envelope[0])*rate;
    t->inter[0]+=.79212042f*(first-t->inter[0]);
    float second=(tanh(t->inter[0]*1.15+.08)-.07982976911113136)*1.0064136649895055;
    t->tone_state[0]+=(second-t->tone_state[0])*(1-expf(-6.283185307179586f*s.tone/RATE));
    float blocked=t->tone_state[0]-t->dc_input[0]+.99790780f*t->dc_output[0];
    t->dc_input[0]=t->tone_state[0];t->dc_output[0]=blocked;return blocked*makeup;
}
static double render_sine(TubeSettings settings,float amplitude,int hz,int compare) {
    Tube tube,plain;tube_reset(&tube);tube_reset(&plain);double energy=0;
    for(int i=0;i<N+RATE;i++) {
        float input=(float)(amplitude*sin(6.283185307179586*hz*i/RATE)),frame[2]={input,0};
        tube_process(&tube,settings,1,frame);
        float other=compare?point_sample(&plain,settings,input):0;
        if(i>=RATE){processed[i-RATE]=frame[0];pointwise[i-RATE]=other;energy+=frame[0]*frame[0];}
    }
    return sqrt(energy/N);
}
int main(void) {
    TubeSettings settings=tube_default();CHECK(tube_valid(settings) && settings.tone==TUBE_TONE_OFF);settings.drive=NAN;CHECK(!tube_valid(settings));settings=tube_default();settings.tone=0;CHECK(!tube_valid(settings));
    Tube tube;tube_reset(&tube);settings=tube_default();
    for(int i=0;i<RATE;i++){float frame[2]={0};tube_process(&tube,settings,1,frame);CHECK(frame[0]==0 && frame[1]==0);}
    tube_reset(&tube);
    for(int i=0;i<2000;i++){float x=.5f*sinf(i*.1f),frame[2]={x,-x};tube_process(&tube,settings,0,frame);CHECK(frame[0]==x && frame[1]==-x);}
    tube_reset(&tube);
    for(int i=0;i<RATE;i++){float frame[2]={.8f*sinf(i*.1f),0};tube_process(&tube,settings,1,frame);CHECK(frame[1]==0 && isfinite(frame[0]));}
    for(int block=64;block<=512;block*=8) {
        tube_reset(&tube);settings=tube_default();float previous[2]={0},largest_jump=0;
        for(int start=0;start<20000;start+=block)for(int i=start;i<start+block && i<20000;i++) {
            if(i==4096)settings.drive=36;if(i==8192)settings.tone=1200;if(i==12288)settings.output=12;
            float frame[2]={.1f*sinf(i*.07f),.2f*cosf(i*.04f)};tube_process(&tube,settings,i<16384?1:0,frame);
            CHECK(isfinite(frame[0]) && isfinite(frame[1]));
            for(int side=0;side<2;side++){largest_jump=fmaxf(largest_jump,fabsf(frame[side]-previous[side]));previous[side]=frame[side];}
            if(block==64)memcpy(reference[i],frame,sizeof frame);else CHECK(!memcmp(reference[i],frame,sizeof frame));
        }
        CHECK(largest_jump<.15f);
    }
    settings=(TubeSettings){24,6500,-3};
    double small=render_sine(settings,.02f,1000,0)/.02,medium=render_sine(settings,.2f,1000,0)/.2,large=render_sine(settings,.8f,1000,0)/.8;
    CHECK(small>medium && medium>large);
    render_sine((TubeSettings){12,6500,-3},.3f,1000,0);
    double fundamental=magnitude(processed,1000),second=magnitude(processed,2000),third=magnitude(processed,3000),ninth=magnitude(processed,9000);
    CHECK(second/fundamental>.001 && third/fundamental>.01 && ninth<third*.18);
    double sum=0;for(int i=0;i<N;i++)sum+=processed[i];CHECK(fabs(sum/N)<.00001);
    printf("Tube harmonics: H2 %.2f dBc, H3 %.2f dBc, H9 %.2f dBc.\n",20*log10(second/fundamental),20*log10(third/fundamental),20*log10(ninth/fundamental));
    settings=(TubeSettings){24,18000,0};render_sine(settings,.5f,7000,1);
    const int aliases[]={1000,6000,13000,20000};double aa=0,naive=0;
    for(unsigned i=0;i<sizeof aliases/sizeof *aliases;i++){double a=magnitude(processed,aliases[i]),b=magnitude(pointwise,aliases[i]);aa+=a*a;naive+=b*b;}
    double aa_fundamental=magnitude(processed,7000),plain_fundamental=magnitude(pointwise,7000);
    double reduction=10*log10((naive/(plain_fundamental*plain_fundamental))/(aa/(aa_fundamental*aa_fundamental)+1e-30));
    printf("7 kHz / +24 dB drive: selected folded harmonics reduced %.2f dB relative to fundamental vs point-sampled model.\n",reduction);
    CHECK(reduction>=6);
    settings=tube_default();settings.tone=TUBE_TONE_MAX;render_sine(settings,.1f,10000,0);
    double filtered_top=magnitude(processed,10000);settings.tone=TUBE_TONE_OFF;CHECK(tube_valid(settings));render_sine(settings,.1f,10000,0);
    CHECK(magnitude(processed,10000)>filtered_top*1.03);
    tube_reset(&tube);settings=tube_default();float before=0,max_jump=0;
    for(int i=0;i<48000;i++) {
        if(i==4096)settings.tone=TUBE_TONE_OFF;
        float frame[2]={.1f*sinf(i*.04f),0};tube_process(&tube,settings,1,frame);
        CHECK(isfinite(frame[0]));max_jump=fmaxf(max_jump,fabsf(frame[0]-before));before=frame[0];
    }
    CHECK(max_jump<.05f && tube.tone_coefficient==1);
    tube_reset(&tube);settings=(TubeSettings){36,18000,12};
    for(int i=0;i<RATE;i++){float frame[2]={i<24000?8:-8,.0000001f};tube_process(&tube,settings,1,frame);CHECK(isfinite(frame[0]) && isfinite(frame[1]) && fabsf(frame[0])<12);}
    for(int i=0;i<RATE;i++){float frame[2]={.2f,-.1f};tube_process(&tube,settings,0,frame);if(i==RATE-1)CHECK(fabsf(frame[0]-.2f)<.000001f && fabsf(frame[1]+.1f)<.000001f);}
    tube_reset(&tube);for(int i=0;i<1000;i++){float frame[2]={0};tube_process(&tube,settings,1,frame);CHECK(frame[0]==0 && frame[1]==0);}
    printf("Tube: compression, harmonic balance, DC rejection, alias reduction, bypass, stereo isolation, reset and block determinism passed (%zu bytes).\n",sizeof(Tube));return 0;
}
