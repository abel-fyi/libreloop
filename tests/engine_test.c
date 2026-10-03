// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed: %s:%d: %s\n",__FILE__,__LINE__,#x); return 1; } } while(0)
/* Fixtures preserve each older text layout, including the original four lanes. */
static int legacy_save(const char *path,Project *p,int version) {
    FILE *f=fopen(path,"w"); if(!f) return 0;
    fprintf(f,"HOMEBEAT %d\n%.9g %.9g",version,p->bpm,p->master);
    if(version>=12) fprintf(f," %d",p->channel_count);
    fputc('\n',f);
    int channels=version>=12?CHANNELS:4,inserts=version>=12?INSERTS:16,clips=version>=14?CLIPS:BARS;
    for(int c=0;c<channels;c++) fprintf(f,"%.9g %.9g %u\n",p->volume[c],p->pan[c],p->mute[c]);
    for(int a=0;a<PATTERNS;a++) for(int c=0;c<channels;c++) for(int step=0;step<(version>=4?NOTES:STEPS);step++) {
        if(version>=4) { Note n=p->notes[a][c][step]; fprintf(f,"%u %u %.0f %.0f\n",n.pitch,n.velocity,n.start,n.length); }
        else { Note *n=note_at(p,a,c,step,60); fprintf(f,"%u %u\n",n?n->pitch:0,n?n->velocity:0); }
    }
    for(int l=0;l<(version>=10?LANES:4);l++) for(int b=0;b<clips;b++) fprintf(f,"%u\n",p->clips[l][b]);
    for(int c=0;c<channels;c++) fprintf(f,"%s\n",p->paths[c]);
    if(version>=2) for(int a=0;a<PATTERNS;a++) fprintf(f,"%s\n",p->pattern_names[a]);
    if(version>=3) {
        fprintf(f,"%d\n",p->insert_count); for(int c=0;c<channels;c++) fprintf(f,"%u\n",p->route[c]);
        for(int i=0;i<inserts;i++) fprintf(f,"%.9g %.9g %u\n",p->insert_volume[i],p->insert_pan[i],p->insert_mute[i]);
    }
    if(version>=5) {
        for(int a=0;a<PATTERNS;a++) fprintf(f,"%.0f\n",p->pattern_steps[a]);
        for(int l=0;l<(version>=10?LANES:4);l++) for(int b=0;b<clips;b++) fprintf(f,"%.0f\n",p->clip_steps[l][b]);
    }
    if(version>=6) fprintf(f,"%.9g\n",p->master_pitch);
    if(version>=7) for(int i=0;i<inserts;i++) fprintf(f,"%u\n",p->insert_output[i]);
    if(version>=9) fprintf(f,"%d\n",p->pattern_count);
    if(version>=12) for(int c=0;c<CHANNELS;c++) fprintf(f,"%s\n",p->channel_names[c]);
    if(version>=13) for(int l=0;l<LANES;l++) for(int b=0;b<clips;b++) fprintf(f,"%.9g\n",p->clip_starts[l][b]);
    if(version>=15) for(int c=0;c<CHANNELS;c++) { Sampler a=p->sampler[c]; fprintf(f,"%.9g %.9g %.9g %.9g %u %u\n",a.pitch,a.time,a.start,a.length,a.flags,a.stretch); }
    if(version>=16) {
        for(int i=0;i<INSERTS;i++) fprintf(f,"%.9g\n",p->insert_width[i]);
        fprintf(f,"%.9g %u\n",p->master_width,p->master_mute);
        for(int l=0;l<LANES;l++) fprintf(f,"%u\n",p->lane_mute[l]);
    }
    if(version>=17) fprintf(f,"%.9g\n",p->swing);
    return fclose(f)==0;
}
int main(void) {
    CHECK(snap_interval(SNAP_STEP,10)==1 && snap_interval(SNAP_HALF_STEP,10)==.5f);
    CHECK(fabsf(snap_interval(SNAP_SIXTH_BEAT,10)-2.f/3)<.00001f);
    CHECK(snap_interval(SNAP_AUTO,100)<snap_interval(SNAP_AUTO,2));
    Project fine,roundtrip; project_default(&fine); memset(fine.notes,0,sizeof fine.notes);
    fine.volume[0]=fine.master=1; fine.route[0]=0;
    CHECK(note_add(&fine,0,0,.5f,60,.5f)); CHECK(note_add(&fine,0,0,4.f/3,64,2.f/3));
    fine.clips[0][0]=1; fine.clip_starts[0][0]=.0625f; fine.clip_steps[0][0]=2.5f;
    CHECK(project_save("fractional.hbt",&fine) && project_load("fractional.hbt",&roundtrip));
    CHECK(memcmp(&fine,&roundtrip,sizeof fine)==0); remove("fractional.hbt");
    float sustained[RATE]; for(int i=0;i<RATE;i++) sustained[i]=.25f;
    Sample exact[CHANNELS]={{sustained,RATE}}; float timed[20000],split[20000]; Player clock; player_reset(&clock);
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
    Project deleted,reloaded; project_default(&deleted);
    CHECK(deleted.pattern_count==1 && deleted.channel_count==4 && deleted.insert_count==100);
    for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) CHECK(deleted.clips[l][b]==0);
    deleted.route[0]=deleted.route[1]=2; deleted.insert_volume[1]=.35f; deleted.insert_mute[1]=3;
    CHECK(!insert_reset(&deleted,0) && !insert_reset(&deleted,101));
    CHECK(insert_reset(&deleted,2)); CHECK(deleted.insert_count==100 && deleted.route[0]==2);
    CHECK(deleted.insert_volume[1]==1 && deleted.insert_mute[1]==0);
    deleted.channel_count=5; strcpy(deleted.channel_names[4],"Extra"); deleted.route[4]=100;
    CHECK(note_add(&deleted,0,4,0,60,2)); CHECK(channel_delete(&deleted,1));
    CHECK(deleted.channel_count==4 && !strcmp(deleted.channel_names[3],"Extra") && deleted.route[3]==100);
    CHECK(note_at(&deleted,0,3,0,60)); CHECK(!channel_delete(&deleted,4));
    CHECK(project_save("test-project.hbt",&deleted) && project_load("test-project.hbt",&reloaded)); CHECK(memcmp(&deleted,&reloaded,sizeof deleted)==0);
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
    Project p,q; project_default(&p);
    strcpy(p.paths[0],"/tmp/sample with spaces.wav"); strcpy(p.pattern_names[0],"Drums + Bass");
    memset(p.pattern_names[7],'X',PATTERN_NAME-1); p.pattern_names[7][PATTERN_NAME-1]=0;
    p.insert_count=6; p.route[0]=5; p.route[1]=5; p.insert_volume[4]=.35f; p.insert_pan[4]=-.25f; p.insert_mute[5]=1;
    for(int v=1;v<=3;v++) {
        CHECK(legacy_save("legacy.hbt",&p,v)); CHECK(project_load("legacy.hbt",&q));
        CHECK(!strcmp(q.pattern_names[0],v==1?"Pattern 1":"Drums + Bass"));
        CHECK(!strcmp(q.paths[0],p.paths[0])); CHECK(q.insert_count==(v==3?6:4)); CHECK(q.route[0]==(v==3?5:1));
        for(int a=0;a<PATTERNS;a++) for(int c=0;c<CHANNELS;c++) for(int step=0;step<STEPS;step++) {
            Note *before=note_at(&p,a,c,step,60),*after=note_at(&q,a,c,step,60);
            CHECK((before!=NULL)==(after!=NULL)); if(before) CHECK(after->velocity==before->velocity && after->length==0);
        }
    }
    p.pattern_count=3;
    CHECK(note_add(&p,2,3,4,60,3)); CHECK(note_add(&p,2,3,4,64,4)); CHECK(note_add(&p,2,3,4,67,3));
    CHECK(project_save("test-project.hbt",&p)); CHECK(project_load("test-project.hbt",&q)); CHECK(memcmp(&p,&q,sizeof p)==0);
    FILE *old,*legacy; char line[1100];
    for(int version=4;version<=15;version++) {
        CHECK(legacy_save("legacy.hbt",&p,version)); CHECK(project_load("legacy.hbt",&q));
        CHECK(q.pattern_count==(version>=9?p.pattern_count:PATTERNS)); q.pattern_count=p.pattern_count;
        CHECK(memcmp(&p,&q,sizeof p)==0);
    }
    FILE *f=fopen("bad-project.hbt","w"); CHECK(f); fputs("HOMEBEAT 4\n0 nan\n",f); fclose(f);
    CHECK(!project_load("bad-project.hbt",&q)); CHECK(memcmp(&p,&q,sizeof p)==0);
    f=fopen("bad-project.hbt","w"); CHECK(f); fputs("HOMEBEAT 4\n120 .7\n",f);
    for(int c=0;c<CHANNELS;c++) fputs(".7 0 0\n",f);
    fputs("60 100 15 2\n",f); fclose(f); /* Note exceeds the pattern boundary. */
    CHECK(!project_load("bad-project.hbt",&q)); CHECK(memcmp(&p,&q,sizeof p)==0);
    Sample s[CHANNELS]; samples_default(s); for(int c=0;c<4;c++) CHECK(s[c].data);
    Project routing; project_default(&routing); memset(routing.notes,0,sizeof routing.notes);
    routing.notes[0][0][0]=(Note){60,127,0,0}; routing.notes[0][1][0]=(Note){60,127,0,0};
    routing.volume[0]=routing.volume[1]=.5f; routing.master=1;
    routing.route[0]=routing.route[1]=1; routing.insert_volume[0]=.5f; routing.insert_pan[0]=.5f;
    float one=.2f,two=.4f,result[2]; Sample fixture[CHANNELS]={{&one,1},{&two,1},{0},{0}};
    Player rp; player_reset(&rp); render(&rp,&routing,fixture,result,1);
    CHECK(fabsf(result[0]-tanhf(.075f))<.00001f && fabsf(result[1]-tanhf(.15f))<.00001f);
    routing.route[0]=0; player_reset(&rp); render(&rp,&routing,fixture,result,1);
    CHECK(fabsf(result[0]-tanhf(.15f))<.00001f && fabsf(result[1]-tanhf(.2f))<.00001f);
    routing.insert_mute[0]=1; player_reset(&rp); render(&rp,&routing,fixture,result,1);
    CHECK(fabsf(result[0]-tanhf(.1f))<.00001f && fabsf(result[1]-tanhf(.1f))<.00001f);
    /* Master pitch changes playback rate, including active voices, without
       changing the pattern clock or stored note pitches. */
    Project pitched; project_default(&pitched); memset(pitched.notes,0,sizeof pitched.notes);
    pitched.notes[0][0][0]=(Note){60,127,0,4}; pitched.master_pitch=12;
    float ramp[]={.1f,.2f,.3f,.4f,.5f,.6f}; Sample pitched_sample[CHANNELS]={{ramp,6},{0},{0},{0}};
    float pitched_out[4]; player_reset(&rp); render(&rp,&pitched,pitched_sample,pitched_out,2);
    CHECK(rp.voices[0].position==4 && rp.frame==2 && rp.voices[0].remaining==24000-2);
    CHECK(fabsf(pitched_out[2]-tanhf(.3f))<.00001f);
    pitched.master_pitch=-12; render(&rp,&pitched,pitched_sample,result,1); CHECK(rp.voices[0].position==4.5 && rp.frame==3);
    CHECK(pitched.notes[0][0][0].pitch==60 && pitched.bpm==120);
    CHECK(project_save("test-project.hbt",&pitched)); CHECK(project_load("test-project.hbt",&q)); CHECK(memcmp(&pitched,&q,sizeof q)==0);
    old=fopen("test-project.hbt","r"); legacy=fopen("bad-project.hbt","w"); CHECK(old && legacy);
    for(int i=0;i<1+1+CHANNELS+PATTERNS*CHANNELS*NOTES+LANES*CLIPS+CHANNELS+PATTERNS+1+CHANNELS+INSERTS+PATTERNS+LANES*CLIPS;i++) { CHECK(fgets(line,sizeof line,old)); fputs(line,legacy); }
    fputs("nan\n",legacy); fclose(old); fclose(legacy); CHECK(!project_load("bad-project.hbt",&q)); CHECK(memcmp(&pitched,&q,sizeof q)==0);
    Project bus; project_default(&bus); memset(bus.notes,0,sizeof bus.notes);
    bus.notes[0][0][0]=(Note){60,127,0,0}; bus.notes[0][1][0]=(Note){60,127,0,0};
    bus.volume[0]=bus.volume[1]=bus.master=1; bus.insert_volume[0]=.5f; bus.insert_volume[1]=.25f; bus.insert_volume[2]=.4f; bus.insert_pan[2]=.5f;
    CHECK(insert_connect(&bus,1,3) && insert_connect(&bus,2,3)); CHECK(!insert_connect(&bus,3,1) && !insert_connect(&bus,1,1));
    player_reset(&rp); render(&rp,&bus,fixture,result,1); CHECK(fabsf(result[0]-tanhf(.04f))<.00001f && fabsf(result[1]-tanhf(.08f))<.00001f);
    bus.insert_mute[0]=2; player_reset(&rp); render(&rp,&bus,fixture,result,1);
    CHECK(fabsf(result[0]-tanhf(.02f))<.00001f); /* Solo one source, keep its bus audible. */
    bus.insert_mute[2]=2; player_reset(&rp); render(&rp,&bus,fixture,result,1);
    CHECK(fabsf(result[0]-tanhf(.04f))<.00001f); /* Solo bus includes both routed sources. */
    CHECK(project_save("test-project.hbt",&bus) && project_load("test-project.hbt",&q)); CHECK(memcmp(&bus,&q,sizeof q)==0);
    bus.insert_mute[0]=3; bus.insert_mute[2]=0; player_reset(&rp); render(&rp,&bus,fixture,result,1); CHECK(result[0]==0);
    bus.insert_mute[0]=0;
    CHECK(insert_connect(&bus,2,255)); player_reset(&rp); render(&rp,&bus,fixture,result,1); CHECK(fabsf(result[0]-tanhf(.02f))<.00001f);
    bus.insert_mute[2]=1; player_reset(&rp); render(&rp,&bus,fixture,result,1); CHECK(result[0]==0 && result[1]==0);
    bus.bpm=120.5f; bus.clips[99][0]=1; CHECK(project_save("test-project.hbt",&bus)); CHECK(project_load("test-project.hbt",&q)); CHECK(memcmp(&bus,&q,sizeof q)==0);
    CHECK(insert_reset(&bus,3)); CHECK(bus.insert_output[0]==3 && bus.insert_output[1]==255);
    /* Reject a saved feedback cycle without replacing the current project. */
    bus.insert_output[0]=2; bus.insert_output[1]=1; CHECK(project_save("bad-project.hbt",&bus)); CHECK(!project_load("bad-project.hbt",&q));
    CHECK(q.bpm==120.5f && q.clips[99][0]==1);
    Player a,b; player_reset(&a); player_reset(&b); float whole[24000],chunks[24000];
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
    CHECK(fabsf(whole[100]-tanhf(.4f))<.00001f); CHECK(fabsf(whole[14000]-tanhf(.2f))<.00001f);
    render(&a,&p,held,result,1); CHECK(result[0]==0 && result[1]==0); free(sustain);
    CHECK(note_move(&p,0,0,c,2,67,16)); CHECK(c->start==2 && c->pitch==67 && c->length==1 && c->velocity==127);
    CHECK(!note_move(&p,0,0,e,2,67,16)); CHECK(e->start==0 && e->pitch==64);
    CHECK(!note_move(&p,0,0,c,16,60,16) && !note_move(&p,0,0,c,7,60,7));
    Note group_before[NOTES]; memcpy(group_before,p.notes[0][0],sizeof group_before); uint8_t selected[NOTES]={0};
    selected[c-p.notes[0][0]]=selected[e-p.notes[0][0]]=1;
    CHECK(notes_move(&p,0,0,group_before,selected,.5f,2,16));
    CHECK(c->start==2.5f && c->pitch==69 && c->length==1 && c->velocity==127);
    CHECK(e->start==.5f && e->pitch==66 && e->length==2);
    CHECK(!notes_move(&p,0,0,group_before,selected,15,2,16) && c->start==2.5f && e->start==.5f);
    CHECK(!notes_move(&p,0,0,group_before,selected,.5f,70,16) && c->pitch==69);
    Note *occupied=note_add(&p,0,0,3,69,1); CHECK(occupied);
    CHECK(!notes_move(&p,0,0,group_before,selected,1,2,16) && c->start==2.5f && e->start==.5f);

    /* Later bars trigger once, blank extended space stays silent, and clip lengths
       control song/export boundaries independently of shared source length. */
    project_default(&p); memset(p.notes,0,sizeof p.notes); memset(p.clips,0,sizeof p.clips);
    p.pattern_steps[0]=48; p.clips[0][0]=1; p.clip_steps[0][0]=32;
    CHECK(note_add(&p,0,0,20,60,2)); CHECK(!note_add(&p,0,0,48,60,1));
    CHECK(project_save("test-project.hbt",&p)); CHECK(project_load("test-project.hbt",&q)); CHECK(memcmp(&p,&q,sizeof p)==0);
    player_reset(&a); a.song=1; a.frame=20*6000; render(&a,&p,s,result,1); CHECK(a.voices[0].gain>0);
    player_reset(&a); a.song=1; a.frame=19*6000; render(&a,&p,s,result,1); CHECK(a.voices[0].gain==0);
    CHECK(song_steps(&p)==32); CHECK(export_wav("test-export.wav",&p,s));
    f=fopen("test-export.wav","rb"); CHECK(f); CHECK(fseek(f,0,SEEK_END)==0); CHECK(ftell(f)==44+4*RATE*4); fclose(f);
    project_default(&p); for(int bar=0;bar<8;bar++) p.clips[0][bar]=1; CHECK(export_wav("test-export.wav",&p,s));
    f=fopen("test-export.wav","rb"); CHECK(f); char header[44]; CHECK(fread(header,1,44,f)==44); CHECK(!memcmp(header,"RIFF",4) && !memcmp(header+8,"WAVE",4));
    CHECK(fseek(f,0,SEEK_END)==0); CHECK(ftell(f)==44+8*2*RATE*4); fclose(f);
    /* Live audition shares routing/gates, supports chords, and never schedules patterns. */
    project_default(&p); p.volume[0]=p.master=1; p.route[0]=0;
    float constant[1024],live_out[32]; for(int i=0;i<1024;i++) constant[i]=1;
    Sample live_samples[CHANNELS]={0}; live_samples[0]=(Sample){constant,1024};
    Player keys; player_reset(&keys);
    for(int i=0;i<32;i++) live_out[i]=.125f;
    render_live(&keys,&p,live_samples,live_out,16);
    for(int i=0;i<32;i++) CHECK(live_out[i]==.125f);
    CHECK(keys.frame==16 && keys.last_step==-1);
    keys.voices[0]=(Voice){0,0,1,-1,.4f}; keys.voices[1]=(Voice){0,0,2,-1,.4f};
    render_live(&keys,&p,live_samples,live_out,16);
    CHECK(fabsf(live_out[0]-tanhf(.925f))<.00001f && keys.last_step==-1);
    CHECK(keys.voices[0].position==16 && keys.voices[1].position==32);
    keys.voices[0].remaining=1; render_live(&keys,&p,live_samples,live_out,1);
    CHECK(keys.voices[0].gain==0 && keys.voices[1].gain==.4f);
    p.mute[0]=1; for(int i=0;i<32;i++) live_out[i]=.125f;
    render_live(&keys,&p,live_samples,live_out,16);
    for(int i=0;i<32;i++) CHECK(live_out[i]==.125f);
    /* Sparse voice slots keep their mix order and stop independently. */
    player_reset(&keys); p.mute[0]=0;
    keys.voices[7]=(Voice){0,0,1,-1,.2f}; keys.voices[127]=(Voice){0,0,1,1,.4f};
    memset(live_out,0,sizeof live_out); render_live(&keys,&p,live_samples,live_out,2);
    CHECK(fabsf(live_out[0]-tanhf(.2f+.4f/(RATE*.005f)))<.00001f);
    CHECK(fabsf(live_out[2]-tanhf(.2f))<.00001f);
    CHECK(keys.voices[127].gain==0 && keys.voices[7].position==2 && keys.frame==2);
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
    CHECK(fabsf(result[0]-tanhf(2))<.00001f);
    p.lane_mute[0]=1; render(&a,&p,live_samples,result,1);
    CHECK(fabsf(result[0]-tanhf(1))<.00001f);
    solo_toggle(p.lane_mute,LANES,0); render(&a,&p,live_samples,result,1);
    CHECK(fabsf(result[0]-tanhf(1))<.00001f);
    solo_toggle(p.lane_mute,LANES,1); render(&a,&p,live_samples,result,1);
    CHECK(fabsf(result[0]-tanhf(2))<.00001f);
    solo_toggle(p.lane_mute,LANES,1);
    p.lane_mute[0]=3; render(&a,&p,live_samples,result,1); CHECK(result[0]==0);
    player_reset(&a); render(&a,&p,live_samples,result,1); CHECK(result[0]>0);
    /* Width operates on a summed stereo bus; meters read that same signal. */
    p.pan[0]=-1; p.insert_width[0]=0; float peaks[INSERTS+1][2]={{0}};
    player_reset(&a); render_mixer(&a,NULL,&p,live_samples,result,1,1,peaks);
    CHECK(fabsf(result[0]-tanhf(.5f))<.00001f && result[0]==result[1]);
    CHECK(peaks[1][0]==.5f && peaks[1][1]==.5f);
    p.insert_width[0]=2; player_reset(&a); render(&a,&p,live_samples,result,1);
    CHECK(fabsf(result[0]-tanhf(1.5f))<.00001f && fabsf(result[1]-tanhf(-.5f))<.00001f);
    p.insert_width[0]=1; p.pan[0]=0; player_reset(&a); player_reset(&keys);
    keys.voices[0]=(Voice){0,0,1,-1,1,-1}; memset(peaks,0,sizeof peaks);
    render_mixer(&a,&keys,&p,live_samples,result,1,1,peaks);
    CHECK(peaks[0][0]==2 && peaks[1][0]==2 && fabsf(result[0]-tanhf(2))<.00001f);
    p.master_mute=1; player_reset(&a); render(&a,&p,live_samples,result,1); CHECK(result[0]==0);
    p.master_width=.4f; p.insert_width[99]=1.7f; p.mute[3]=2;
    CHECK(project_save("test-project.hbt",&p) && project_load("test-project.hbt",&q));
    CHECK(memcmp(&p,&q,sizeof p)==0);
    p.master_width=NAN; CHECK(!project_save("bad-project.hbt",&p));
    /* Fractional playback loops retrigger at the start, independent of block size. */
    project_default(&p); memset(p.notes,0,sizeof p.notes); p.volume[0]=p.master=1; p.route[0]=0;
    CHECK(note_add(&p,0,0,.5f,60,.5f)); p.notes[0][0][0].velocity=127;
    player_reset(&a); player_seek(&a,&p,.5f); a.loop_start=.5f; a.loop_end=1.5f;
    render(&a,&p,exact,whole,6001);
    CHECK(a.frame==3001 && fabsf(whole[0]-tanhf(.25f))<.00001f);
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
    CHECK(project_save("test-project.hbt",&p) && project_load("test-project.hbt",&roundtrip) && roundtrip.swing==1);
    p.swing=NAN; CHECK(!project_save("bad-project.hbt",&p));
    p.swing=0; CHECK(legacy_save("legacy.hbt",&p,15)); roundtrip.swing=1;
    CHECK(project_load("legacy.hbt",&roundtrip) && roundtrip.swing==0);
    project_default(&p); CHECK(p.master==1 && p.insert_volume[0]==1 && p.insert_volume[99]==1);
    for(int ch=0;ch<CHANNELS;ch++) CHECK(p.volume[ch]==1);
    CHECK(fader_gain(0)==0 && fabsf(fader_gain(1)-MIXER_GAIN_MAX)<.00001f);
    CHECK(fabsf(fader_gain(fader_position(1))-1)<.00001f && fader_position(1)==.75f);
    const float gains[]={.01f,.25f,.5f,1,1.5f,2};
    for(unsigned i=0;i<sizeof gains/sizeof *gains;i++) CHECK(fabsf(fader_gain(fader_position(gains[i]))-gains[i])<.00001f);
    CHECK(gain_db(1)==0 && gain_db(2)>6);
    for(int version=16;version<=18;version++) {
        p.master=version>=18?1.5f:1; p.swing=.35f;
        CHECK(legacy_save("legacy.hbt",&p,version) && project_load("legacy.hbt",&q));
        CHECK(q.master==p.master && q.swing==(version>=17?p.swing:0));
        CHECK(!strcmp(q.audio_io[0][1],"@default") && !q.audio_io[1][0][0]);
        CHECK(q.effect_mix[1][0]==1 && !q.effect_bypass[1][0] && !strcmp(q.track_names[0],"Track 1"));
    }
    p.master=2.1f; CHECK(legacy_save("legacy.hbt",&p,18));
    CHECK(!project_load("legacy.hbt",&q) && q.master==1.5f);
    p.master=1.5f; p.insert_volume[0]=2;
    snprintf(p.audio_io[0][1],128,"USB output"); snprintf(p.audio_io[1][0],128,"USB input");
    p.effect_mix[1][0]=.25f; p.effect_bypass[1][0]=1;
    snprintf(p.track_names[0],PATTERN_NAME,"Drums");
    snprintf(p.insert_names[0],PATTERN_NAME,"Drum bus");
    p.pattern_colors[0]=pattern_palette[5];
    CHECK(project_save("test-project.hbt",&p) && project_load("test-project.hbt",&q));
    CHECK(q.effect_mix[1][0]==.25f && q.effect_bypass[1][0]==1 && !strcmp(q.track_names[0],"Drums"));
    CHECK(!strcmp(q.insert_names[0],"Drum bus") && !strcmp(q.insert_names[99],"Insert 100"));
    for(int version=19;version<=27;version++) {
        FILE *current=fopen("test-project.hbt","r"),*old=fopen("recent-legacy.hbt","w"); CHECK(current && old);
        char line[2048]; int lines=0; while(fgets(line,sizeof line,current)) lines++;
        int omitted=(version<27?LANES*CLIPS:0)+(version<26?CHANNELS:0)+(version<25?CHANNELS:0)+(version<23?CHANNELS:0)+(version<22?PATTERNS:0)+(version<21?INSERTS:0)+(version<20?LANES+(INSERTS+1)*10:0);
        rewind(current); fprintf(old,"HOMEBEAT %d\n",version); CHECK(fgets(line,sizeof line,current));
        for(int i=1;i<lines-omitted;i++) { CHECK(fgets(line,sizeof line,current)); fputs(line,old); }
        fclose(current); fclose(old);
        Project previous; CHECK(project_load("recent-legacy.hbt",&previous));
        CHECK(previous.clip_offsets[0][0]==0);
        CHECK(!strcmp(previous.insert_names[0],version>=21?"Drum bus":"Insert 1"));
        CHECK(!strcmp(previous.track_names[0],version>=20?"Drums":"Track 1"));
        CHECK(previous.effect_mix[1][0]==(version>=20?.25f:1));
        CHECK(previous.pattern_colors[0]==pattern_palette[version>=22?5:0]);
        CHECK(previous.channel_audio[0]==0 && previous.audio_seconds[0]==0);
        CHECK(!strcmp(previous.audio_io[0][1],"USB output"));
        remove("recent-legacy.hbt");
    }
    CHECK(q.pattern_colors[0]==pattern_palette[5] && q.pattern_colors[1]==pattern_palette[1]);
    CHECK(q.effect_mix[0][0]==1 && q.effect_bypass[0][0]==0);
    p.pattern_colors[0]=0x1000000; CHECK(!project_save("bad-project.hbt",&p)); p.pattern_colors[0]=pattern_palette[5];
    p.effect_mix[1][0]=NAN; CHECK(!project_save("bad-project.hbt",&p)); p.effect_mix[1][0]=.25f;
    p.effect_bypass[1][0]=2; CHECK(!project_save("bad-project.hbt",&p)); p.effect_bypass[1][0]=1;
    CHECK(q.master==1.5f && q.insert_volume[0]==2 && !strcmp(q.audio_io[0][1],"USB output") && !strcmp(q.audio_io[1][0],"USB input"));
    CHECK(insert_reset(&q,1) && q.effect_mix[1][0]==1 && q.effect_bypass[1][0]==0);
    CHECK(!strcmp(q.insert_names[0],"Drum bus"));
    p.insert_names[1][0]='\n'; CHECK(!project_save("bad-project.hbt",&p)); snprintf(p.insert_names[1],PATTERN_NAME,"Insert 2");
    p.audio_io[1][0][0]='\n'; CHECK(!project_save("bad-project.hbt",&p)); p.audio_io[1][0][0]='U';
    p.insert_volume[0]=2.1f; CHECK(!project_save("bad-project.hbt",&p));
    for(int ch=0;ch<CHANNELS;ch++) free(s[ch].data);
    remove("test-project.hbt"); remove("legacy.hbt"); remove("bad-project.hbt"); remove("test-export.wav");
    p.insert_volume[0]=2; p.channel_pitch[0]=-.5f; p.pitch_range[0]=48;
    CHECK(project_save("pitch.hbt",&p) && project_load("pitch.hbt",&q));
    CHECK(q.channel_pitch[0]==-.5f && q.pitch_range[0]==48); remove("pitch.hbt");
    p.pitch_range[0]=1.5f; CHECK(!project_save("bad-project.hbt",&p)); p.pitch_range[0]=2;
    /* Boosted channel volume survives save/load and reaches the audio renderer. */
    project_default(&p); p.volume[0]=VOLUME_KNOB_MAX; p.route[0]=0;
    CHECK(project_save("boost.hbt",&p) && project_load("boost.hbt",&q));
    CHECK(q.volume[0]==VOLUME_KNOB_MAX);
    float quiet[]={.1f}; Sample boosted[CHANNELS]={{quiet,1}}; float boosted_pcm[2];
    memset(p.notes,0,sizeof p.notes); CHECK(note_add(&p,0,0,0,60,0)); p.notes[0][0][0].velocity=127;
    player_reset(&a); render(&a,&p,boosted,boosted_pcm,1);
    CHECK(fabsf(boosted_pcm[0]-tanhf(.1f*VOLUME_KNOB_MAX))<1e-6);
    p.volume[0]=VOLUME_KNOB_MAX+.01f; CHECK(!project_save("invalid-boost.hbt",&p));
    p.volume[0]=NAN; CHECK(!project_save("invalid-boost.hbt",&p));
    p.volume[0]=-1; CHECK(!project_save("invalid-boost.hbt",&p));
    remove("boost.hbt"); remove("invalid-boost.hbt");
    puts("Project versions 1-12, master pitch, clip lengths, extended patterns, chords and gates, routing, render consistency, mute and WAV duration passed."); return 0;
}
