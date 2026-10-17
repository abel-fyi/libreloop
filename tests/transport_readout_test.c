// SPDX-License-Identifier: GPL-3.0-only
#include "transport_readout.h"
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void) {
    char out[40];
    transport_seconds(out,sizeof out,0,44100);CHECK(!strcmp(out,"0.000"));
    transport_seconds(out,sizeof out,44,44100);CHECK(!strcmp(out,"0.000"));
    transport_seconds(out,sizeof out,45,44100);CHECK(!strcmp(out,"0.001"));
    transport_seconds(out,sizeof out,44099,44100);CHECK(!strcmp(out,"0.999"));
    transport_seconds(out,sizeof out,44100,44100);CHECK(!strcmp(out,"1.000"));
    transport_seconds(out,sizeof out,48000*62ULL+5928,48000);CHECK(!strcmp(out,"62.123"));
    transport_seconds(out,sizeof out,UINT64_MAX,48000);CHECK(!strcmp(out,"384307168202282.325"));
    transport_seconds(out,2,44100,44100);CHECK(out[1]==0);
    transport_clock(out,sizeof out,0,44100);CHECK(!strcmp(out,"00:00:00"));
    transport_clock(out,sizeof out,440,44100);CHECK(!strcmp(out,"00:00:00"));
    transport_clock(out,sizeof out,441,44100);CHECK(!strcmp(out,"00:00:01"));
    transport_clock(out,sizeof out,44099,44100);CHECK(!strcmp(out,"00:00:99"));
    transport_clock(out,sizeof out,44100*60ULL-1,44100);CHECK(!strcmp(out,"00:59:99"));
    transport_clock(out,sizeof out,44100*60ULL,44100);CHECK(!strcmp(out,"01:00:00"));
    transport_clock(out,sizeof out,48000*62ULL+5928,48000);CHECK(!strcmp(out,"01:02:12"));
    transport_clock(out,sizeof out,48000*6000ULL,48000);CHECK(!strcmp(out,"100:00:00"));
    transport_clock(out,sizeof out,UINT64_MAX,48000);CHECK(!strcmp(out,"6405119470038:02:32"));
    transport_clock(out,sizeof out,0,0);CHECK(!strcmp(out,"00:00:00"));
    puts("Transport seconds/milliseconds respect sample boundaries at 44.1/48 kHz without overflow.");
    return 0;
}
