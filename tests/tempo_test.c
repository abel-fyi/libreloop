// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include "arrangement.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
static Project project;
static float frequency(const float *pcm,unsigned frames) {
    unsigned crosses=0,start=frames/4,end=frames*3/4;
    for(unsigned i=start+1;i<end;i++) if(pcm[i*2-2]<=0 && pcm[i*2]>0) crosses++;
    return crosses*(float)RATE/(end-start);
}
int main(void) {
    float *pcm=malloc(RATE*4*2*sizeof *pcm),*out=malloc(RATE*2*2*sizeof *out); CHECK(pcm && out);
    for(unsigned i=0;i<RATE*4;i++) { pcm[i*2]=.3f*sinf(i*440*2*3.14159265359/RATE); pcm[i*2+1]=-.7f*pcm[i*2]; }
    Sample source={pcm,RATE*4,2},samples[CHANNELS]={source};
    project_new(&project); project.channel_audio[0]=1; project.audio_seconds[0]=4;
    project.sampler[0].fit_bpm=120;
    CHECK(arrangement_place(&project,0,0,PATTERNS,32)>=0);
    project.clips[1][0]=PATTERNS+1; project.clip_steps[1][0]=.75f; project.clip_offsets[1][0]=.5f;
    float full=clip_length(&project,0,0),cropped=clip_length(&project,1,0);
    double seek=audio_clip_position(&project,1,0,2);
    for(int i=0;i<1000;i++) {
        project.bpm=30+fmodf(i*1.137f,270);
        CHECK(clip_length(&project,0,0)==full && clip_length(&project,1,0)==cropped);
        CHECK(audio_clip_position(&project,1,0,2)==seek && project.clip_offsets[1][0]==.5f);
    }
    project.clips[1][0]=0; project.bpm=240;
    Player player; player_reset(&player); player.song=1;
    render(&player,&project,samples,out,RATE);
    CHECK(fabsf(frequency(out,RATE)-880)<4);
    project.sampler[0].stretch=1; player_reset(&player); player.song=1;
    render(&player,&project,samples,out,RATE);
    CHECK(fabsf(frequency(out,RATE)-440)<6);
    /* Continuous source phase and bounded jumps while changing BPM each buffer. */
    for(int mode=0;mode<2;mode++) {
        project.sampler[0].stretch=mode; project.bpm=120;
        player_reset(&player); player.song=1; render(&player,&project,samples,out,512);
        float previous=out[1022],maximum=0; double cursor=player.voices[0].sampler.position;
        for(int block=0;block<180;block++) {
            double song_step=player.frame*project.bpm/(RATE*15.0);
            project.bpm=block<60?240: block<120?60:180;
            render(&player,&project,samples,out,128);
            CHECK(player.voices[0].sampler.position>cursor && !player.channel_trigger[0]);
            CHECK(fabs(player.frame*project.bpm/(RATE*15.0)-song_step-128*project.bpm/(RATE*15.0))<.001);
            cursor=player.voices[0].sampler.position;
            for(int i=0;i<128;i++) {
                CHECK(isfinite(out[i*2]) && fabsf(out[i*2])<=.301f);
                CHECK(fabsf(out[i*2+1]+.7f*out[i*2])<.0001f);
                maximum=fmaxf(maximum,fabsf(out[i*2]-previous)); previous=out[i*2];
            }
        }
        CHECK(maximum<.08f); printf("%s maximum sample jump: %.6f\n",mode?"Stretch":"Resample",maximum);
    }
    /* A tempo edit slews rather than stepping directly to its target. */
    SamplerVoice voice; sampler_voice_reset(&voice,0,1); float stereo[2];
    sampler_voice_sample(&voice,source,1,1,0,stereo); double position=voice.position;
    sampler_voice_sample(&voice,source,1,2,0,stereo);
    CHECK(voice.position-position>1 && voice.position-position<1.01);
    /* Reference seconds and clip geometry survive project persistence. */
    static Project loaded;
    project.clips[1][0]=PATTERNS+1;
    CHECK(project_save("tempo.llp",&project) && project_load("tempo.llp",&loaded)); remove("tempo.llp");
    CHECK(clip_length(&loaded,0,0)==full && clip_length(&loaded,1,0)==cropped);
    /* Edits at a new BPM still use reference seconds when cutting/resizing. */
    Arrangement edit={.tool=CUT,.snap=1,.source_pattern=-1};
    arrangement_press(&edit,&loaded,.5f,0,0,0,0,0); arrangement_release(&edit);
    CHECK(loaded.clip_steps[0][0]==1 && loaded.clip_steps[0][1]==3 && loaded.clip_offsets[0][1]==1);
    edit.tool=PENCIL; arrangement_press(&edit,&loaded,.49f,0,0,1,0,0);
    arrangement_drag(&edit,&loaded,.25f,0); arrangement_release(&edit);
    CHECK(loaded.clip_steps[0][0]==.5f && clip_length(&loaded,0,0)==4);
    free(pcm); free(out); puts("Fixed geometry, reference offsets, pitch, stereo, tempo slew and musical cursor passed."); return 0;
}
