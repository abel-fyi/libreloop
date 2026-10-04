// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Twelve evenly spaced rainbow hues, in matched light and dark rows.
// Each row shares OKLCH lightness and chroma to keep visual weight balanced.
const uint32_t pattern_palette[COLOR_COUNT]={
    0xd49e99,0xcfa487,0xc1ab7f,0xacb384,0x95b995,0x82bcac,0x7cbac2,0x86b5d2,0x9aaed8,0xb0a6d3,0xc4a0c5,0xd09db0,
    0x855450,0x80593f,0x746036,0x62683b,0x4b6d4c,0x366f61,0x2f6e75,0x3c6983,0x516389,0x665c85,0x765678,0x815465
};
float grid_interval(float pixels) {
    TimelineGrid grid=timeline_grid_layout(pixels);
    return grid.lines>0?grid.lines:grid.labels;
}
float snap_floor(float value,float interval) { return interval>0?floorf(value/interval)*interval:value; }
float snap_round(float value,float interval) { return interval>0?roundf(value/interval)*interval:value; }
void timeline_zoom(float *span,float *start,float wheel,float anchor,float unit) {
    (void)unit;
    float next=*span*expf(-wheel*logf(1.08f));
    if(!isfinite(next) || next<=0 || !isfinite(1/next)) return;
    *start=fmaxf(0,*start+anchor*(*span-next)); *span=next;
}
TimelineGrid timeline_grid_layout(float pixels) {
    TimelineGrid grid={0,STEPS,0};
    if(!isfinite(pixels) || pixels<=0) return grid;
    float bar=STEPS*pixels;
    while(grid.labels*pixels+.001f<30 && grid.labels<1e30f) grid.labels*=2;
    if(bar+.001f>=12) {
        grid.lines=.25f;
        while(grid.lines*pixels+.001f>=32) grid.lines*=.5f;
        while(grid.lines*pixels+.001f<16 && grid.lines<STEPS) grid.lines*=2;
    }
    /* Four-bar blocks survive after fine lines disappear, then fade at overview scale. */
    grid.band_alpha=.09f*fminf(1,fmaxf(0,(bar-2)/2));
    return grid;
}
float timeline_thumb(float width,float span,float range) { return fminf(width,fmaxf(24,width*span/range)); }
float gain_db(float gain) { return gain>0?20*log10f(gain):-INFINITY; }
/* Reserve the top quarter of fader travel for boost; bottom is silence. */
float fader_position(float gain) {
    if(gain<=0) return 0;
    float db=gain_db(gain);
    return fmaxf(0,fminf(1,gain<=1?.75f*(db+60)/60:.75f+.25f*db/gain_db(MIXER_GAIN_MAX)));
}
float fader_gain(float position) {
    if(position<=0) return 0;
    position=fminf(1,position);
    float db=position<=.75f?60*(position/.75f-1):(position-.75f)/.25f*gain_db(MIXER_GAIN_MAX);
    return fminf(MIXER_GAIN_MAX,powf(10,db/20));
}
void project_default(Project *p) {
    memset(p, 0, sizeof *p); p->bpm=120; p->master=1; p->master_width=1; p->pattern_count=1; p->channel_count=4; snprintf(p->audio_io[0][1],128,"@default");
    for(int pat=0;pat<PATTERNS;pat++) { p->pattern_colors[pat]=pattern_palette[pat]; p->pattern_steps[pat]=STEPS; snprintf(p->pattern_names[pat],PATTERN_NAME,"Pattern %d",pat+1); }
    for(int id=0;id<=INSERTS;id++) for(int slot=0;slot<10;slot++) p->effect_mix[id][slot]=1;
    for(int l=0;l<LANES;l++) snprintf(p->track_names[l],PATTERN_NAME,"Track %d",l+1);
    const char *names[]={"Kick","Snare","Hat","Tone"};
    for(int c=0;c<CHANNELS;c++) { p->pitch_range[c]=2; p->volume[c]=1; p->sampler[c].time=p->sampler[c].length=1; if(c<4) snprintf(p->channel_names[c],PATTERN_NAME,"%s",names[c]); else snprintf(p->channel_names[c],PATTERN_NAME,"Channel %d",c+1); }
    for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) p->clip_starts[l][b]=b;
    for(int i=0;i<INSERTS;i++) snprintf(p->insert_names[i],PATTERN_NAME,"Insert %d",i+1);
    p->insert_count=INSERTS;
    for(int i=0;i<INSERTS;i++) p->insert_volume[i]=p->insert_width[i]=1;
    for(int c=0;c<4;c++) p->route[c]=c+1;
    for(int pat=0;pat<1;pat++) for(int i=0;i<STEPS;i++) {
        if(i%4==0) p->notes[pat][0][i]=(Note){60,100,i,0};
        if(i%8==4) p->notes[pat][1][i]=(Note){60,100,i,0};
        if(i%2==0) p->notes[pat][2][i]=(Note){60,70,i,0};
        if(pat && i==15) p->notes[pat][1][i]=(Note){60,75,i,0};
    }
}
/* Keep legacy defaults for old project formats; new sessions start empty. */
void project_new(Project *p) {
    project_default(p); memset(p->notes,0,sizeof p->notes); p->channel_count=1;
    memset(p->route,0,sizeof p->route);
    for(int c=0;c<CHANNELS;c++) {
        snprintf(p->paths[c],sizeof p->paths[c],"%s",SAMPLE_EMPTY);
        snprintf(p->channel_names[c],PATTERN_NAME,"Channel %d",c+1);
    }
    snprintf(p->channel_names[0],PATTERN_NAME,"Sampler");
}
void project_demo(Project *p) {
    project_default(p); p->bpm=110; p->pattern_count=2; p->pattern_steps[1]=32;
    snprintf(p->pattern_names[0],PATTERN_NAME,"Intro"); snprintf(p->pattern_names[1],PATTERN_NAME,"Groove");
    snprintf(p->channel_names[3],PATTERN_NAME,"Bass"); snprintf(p->track_names[0],PATTERN_NAME,"Demo groove");
    for(int c=0;c<CHANNELS;c++) {
        if(c<4) p->volume[c]=c==2?.18f:.35f;
        else snprintf(p->paths[c],sizeof p->paths[c],"%s",SAMPLE_EMPTY);
    }
    for(int c=0;c<3;c++) for(int n=0;n<16;n++) if(p->notes[0][c][n].velocity) {
        p->notes[1][c][n]=p->notes[0][c][n]; p->notes[1][c][n+16]=p->notes[0][c][n]; p->notes[1][c][n+16].start+=16;
    }
    const int pitches[]={60,60,67,63,65,65,67,58};
    for(int n=0;n<8;n++) p->notes[1][3][n]=(Note){pitches[n],85,n*4,3};
    for(int b=0;b<8;b+=b<2?1:2) { p->clips[0][b]=b<2?1:2; p->clip_steps[0][b]=b<2?16:32; }
}
int channel_delete(Project *p,int c) {
    if(c<0 || c>=p->channel_count) return 0;
    for(int i=c;i<p->channel_count-1;i++) {
        p->sampler[i]=p->sampler[i+1]; p->channel_pitch[i]=p->channel_pitch[i+1]; p->pitch_range[i]=p->pitch_range[i+1];
        p->channel_colors[i]=p->channel_colors[i+1]; p->channel_audio[i]=p->channel_audio[i+1]; p->audio_seconds[i]=p->audio_seconds[i+1];
        p->volume[i]=p->volume[i+1]; p->pan[i]=p->pan[i+1]; p->mute[i]=p->mute[i+1]; p->route[i]=p->route[i+1];
        memcpy(p->paths[i],p->paths[i+1],sizeof p->paths[i]);
        memmove(p->channel_names[i],p->channel_names[i+1],PATTERN_NAME);
        size_t length=strlen(p->channel_names[i]); memset(p->channel_names[i]+length,0,PATTERN_NAME-length);
        for(int pat=0;pat<PATTERNS;pat++) memcpy(p->notes[pat][i],p->notes[pat][i+1],sizeof p->notes[pat][i]);
    }
    for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) {
        int id=p->clips[l][b];
        if(id==PATTERNS+c+1) { p->clips[l][b]=0; p->clip_steps[l][b]=0; }
        else if(id>PATTERNS+c+1 && id<=AUTOMATION_SOURCE) p->clips[l][b]--;
    }
    for(int i=p->automation_count-1;i>=0;i--) {
        ParameterTarget *t=&p->automations[i].target;
        if(t->parameter==PARAM_CHANNEL_VOLUME || t->parameter==PARAM_CHANNEL_PAN || t->parameter==PARAM_CHANNEL_PITCH || t->parameter==PARAM_CHANNEL_MUTE || t->parameter==PARAM_PITCH_RANGE) {
            if(t->owner==(unsigned)c) automation_delete(p,i);
            else if(t->owner>(unsigned)c) t->owner--;
        }
    }
    int last=--p->channel_count; p->channel_colors[last]=0; p->channel_pitch[last]=0; p->pitch_range[last]=2; p->channel_audio[last]=0; p->audio_seconds[last]=0; p->sampler[last]=(Sampler){.time=1,.length=1}; p->volume[last]=1; p->pan[last]=0; p->mute[last]=p->route[last]=0;
    memset(p->paths[last],0,sizeof p->paths[last]); memset(p->channel_names[last],0,PATTERN_NAME); snprintf(p->channel_names[last],PATTERN_NAME,"Channel %d",last+1);
    for(int pat=0;pat<PATTERNS;pat++) memset(p->notes[pat][last],0,sizeof p->notes[pat][last]);
    return 1;
}
int pattern_delete(Project *p,int pattern) {
    if(pattern<0 || pattern>=p->pattern_count) return 0;
    for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) {
        if(p->clips[l][b]==pattern+1) { p->clips[l][b]=0; p->clip_steps[l][b]=0; }
        else if(p->clips[l][b]>pattern+1 && p->clips[l][b]<=PATTERNS) p->clips[l][b]--;
    }
    for(int i=pattern;i<p->pattern_count-1;i++) {
        memcpy(p->notes[i],p->notes[i+1],sizeof p->notes[i]);
        memcpy(p->pattern_names[i],p->pattern_names[i+1],PATTERN_NAME);
        p->pattern_steps[i]=p->pattern_steps[i+1]; p->pattern_colors[i]=p->pattern_colors[i+1];
    }
    int last=p->pattern_count>1?--p->pattern_count:0;
    memset(p->notes[last],0,sizeof p->notes[last]); p->pattern_steps[last]=STEPS; p->pattern_colors[last]=pattern_palette[last];
    snprintf(p->pattern_names[last],PATTERN_NAME,"Pattern %d",last+1);
    return 1;
}
int insert_reset(Project *p,int id) {
    if(id<1 || id>p->insert_count) return 0;
    p->insert_volume[id-1]=p->insert_width[id-1]=1; p->insert_pan[id-1]=0; p->insert_mute[id-1]=0; p->insert_output[id-1]=0;
    memset(p->audio_io[id],0,sizeof p->audio_io[id]);
    for(int slot=0;slot<10;slot++) { p->effect_mix[id][slot]=1; p->effect_bypass[id][slot]=0; }
    return 1;
}
int insert_connect(Project *p,int source,int destination) {
    if(source<1 || source>p->insert_count || destination<0 || (destination>p->insert_count && destination!=255)) return 0;
    int next=destination;
    for(int hops=0;next && next!=255;hops++) {
        if(next==source || next>p->insert_count || hops>=INSERTS) return 0;
        next=p->insert_output[next-1];
    }
    p->insert_output[source-1]=destination; return 1;
}
Note *note_at(Project *p,int pat,int channel,float start,int pitch) {
    if(pat<0 || pat>=PATTERNS || channel<0 || channel>=CHANNELS) return NULL;
    for(int i=0;i<NOTES;i++) { Note *n=&p->notes[pat][channel][i]; if(n->velocity && fabsf(n->start-start)<.00001f && n->pitch==pitch) return n; }
    return NULL;
}
Note *note_add(Project *p,int pat,int channel,float start,int pitch,float length) {
    if(pat<0 || pat>=PATTERNS || channel<0 || channel>=CHANNELS || !isfinite(start) || !isfinite(length) || start<0 || start>=p->pattern_steps[pat] || pitch<0 || pitch>127 || length<0 || length>p->pattern_steps[pat]-start) return NULL;
    Note *n=note_at(p,pat,channel,start,pitch); if(n) return n;
    for(int i=0;i<NOTES;i++) if(!p->notes[pat][channel][i].velocity) {
        n=&p->notes[pat][channel][i]; *n=(Note){pitch,100,start,length}; return n;
    }
    return NULL;
}
int note_move(Project *p,int pat,int channel,Note *note,float start,int pitch,float limit) {
    if(pat<0 || pat>=PATTERNS || channel<0 || channel>=p->channel_count || !note || !note->velocity || limit>p->pattern_steps[pat]) return 0;
    float length=note->length?note->length:1;
    if(!isfinite(start) || !isfinite(limit) || start<0 || start+length>limit || pitch<0 || pitch>127) return 0;
    Note *other=note_at(p,pat,channel,start,pitch); if(other && other!=note) return 0;
    note->start=start; note->pitch=pitch; return 1;
}
int notes_move(Project *p,int pat,int channel,const Note before[NOTES],const uint8_t selected[NOTES],float dx,int dy,float limit) {
    if(pat<0 || pat>=PATTERNS || channel<0 || channel>=p->channel_count || !isfinite(dx) || !isfinite(limit) || limit>p->pattern_steps[pat]) return 0;
    for(int i=0;i<NOTES;i++) if(selected[i]) {
        Note n=before[i]; float start=n.start+dx; int pitch=n.pitch+dy;
        if(!n.velocity || start<0 || start+(n.length?n.length:1)>limit || pitch<0 || pitch>127) return 0;
        for(int j=0;j<NOTES;j++) if(!selected[j]) {
            Note other=p->notes[pat][channel][j];
            if(other.velocity && other.pitch==pitch && fabsf(other.start-start)<.00001f) return 0;
        }
    }
    for(int i=0;i<NOTES;i++) if(selected[i]) { p->notes[pat][channel][i]=before[i]; p->notes[pat][channel][i].start+=dx; p->notes[pat][channel][i].pitch+=dy; }
    return 1;
}
void samples_default(Sample s[CHANNELS]) {
    uint32_t seed=1; memset(s,0,CHANNELS*sizeof *s);
    for(int c=0;c<4;c++) {
        s[c].frames=RATE/2; s[c].data=calloc(s[c].frames,sizeof(float));
        if(!s[c].data) { s[c].frames=0; continue; }
        double phase=0;
        for(unsigned i=0;i<s[c].frames;i++) {
            double t=(double)i/RATE;
            seed=seed*1664525u+1013904223u;
            double noise=(double)(seed>>8)/8388608.0-1;
            phase+=6.28318530718*(c==0?48+110*exp(-t*35):c==3?130.8128:180)/RATE;
            double x=c==0?sin(phase)*exp(-t*12):c==1?(noise*.8+sin(phase)*.2)*exp(-t*22):c==2?noise*exp(-t*65):sin(phase)*exp(-t*8);
            s[c].data[i]=(float)(x*fmin(t*1000,1));
        }
    }
}
void player_reset(Player *p) { memset(p,0,sizeof *p); p->last_step=-1; }
void player_seek(Player *p,const Project *pr,float step) {
    p->start_step=isfinite(step)?fmaxf(0,step):0;
    p->frame=(uint64_t)llround(p->start_step*RATE*60.0/pr->bpm/4); p->last_step=-1; p->clock_bpm=pr->bpm;
    memset(p->voices,0,sizeof p->voices);
}
float channel_speed(const Project *p,int channel) { return p->channel_pitch[channel]?powf(2,p->channel_pitch[channel]*p->pitch_range[channel]/12):1; }
float clip_source_steps(const Project *p,int source) {
    if(source<0 || source>=SOURCES) return 0;
    if(source>=AUTOMATION_SOURCE) return source-AUTOMATION_SOURCE<p->automation_count?p->automations[source-AUTOMATION_SOURCE].steps:0;
    return source<PATTERNS?p->pattern_steps[source]:p->audio_seconds[source-PATTERNS]*audio_source_bpm(p,source-PATTERNS)/15/channel_speed(p,source-PATTERNS);
}
float clip_offset_steps(const Project *p,int lane,int clip) {
    int source=p->clips[lane][clip]-1;
    float offset=p->clip_offsets[lane][clip];
    return AUDIO_SOURCE(source)?offset*audio_source_bpm(p,source-PATTERNS)/15/channel_speed(p,source-PATTERNS):offset;
}
float clip_length(const Project *p,int lane,int bar) {
    float length=p->clip_steps[lane][bar]; int source=p->clips[lane][bar]-1;
    if(AUDIO_SOURCE(source)) return audio_clip_steps(p,lane,bar);
    return length?length:STEPS;
}
float song_steps(const Project *p) {
    float end=STEPS;
    for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) if(p->clips[l][b] && p->clip_starts[l][b]*STEPS+clip_length(p,l,b)>end) end=p->clip_starts[l][b]*STEPS+clip_length(p,l,b);
    return end;
}
int solo_any(const uint8_t *states,int count) {
    for(int i=0;i<count;i++) if(states[i]&2) return 1;
    return 0;
}
void solo_toggle(uint8_t *states,int count,int selected) {
    if(selected<0 || selected>=count) return;
    if(states[selected]&2) states[selected]&=~2;
    else states[selected]=(states[selected]&~1)|2;
}
static void trigger(Player *p,const Project *pr,int pat,int64_t tick,float remaining,int lane,int entering,float swing,const float *channel_speeds,double master_speed,const uint8_t *channel_mutes) {
    for(int c=0;c<pr->channel_count;c++) for(int i=0;i<NOTES;i++) {
        Note n=pr->notes[pat][c][i];
        if(!n.velocity || (channel_mutes[c]&1)) continue;
        double start=n.start;
        if(swing>0) {
            /* Warp each two-step pair without changing its duration or note order. */
            double pair=floor(start/2)*2,phase=start-pair,delay=swing*.5;
            start=pair+(phase<1?phase*(1+delay):1+delay+(phase-1)*(1-delay));
        }
        int64_t onset=llround(start*96.0);
        double elapsed=(tick-onset)/96.0;
        if(onset!=tick && !(entering && n.length && elapsed>0 && elapsed<n.length)) continue;
        int v=0; while(v<127 && p->voices[v].gain!=0) v++;
        p->channel_trigger[c]=1;
        if(lane>=0 && pr->volume[c]>0) p->lane_trigger[lane]=1;
        double speed=pow(2,((int)n.pitch-60)/12.0),frames_per_step=RATE*60.0/pr->bpm/4;
        p->voices[v]=(Voice){.channel=c,.position=elapsed*frames_per_step*speed*channel_speeds[c]*master_speed*(pr->sampler[c].fit_bpm?pr->bpm/pr->sampler[c].fit_bpm:1),.speed=speed,.remaining=n.length?fmin(n.length-elapsed,remaining)*frames_per_step:-1,.gain=n.velocity/127.f,.lane=lane};
    }
}
static int active_voices(Player *p,Player *live,Voice *active[256]) {
    int count=0;
    for(int v=0;v<128;v++) if(p->voices[v].gain) active[count++]=&p->voices[v];
    if(live) for(int v=0;v<128;v++) if(live->voices[v].gain) active[count++]=&live->voices[v];
    return count;
}
static void render_audio(Player *p,Player *live,const Project *pr,const Sample s[CHANNELS],float *out,unsigned frames,int sequence,float peaks[INSERTS+1][2],const MixerIO *io) {
    memset(p->channel_active,0,sizeof p->channel_active); memset(p->channel_trigger,0,sizeof p->channel_trigger);
    memset(p->lane_active,0,sizeof p->lane_active); memset(p->lane_trigger,0,sizeof p->lane_trigger);
    int tempo_changed=sequence && p->clock_bpm && p->clock_bpm!=pr->bpm;
    if(tempo_changed) {
        double ratio=p->clock_bpm/pr->bpm;
        p->frame=llround(p->frame*ratio);
        for(int v=0;v<128;v++) if(p->voices[v].gain && p->voices[v].remaining>=0) p->voices[v].remaining*=ratio;
    }
    p->clock_bpm=pr->bpm;
    int resync=sequence && p->song && p->audio_resync;
    p->audio_resync=0;
    if(resync || tempo_changed) for(int v=0;v<128;v++) if(p->voices[v].audio_clip && (resync || !pr->sampler[p->voices[v].channel].fit_bpm)) p->voices[v].gain=0;
    Voice *active[256]; int count=active_voices(p,live,active);
    if(!sequence && !count && !io) { p->frame+=frames; return; }
    int loop=isfinite(p->loop_start) && isfinite(p->loop_end) && p->loop_start>=0 && p->loop_end>p->loop_start;
    float begin=loop?p->loop_start:p->song?0:p->start_step;
    float lengths[LANES][CLIPS],song_end=STEPS;
    if(sequence && p->song) for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) if(pr->clips[l][b]) {
        lengths[l][b]=clip_length(pr,l,b); song_end=fmaxf(song_end,pr->clip_starts[l][b]*STEPS+lengths[l][b]);
    }
    float end=loop?p->loop_end:p->song?song_end:pr->pattern_steps[p->pattern];
    if(end<=begin) begin=0;
    double stepframes=RATE*60.0/pr->bpm/4/96,pitch_speed=pow(2,pr->master_pitch/12.0);
    uint64_t begin_frame=(uint64_t)llround(begin*stepframes*96),end_frame=(uint64_t)llround(end*stepframes*96);
    float gain_left[CHANNELS],gain_right[CHANNELS],speed[CHANNELS];
    uint8_t channel_mutes[CHANNELS]; memcpy(channel_mutes,pr->mute,sizeof channel_mutes);
    int channel_solo=solo_any(pr->mute,pr->channel_count),insert_solo=solo_any(pr->insert_mute,pr->insert_count);
    int lane_solo=solo_any(pr->lane_mute,LANES),lane_enabled[LANES];
    for(int l=0;l<LANES;l++) lane_enabled[l]=!(pr->lane_mute[l]&1) && (!lane_solo || (pr->lane_mute[l]&2));
    /* Sort only connected buses once per block, so shared buses are processed once. */
    int used[INSERTS+1]={1},pending[INSERTS+1]={0},order[INSERTS+1],buses_count=0;
    uint8_t input_audible[INSERTS+1]; memset(input_audible,1,sizeof input_audible);
    if(io) for(int bus=0;bus<=pr->insert_count;bus++) if(io->active[bus]) {
        int id=bus,hops=0,audible=!insert_solo;
        while(id && id!=255 && hops++<INSERTS) { used[id]=1; audible|=pr->insert_mute[id-1]&2; id=pr->insert_output[id-1]; }
        input_audible[bus]=audible;
    }
    for(int c=0;c<pr->channel_count;c++) {
        speed[c]=channel_speed(pr,c);
        int id=pr->route[c],hops=0,audible=!insert_solo;
        while(id && id!=255 && hops++<INSERTS) {
            used[id]=1; audible|=pr->insert_mute[id-1]&2; id=pr->insert_output[id-1];
        }
        float gain=(pr->mute[c]&1) || (channel_solo && !(pr->mute[c]&2)) || !audible?0:pr->volume[c];
        gain_left[c]=gain*fminf(1,1-pr->pan[c]); gain_right[c]=gain*fminf(1,1+pr->pan[c]);
    }
    for(int id=1;id<=pr->insert_count;id++) if(used[id] && pr->insert_output[id-1]!=255) pending[pr->insert_output[id-1]]++;
    for(int id=0;id<=pr->insert_count;id++) if(used[id] && !pending[id]) order[buses_count++]=id;
    for(int at=0;at<buses_count;at++) {
        int id=order[at]; if(!id) continue;
        int dest=pr->insert_output[id-1];
        if(dest!=255 && !--pending[dest]) order[buses_count++]=dest;
    }
    float bus_left[INSERTS+1],bus_right[INSERTS+1],bus_width[INSERTS+1];
    bus_left[0]=bus_right[0]=pr->master_mute?0:pr->master; bus_width[0]=pr->master_width;
    for(int id=1;id<=pr->insert_count;id++) {
        int i=id-1; float gain=(pr->insert_mute[i]&1)?0:pr->insert_volume[i],pan=pr->insert_pan[i];
        bus_left[id]=gain*fminf(1,1-pan); bus_right[id]=gain*fminf(1,1+pan); bus_width[id]=pr->insert_width[i];
    }
    /* Bind once per buffer. Only automated controls are recomputed per sample;
       no project copies, allocation, locks or sample rebuilding in this path. */
    struct { int automation; float start,end,offset; } automation_clips[LANES*CLIPS];
    int automation_clips_count=0;
    float channel_controls[CHANNELS][5],insert_controls[INSERTS][4],master_controls[5]={pr->master,pr->master_width,pr->master_pitch,pr->master_mute,pr->swing};
    float *bindings[AUTOMATIONS]={0},baseline[AUTOMATIONS],low[AUTOMATIONS],high[AUTOMATIONS];
    uint8_t channel_changed[CHANNELS]={0},insert_changed[INSERTS]={0}; int master_changed=0,master_pitch_changed=0;
    uint8_t channel_pitch_changed[CHANNELS]={0};
    if(sequence && p->song && pr->automation_count) {
        for(int c=0;c<pr->channel_count;c++) { channel_controls[c][0]=pr->volume[c]; channel_controls[c][1]=pr->pan[c]; channel_controls[c][2]=pr->channel_pitch[c]; channel_controls[c][3]=!!(pr->mute[c]&1); channel_controls[c][4]=pr->pitch_range[c]; }
        for(int i=0;i<pr->insert_count;i++) { insert_controls[i][0]=pr->insert_volume[i]; insert_controls[i][1]=pr->insert_pan[i]; insert_controls[i][2]=pr->insert_width[i]; insert_controls[i][3]=!!(pr->insert_mute[i]&1); }
        for(int a=0;a<pr->automation_count;a++) {
            ParameterTarget t=pr->automations[a].target; unsigned id=t.parameter,c=t.owner;
            if(!parameter_info(pr,t,&baseline[a],&low[a],&high[a])) continue;
            if(id<=PARAM_CHANNEL_PITCH || id==PARAM_CHANNEL_MUTE || id==PARAM_PITCH_RANGE) { bindings[a]=&channel_controls[c][id==PARAM_CHANNEL_MUTE?3:id==PARAM_PITCH_RANGE?4:id-PARAM_CHANNEL_VOLUME]; channel_changed[c]=1; if(id==PARAM_CHANNEL_PITCH || id==PARAM_PITCH_RANGE) channel_pitch_changed[c]=1; }
            else if(id<=PARAM_INSERT_WIDTH || id==PARAM_INSERT_MUTE) { bindings[a]=&insert_controls[c][id==PARAM_INSERT_MUTE?3:id-PARAM_INSERT_VOLUME]; insert_changed[c]=1; }
            else { bindings[a]=&master_controls[id==PARAM_MASTER_MUTE?3:id==PARAM_SWING?4:id-PARAM_MASTER_VOLUME]; master_changed=1; if(id==PARAM_MASTER_PITCH) master_pitch_changed=1; }
        }
        for(int l=0;l<LANES;l++) if(lane_enabled[l]) for(int b=0;b<CLIPS;b++) if(pr->clips[l][b]>AUTOMATION_SOURCE) {
            int a=pr->clips[l][b]-AUTOMATION_SOURCE-1;
            if(a>=pr->automation_count || !bindings[a]) continue;
            int n=automation_clips_count++;
            automation_clips[n].automation=a; automation_clips[n].start=pr->clip_starts[l][b]*STEPS;
            automation_clips[n].end=automation_clips[n].start+clip_length(pr,l,b); automation_clips[n].offset=pr->clip_offsets[l][b];
        }
    }
    float buses[INSERTS+1][2];
    for(unsigned f=0;f<frames;f++,p->frame++) {
        if(live) live->frame++;
        int64_t step=(int64_t)(p->frame/stepframes);
        if(sequence && !io && p->frame>=end_frame) { p->frame=begin_frame; step=(int64_t)(p->frame/stepframes); p->last_step=-1; if(loop) memset(p->voices,0,sizeof p->voices); }
        if(automation_clips_count) {
            double position=p->frame/(stepframes*96);
            for(int a=0;a<pr->automation_count;a++) if(bindings[a]) *bindings[a]=baseline[a];
            double held_end[AUTOMATIONS]; for(int a=0;a<pr->automation_count;a++) held_end[a]=-INFINITY;
            for(int n=0;n<automation_clips_count;n++) if(position>=automation_clips[n].end) {
                int a=automation_clips[n].automation;
                if(automation_clips[n].end<held_end[a]) continue;
                *bindings[a]=low[a]+(high[a]-low[a])*automation_value(&pr->automations[a],automation_clips[n].end-automation_clips[n].start+automation_clips[n].offset);
                for(int i=0;i<pr->automation_count;i++) if(bindings[i]==bindings[a]) held_end[i]=automation_clips[n].end;
            }
            /* Active clips override held values; later tracks/slots win overlaps. */
            for(int n=0;n<automation_clips_count;n++) if(position>=automation_clips[n].start && position<automation_clips[n].end) {
                int a=automation_clips[n].automation;
                *bindings[a]=low[a]+(high[a]-low[a])*automation_value(&pr->automations[a],position-automation_clips[n].start+automation_clips[n].offset);
            }
            for(int c=0;c<pr->channel_count;c++) if(channel_changed[c]) {
                float gain=channel_controls[c][0],pan=channel_controls[c][1]; int audible=!insert_solo,id=pr->route[c],hops=0;
                while(id && id!=255 && hops++<INSERTS) { audible|=pr->insert_mute[id-1]&2; id=pr->insert_output[id-1]; }
                channel_mutes[c]=(pr->mute[c]&~1)|(channel_controls[c][3]>=.5f);
                if(channel_controls[c][3]>=.5f || (channel_solo && !(pr->mute[c]&2)) || !audible) gain=0;
                gain_left[c]=gain*fminf(1,1-pan); gain_right[c]=gain*fminf(1,1+pan);
                if(channel_pitch_changed[c]) speed[c]=pow(2,channel_controls[c][2]*roundf(channel_controls[c][4])/12);
            }
            for(int i=0;i<pr->insert_count;i++) if(insert_changed[i]) {
                float gain=insert_controls[i][3]>=.5f?0:insert_controls[i][0],pan=insert_controls[i][1];
                bus_left[i+1]=gain*fminf(1,1-pan); bus_right[i+1]=gain*fminf(1,1+pan); bus_width[i+1]=insert_controls[i][2];
            }
            if(master_changed) { bus_left[0]=bus_right[0]=master_controls[3]>=.5f?0:master_controls[0]; bus_width[0]=master_controls[1]; if(master_pitch_changed) pitch_speed=pow(2,master_controls[2]/12); }
        }
        if(sequence && (step!=p->last_step || resync || tempo_changed)) {
            int tick=step!=p->last_step,entered=p->last_step<0; p->last_step=step;
            if(p->song) {
                for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) {
                    int pat=pr->clips[l][b]; if(!pat || pat>AUTOMATION_SOURCE || (pat<=PATTERNS && !lane_enabled[l])) continue;
                    int64_t local=step-(int64_t)llround(pr->clip_starts[l][b]*STEPS*96.0); float length=lengths[l][b];
                    if(pat>PATTERNS) {
                        int c=pat-PATTERNS-1;
                        double song_position=p->frame/(stepframes*96),clip_start=pr->clip_starts[l][b]*STEPS;
                        int reconcile=resync || (tempo_changed && !pr->sampler[c].fit_bpm);
                        int inside=reconcile?(song_position>=clip_start && song_position<clip_start+length):(local>=0 && local<length*96);
                        if(inside && (reconcile || (tick && (local==0 || entered)))) {
                            int v=0; while(v<127 && p->voices[v].gain) v++;
                            double position=fmax(0,p->frame-pr->clip_starts[l][b]*STEPS*stepframes*96);
                            if(lane_enabled[l] && (gain_left[c]>0 || gain_right[c]>0)) p->lane_trigger[l]=1;
                            p->channel_trigger[c]=1;
                            p->voices[v]=(Voice){.channel=c,.position=audio_clip_position(pr,l,b,p->frame/(stepframes*96)),.speed=1,.remaining=fmin(length*stepframes*96-position,end_frame-p->frame),.gain=1,.lane=l,.audio_clip=1};
                        }
                    } else if(tick && local>=0 && local<length*96 && local+pr->clip_offsets[l][b]*96<pr->pattern_steps[pat-1]*96) trigger(p,pr,pat-1,local+(int64_t)llround(pr->clip_offsets[l][b]*96),fminf(length-local/96.f,end-step/96.f),l,pr->clip_offsets[l][b]>0 && (local==0 || entered),master_controls[4],speed,pitch_speed,channel_mutes);
                }
            } else trigger(p,pr,p->pattern,step,end-step/96.f,-1,0,master_controls[4],speed,pitch_speed,channel_mutes);
            count=active_voices(p,live,active); resync=tempo_changed=0;
        }
        for(int at=0;at<buses_count;at++) buses[order[at]][0]=buses[order[at]][1]=0;
        if(io && io->input) {
            io->input(io->context,buses);
            for(int id=0;id<=pr->insert_count;id++) if(!input_audible[id]) buses[id][0]=buses[id][1]=0;
        }
        for(int v=0;v<count;v++) {
            Voice *voice=active[v]; if(!voice->gain) continue;
            int c=voice->channel; if(c>=pr->channel_count || voice->position>=s[c].frames) { voice->gain=0; continue; }
            float gain=voice->gain;
            if(pr->sampler[c].fit_bpm) {
                double rate=voice->tempo_rate?voice->tempo_rate:pr->bpm/pr->sampler[c].fit_bpm;
                gain*=fmin(1,(s[c].frames-voice->position)/(rate*voice->speed*pitch_speed*speed[c]*RATE*.005));
            }
            if(voice->remaining>=0) {
                gain*=fminf(1,voice->remaining/(RATE*.005f));
                if(--voice->remaining<=0) voice->gain=0;
            }
            if(sequence && p->song && voice->lane>=0 && voice->lane<LANES && !lane_enabled[voice->lane]) gain=0;
            if(gain>0 && !pr->master_mute && (gain_left[c]>0 || gain_right[c]>0)) p->channel_active[c]=1;
            int id=pr->route[c];
            float stereo[2];
            double rate=pr->sampler[c].fit_bpm?pr->bpm/pr->sampler[c].fit_bpm:1;
            voice_tempo_sample(voice,s[c],voice->speed*pitch_speed*speed[c],rate,pr->sampler[c].fit_bpm && pr->sampler[c].stretch,stereo);
            for(unsigned side=0;side<2;side++) buses[id][side]+=stereo[side]*gain*(side?gain_right[c]:gain_left[c]);
        }
        for(int at=0;at<buses_count;at++) {
            int id=order[at]; float l=buses[id][0]*bus_left[id],r=buses[id][1]*bus_right[id];
            if(bus_width[id]!=1) { float mid=(l+r)*.5f,side=(l-r)*.5f*bus_width[id]; l=mid+side; r=mid-side; }
            if(io && io->output) io->output(io->context,id,l,r);
            if(id) {
                int dest=pr->insert_output[id-1]; if(dest!=255) { buses[dest][0]+=l; buses[dest][1]+=r; }
            } else {
                if(sequence) { out[f*2]=l; out[f*2+1]=r; }
                else { out[f*2]+=l; out[f*2+1]+=r; }
                /* Keep float headroom until the device/export boundary. */
            }
            if(peaks) { peaks[id][0]=fmaxf(peaks[id][0],fabsf(l)); peaks[id][1]=fmaxf(peaks[id][1],fabsf(r)); }
        }
    }
    for(int v=0;v<128;v++) {
        Voice voice=p->voices[v]; int l=voice.lane,c=voice.channel;
        if(voice.gain && l>=0 && l<LANES && c<pr->channel_count && voice.position<s[c].frames && lane_enabled[l] && (gain_left[c]>0 || gain_right[c]>0)) p->lane_active[l]=1;
    }
    for(int c=0;c<pr->channel_count;c++) if(pr->master_mute || !(gain_left[c]>0 || gain_right[c]>0) || !s[c].frames) p->channel_trigger[c]=0;
    for(int l=0;l<LANES;l++) if(!lane_enabled[l] || pr->master_mute) p->lane_active[l]=p->lane_trigger[l]=0;
}
void render(Player *p,const Project *pr,const Sample s[CHANNELS],float *out,unsigned frames) {
    render_audio(p,NULL,pr,s,out,frames,1,NULL,NULL);
}
void render_live(Player *p,const Project *pr,const Sample s[CHANNELS],float *out,unsigned frames) {
    render_audio(p,NULL,pr,s,out,frames,0,NULL,NULL);
}
void render_mixer(Player *p,Player *live,const Project *pr,const Sample s[CHANNELS],float *out,unsigned frames,int sequence,float peaks[INSERTS+1][2]) {
    render_audio(p,live,pr,s,out,frames,sequence,peaks,NULL);
}
void render_mixer_io(Player *p,Player *live,const Project *pr,const Sample s[CHANNELS],float *out,unsigned frames,int sequence,float peaks[INSERTS+1][2],const MixerIO *io) {
    render_audio(p,live,pr,s,out,frames,sequence,peaks,io);
}
/* Text format keeps projects inspectable and avoids ABI-dependent struct dumps. */
static int read_line(FILE *f,char *out,size_t capacity) {
    char line[1025];
    if(capacity>1024 || !fgets(line,(int)capacity+1,f)) return 0;
    char *newline=strchr(line,'\n'); if(!newline || strchr(line,'\r')) return 0;
    *newline=0; memset(out,0,capacity); memcpy(out,line,strlen(line)+1); return 1;
}
int project_save(const char *path,const Project *p) {
    if(!automation_valid(p)) return 0;
    if(p->channel_count<0 || p->channel_count>CHANNELS || p->insert_count<0 || p->insert_count>INSERTS) return 0;
    for(int c=0;c<CHANNELS;c++) if(!p->channel_names[c][0] || strchr(p->channel_names[c],'\n') || strchr(p->channel_names[c],'\r')) return 0;
    for(int c=0;c<CHANNELS;c++) if(strchr(p->paths[c],'\n') || strchr(p->paths[c],'\r')) return 0;
    for(int pat=0;pat<PATTERNS;pat++) if(!p->pattern_names[pat][0] || strchr(p->pattern_names[pat],'\n') || strchr(p->pattern_names[pat],'\r')) return 0;
    for(int c=0;c<CHANNELS;c++) if(!sampler_valid(p->sampler[c])) return 0;
    if(!isfinite(p->master_width) || p->master_width<0 || p->master_width>2 || p->master_mute>1) return 0;
    for(int i=0;i<INSERTS;i++) if(!isfinite(p->insert_width[i]) || p->insert_width[i]<0 || p->insert_width[i]>2) return 0;
    for(int c=0;c<CHANNELS;c++) if(p->mute[c]>3 || !isfinite(p->volume[c]) || p->volume[c]<0 || p->volume[c]>VOLUME_KNOB_MAX) return 0;
    for(int l=0;l<LANES;l++) if(p->lane_mute[l]>3) return 0;
    if(!isfinite(p->swing) || p->swing<0 || p->swing>1) return 0;
    if(!isfinite(p->master) || p->master<0 || p->master>MIXER_GAIN_MAX) return 0;
    for(int i=0;i<INSERTS;i++) if(!isfinite(p->insert_volume[i]) || p->insert_volume[i]<0 || p->insert_volume[i]>MIXER_GAIN_MAX) return 0;
    for(int id=0;id<=INSERTS;id++) for(int io=0;io<2;io++) if(strnlen(p->audio_io[id][io],128)==128 || strchr(p->audio_io[id][io],'\n') || strchr(p->audio_io[id][io],'\r')) return 0;
    for(int id=0;id<=INSERTS;id++) for(int slot=0;slot<10;slot++) if(!isfinite(p->effect_mix[id][slot]) || p->effect_mix[id][slot]<0 || p->effect_mix[id][slot]>1 || p->effect_bypass[id][slot]>1) return 0;
    for(int l=0;l<LANES;l++) if(!p->track_names[l][0] || strnlen(p->track_names[l],PATTERN_NAME)==PATTERN_NAME || strchr(p->track_names[l],'\n') || strchr(p->track_names[l],'\r')) return 0;
    for(int i=0;i<INSERTS;i++) if(!p->insert_names[i][0] || strnlen(p->insert_names[i],PATTERN_NAME)==PATTERN_NAME || strchr(p->insert_names[i],'\n') || strchr(p->insert_names[i],'\r')) return 0;
    for(int i=0;i<PATTERNS;i++) if(p->pattern_colors[i]>0xffffff) return 0;
    for(int c=0;c<CHANNELS;c++) if(p->channel_colors[c]>0xffffff) return 0;
    for(int c=0;c<CHANNELS;c++) if(p->channel_audio[c]>1 || !isfinite(p->audio_seconds[c]) || p->audio_seconds[c]<0 || p->audio_seconds[c]>SAMPLE_MAX_FRAMES*4.0/RATE) return 0;
    for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) if(p->clips[l][b]>PATTERNS && p->clips[l][b]<=AUTOMATION_SOURCE) {
        int c=p->clips[l][b]-PATTERNS-1;
        if(c>=p->channel_count || !p->channel_audio[c]) return 0;
    }
    for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) if(p->clips[l][b]>AUTOMATION_SOURCE && p->clips[l][b]>AUTOMATION_SOURCE+p->automation_count) return 0;
    for(int c=0;c<CHANNELS;c++) if(!isfinite(p->channel_pitch[c]) || fabsf(p->channel_pitch[c])>1 || !isfinite(p->pitch_range[c]) || p->pitch_range[c]<1 || p->pitch_range[c]>48 || p->pitch_range[c]!=roundf(p->pitch_range[c])) return 0;
    char tmp[4096]; if(snprintf(tmp,sizeof tmp,"%s.tmp",path)>=(int)sizeof tmp) return 0;
    FILE *f=fopen(tmp,"w"); if(!f) return 0;
    for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) if(!isfinite(p->clip_offsets[l][b]) || p->clip_offsets[l][b]<0 || p->clip_offsets[l][b]>1e15f) { fclose(f); remove(tmp); return 0; }
    fprintf(f,"HOMEBEAT 32\n%.9g %.9g %d\n",p->bpm,p->master,p->channel_count);
    for(int c=0;c<CHANNELS;c++) fprintf(f,"%.9g %.9g %u\n",p->volume[c],p->pan[c],p->mute[c]);
    for(int a=0;a<PATTERNS;a++) for(int c=0;c<CHANNELS;c++) for(int i=0;i<NOTES;i++) {
        Note n=p->notes[a][c][i]; fprintf(f,"%u %u %.9g %.9g\n",n.pitch,n.velocity,n.start,n.length);
    }
    for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) fprintf(f,"%u\n",p->clips[l][b]);
    for(int c=0;c<CHANNELS;c++) fprintf(f,"%s\n",p->paths[c]);
    for(int pat=0;pat<PATTERNS;pat++) fprintf(f,"%s\n",p->pattern_names[pat]);
    fprintf(f,"%d\n",p->insert_count);
    for(int c=0;c<CHANNELS;c++) fprintf(f,"%u\n",p->route[c]);
    for(int i=0;i<INSERTS;i++) fprintf(f,"%.9g %.9g %u\n",p->insert_volume[i],p->insert_pan[i],p->insert_mute[i]);
    for(int a=0;a<PATTERNS;a++) fprintf(f,"%.9g\n",p->pattern_steps[a]);
    for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) fprintf(f,"%.9g\n",p->clip_steps[l][b]);
    fprintf(f,"%.9g\n",p->master_pitch);
    for(int i=0;i<INSERTS;i++) fprintf(f,"%u\n",p->insert_output[i]);
    fprintf(f,"%d\n",p->pattern_count);
    for(int c=0;c<CHANNELS;c++) fprintf(f,"%s\n",p->channel_names[c]);
    for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) fprintf(f,"%.9g\n",p->clip_starts[l][b]);
    for(int c=0;c<CHANNELS;c++) fprintf(f,"%.9g %.9g %.9g %.9g %u %u\n",p->sampler[c].pitch,p->sampler[c].time,p->sampler[c].start,p->sampler[c].length,p->sampler[c].flags,p->sampler[c].stretch);
    for(int i=0;i<INSERTS;i++) fprintf(f,"%.9g\n",p->insert_width[i]);
    fprintf(f,"%.9g %u\n",p->master_width,p->master_mute);
    for(int l=0;l<LANES;l++) fprintf(f,"%u\n",p->lane_mute[l]);
    fprintf(f,"%.9g\n",p->swing);
    for(int id=0;id<=INSERTS;id++) for(int io=0;io<2;io++) fprintf(f,"%s\n",p->audio_io[id][io]);
    for(int id=0;id<=INSERTS;id++) for(int slot=0;slot<10;slot++) fprintf(f,"%.9g %u\n",p->effect_mix[id][slot],p->effect_bypass[id][slot]);
    for(int l=0;l<LANES;l++) fprintf(f,"%s\n",p->track_names[l]);
    for(int i=0;i<INSERTS;i++) fprintf(f,"%s\n",p->insert_names[i]);
    for(int i=0;i<PATTERNS;i++) fprintf(f,"%u\n",p->pattern_colors[i]);
    for(int c=0;c<CHANNELS;c++) fprintf(f,"%u %.9g\n",p->channel_audio[c],p->audio_seconds[c]);
    for(int c=0;c<CHANNELS;c++) fprintf(f,"%.9g %.9g\n",p->channel_pitch[c],p->pitch_range[c]);
    for(int c=0;c<CHANNELS;c++) fprintf(f,"%.9g\n",p->sampler[c].trim);
    for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) fprintf(f,"%.9g\n",p->clip_offsets[l][b]);
    for(int c=0;c<CHANNELS;c++) fprintf(f,"%u\n",p->channel_colors[c]);
    fprintf(f,"%d\n",p->automation_count);
    for(int i=0;i<p->automation_count;i++) {
        const Automation *a=&p->automations[i];
        fprintf(f,"%u %u %u %.9g %d %u\n%s\n",a->target.parameter,a->target.owner,a->target.slot,a->steps,a->count,a->color,a->name);
        for(int n=0;n<a->count;n++) fprintf(f,"%.9g %.9g\n",a->points[n].step,a->points[n].value);
    }
    for(int c=0;c<CHANNELS;c++) fprintf(f,"%.9g\n",p->sampler[c].fit_bpm);
    int ok=!ferror(f); if(fclose(f)) ok=0;
    if(ok && rename(tmp,path)==0) return 1;
    remove(tmp); return 0;
}
int project_load(const char *path,Project *p) {
    FILE *f=fopen(path,"r"); if(!f) return 0;
    Project q; project_default(&q); memset(q.notes,0,sizeof q.notes); q.pattern_count=PATTERNS; char magic[32]; int version=0; unsigned x,y;
    for(int pat=0;pat<PATTERNS;pat++) { q.pattern_steps[pat]=STEPS; snprintf(q.pattern_names[pat],PATTERN_NAME,"Pattern %d",pat+1); }
    q.insert_count=4;
    for(int i=0;i<INSERTS;i++) q.insert_volume[i]=1;
    for(int c=0;c<4;c++) q.route[c]=c+1;
    int ok=fscanf(f,"%31s %d",magic,&version)==2 && !strcmp(magic,"HOMEBEAT") && version>=1 && version<=32;
    ok=ok && fscanf(f,"%f %f",&q.bpm,&q.master)==2 && isfinite(q.bpm) && q.bpm>=30 && q.bpm<=300 && isfinite(q.master) && q.master>=0 && q.master<=(version>=18?MIXER_GAIN_MAX:1);
    if(version>=12) ok=ok && fscanf(f,"%d",&q.channel_count)==1 && q.channel_count>=0 && q.channel_count<=CHANNELS;
    int channels=version>=12?CHANNELS:4,inserts=version>=12?INSERTS:16,clips=version>=14?CLIPS:BARS;
    for(int c=0;ok && c<channels;c++) {
        ok=fscanf(f,"%f %f %u",&q.volume[c],&q.pan[c],&x)==3 && isfinite(q.volume[c]) && isfinite(q.pan[c]) && q.volume[c]>=0 && q.volume[c]<=(version>=28?VOLUME_KNOB_MAX:1) && fabsf(q.pan[c])<=1 && x<=(version>=16?3u:1u);
        if(ok) q.mute[c]=x;
    }
    for(int a=0;ok && a<PATTERNS;a++) for(int c=0;ok && c<channels;c++) for(int i=0;ok && i<(version>=4?NOTES:STEPS);i++) {
        float start=i,length=0;
        ok=fscanf(f,"%u %u",&x,&y)==2 && x<=127 && y<=127;
        if(version>=4) { float limit=version>=14?1e15f:version==4?STEPS:BARS*STEPS; ok=ok && fscanf(f,"%f %f",&start,&length)==2 && isfinite(start) && isfinite(length) && start>=0 && length>=0 && start<limit && length<=limit-start; }
        if(ok) q.notes[a][c][i]=(Note){x,y,start,length};
    }
    for(int l=0;ok && l<(version>=10?LANES:4);l++) for(int b=0;ok && b<clips;b++) { ok=fscanf(f,"%u",&x)==1 && x<=(unsigned)(version>=30?SOURCES:version>=23?PATTERNS+CHANNELS:PATTERNS); if(ok) q.clips[l][b]=x; }
    int ch; do { ch=fgetc(f); } while(ch!=EOF && ch!='\n');
    for(int c=0;ok && c<channels;c++) {
        ok=read_line(f,q.paths[c],sizeof q.paths[c]);
    }
    if(version>=2) for(int pat=0;ok && pat<PATTERNS;pat++) {
        ok=read_line(f,q.pattern_names[pat],PATTERN_NAME) && q.pattern_names[pat][0];
    }
    if(version>=3) {
        ok=ok && fscanf(f,"%d",&q.insert_count)==1 && q.insert_count>=0 && q.insert_count<=INSERTS;
        for(int c=0;ok && c<channels;c++) { ok=fscanf(f,"%u",&x)==1 && x<=(unsigned)q.insert_count; if(ok) q.route[c]=x; }
        for(int i=0;ok && i<inserts;i++) {
            ok=fscanf(f,"%f %f %u",&q.insert_volume[i],&q.insert_pan[i],&x)==3 && isfinite(q.insert_volume[i]) && q.insert_volume[i]>=0 && q.insert_volume[i]<=(version>=18?MIXER_GAIN_MAX:1) && isfinite(q.insert_pan[i]) && fabsf(q.insert_pan[i])<=1 && x<=(version>=11?3u:1u);
            if(ok) q.insert_mute[i]=x;
        }
    }
    if(version>=5) {
        for(int a=0;ok && a<PATTERNS;a++) { ok=fscanf(f,"%f",&q.pattern_steps[a])==1 && isfinite(q.pattern_steps[a]) && q.pattern_steps[a]>=STEPS && q.pattern_steps[a]<=(version>=14?1e15f:BARS*STEPS); }
        for(int l=0;ok && l<(version>=10?LANES:4);l++) for(int b=0;ok && b<clips;b++) { float length; ok=fscanf(f,"%f",&length)==1 && isfinite(length) && length>=0 && length<=(version>=14?1e15f:(version>=13?BARS:BARS-b)*STEPS); if(ok) q.clip_steps[l][b]=length; }
    }
    if(version>=6) ok=ok && fscanf(f,"%f",&q.master_pitch)==1 && isfinite(q.master_pitch) && fabsf(q.master_pitch)<=12;
    if(version>=7) {
        for(int i=0;ok && i<inserts;i++) { ok=fscanf(f,"%u",&x)==1 && (x<=(unsigned)q.insert_count || x==255); if(ok) q.insert_output[i]=x; }
        for(int i=1;ok && i<=q.insert_count;i++) ok=insert_connect(&q,i,q.insert_output[i-1]);
    }
    if(version>=9) {
        ok=ok && fscanf(f,"%d",&q.pattern_count)==1 && q.pattern_count>=1 && q.pattern_count<=PATTERNS;
        for(int l=0;ok && l<LANES;l++) for(int b=0;ok && b<clips;b++) ok=q.clips[l][b]<=q.pattern_count || (version>=23 && q.clips[l][b]>PATTERNS);
    }
    if(version>=12) {
        do { ch=fgetc(f); } while(ch!=EOF && ch!='\n');
        for(int c=0;ok && c<CHANNELS;c++) ok=read_line(f,q.channel_names[c],PATTERN_NAME) && q.channel_names[c][0];
    }
    if(version>=13) for(int l=0;ok && l<LANES;l++) for(int b=0;ok && b<clips;b++) {
        float start; ok=fscanf(f,"%f",&start)==1 && isfinite(start) && start>=0 && start<(version>=14?1e15f:BARS);
        if(ok) { q.clip_starts[l][b]=start; if(version<14 && q.clips[l][b]) ok=start*STEPS+clip_length(&q,l,b)<=BARS*STEPS+.0001f; }
    }
    if(version>=15) for(int c=0;ok && c<CHANNELS;c++) {
        Sampler *s=&q.sampler[c];
        ok=fscanf(f,"%f %f %f %f %u %u",&s->pitch,&s->time,&s->start,&s->length,&x,&y)==6 && x<=7 && y<=1;
        if(ok) { s->flags=x; s->stretch=y; ok=sampler_valid(*s); }
    }
    if(version>=16) {
        for(int i=0;ok && i<INSERTS;i++) ok=fscanf(f,"%f",&q.insert_width[i])==1 && isfinite(q.insert_width[i]) && q.insert_width[i]>=0 && q.insert_width[i]<=2;
        ok=ok && fscanf(f,"%f %u",&q.master_width,&x)==2 && isfinite(q.master_width) && q.master_width>=0 && q.master_width<=2 && x<=1;
        if(ok) q.master_mute=x;
        for(int l=0;ok && l<LANES;l++) { ok=fscanf(f,"%u",&x)==1 && x<=3; if(ok) q.lane_mute[l]=x; }
    }
    if(version>=17) ok=ok && fscanf(f,"%f",&q.swing)==1 && isfinite(q.swing) && q.swing>=0 && q.swing<=1;
    if(version>=19) {
        ok=ok && fgetc(f)=='\n';
        for(int id=0;ok && id<=INSERTS;id++) for(int io=0;ok && io<2;io++) ok=read_line(f,q.audio_io[id][io],128);
    }
    if(version>=20) {
        for(int id=0;ok && id<=INSERTS;id++) for(int slot=0;ok && slot<10;slot++) {
            ok=fscanf(f,"%f %u",&q.effect_mix[id][slot],&x)==2 && isfinite(q.effect_mix[id][slot]) && q.effect_mix[id][slot]>=0 && q.effect_mix[id][slot]<=1 && x<=1;
            if(ok) q.effect_bypass[id][slot]=x;
        }
        ok=ok && fgetc(f)=='\n';
        for(int l=0;ok && l<LANES;l++) ok=read_line(f,q.track_names[l],PATTERN_NAME) && q.track_names[l][0];
    }
    if(version>=21) for(int i=0;ok && i<INSERTS;i++) ok=read_line(f,q.insert_names[i],PATTERN_NAME) && q.insert_names[i][0];
    if(version>=22) for(int i=0;ok && i<PATTERNS;i++) { ok=fscanf(f,"%u",&x)==1 && x<=0xffffff; if(ok) q.pattern_colors[i]=x; }
    if(version>=23) {
        for(int c=0;ok && c<CHANNELS;c++) {
            ok=fscanf(f,"%u %f",&x,&q.audio_seconds[c])==2 && x<=1 && isfinite(q.audio_seconds[c]) && q.audio_seconds[c]>=0 && q.audio_seconds[c]<=SAMPLE_MAX_FRAMES*4.0/RATE;
            if(ok) q.channel_audio[c]=x;
        }
        for(int l=0;ok && l<LANES;l++) for(int b=0;ok && b<CLIPS;b++) if(q.clips[l][b]>PATTERNS && q.clips[l][b]<=AUTOMATION_SOURCE) {
            int c=q.clips[l][b]-PATTERNS-1;
            ok=c<q.channel_count && q.channel_audio[c];
        }
    }
    if(version>=25) for(int c=0;ok && c<CHANNELS;c++) ok=fscanf(f,"%f %f",&q.channel_pitch[c],&q.pitch_range[c])==2 && isfinite(q.channel_pitch[c]) && fabsf(q.channel_pitch[c])<=1 && isfinite(q.pitch_range[c]) && q.pitch_range[c]>=1 && q.pitch_range[c]<=48 && q.pitch_range[c]==roundf(q.pitch_range[c]);
    if(version>=26) for(int c=0;ok && c<CHANNELS;c++) ok=fscanf(f,"%f",&q.sampler[c].trim)==1 && isfinite(q.sampler[c].trim) && q.sampler[c].trim>=0 && q.sampler[c].trim<=1;
    if(version>=27) for(int l=0;ok && l<LANES;l++) for(int b=0;ok && b<CLIPS;b++) ok=fscanf(f,"%f",&q.clip_offsets[l][b])==1 && isfinite(q.clip_offsets[l][b]) && q.clip_offsets[l][b]>=0 && q.clip_offsets[l][b]<=1e15f;
    if(version>=29) for(int c=0;ok && c<CHANNELS;c++) { ok=fscanf(f,"%u",&x)==1 && x<=0xffffff; if(ok) q.channel_colors[c]=x; }
    if(version>=30) {
        ok=ok && fscanf(f,"%d",&q.automation_count)==1 && q.automation_count>=0 && q.automation_count<=AUTOMATIONS;
        for(int i=0;ok && i<q.automation_count;i++) {
            Automation *a=&q.automations[i];
            ok=fscanf(f,"%u %u %u %f %d %u",&a->target.parameter,&a->target.owner,&a->target.slot,&a->steps,&a->count,&a->color)==6 && a->count>=1 && a->count<=AUTOMATION_POINTS;
            ok=ok && fgetc(f)=='\n' && read_line(f,a->name,sizeof a->name);
            for(int n=0;ok && n<a->count;n++) ok=fscanf(f,"%f %f",&a->points[n].step,&a->points[n].value)==2;
        }
        ok=ok && automation_valid(&q);
        for(int l=0;ok && l<LANES;l++) for(int b=0;ok && b<CLIPS;b++) if(q.clips[l][b]>AUTOMATION_SOURCE) ok=q.clips[l][b]<=AUTOMATION_SOURCE+q.automation_count;
    }
    if(version>=31) for(int c=0;ok && c<CHANNELS;c++) {
        Sampler *s=&q.sampler[c];
        ok=fscanf(f,"%f",&s->fit_bpm)==1;
        ok=ok && sampler_valid(*s);
    }
    /* Version 31 stored BPM-fitted PCM lengths/offsets; restore reference seconds. */
    if(version==31 && ok) for(int c=0;c<CHANNELS;c++) if(q.sampler[c].fit_bpm) {
        float ratio=q.bpm/q.sampler[c].fit_bpm;
        q.audio_seconds[c]*=ratio;
        for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) if(q.clips[l][b]==PATTERNS+c+1) {
            q.clip_steps[l][b]*=ratio; q.clip_offsets[l][b]*=ratio;
        }
    }
    /* Version 23 imported full clips with a fixed cap; convert only that old layout. */
    if(version==23 && ok) for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) if(q.clips[l][b]>PATTERNS && q.clips[l][b]<=AUTOMATION_SOURCE) {
        int c=q.clips[l][b]-PATTERNS-1;
        if(fabsf(q.clip_steps[l][b]-q.audio_seconds[c])<.00001f) q.clip_steps[l][b]=0;
    }
    fclose(f); if(ok) *p=q; return ok;
}
static void le(FILE *f,uint32_t x,int n) { for(int i=0;i<n;i++) fputc((x>>(i*8))&255,f); }
int export_wav(const char *path,const Project *pr,const Sample s[CHANNELS]) {
    double total=ceil(song_steps(pr)*RATE*60.0/pr->bpm/4);
    if(total>(UINT32_MAX-36)/4 || !isfinite(total)) return 0;
    uint32_t frames=(uint32_t)total;
    FILE *f=fopen(path,"wb"); if(!f) return 0;
    fwrite("RIFF",1,4,f); le(f,36+frames*4,4); fwrite("WAVEfmt ",1,8,f); le(f,16,4); le(f,1,2); le(f,2,2); le(f,RATE,4); le(f,RATE*4,4); le(f,4,2); le(f,16,2); fwrite("data",1,4,f); le(f,frames*4,4);
    Player player; player_reset(&player); player.song=1; float block[1024];
    for(uint32_t i=0;i<frames;) {
        unsigned n=frames-i>512?512:frames-i; render(&player,pr,s,block,n);
        for(unsigned j=0;j<n*2;j++) {
            float x=isfinite(block[j])?fmaxf(-1,fminf(1,block[j])):0;
            le(f,(uint16_t)(int16_t)(x*32767),2);
        }
        i+=n;
    }
    int ok=!ferror(f); if(fclose(f)) ok=0; return ok;
}
