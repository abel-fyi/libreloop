// SPDX-License-Identifier: GPL-3.0-only
#include "audio.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(void) {
    Project p; Sample samples[CHANNELS],decoded={0}; project_default(&p); for(int b=0;b<32;b++) p.clips[0][b]=1; samples_default(samples);
    int ok=export_wav("decoder-fixture.wav",&p,samples) && sample_load("decoder-fixture.wav",&decoded);
    ok=ok && decoded.frames==32*2*RATE && decoded.channels==2;
    Sample processed={0};
    ok=ok && sample_process(decoded,(Sampler){.time=1,.length=1},&processed) && processed.frames==decoded.frames;
    free(processed.data);
    double power=0;
    for(unsigned i=0;i<(size_t)decoded.frames*sample_channels(decoded);i++) { if(!isfinite(decoded.data[i])) ok=0; power+=decoded.data[i]*decoded.data[i]; }
    ok=ok && power>1;
    Sample unused={0}; ok=ok && !sample_load("does-not-exist.wav",&unused);
    free(decoded.data); for(int c=0;c<CHANNELS;c++) free(samples[c].data);
    remove("decoder-fixture.wav");
    /* Independent PCM fixture: anti-phase stereo must not disappear in decoding. */
    const unsigned char wav[]={
        'R','I','F','F',44,0,0,0,'W','A','V','E','f','m','t',' ',16,0,0,0,
        1,0,2,0,128,187,0,0,0,238,2,0,4,0,16,0,'d','a','t','a',8,0,0,0,
        0,32,0,224,0,64,0,192
    };
    FILE *fixture=fopen("stereo-fixture.wav","wb");
    if(!fixture) return 1;
    ok=ok && fwrite(wav,1,sizeof wav,fixture)==sizeof wav; fclose(fixture);
    Sample stereo={0},cooked={0};
    ok=ok && sample_load("stereo-fixture.wav",&stereo) && stereo.channels==2 && stereo.frames==2;
    if(stereo.data) ok=ok && fabsf(stereo.data[0]-.25f)<1e-6 && fabsf(stereo.data[1]+.25f)<1e-6
        && fabsf(stereo.data[2]-.5f)<1e-6 && fabsf(stereo.data[3]+.5f)<1e-6;
    ok=ok && sample_process(stereo,(Sampler){.time=1,.length=1},&cooked) && cooked.channels==2;
    if(cooked.data) ok=ok && !memcmp(cooked.data,stereo.data,sizeof(float)*4);
    project_default(&p); memset(samples,0,sizeof samples); memset(p.notes,0,sizeof p.notes);
    p.channel_audio[0]=1; p.audio_seconds[0]=1; p.volume[0]=p.master=1; p.route[0]=0;
    p.clips[0][0]=PATTERNS+1; samples[0]=cooked;
    Sample roundtrip={0};
    ok=ok && export_wav("stereo-export.wav",&p,samples) && sample_load("stereo-export.wav",&roundtrip)
        && roundtrip.channels==2 && roundtrip.frames==2*RATE;
    if(roundtrip.data) ok=ok && roundtrip.data[0]>.2f && roundtrip.data[1]<-.2f;
    free(stereo.data); free(cooked.data); free(roundtrip.data);
    remove("stereo-fixture.wav"); remove("stereo-export.wav");
    const unsigned char mono_wav[]={
        'R','I','F','F',40,0,0,0,'W','A','V','E','f','m','t',' ',16,0,0,0,
        1,0,1,0,128,187,0,0,0,119,1,0,2,0,16,0,'d','a','t','a',4,0,0,0,
        0,32,0,224
    };
    fixture=fopen("mono-fixture.wav","wb"); if(!fixture) return 1;
    ok=ok && fwrite(mono_wav,1,sizeof mono_wav,fixture)==sizeof mono_wav; fclose(fixture);
    Sample mono={0};
    ok=ok && sample_load("mono-fixture.wav",&mono) && mono.channels==1 && mono.frames==2;
    if(mono.data) ok=ok && fabsf(mono.data[0]-.25f)<1e-6 && fabsf(mono.data[1]+.25f)<1e-6;
    free(mono.data); remove("mono-fixture.wav");
    puts(ok?"Mono/stereo decoding, channel separation, WAV export and missing-file rejection passed.":"Decoder test failed."); return !ok;
}
