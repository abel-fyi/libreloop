// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include "waveform.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
int main(void) {
    float data[4096];
    for(unsigned i=0;i<2048;i++) { data[i*2]=.5f*sinf(i*.1f); data[i*2+1]=-.5f*data[i*2]; }
    Sample source={data,2048,2},out={0}; Sampler settings={.time=1,.length=1};
    CHECK(sample_process(source,settings,&out) && out.channels==2 && out.frames==2048);
    CHECK(!memcmp(data,out.data,sizeof data)); free(out.data);
    settings.start=.25f; settings.length=.5f; settings.flags=SAMPLE_REVERSE|SAMPLE_POLARITY|SAMPLE_NORMALIZE;
    CHECK(sample_process(source,settings,&out) && out.channels==2 && out.frames==768);
    CHECK(out.data[0]*data[1279*2]<=0);
    for(unsigned i=0;i<out.frames;i++) CHECK(fabsf(out.data[i*2+1]+.5f*out.data[i*2])<1e-6);
    free(out.data);
    for(int mode=0;mode<3;mode++) {
        settings=(Sampler){.time=mode==2?.5f:2,.length=1,.stretch=mode!=0,.pitch=mode==2?7:0};
        CHECK(sample_process(source,settings,&out) && out.channels==2);
        double power=0;
        for(unsigned i=0;i<out.frames;i++) { CHECK(isfinite(out.data[i*2])); CHECK(fabsf(out.data[i*2+1]+.5f*out.data[i*2])<1e-6); power+=out.data[i*2]*out.data[i*2]; }
        CHECK(power>1); free(out.data);
    }
    float tail[]={0,0,0,.5f,0,0}; Sample trimmed=sample_trim((Sample){tail,3,2},1);
    CHECK(trimmed.frames==1 && trimmed.data==tail+2 && trimmed.data[1]==.5f);
    float boundaries[]={0,.2f,0,0,.3f,0};
    trimmed=sample_trim((Sample){boundaries,3,2},1);
    CHECK(trimmed.frames==3 && trimmed.data==boundaries); /* Either channel protects both edges. */
    Waveform wave={0}; CHECK(waveform_build(&wave,(Sample){tail,3,2}));
    CHECK(waveform_range(&wave,(Sample){tail,3,2},1,2).high==.5f); free(wave.tree);
    Project project; project_default(&project); memset(project.notes,0,sizeof project.notes);
    project.volume[0]=project.master=1; project.route[0]=0; project.channel_audio[0]=1;
    project.audio_seconds[0]=1; project.clips[0][0]=PATTERNS+1;
    Sample samples[CHANNELS]={{tail,3,2}}; Player player; float pcm[6];
    player_reset(&player); player.song=1; render(&player,&project,samples,pcm,3);
    CHECK(pcm[2]==0 && pcm[3]>.4f); /* right-only remains right-only */
    project.master_width=0; player_reset(&player); player.song=1; render(&player,&project,samples,pcm,3);
    CHECK(pcm[2]>0 && pcm[2]==pcm[3]); /* explicit mono still works */
    project.master_width=1; project.route[0]=1;
    player_reset(&player); player.song=1; render(&player,&project,samples,pcm,3);
    CHECK(pcm[2]==0 && pcm[3]>0); /* stereo survives insert routing */
    puts("Stereo processing, linked stretch, trimming, waveform and mixer routing passed."); return 0;
}
