// SPDX-License-Identifier: GPL-3.0-only
#include "audio.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
int main(void) {
    Project p; Sample samples[CHANNELS],decoded={0}; project_default(&p); for(int b=0;b<32;b++) p.clips[0][b]=1; samples_default(samples);
    int ok=export_wav("decoder-fixture.wav",&p,samples) && sample_load("decoder-fixture.wav",&decoded);
    ok=ok && decoded.frames==32*2*RATE;
    Sample processed={0};
    ok=ok && sample_process(decoded,(Sampler){.time=1,.length=1},&processed) && processed.frames==decoded.frames;
    free(processed.data);
    double power=0;
    for(unsigned i=0;i<decoded.frames;i++) { if(!isfinite(decoded.data[i])) ok=0; power+=decoded.data[i]*decoded.data[i]; }
    ok=ok && power>1;
    Sample unused={0}; ok=ok && !sample_load("does-not-exist.wav",&unused);
    free(decoded.data); for(int c=0;c<CHANNELS;c++) free(samples[c].data);
    remove("decoder-fixture.wav");
    puts(ok?"Sample decoding, frame count and missing-file rejection passed.":"Decoder test failed."); return !ok;
}
