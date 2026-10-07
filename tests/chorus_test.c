// SPDX-License-Identifier: GPL-3.0-only
#include "chorus.h"
#include "engine.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
static Project project,loaded;
static double wet_energy(float hz) {
    Chorus c; chorus_reset(&c); double energy=0;
    for(int i=0;i<RATE*2;i++) {
        float x=.2f*sinf(6.28318530718f*hz*i/RATE),out[2]={x,x};
        chorus_process(&c,(ChorusSettings){.513f,0},1,out);
        if(i>=RATE) energy+=out[0]*out[0];
    }
    return energy/RATE;
}
int main(void) {
    Chorus a,b; chorus_reset(&a); chorus_reset(&b);
    ChorusSettings settings=chorus_default(); CHECK(chorus_valid(settings));
    CHECK(!chorus_valid((ChorusSettings){NAN,3}) && !chorus_valid((ChorusSettings){.8f,9}));
    float stereo_difference=0,maximum_jump=0,previous=0;
    for(int i=0;i<RATE;i++) {
        float x=.2f*sinf(i*.07f),stereo[2]={x,x},silent[2]={0,0};
        if(i==RATE/2) settings=(ChorusSettings){5,8};
        chorus_process(&a,settings,.5f,stereo); chorus_process(&b,settings,.5f,silent);
        CHECK(isfinite(stereo[0]) && fabsf(stereo[0])<=.201f && fabsf(stereo[1])<=.201f);
        CHECK(silent[0]==0 && silent[1]==0); /* Instances share no buffers. */
        stereo_difference=fmaxf(stereo_difference,fabsf(stereo[0]-stereo[1]));
        maximum_jump=fmaxf(maximum_jump,fabsf(stereo[0]-previous)); previous=stereo[0];
    }
    CHECK(stereo_difference>.01f && maximum_jump<.08f);
    /* The wet path softens treble but preserves body. Zero depth must not
       introduce stereo movement; a mono fold-down retains usable level. */
    CHECK(wet_energy(100)>.019 && wet_energy(12000)<wet_energy(100)*.25);
    chorus_reset(&b); double mono_energy=0,side_energy=0,dry_energy=0;
    for(int i=0;i<RATE*3;i++) {
        float x=.15f*sinf(i*.057595865f)+.05f*sinf(i*.1727876f),out[2]={x,x};
        chorus_process(&b,chorus_default(),.5f,out);
        if(i>=RATE) {
            double mid=(out[0]+out[1])*.5,side=(out[0]-out[1])*.5;
            mono_energy+=mid*mid; side_energy+=side*side; dry_energy+=x*x;
        }
    }
    CHECK(mono_energy>dry_energy*.15 && side_energy>dry_energy*.01);
    chorus_reset(&b);
    for(int i=0;i<4096;i++) {
        float x=.2f*sinf(i*.07f),out[2]={x,-x};
        chorus_process(&b,(ChorusSettings){.513f,0},.5f,out);
        CHECK(out[0]==-out[1]); /* Existing stereo content is not collapsed. */
    }
    for(int i=0;i<RATE;i++) { float stereo[2]={.1f,-.1f}; chorus_process(&a,settings,0,stereo); if(i==RATE-1) CHECK(fabsf(stereo[0]-.1f)<.00001f && fabsf(stereo[1]+.1f)<.00001f); }
    chorus_reset(&a); CHECK(!a.ready && a.cursor==0 && a.delay[480][0]==0);
    project_new(&project); project.effect_type[0][0]=EFFECT_CHORUS; project.chorus[0][0]=settings; project.effect_mix[0][0]=.5f;
    CHECK(project_save("chorus.hbt",&project) && project_load("chorus.hbt",&loaded));
    CHECK(loaded.effect_type[0][0]==EFFECT_CHORUS && loaded.chorus[0][0].rate==5 && loaded.chorus[0][0].depth==8 && loaded.effect_mix[0][0]==.5f); remove("chorus.hbt");
    ParameterTarget target; CHECK(parameter_from_pointer(&project,&project.chorus[0][0].rate,&target));
    CHECK(target.parameter==PARAM_CHORUS_RATE && target.owner==0 && target.slot==0);
    int index=automation_create(&project,(ParameterTarget){PARAM_EFFECT_MIX,0,0},"Chorus wet",16); CHECK(index>=0);
    project.automations[index].points[0].value=project.automations[index].points[1].value=0;
    project.clips[0][0]=AUTOMATION_SOURCE+index+1;
    project.notes[0][0][0]=(Note){60,127,0,16};
    float *pcm=malloc(RATE*2*sizeof *pcm); CHECK(pcm); for(int i=0;i<RATE*2;i++) pcm[i]=.2f;
    Sample samples[CHANNELS]={{.data=pcm,.frames=RATE*2}};
    EffectRack *rack=effects_create(INSERTS+1); CHECK(rack);
    uint8_t types[EFFECT_SLOTS]={EFFECT_CHORUS,EFFECT_CHORUS},bypass[EFFECT_SLOTS]={0};
    ChorusSettings chain[EFFECT_SLOTS]; float mix[EFFECT_SLOTS];
    for(int i=0;i<EFFECT_SLOTS;i++) { chain[i]=chorus_default(); mix[i]=.5f; }
    chorus_reset(&a); chorus_reset(&b);
    for(int i=0;i<2048;i++) {
        float input=.2f*sinf(i*.07f),actual[2]={input,input},expected[2]={input,input},other[2]={0,0};
        effects_process(rack,0,types,chain,mix,bypass,actual);
        chorus_process(&a,chain[0],mix[0],expected); chorus_process(&b,chain[1],mix[1],expected);
        CHECK(actual[0]==expected[0] && actual[1]==expected[1]);
        effects_process(rack,1,types,chain,mix,bypass,other);
        CHECK(other[0]==0 && other[1]==0);
    }
    /* Removal fades dry instead of abruptly dropping the wet signal. */
    types[0]=types[1]=EFFECT_EMPTY;
    for(int i=0;i<RATE;i++) { float actual[2]={.1f,-.1f}; effects_process(rack,0,types,chain,mix,bypass,actual); if(i==RATE-1) CHECK(actual[0]==.1f && actual[1]==-.1f); }
    effects_reset(rack);
    /* A configured insert advances its delayed tail even without routed voices. */
    project.effect_type[1][0]=EFFECT_CHORUS; types[0]=EFFECT_CHORUS;
    for(int i=0;i<4096;i++) { float input[2]={.2f,.2f}; effects_process(rack,1,types,chain,mix,bypass,input); }
    Player tail; player_reset(&tail); tail.effects=rack; float tail_out[128]={0};
    render_live(&tail,&project,samples,tail_out,64);
    CHECK(tail_out[0]>.05f);
    project.insert_mute[2]=2; memset(tail_out,0,sizeof tail_out);
    render_live(&tail,&project,samples,tail_out,64); CHECK(tail_out[0]==0);
    project.insert_mute[2]=0; project.effect_type[1][0]=EFFECT_EMPTY; effects_reset(rack);
    Player player; player_reset(&player); player.song=1; player.effects=rack;
    project.clips[1][0]=1; float out[1024]; render(&player,&project,samples,out,512);
    for(int i=0;i<1024;i++) CHECK(fabsf(out[i]-.2f)<.00001f); /* Automation overrides the manual wet knob. */
    project.automation_count=0; project.clips[0][0]=0;
    effects_reset(rack); player_reset(&player); player.effects=rack; player.song=1;
    render(&player,&project,samples,out,512); CHECK(out[100]<.2f);
    CHECK(export_wav("chorus.wav",&project,samples));
    FILE *f=fopen("chorus.wav","rb"); CHECK(f && !fseek(f,44,SEEK_SET));
    for(int i=0;i<1024;i++) { int low=fgetc(f),high=fgetc(f); CHECK(low>=0 && high>=0); CHECK(abs((int16_t)(low|(high<<8))-(int16_t)(out[i]*32767))<=1); }
    fclose(f); remove("chorus.wav"); effects_free(rack); free(pcm);
    project.chorus[0][0].depth=NAN; CHECK(!project_save("chorus-invalid.hbt",&project));
    puts("Chorus stereo, smoothing, instance isolation, bypass, automation, project state and export passed."); return 0;
}
