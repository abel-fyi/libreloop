// SPDX-License-Identifier: GPL-3.0-only
#include "project_check.h"
#include "engine.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed: %s:%d: %s\n",__FILE__,__LINE__,#x); return 1; } } while(0)
int main(void) {
    /* Keep large project and player fixtures off the stack as device state grows. */
    CHECK(grid_interval(100)<grid_interval(2));
    static Project fine,roundtrip; project_default(&fine); memset(fine.notes,0,sizeof fine.notes);
    fine.volume[0]=fine.master=1; fine.route[0]=0;
    CHECK(note_add(&fine,0,0,.5f,60,.5f)); CHECK(note_add(&fine,0,0,4.f/3,64,2.f/3));
    fine.clips[0][0]=1; fine.clip_starts[0][0]=.0625f; fine.clip_steps[0][0]=2.5f;
    /* Different padding must not make a save/load round trip fail. */
    unsigned char *note_bytes=(unsigned char *)&fine.notes[0][0][0];
    for(size_t i=offsetof(Note,velocity)+sizeof fine.notes[0][0][0].velocity;i<offsetof(Note,start);i++) note_bytes[i]=0xa5;
    unsigned char *sampler_bytes=(unsigned char *)&fine.sampler[0];
    for(size_t i=offsetof(Sampler,stretch)+sizeof fine.sampler[0].stretch;i<offsetof(Sampler,fit_bpm);i++) sampler_bytes[i]=0x5a;
    CHECK(project_save("fractional.llp",&fine) && project_load("fractional.llp",&roundtrip));
    CHECK(project_equal(&fine,&roundtrip)); remove("fractional.llp");
    float sustained[RATE]; for(int i=0;i<RATE;i++) sustained[i]=.25f;
    Sample exact[CHANNELS]={{sustained,RATE}}; float timed[20000],split[20000]; static Player clock; player_reset(&clock);
    render(&clock,&fine,exact,timed,10000);
    for(int i=0;i<3000*2;i++) CHECK(timed[i]==0);
    CHECK(timed[3000*2]>0 && timed[6100*2]==0 && timed[8000*2]>0);
    player_reset(&clock); render(&clock,&fine,exact,split,3211); render(&clock,&fine,exact,split+6422,6789);
    CHECK(memcmp(timed,split,sizeof timed)==0);
    player_reset(&clock); clock.song=1; render(&clock,&fine,exact,timed,10000);
    for(int i=0;i<9000*2;i++) CHECK(timed[i]==0);
    CHECK(timed[9000*2]>0);
    fine.pattern_steps[0]=6000*STEPS;
    CHECK(note_add(&fine,0,0,5000*STEPS,60,1));
    player_reset(&clock); clock.frame=(uint64_t)5000*STEPS*6000;
    render(&clock,&fine,exact,timed,1); CHECK(timed[0]>0);
    fine.clip_starts[0][0]=10000; fine.clip_steps[0][0]=6000*STEPS;
    player_reset(&clock); clock.song=1; clock.frame=(uint64_t)15000*STEPS*6000;
    render(&clock,&fine,exact,timed,1); CHECK(timed[0]>0);
    player_reset(&clock); player_seek(&clock,&fine,5000*STEPS);
    CHECK(clock.frame==(uint64_t)5000*STEPS*6000 && clock.start_step==5000*STEPS);
    render(&clock,&fine,exact,timed,1); CHECK(timed[0]>0);
    clock.frame=(uint64_t)fine.pattern_steps[0]*6000;
    render(&clock,&fine,exact,timed,1); CHECK(clock.frame==(uint64_t)5000*STEPS*6000+1 && timed[0]>0);
    player_seek(&clock,&fine,20000*STEPS); render(&clock,&fine,exact,timed,1);
    CHECK(clock.frame==1); /* Starting beyond the end plays from zero immediately. */
    player_reset(&clock); clock.song=1; player_seek(&clock,&fine,song_steps(&fine));
    render(&clock,&fine,exact,timed,1); CHECK(clock.frame==1);
    /* A Song marker selects the initial seek, not the automatic loop origin. */
    player_reset(&clock); clock.song=1; player_seek(&clock,&fine,15000*STEPS);
    render(&clock,&fine,exact,timed,1); CHECK(timed[0]>0 && clock.frame==(uint64_t)15000*STEPS*6000+1);
    clock.frame=(uint64_t)llround(song_steps(&fine)*6000.0);
    render(&clock,&fine,exact,timed,1); CHECK(clock.frame==1 && clock.start_step==15000*STEPS);
    player_seek(&clock,&fine,clock.start_step); CHECK(clock.frame==(uint64_t)15000*STEPS*6000);
    static Project deleted,reloaded; project_default(&deleted);
    CHECK(deleted.pattern_count==1 && deleted.channel_count==4 && deleted.insert_count==100);
    for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) CHECK(deleted.clips[l][b]==0);
    deleted.route[0]=deleted.route[1]=2; deleted.insert_volume[1]=.35f; deleted.insert_mute[1]=3;
    CHECK(!insert_reset(&deleted,0) && !insert_reset(&deleted,101));
    CHECK(insert_reset(&deleted,2)); CHECK(deleted.insert_count==100 && deleted.route[0]==2);
    CHECK(deleted.insert_volume[1]==1 && deleted.insert_mute[1]==0);
    deleted.channel_colors[4]=pattern_palette[2]; deleted.channel_count=5; strcpy(deleted.channel_names[4],"Extra"); deleted.route[4]=100;
    CHECK(note_add(&deleted,0,4,0,60,2)); CHECK(channel_delete(&deleted,1));
    CHECK(deleted.channel_colors[3]==pattern_palette[2] && deleted.channel_colors[4]==0);
    CHECK(deleted.channel_count==4 && !strcmp(deleted.channel_names[3],"Extra") && deleted.route[3]==100);
    CHECK(note_at(&deleted,0,3,0,60)); CHECK(!channel_delete(&deleted,4));
    CHECK(project_save("test-project.llp",&deleted) && project_load("test-project.llp",&reloaded)); CHECK(project_equal(&deleted,&reloaded));
    project_default(&reloaded); reloaded.pattern_count=3;
    CHECK(note_add(&reloaded,1,0,2,62,1) && note_add(&reloaded,2,0,3,67,2));
    snprintf(reloaded.pattern_names[2],PATTERN_NAME,"Melody"); reloaded.pattern_steps[2]=32;
    reloaded.pattern_colors[2]=pattern_palette[4];
    reloaded.clips[0][0]=1; reloaded.clips[1][0]=2; reloaded.clips[2][0]=3;
    CHECK(!pattern_delete(&reloaded,-1) && !pattern_delete(&reloaded,3));
    CHECK(pattern_delete(&reloaded,1) && reloaded.pattern_count==2);
    CHECK(reloaded.clips[0][0]==1 && reloaded.clips[1][0]==0 && reloaded.clips[2][0]==2);
    CHECK(!strcmp(reloaded.pattern_names[1],"Melody") && reloaded.pattern_steps[1]==32 && reloaded.pattern_colors[1]==pattern_palette[4]);
    CHECK(note_at(&reloaded,1,0,3,67) && !note_at(&reloaded,1,0,2,62));
    CHECK(pattern_delete(&reloaded,0) && reloaded.pattern_count==1 && note_at(&reloaded,0,0,3,67));
    CHECK(pattern_delete(&reloaded,0) && reloaded.pattern_count==1 && reloaded.pattern_steps[0]==STEPS);
    CHECK(!strcmp(reloaded.pattern_names[0],"Pattern 1") && !reloaded.clips[2][0]);
    for(int c=0;c<CHANNELS;c++) for(int n=0;n<NOTES;n++) CHECK(!reloaded.notes[0][c][n].velocity);
    static Project p,q; project_default(&p);
    strcpy(p.paths[0],"/tmp/sample with spaces.wav"); strcpy(p.pattern_names[0],"Drums + Bass");
    memset(p.pattern_names[7],'X',PATTERN_NAME-1); p.pattern_names[7][PATTERN_NAME-1]=0;
    p.insert_count=6; p.route[0]=5; p.route[1]=5; p.insert_volume[4]=.35f; p.insert_pan[4]=-.25f; p.insert_mute[5]=1;
    p.pattern_count=3;
    CHECK(note_add(&p,2,3,4,60,3)); CHECK(note_add(&p,2,3,4,64,4)); CHECK(note_add(&p,2,3,4,67,3));
    CHECK(project_save("test-project.llp",&p)); CHECK(project_load("test-project.llp",&q)); CHECK(project_equal(&p,&q));
    Sample s[CHANNELS]; samples_default(s); for(int c=0;c<4;c++) CHECK(s[c].data);
    static Project routing; project_default(&routing); memset(routing.notes,0,sizeof routing.notes);
    routing.notes[0][0][0]=(Note){60,127,0,0}; routing.notes[0][1][0]=(Note){60,127,0,0};
    routing.volume[0]=routing.volume[1]=.5f; routing.master=1;
    routing.route[0]=routing.route[1]=1; routing.insert_volume[0]=.5f; routing.insert_pan[0]=.5f;
    float one=.2f,two=.4f,result[2]; Sample fixture[CHANNELS]={{&one,1},{&two,1},{0},{0}};
    static Player rp; player_reset(&rp); render(&rp,&routing,fixture,result,1);
    CHECK(fabsf(result[0]-.075f)<.00001f && fabsf(result[1]-.15f)<.00001f);
    routing.route[0]=0; player_reset(&rp); render(&rp,&routing,fixture,result,1);
    CHECK(fabsf(result[0]-.15f)<.00001f && fabsf(result[1]-.2f)<.00001f);
    routing.insert_mute[0]=1; player_reset(&rp); render(&rp,&routing,fixture,result,1);
    CHECK(fabsf(result[0]-.1f)<.00001f && fabsf(result[1]-.1f)<.00001f);
    /* Master pitch changes playback rate, including active voices, without
       changing the pattern clock or stored note pitches. */
    static Project pitched; project_default(&pitched); memset(pitched.notes,0,sizeof pitched.notes);
    pitched.notes[0][0][0]=(Note){60,127,0,4}; pitched.master_pitch=12;
    float ramp[]={.1f,.2f,.3f,.4f,.5f,.6f}; Sample pitched_sample[CHANNELS]={{ramp,6},{0},{0},{0}};
    float pitched_out[4]; player_reset(&rp); render(&rp,&pitched,pitched_sample,pitched_out,2);
    CHECK(rp.voices[0].sampler.position==4 && rp.frame==2 && rp.voices[0].remaining==24000-2);
    CHECK(fabsf(pitched_out[2]-.3f)<.00001f);
    pitched.master_pitch=-12; render(&rp,&pitched,pitched_sample,result,1); CHECK(rp.voices[0].sampler.position==4.5 && rp.frame==3);
    CHECK(pitched.notes[0][0][0].pitch==60 && pitched.bpm==120);
    CHECK(project_save("test-project.llp",&pitched)); CHECK(project_load("test-project.llp",&q)); CHECK(project_equal(&pitched,&q));
    static Project bus; project_default(&bus); memset(bus.notes,0,sizeof bus.notes);
    bus.notes[0][0][0]=(Note){60,127,0,0}; bus.notes[0][1][0]=(Note){60,127,0,0};
    bus.volume[0]=bus.volume[1]=bus.master=1; bus.insert_volume[0]=.5f; bus.insert_volume[1]=.25f; bus.insert_volume[2]=.4f; bus.insert_pan[2]=.5f;
    CHECK(insert_connect(&bus,1,3) && insert_connect(&bus,2,3)); CHECK(!insert_connect(&bus,3,1) && !insert_connect(&bus,1,1));
    player_reset(&rp); render(&rp,&bus,fixture,result,1); CHECK(fabsf(result[0]-.04f)<.00001f && fabsf(result[1]-.08f)<.00001f);
    bus.insert_mute[0]=2; player_reset(&rp); render(&rp,&bus,fixture,result,1);
    CHECK(fabsf(result[0]-.02f)<.00001f); /* Solo one source, keep its bus audible. */
    bus.insert_mute[2]=2; player_reset(&rp); render(&rp,&bus,fixture,result,1);
    CHECK(fabsf(result[0]-.04f)<.00001f); /* Solo bus includes both routed sources. */
    CHECK(project_save("test-project.llp",&bus) && project_load("test-project.llp",&q)); CHECK(project_equal(&bus,&q));
    bus.insert_mute[0]=3; bus.insert_mute[2]=0; player_reset(&rp); render(&rp,&bus,fixture,result,1); CHECK(result[0]==0);
    bus.insert_mute[0]=0;
    CHECK(insert_connect(&bus,2,255)); player_reset(&rp); render(&rp,&bus,fixture,result,1); CHECK(fabsf(result[0]-.02f)<.00001f);
    bus.insert_mute[2]=1; player_reset(&rp); render(&rp,&bus,fixture,result,1); CHECK(result[0]==0 && result[1]==0);
    bus.bpm=120.5f; bus.clips[99][0]=1; CHECK(project_save("test-project.llp",&bus)); CHECK(project_load("test-project.llp",&q)); CHECK(project_equal(&bus,&q));
    CHECK(insert_reset(&bus,3)); CHECK(bus.insert_output[0]==3 && bus.insert_output[1]==255);
    /* Refuse to save a feedback cycle. */
    bus.insert_output[0]=2; bus.insert_output[1]=1; CHECK(!project_save("bad-project.llp",&bus));
    CHECK(q.bpm==120.5f && q.clips[99][0]==1);
    static Player a,b; player_reset(&a); player_reset(&b); float whole[24000],chunks[24000];
    render(&a,&p,s,whole,12000); for(int i=0;i<12000;i+=100) render(&b,&p,s,chunks+i*2,100);
    CHECK(memcmp(whole,chunks,sizeof whole)==0);
    double power=0; for(int i=0;i<24000;i++) { CHECK(isfinite(whole[i]) && fabsf(whole[i])<=1); power+=whole[i]*whole[i]; } CHECK(power>1);
    player_reset(&a); a.song=1; memset(p.clips,0,sizeof p.clips); p.clips[3][0]=1;
    render(&a,&p,s,chunks,12000); CHECK(memcmp(whole,chunks,sizeof whole)==0);
    for(int c=0;c<CHANNELS;c++) p.pan[c]=1;
    player_reset(&a); render(&a,&p,s,chunks,12000); for(int i=0;i<12000;i++) CHECK(chunks[i*2]==0);
    memset(p.mute,1,sizeof p.mute); player_reset(&a); render(&a,&p,s,whole,12000); for(int i=0;i<24000;i++) CHECK(whole[i]==0);
    /* Polyphonic notes have independent gates; a sustained fixture exposes note-offs. */
    project_default(&p); memset(p.notes,0,sizeof p.notes); p.volume[0]=1; p.master=1;
    float *sustain=malloc(RATE*sizeof(float)); CHECK(sustain); for(int i=0;i<RATE;i++) sustain[i]=.2f;
    Sample held[CHANNELS]={{sustain,RATE},{0},{0},{0}};
    Note *c=note_add(&p,0,0,0,60,1),*e=note_add(&p,0,0,0,64,2); CHECK(c && e && c!=e);
    c->velocity=e->velocity=127; player_reset(&a); render(&a,&p,held,whole,12000);
    CHECK(fabsf(whole[100]-.4f)<.00001f); CHECK(fabsf(whole[14000]-.2f)<.00001f);
    render(&a,&p,held,result,1); CHECK(result[0]==0 && result[1]==0); free(sustain);
    CHECK(note_move(&p,0,0,c,2,67,16)); CHECK(c->start==2 && c->pitch==67 && c->length==1 && c->velocity==127);
    CHECK(note_move(&p,0,0,e,2,67,16)); CHECK(e->start==2 && e->pitch==67);
    e->start=0; e->pitch=64;
    CHECK(!note_move(&p,0,0,c,16,60,16) && !note_move(&p,0,0,c,7,60,7));
    Note group_before[NOTES]; memcpy(group_before,p.notes[0][0],sizeof group_before); uint8_t selected[NOTES]={0};
    selected[c-p.notes[0][0]]=selected[e-p.notes[0][0]]=1;
    CHECK(notes_move(&p,0,0,group_before,selected,.5f,2,16));
    CHECK(c->start==2.5f && c->pitch==69 && c->length==1 && c->velocity==127);
    CHECK(e->start==.5f && e->pitch==66 && e->length==2);
    CHECK(!notes_move(&p,0,0,group_before,selected,15,2,16) && c->start==2.5f && e->start==.5f);
    CHECK(!notes_move(&p,0,0,group_before,selected,.5f,70,16) && c->pitch==69);
    Note *occupied=note_add(&p,0,0,3,69,1); CHECK(occupied);
    CHECK(notes_move(&p,0,0,group_before,selected,1,2,16) && c->start==3 && e->start==1);
    CHECK(occupied->start==c->start && occupied->pitch==c->pitch);

    /* Later bars trigger once, blank extended space stays silent, and clip lengths
       control song/export boundaries independently of shared source length. */
    project_default(&p); memset(p.notes,0,sizeof p.notes); memset(p.clips,0,sizeof p.clips);
    p.pattern_steps[0]=48; p.clips[0][0]=1; p.clip_steps[0][0]=32;
    CHECK(note_add(&p,0,0,20,60,2)); CHECK(!note_add(&p,0,0,48,60,1));
    CHECK(project_save("test-project.llp",&p)); CHECK(project_load("test-project.llp",&q)); CHECK(project_equal(&p,&q));
    player_reset(&a); a.song=1; a.frame=20*6000; render(&a,&p,s,result,1); CHECK(a.voices[0].gain>0);
    player_reset(&a); a.song=1; a.frame=19*6000; render(&a,&p,s,result,1); CHECK(a.voices[0].gain==0);
    CHECK(song_steps(&p)==32); CHECK(export_wav("test-export.wav",&p,s));
    FILE *f=fopen("test-export.wav","rb"); CHECK(f); CHECK(fseek(f,0,SEEK_END)==0); CHECK(ftell(f)==44+4*RATE*4); fclose(f);
    project_default(&p); for(int bar=0;bar<8;bar++) p.clips[0][bar]=1; CHECK(export_wav("test-export.wav",&p,s));
    f=fopen("test-export.wav","rb"); CHECK(f); char header[44]; CHECK(fread(header,1,44,f)==44); CHECK(!memcmp(header,"RIFF",4) && !memcmp(header+8,"WAVE",4));
    CHECK(fseek(f,0,SEEK_END)==0); CHECK(ftell(f)==44+8*2*RATE*4); fclose(f);
    /* Live audition shares routing/gates, supports chords, and never schedules patterns. */
    project_default(&p); p.volume[0]=p.master=1; p.route[0]=0;
    float constant[1024],live_out[32]; for(int i=0;i<1024;i++) constant[i]=1;
    Sample live_samples[CHANNELS]={0}; live_samples[0]=(Sample){constant,1024};
    static Player keys; player_reset(&keys);
    for(int i=0;i<32;i++) live_out[i]=.125f;
    render_live(&keys,&p,live_samples,live_out,16);
    for(int i=0;i<32;i++) CHECK(live_out[i]==.125f);
    CHECK(keys.frame==16 && keys.last_step==-1);
    keys.voices[0]=(Voice){.channel=0,.sampler={.position=0,.speed=1},.remaining=-1,.gain=.4f}; keys.voices[1]=(Voice){.channel=0,.sampler={.position=0,.speed=2},.remaining=-1,.gain=.4f};
    render_live(&keys,&p,live_samples,live_out,16);
    CHECK(fabsf(live_out[0]-.925f)<.00001f && keys.last_step==-1);
    CHECK(keys.voices[0].sampler.position==16 && keys.voices[1].sampler.position==32);
    keys.voices[0].remaining=1; render_live(&keys,&p,live_samples,live_out,1);
    CHECK(keys.voices[0].gain==0 && keys.voices[1].gain==.4f);
    p.mute[0]=1; for(int i=0;i<32;i++) live_out[i]=.125f;
    render_live(&keys,&p,live_samples,live_out,16);
    for(int i=0;i<32;i++) CHECK(live_out[i]==.125f);
    /* Sparse voice slots keep their mix order and stop independently. */
    player_reset(&keys); p.mute[0]=0;
    keys.voices[7]=(Voice){.channel=0,.sampler={.position=0,.speed=1},.remaining=-1,.gain=.2f}; keys.voices[127]=(Voice){.channel=0,.sampler={.position=0,.speed=1},.remaining=1,.gain=.4f};
    memset(live_out,0,sizeof live_out); render_live(&keys,&p,live_samples,live_out,2);
    CHECK(fabsf(live_out[0]-(.2f+.4f/(RATE*.005f)))<.00001f);
    CHECK(fabsf(live_out[2]-.2f)<.00001f);
    CHECK(keys.voices[127].gain==0 && keys.voices[7].sampler.position==2 && keys.frame==2);
    /* Mute/solo preserves other manual mutes and affects existing song tails. */
    uint8_t states[3]={1,0,0}; solo_toggle(states,3,1);
    CHECK(states[0]==1 && states[1]==2 && solo_any(states,3));
    solo_toggle(states,3,2); CHECK(states[1]==2 && states[2]==2);
    solo_toggle(states,3,2); CHECK(states[0]==1 && states[1]==2 && states[2]==0);
    solo_toggle(states,3,1); CHECK(states[0]==1 && !solo_any(states,3));
    project_default(&p); memset(p.notes,0,sizeof p.notes);
    p.volume[0]=p.master=1; p.route[0]=1;
    CHECK(note_add(&p,0,0,0,60,0)); p.notes[0][0][0].velocity=127;
    p.clips[0][0]=p.clips[1][0]=1;
    player_reset(&a); a.song=1; render(&a,&p,live_samples,result,1);
    CHECK(fabsf(result[0]-2)<.00001f);
    p.lane_mute[0]=1; render(&a,&p,live_samples,result,1);
    CHECK(fabsf(result[0]-1)<.00001f);
    solo_toggle(p.lane_mute,LANES,0); render(&a,&p,live_samples,result,1);
    CHECK(fabsf(result[0]-1)<.00001f);
    solo_toggle(p.lane_mute,LANES,1); render(&a,&p,live_samples,result,1);
    CHECK(fabsf(result[0]-2)<.00001f);
    solo_toggle(p.lane_mute,LANES,1);
    p.lane_mute[0]=3; render(&a,&p,live_samples,result,1); CHECK(result[0]==0);
    player_reset(&a); render(&a,&p,live_samples,result,1); CHECK(result[0]>0);
    /* Width operates on a summed stereo bus; meters read that same signal. */
    p.pan[0]=-1; p.insert_width[0]=0; float peaks[INSERTS+1][2]={{0}};
    player_reset(&a); render_mixer(&a,NULL,&p,live_samples,result,1,1,peaks);
    CHECK(fabsf(result[0]-.5f)<.00001f && result[0]==result[1]);
    CHECK(peaks[1][0]==.5f && peaks[1][1]==.5f);
    p.insert_width[0]=2; player_reset(&a); render(&a,&p,live_samples,result,1);
    CHECK(fabsf(result[0]-1.5f)<.00001f && fabsf(result[1]+.5f)<.00001f);
    p.insert_width[0]=1; p.pan[0]=0; player_reset(&a); player_reset(&keys);
    keys.voices[0]=(Voice){.channel=0,.sampler={.position=0,.speed=1},.remaining=-1,.gain=1,.lane=-1}; memset(peaks,0,sizeof peaks);
    render_mixer(&a,&keys,&p,live_samples,result,1,1,peaks);
    CHECK(peaks[0][0]==2 && peaks[1][0]==2 && fabsf(result[0]-2)<.00001f);
    p.master_mute=1; player_reset(&a); render(&a,&p,live_samples,result,1); CHECK(result[0]==0);
    p.master_width=.4f; p.insert_width[99]=1.7f; p.mute[3]=2;
    CHECK(project_save("test-project.llp",&p) && project_load("test-project.llp",&q));
    CHECK(project_equal(&p,&q));
    p.master_width=NAN; CHECK(!project_save("bad-project.llp",&p));
    /* Fractional playback loops retrigger at the start, independent of block size. */
    project_default(&p); memset(p.notes,0,sizeof p.notes); p.volume[0]=p.master=1; p.route[0]=0;
    CHECK(note_add(&p,0,0,.5f,60,.5f)); p.notes[0][0][0].velocity=127;
    player_reset(&a); player_seek(&a,&p,.5f); a.loop_start=.5f; a.loop_end=1.5f;
    render(&a,&p,exact,whole,6001);
    CHECK(a.frame==3001 && fabsf(whole[0]-.25f)<.00001f);
    CHECK(whole[6000]==0 && whole[12000]==whole[0]);
    player_reset(&b); player_seek(&b,&p,.5f); b.loop_start=.5f; b.loop_end=1.5f;
    render(&b,&p,exact,chunks,3333); render(&b,&p,exact,chunks+6666,2668);
    CHECK(memcmp(whole,chunks,6001*2*sizeof(float))==0);
    p.clips[0][0]=1; player_reset(&b); b.song=1; player_seek(&b,&p,.5f); b.loop_start=.5f; b.loop_end=1.5f;
    render(&b,&p,exact,chunks,6001); CHECK(memcmp(whole,chunks,6001*2*sizeof(float))==0);
    p.notes[0][0][0].length=4;
    player_reset(&b); b.song=1; player_seek(&b,&p,.5f); b.loop_start=.5f; b.loop_end=1.5f;
    render(&b,&p,exact,chunks,6001); CHECK(chunks[12000]==chunks[0]); /* No overlapping notes across a loop boundary. */
    /* Swing moves odd sixteenths, preserves even steps and block boundaries. */
    project_default(&p); memset(p.notes,0,sizeof p.notes); p.volume[0]=p.master=1; p.route[0]=0; p.swing=1;
    CHECK(note_add(&p,0,0,1,60,.1f)); CHECK(note_add(&p,0,0,2,60,.1f));
    p.notes[0][0][0].velocity=p.notes[0][0][1].velocity=127;
    player_reset(&a); render(&a,&p,exact,whole,12000);
    CHECK(whole[6000*2]==0 && whole[8999*2]==0 && whole[9000*2]>0);
    player_reset(&b); render(&b,&p,exact,chunks,7777); render(&b,&p,exact,chunks+7777*2,4223);
    CHECK(memcmp(whole,chunks,sizeof whole)==0);
    player_reset(&b); render(&b,&p,exact,chunks,12000); render(&b,&p,exact,timed,1); CHECK(timed[0]>0);
    p.clips[0][0]=1; player_reset(&b); b.song=1; render(&b,&p,exact,chunks,12000);
    CHECK(memcmp(whole,chunks,sizeof whole)==0);
    CHECK(project_save("test-project.llp",&p) && project_load("test-project.llp",&roundtrip) && roundtrip.swing==1);
    p.swing=NAN; CHECK(!project_save("bad-project.llp",&p));
    project_default(&p); CHECK(p.master==1 && p.insert_volume[0]==1 && p.insert_volume[99]==1);
    for(int ch=0;ch<CHANNELS;ch++) CHECK(p.volume[ch]==1);
    CHECK(fader_gain(0)==0 && fabsf(fader_gain(1)-MIXER_GAIN_MAX)<.00001f);
    CHECK(fabsf(fader_gain(fader_position(1))-1)<.00001f && fader_position(1)==.75f);
    const float gains[]={.01f,.25f,.5f,1,1.5f,2};
    for(unsigned i=0;i<sizeof gains/sizeof *gains;i++) CHECK(fabsf(fader_gain(fader_position(gains[i]))-gains[i])<.00001f);
    CHECK(gain_db(1)==0 && gain_db(2)>6);
    p.master=1.5f; p.insert_volume[0]=2;
    snprintf(p.audio_io[0][1],128,"USB output"); snprintf(p.audio_io[1][0],128,"USB input");
    p.effect_mix[1][0]=.25f; p.effect_bypass[1][0]=1;
    snprintf(p.track_names[0],PATTERN_NAME,"Drums");
    snprintf(p.insert_names[0],PATTERN_NAME,"Drum bus");
    p.pattern_colors[0]=pattern_palette[5];
    CHECK(project_save("test-project.llp",&p) && project_load("test-project.llp",&q));
    CHECK(q.effect_mix[1][0]==.25f && q.effect_bypass[1][0]==1 && !strcmp(q.track_names[0],"Drums"));
    CHECK(!strcmp(q.insert_names[0],"Drum bus") && !strcmp(q.insert_names[99],"Insert 100"));
    CHECK(q.pattern_colors[0]==pattern_palette[5] && q.pattern_colors[1]==pattern_palette[1]);
    CHECK(q.effect_mix[0][0]==1 && q.effect_bypass[0][0]==0);
    p.pattern_colors[0]=0x1000000; CHECK(!project_save("bad-project.llp",&p)); p.pattern_colors[0]=pattern_palette[5];
    p.effect_mix[1][0]=NAN; CHECK(!project_save("bad-project.llp",&p)); p.effect_mix[1][0]=.25f;
    p.effect_bypass[1][0]=2; CHECK(!project_save("bad-project.llp",&p)); p.effect_bypass[1][0]=1;
    CHECK(q.master==1.5f && q.insert_volume[0]==2 && !strcmp(q.audio_io[0][1],"USB output") && !strcmp(q.audio_io[1][0],"USB input"));
    CHECK(insert_reset(&q,1) && q.effect_mix[1][0]==1 && q.effect_bypass[1][0]==0);
    CHECK(!strcmp(q.insert_names[0],"Drum bus"));
    p.insert_names[1][0]='\n'; CHECK(!project_save("bad-project.llp",&p)); snprintf(p.insert_names[1],PATTERN_NAME,"Insert 2");
    p.audio_io[1][0][0]='\n'; CHECK(!project_save("bad-project.llp",&p)); p.audio_io[1][0][0]='U';
    p.insert_volume[0]=2.1f; CHECK(!project_save("bad-project.llp",&p));
    for(int ch=0;ch<CHANNELS;ch++) free(s[ch].data);
    remove("test-project.llp"); remove("bad-project.llp"); remove("test-export.wav");
    p.insert_volume[0]=2; p.channel_pitch[0]=-.5f; p.pitch_range[0]=48;
    CHECK(project_save("pitch.llp",&p) && project_load("pitch.llp",&q));
    CHECK(q.channel_pitch[0]==-.5f && q.pitch_range[0]==48); remove("pitch.llp");
    p.pitch_range[0]=1.5f; CHECK(!project_save("bad-project.llp",&p)); p.pitch_range[0]=2;
    /* Boosted channel volume survives save/load and reaches the audio renderer. */
    project_default(&p); p.volume[0]=VOLUME_KNOB_MAX; p.route[0]=0;
    CHECK(project_save("boost.llp",&p) && project_load("boost.llp",&q));
    CHECK(q.volume[0]==VOLUME_KNOB_MAX);
    float quiet[]={.1f}; Sample boosted[CHANNELS]={{quiet,1}}; float boosted_pcm[2];
    memset(p.notes,0,sizeof p.notes); CHECK(note_add(&p,0,0,0,60,0)); p.notes[0][0][0].velocity=127;
    player_reset(&a); render(&a,&p,boosted,boosted_pcm,1);
    CHECK(fabsf(boosted_pcm[0]-(.1f*VOLUME_KNOB_MAX))<1e-6);
    p.volume[0]=VOLUME_KNOB_MAX+.01f; CHECK(!project_save("invalid-boost.llp",&p));
    p.volume[0]=NAN; CHECK(!project_save("invalid-boost.llp",&p));
    p.volume[0]=-1; CHECK(!project_save("invalid-boost.llp",&p));
    remove("boost.llp"); remove("invalid-boost.llp");
    project_default(&p); p.channel_colors[0]=pattern_palette[3];
    CHECK(project_save("color.llp",&p) && project_load("color.llp",&q) && project_equal(&p,&q)); remove("color.llp");
    p.channel_colors[0]=0x1000000; CHECK(!project_save("invalid-color.llp",&p)); remove("invalid-color.llp");
    project_new(&p); CHECK(p.channel_count==1 && p.volume[0]==1 && !strcmp(p.channel_names[0],"Sampler"));
    for(int c=0;c<CHANNELS;c++) CHECK(!strcmp(p.paths[c],SAMPLE_EMPTY));
    for(int pat=0;pat<PATTERNS;pat++) for(int c=0;c<CHANNELS;c++) for(int n=0;n<NOTES;n++) CHECK(!p.notes[pat][c][n].velocity);
    for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) CHECK(!p.clips[l][b]);
    CHECK(project_save("new.llp",&p) && project_load("new.llp",&q) && project_equal(&p,&q)); remove("new.llp");
    project_demo(&p); CHECK(p.pattern_count==2 && song_steps(&p)==128);
    CHECK(project_save("demo.llp",&p) && project_load("demo.llp",&q) && project_equal(&p,&q)); remove("demo.llp");
    Sample demo[CHANNELS]; samples_default(demo); static Player demo_player; player_reset(&demo_player); demo_player.song=1;
    float demo_pcm[512],peak=0;
    for(int block=0;block<4000;block++) {
        render(&demo_player,&p,demo,demo_pcm,256);
        for(int n=0;n<512;n++) { CHECK(isfinite(demo_pcm[n]) && fabsf(demo_pcm[n])<=1); peak=fmaxf(peak,fabsf(demo_pcm[n])); }
    }
    CHECK(peak>.05f); for(int c=0;c<CHANNELS;c++) free(demo[c].data);
    /* Resize selected notes from a fixed snapshot, without changing pitches,
       starts, or unselected notes; the shortest note bounds a group shrink. */
    project_new(&p); p.notes[0][0][0]=(Note){60,100,0,2};
    p.notes[0][0][1]=(Note){64,90,4,4}; p.notes[0][0][2]=(Note){67,80,8,3};
    Note resize_before[NOTES]; memcpy(resize_before,p.notes[0][0],sizeof resize_before);
    uint8_t resized[NOTES]={1,1};
    CHECK(notes_resize(&p,0,0,resize_before,resized,3,1)==11);
    CHECK(p.notes[0][0][0].length==5 && p.notes[0][0][1].length==7 && p.notes[0][0][2].length==3);
    CHECK(p.notes[0][0][1].start==4 && p.notes[0][0][1].pitch==64);
    notes_resize(&p,0,0,resize_before,resized,-20,1);
    CHECK(p.notes[0][0][0].length==1 && p.notes[0][0][1].length==3);
    notes_resize(&p,0,0,resize_before,resized,0,1);
    CHECK(!memcmp(resize_before,p.notes[0][0],sizeof resize_before));
    resize_before[0].length=0;
    notes_resize(&p,0,0,resize_before,resized,0,.01f); CHECK(p.notes[0][0][0].length==0);
    notes_resize(&p,0,0,resize_before,resized,.5f,.01f);
    CHECK(p.notes[0][0][0].length==1.5f && p.notes[0][0][1].length==4.5f);
    /* Hit testing follows drawing order. An erase-stroke snapshot continues
       to hit the deleted top note, protecting the note underneath until a new
       click takes a fresh snapshot. */
    Note overlap[NOTES]={{60,100,0,4},{60,100,1,2},{64,100,1,2}};
    Note stroke[NOTES]; memcpy(stroke,overlap,sizeof stroke);
    CHECK(note_hit(overlap,1.5f,60)==1 && note_hit(overlap,.5f,60)==0);
    overlap[note_hit(stroke,1.5f,60)].velocity=0;
    CHECK(note_hit(stroke,1.5f,60)==1 && overlap[0].velocity==100);
    CHECK(note_hit(overlap,1.5f,60)==0 && note_hit(overlap,1.5f,64)==2);
    CHECK(note_hit(overlap,4,60)==-1);
    /* Velocity paint follows crossed note starts, including chord members;
       line previews restore notes that fall outside the revised ramp. */
    Note velocities[NOTES]={{60,80,0,2},{64,80,2,2},{67,90,2,2},{69,70,4,2},{72,65,8,2}};
    Note velocity_before[NOTES]; memcpy(velocity_before,velocities,sizeof velocities);
    notes_velocity(velocities,NULL,0,4,20,100,0);
    CHECK(velocities[0].velocity==20 && velocities[1].velocity==60 && velocities[2].velocity==60 && velocities[3].velocity==100 && velocities[4].velocity==65);
    notes_velocity(velocities,velocity_before,0,4,20,100,0);
    notes_velocity(velocities,velocity_before,0,2,20,100,0);
    CHECK(velocities[1].velocity==100 && velocities[2].velocity==100 && velocities[3].velocity==70);
    notes_velocity(velocities,velocity_before,4,0,100,20,0);
    CHECK(velocities[0].velocity==20 && velocities[1].velocity==60 && velocities[3].velocity==100);
    notes_velocity(velocities,NULL,2.05f,2.05f,0,200,.1f);
    CHECK(velocities[1].velocity==127 && velocities[2].velocity==127);
    notes_velocity(velocities,NULL,0,0,0,-10,0); CHECK(velocities[0].velocity==1);
    CHECK(velocities[1].start==2 && velocities[1].pitch==64 && velocities[1].length==2);
    project_new(&p);
    Note *stack_bottom=note_add(&p,0,0,2,60,4),*stack_top=note_add(&p,0,0,2,60,2);
    CHECK(stack_bottom && stack_top && stack_bottom!=stack_top);
    CHECK(note_hit(p.notes[0][0],2.5f,60)==stack_top-p.notes[0][0]);
    CHECK(project_save("stacked-notes.llp",&p) && project_load("stacked-notes.llp",&q));
    CHECK(q.notes[0][0][0].velocity && q.notes[0][0][1].velocity && q.notes[0][0][0].start==q.notes[0][0][1].start);
    remove("stacked-notes.llp");
    float stack_pcm[64]; for(int i=0;i<64;i++) stack_pcm[i]=.1f;
    Sample stack_samples[CHANNELS]={{.data=stack_pcm,.frames=64}};
    static Player stacked_player; player_reset(&stacked_player); stacked_player.frame=12000;
    float stack_out[16]; render(&stacked_player,&p,stack_samples,stack_out,8);
    CHECK(fabsf(stack_out[2]-.2f*100/127)<.00001f); /* Both notes produce a voice. */

    puts("Project persistence, master pitch, clip lengths, extended patterns, chords and gates, routing, render consistency, mute and WAV duration passed."); return 0;
}
