// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include <stdio.h>
#include <stdlib.h>
static void le(FILE *f,unsigned x,int n) { for(int i=0;i<n;i++) fputc((x>>(8*i))&255,f); }
int main(int argc,char **argv) {
    if(argc!=2) return 1;
    const char *names[]={"drums/Kick","drums/Snare","drums/Hat","melodic/Tone"}; Sample s[CHANNELS]; samples_default(s);
    for(int c=0;c<4;c++) {
        char path[4096]; snprintf(path,sizeof path,"%s/%s.wav",argv[1],names[c]);
        FILE *f=fopen(path,"wb"); if(!f || !s[c].data) return 1;
        unsigned bytes=s[c].frames*2;
        fwrite("RIFF",1,4,f); le(f,36+bytes,4); fwrite("WAVEfmt ",1,8,f); le(f,16,4); le(f,1,2); le(f,1,2); le(f,RATE,4); le(f,RATE*2,4); le(f,2,2); le(f,16,2); fwrite("data",1,4,f); le(f,bytes,4);
        for(unsigned i=0;i<s[c].frames;i++) le(f,(unsigned short)(short)(s[c].data[i]*32767),2);
        int ok=!ferror(f); if(fclose(f)) ok=0; free(s[c].data); if(!ok) return 1;
    }
    return 0;
}
