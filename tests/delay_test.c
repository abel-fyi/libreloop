// SPDX-License-Identifier: GPL-3.0-only
#include "delay.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
static float reference[40000][2];
int main(void) {
    DelaySettings settings=delay_default();CHECK(delay_valid(settings));
    CHECK(delay_storage_bytes()>48000*4*2*sizeof(float) && delay_storage_bytes()<2*1024*1024);
    settings.time_ms=NAN;CHECK(!delay_valid(settings));settings=delay_default();
    settings.feedback=1.01f;CHECK(!delay_valid(settings));settings=delay_default();
    settings.division=DELAY_DIVISIONS;CHECK(!delay_valid(settings));
    settings=(DelaySettings){1,.5f,20000,0,1,0,0};
    Delay *delay=delay_create();CHECK(delay);
    /* Exact impulse positions/amplitudes at the 48-sample one-millisecond tap. */
    for(int i=0;i<200;i++) {
        float frame[2]={i==0,i==0};delay_process(delay,settings,120,1,frame);
        float expected=i==48?1:i==96?.5f:i==144?.25f:i==192?.125f:0;
        CHECK(frame[0]==expected && frame[1]==expected);
    }
    delay_reset(delay);settings.stereo=1;
    for(int i=0;i<160;i++) {
        float frame[2]={i==0,0};delay_process(delay,settings,120,1,frame);
        CHECK(frame[0]==(i==48?1:i==144?.25f:0));CHECK(frame[1]==(i==96?.5f:0));
    }
    /* Reset must invalidate old ring contents without clearing them on audio. */
    delay_reset(delay);for(int i=0;i<200;i++) {float frame[2]={0};delay_process(delay,settings,120,1,frame);CHECK(frame[0]==0 && frame[1]==0);}
    settings=delay_legacy();settings.division=6;
    CHECK(delay_time_ms(settings,120)==500 && delay_time_ms(settings,60)==1000);
    settings.division=8;CHECK(delay_time_ms(settings,30)==4000);
    settings.division=3;CHECK(fabsf(delay_time_ms(settings,120)-500.f/3)<.0001f);
    settings.sync=0;settings.time_ms=250;CHECK(delay_time_ms(settings,30)==250);
    /* Time automation and tempo changes are sample-deterministic across blocks. */
    for(int block=64;block<=512;block*=8) {
        delay_reset(delay);settings=(DelaySettings){10,.85f,3000,.7f,1,0,4};
        float previous=0,largest=0;
        for(int start=0;start<40000;start+=block)for(int at=start;at<start+block && at<40000;at++) {
            if(at==8192)settings.time_ms=57;
            if(at==16384){settings.sync=1;settings.cutoff=1200;}
            float frame[2]={.05f*sinf(at*.04f),.03f*cosf(at*.03f)};
            delay_process(delay,settings,at<24576?120:180,at<32768?.5f:0,frame);
            CHECK(isfinite(frame[0]) && isfinite(frame[1]) && fabsf(frame[0])<2 && fabsf(frame[1])<2);
            largest=fmaxf(largest,fabsf(frame[0]-previous));previous=frame[0];
            if(block==64)memcpy(reference[at],frame,sizeof frame);else CHECK(!memcmp(reference[at],frame,sizeof frame));
        }
        CHECK(largest<.02f);
    }
    delay_reset(delay);settings=(DelaySettings){4000,0,20000,0,1,0,0};
    for(int i=0;i<=384000;i++) {
        float frame[2]={i==0,0};delay_process(delay,settings,120,1,frame);
        CHECK(frame[0]==(i==192000?1:0) && frame[1]==0);
    }
    delay_reset(delay);settings=(DelaySettings){1,.95f,100,1,2,0,0};
    for(int i=0;i<48000;i++) {float frame[2]={.1f,-.1f};delay_process(delay,settings,120,1,frame);CHECK(isfinite(frame[0]) && isfinite(frame[1]));}
    /* Compact delay: input pan/gain, channel routing, independent dry and offset. */
    for(unsigned mode=DELAY_NORMAL;mode<=DELAY_PINGPONG;mode++) {
        delay_reset(delay);settings=delay_default();settings.input_gain=1;settings.steps=.125f;settings.dry=0;settings.cutoff=20000;settings.feedback=.5f;settings.mode=mode;
        for(int i=0;i<2300;i++) {
            float frame[2]={i==0,0};delay_process(delay,settings,120,1,frame);
            float left=0,right=0;
            if(i==750 || i==1500 || i==2250) {
                float level=i==750?1:i==1500?.5f:.25f;
                if(mode==DELAY_NORMAL || (mode==DELAY_PINGPONG && i==1500))left=level;else right=level;
            }
            CHECK(frame[0]==left && frame[1]==right);
        }
    }
    for(unsigned mode=DELAY_NORMAL;mode<=DELAY_PINGPONG;mode++)for(int sign=-1;sign<=1;sign+=2) {
        delay_reset(delay);settings=delay_default();settings.input_gain=1;settings.steps=.125f;settings.dry=0;settings.cutoff=20000;settings.feedback=0;settings.offset=sign;settings.mode=mode;
        for(int i=0;i<1200;i++) {
            float frame[2]={i==0,i==0};delay_process(delay,settings,120,1,frame);
            CHECK(frame[0]==(i==(sign<0?1125:750)) && frame[1]==(i==(sign>0?1125:750)));
        }
    }
    delay_reset(delay);settings.mode=DELAY_NORMAL;settings.input_gain=0;settings.dry=.4f;
    float dry[2]={.5f,-.5f};delay_process(delay,settings,120,1,dry);CHECK(dry[0]==.2f && dry[1]==-.2f);
    delay_reset(delay);settings.input_gain=1;settings.input_pan=-1;settings.dry=0;settings.offset=0;
    for(int i=0;i<800;i++) {float frame[2]={i==0,i==0};delay_process(delay,settings,120,1,frame);CHECK(frame[0]==(i==750) && frame[1]==0);}
    delay_reset(delay);settings.input_pan=0;settings.feedback=1;
    for(int i=0;i<5000;i++) {float frame[2]={i==0,0};delay_process(delay,settings,120,1,frame);CHECK(frame[0]==(i>0 && i%750==0) && frame[1]==0);}
    for(int block=64;block<=512;block*=8) {
        delay_reset(delay);settings=delay_default();settings.cutoff=1200;settings.steps=.125f;
        for(int start=0;start<20000;start+=block)for(int at=start;at<start+block && at<20000;at++) {
            if(at==4096)settings.steps=2;
            if(at==8192)settings.offset=.5f;
            if(at==12288)settings.mode=DELAY_PINGPONG;
            if(at==16384)settings.input_pan=-.25f;
            float frame[2]={.05f*sinf(at*.04f),.03f*cosf(at*.03f)};
            delay_process(delay,settings,at<16384?120:175,1,frame);
            CHECK(isfinite(frame[0]) && isfinite(frame[1]));
            if(block==64)memcpy(reference[at],frame,sizeof frame);else CHECK(!memcmp(reference[at],frame,sizeof frame));
        }
    }
    delay_free(delay);
    printf("Delay core: exact echoes, feedback/ping-pong, sync, reset, four-second ring wrap and block determinism passed (%zu bytes/instance).\n",delay_storage_bytes());return 0;
}
