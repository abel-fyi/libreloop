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
uint32_t next_source_color(const uint32_t *colors,int count) {
    for(int color=0;color<COLOR_COUNT;color++) {
        int used=0;
        for(int i=0;i<count;i++) if(colors[i]==pattern_palette[color]) used=1;
        if(!used) return pattern_palette[color];
    }
    return pattern_palette[count%COLOR_COUNT];
}
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
    for(int id=0;id<=INSERTS;id++) for(int slot=0;slot<10;slot++) { p->effect_mix[id][slot]=1; p->chorus[id][slot]=chorus_default(); p->eq[id][slot]=equalizer_default(); }
    for(int l=0;l<LANES;l++) snprintf(p->track_names[l],PATTERN_NAME,"Track %d",l+1);
    const char *names[]={"Kick","Snare","Hat","Tone"};
    for(int c=0;c<CHANNELS;c++) { p->fm[c]=fm_default(); p->pitch_range[c]=2; p->volume[c]=1; p->sampler[c].time=p->sampler[c].length=1; if(c<4) snprintf(p->channel_names[c],PATTERN_NAME,"%s",names[c]); else snprintf(p->channel_names[c],PATTERN_NAME,"Channel %d",c+1); }
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
int channel_replace_instrument(Project *p,int c,int type) {
    if(c<0 || c>=p->channel_count || (type!=INSTRUMENT_SAMPLER && type!=INSTRUMENT_FM) ||
       (type==INSTRUMENT_FM && p->channel_audio[c])) return 0;
    if(p->instrument[c]==type) return 1;
    if(type!=INSTRUMENT_FM) for(int a=p->automation_count-1;a>=0;a--) {
        ParameterTarget target=p->automations[a].target;
        if(target.owner==(unsigned)c && ((target.parameter>=PARAM_FM_RATIO && target.parameter<=PARAM_FM_LAST) || (target.parameter>=PARAM_DX7_FIRST && target.parameter<=PARAM_DX7_LAST))) automation_delete(p,a);
    }
    p->instrument[c]=type; return 1;
}
int channel_delete(Project *p,int c) {
    if(c<0 || c>=p->channel_count) return 0;
    for(int i=c;i<p->channel_count-1;i++) {
        p->instrument[i]=p->instrument[i+1]; p->fm[i]=p->fm[i+1]; p->sampler[i]=p->sampler[i+1]; p->channel_pitch[i]=p->channel_pitch[i+1]; p->pitch_range[i]=p->pitch_range[i+1];
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
        if(t->parameter==PARAM_CHANNEL_VOLUME || t->parameter==PARAM_CHANNEL_PAN || t->parameter==PARAM_CHANNEL_PITCH || t->parameter==PARAM_CHANNEL_MUTE || t->parameter==PARAM_PITCH_RANGE || ((t->parameter>=PARAM_FM_RATIO && t->parameter<=PARAM_FM_LAST) || (t->parameter>=PARAM_DX7_FIRST && t->parameter<=PARAM_DX7_LAST))) {
            if(t->owner==(unsigned)c) automation_delete(p,i);
            else if(t->owner>(unsigned)c) t->owner--;
        }
    }
    for(int i=p->midi_binding_count-1;i>=0;i--) {
        ParameterTarget *t=&p->midi_bindings[i].target;
        if(t->parameter<=PARAM_CHANNEL_PITCH || t->parameter==PARAM_CHANNEL_MUTE || t->parameter==PARAM_PITCH_RANGE ||
           (t->parameter>=PARAM_FM_RATIO && t->parameter<=PARAM_FM_LAST) || (t->parameter>=PARAM_DX7_FIRST && t->parameter<=PARAM_DX7_LAST)) {
            if(t->owner==(unsigned)c) {memmove(&p->midi_bindings[i],&p->midi_bindings[i+1],(--p->midi_binding_count-i)*sizeof p->midi_bindings[0]);memset(&p->midi_bindings[p->midi_binding_count],0,sizeof p->midi_bindings[0]);}
            else if(t->owner>(unsigned)c)t->owner--;
        }
    }
    int last=--p->channel_count; p->instrument[last]=INSTRUMENT_SAMPLER; p->fm[last]=fm_default(); p->channel_colors[last]=0; p->channel_pitch[last]=0; p->pitch_range[last]=2; p->channel_audio[last]=0; p->audio_seconds[last]=0; p->sampler[last]=(Sampler){.time=1,.length=1}; p->volume[last]=1; p->pan[last]=0; p->mute[last]=p->route[last]=0;
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
    for(int slot=0;slot<EFFECT_SLOTS;slot++) { p->effect_type[id][slot]=EFFECT_EMPTY; p->effect_mix[id][slot]=1; p->chorus[id][slot]=chorus_default(); p->eq[id][slot]=equalizer_default(); p->effect_bypass[id][slot]=0; }
    for(int a=p->automation_count-1;a>=0;a--) {
        ParameterTarget t=p->automations[a].target;
        if(t.owner==(unsigned)id && ((t.parameter>=PARAM_CHORUS_RATE && t.parameter<=PARAM_EFFECT_MIX) || (t.parameter>=PARAM_EQ_FIRST && t.parameter<=PARAM_EQ_LAST))) automation_delete(p,a);
    }
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
int note_hit(const Note notes[NOTES],float step,int pitch) {
    for(int i=NOTES-1;i>=0;i--) {
        Note n=notes[i];
        if(n.velocity && n.pitch==pitch && n.start<=step && n.start+(n.length?n.length:1)>step) return i;
    }
    return -1;
}
void notes_velocity(Note notes[NOTES],const Note before[NOTES],float from,float to,float from_value,float to_value,float radius) {
    if(!isfinite(from) || !isfinite(to) || !isfinite(from_value) || !isfinite(to_value) || !isfinite(radius) || radius<0) return;
    float left=fminf(from,to)-radius,right=fmaxf(from,to)+radius;
    for(int i=0;i<NOTES;i++) {
        if(before) notes[i].velocity=before[i].velocity;
        Note *n=&notes[i];
        if(!n->velocity || n->start<left || n->start>right) continue;
        float t=fabsf(to-from)>.000001f?fmaxf(0,fminf(1,(n->start-from)/(to-from))):1;
        n->velocity=(uint8_t)roundf(fmaxf(1,fminf(127,from_value+(to_value-from_value)*t)));
    }
}
Note *note_add(Project *p,int pat,int channel,float start,int pitch,float length) {
    if(pat<0 || pat>=PATTERNS || channel<0 || channel>=CHANNELS || !isfinite(start) || !isfinite(length) || start<0 || start>=p->pattern_steps[pat] || pitch<0 || pitch>127 || length<0 || length>p->pattern_steps[pat]-start) return NULL;
    for(int i=0;i<NOTES;i++) if(!p->notes[pat][channel][i].velocity) {
        Note *n=&p->notes[pat][channel][i]; *n=(Note){pitch,100,start,length}; return n;
    }
    return NULL;
}
int note_move(Project *p,int pat,int channel,Note *note,float start,int pitch,float limit) {
    if(pat<0 || pat>=PATTERNS || channel<0 || channel>=p->channel_count || !note || !note->velocity || limit>p->pattern_steps[pat]) return 0;
    float length=note->length?note->length:1;
    if(!isfinite(start) || !isfinite(limit) || start<0 || start+length>limit || pitch<0 || pitch>127) return 0;
    note->start=start; note->pitch=pitch; return 1;
}
int notes_move(Project *p,int pat,int channel,const Note before[NOTES],const uint8_t selected[NOTES],float dx,int dy,float limit) {
    if(pat<0 || pat>=PATTERNS || channel<0 || channel>=p->channel_count || !isfinite(dx) || !isfinite(limit) || limit>p->pattern_steps[pat]) return 0;
    for(int i=0;i<NOTES;i++) if(selected[i]) {
        Note n=before[i]; float start=n.start+dx; int pitch=n.pitch+dy;
        if(!n.velocity || start<0 || start+(n.length?n.length:1)>limit || pitch<0 || pitch>127) return 0;
    }
    for(int i=0;i<NOTES;i++) if(selected[i]) { p->notes[pat][channel][i]=before[i]; p->notes[pat][channel][i].start+=dx; p->notes[pat][channel][i].pitch+=dy; }
    return 1;
}
float notes_resize(Project *p,int pat,int channel,const Note before[NOTES],const uint8_t selected[NOTES],float delta,float minimum) {
    if(pat<0 || pat>=PATTERNS || channel<0 || channel>=p->channel_count || !isfinite(delta) || !isfinite(minimum) || minimum<=0) return 0;
    float lower=-INFINITY,end=0;
    for(int i=0;i<NOTES;i++) if(selected[i] && before[i].velocity) {
        float length=before[i].length?before[i].length:1;
        lower=fmaxf(lower,fminf(minimum,length)-length);
    }
    delta=fmaxf(lower,delta);
    for(int i=0;i<NOTES;i++) if(selected[i] && before[i].velocity) {
        p->notes[pat][channel][i]=before[i];
        if(delta!=0) p->notes[pat][channel][i].length=(before[i].length?before[i].length:1)+delta;
        float length=p->notes[pat][channel][i].length;
        end=fmaxf(end,before[i].start+(length?length:1));
    }
    return end;
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
static void trigger(Player *p,const Project *pr,int pat,int64_t tick,float remaining,int lane,int entering,float swing,const float *channel_speeds,double master_speed,const uint8_t *channel_mutes,const FMSettings *fm_settings) {
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
        uint32_t phase[6]; int retain_phase=p->voices[v].instrument==INSTRUMENT_FM && p->voices[v].channel==c && p->voices[v].fm.engine==1 && pr->instrument[c]==INSTRUMENT_FM && fm_settings[c].engine==1 && !fm_settings[c].dx7.value[136];
        if(retain_phase) memcpy(phase,p->voices[v].fm.dx7.phase,sizeof phase);
        p->voices[v]=(Voice){.channel=c,.sampler={.position=elapsed*frames_per_step*speed*channel_speeds[c]*master_speed*(pr->sampler[c].fit_bpm?pr->bpm/pr->sampler[c].fit_bpm:1),.speed=speed},.remaining=n.length?fmin(n.length-elapsed,remaining)*frames_per_step:-1,.gain=n.velocity/127.f,.lane=lane,.pattern=pat,.note_id=i+1};
        if(pr->instrument[c]==INSTRUMENT_FM) {
            Voice *voice=&p->voices[v]; voice->instrument=INSTRUMENT_FM;
            voice->remaining=fmin(n.length?n.length-elapsed:1,remaining)*frames_per_step;
            fm_note_on_velocity(&voice->fm,261.6255653005986*speed,fm_settings[c],n.velocity/127.f);
            if(retain_phase) memcpy(voice->fm.dx7.phase,phase,sizeof phase);
        }
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
    if(!sequence && !count && !io && !p->effects) { p->frame+=frames; return; }
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
    uint8_t effect_audible[INSERTS+1]; memset(effect_audible,1,sizeof effect_audible);
    /* Keep configured chains advancing on silence, including their routed tails. */
    for(int bus=0;p->effects && bus<=pr->insert_count;bus++) {
        int active=0; for(int slot=0;slot<EFFECT_SLOTS;slot++) active|=pr->effect_type[bus][slot]!=EFFECT_EMPTY;
        if(!active) continue;
        int id=bus,hops=0,audible=!insert_solo || bus==0;
        while(id && id!=255 && hops++<INSERTS) { used[id]=1; audible|=pr->insert_mute[id-1]&2; id=pr->insert_output[id-1]; }
        effect_audible[bus]=audible;
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
    ChorusSettings effect_controls[INSERTS+1][EFFECT_SLOTS];
    EQSettings eq_controls[INSERTS+1][EFFECT_SLOTS]; memcpy(eq_controls,pr->eq,sizeof eq_controls);
    FMSettings fm_controls[CHANNELS]; memcpy(fm_controls,pr->fm,sizeof fm_controls);
    float effect_mixes[INSERTS+1][EFFECT_SLOTS];
    memcpy(effect_controls,pr->chorus,sizeof effect_controls); memcpy(effect_mixes,pr->effect_mix,sizeof effect_mixes);
    float *bindings[AUTOMATIONS]={0},baseline[AUTOMATIONS],low[AUTOMATIONS],high[AUTOMATIONS];
    uint8_t channel_changed[CHANNELS]={0},insert_changed[INSERTS]={0}; int master_changed=0,master_pitch_changed=0;
    uint8_t channel_pitch_changed[CHANNELS]={0};
    if(sequence && p->song && pr->automation_count) {
        for(int c=0;c<pr->channel_count;c++) { channel_controls[c][0]=pr->volume[c]; channel_controls[c][1]=pr->pan[c]; channel_controls[c][2]=pr->channel_pitch[c]; channel_controls[c][3]=!!(pr->mute[c]&1); channel_controls[c][4]=pr->pitch_range[c]; }
        for(int i=0;i<pr->insert_count;i++) { insert_controls[i][0]=pr->insert_volume[i]; insert_controls[i][1]=pr->insert_pan[i]; insert_controls[i][2]=pr->insert_width[i]; insert_controls[i][3]=!!(pr->insert_mute[i]&1); }
        for(int a=0;a<pr->automation_count;a++) {
            ParameterTarget t=pr->automations[a].target; unsigned id=t.parameter,c=t.owner;
            if(!parameter_info(pr,t,&baseline[a],&low[a],&high[a])) continue;
            if((id>=PARAM_FM_RATIO && id<=PARAM_FM_LAST) || (id>=PARAM_DX7_FIRST && id<=PARAM_DX7_LAST)) {
                FMSettings *settings=&fm_controls[c];
                bindings[a]=(float *)fm_parameter_pointer(settings,id);
            }
            else if(id>=PARAM_EQ_FIRST && id<=PARAM_EQ_LAST) {
                EQBand *b=&eq_controls[c][t.slot].bands[(id-PARAM_EQ_FIRST)/3];
                bindings[a]=(id-PARAM_EQ_FIRST)%3==0?&b->frequency:(id-PARAM_EQ_FIRST)%3==1?&b->gain:&b->q;
            }
            else if(id>=PARAM_CHORUS_RATE && id<=PARAM_EFFECT_MIX) {
                bindings[a]=id==PARAM_CHORUS_RATE?&effect_controls[c][t.slot].rate:id==PARAM_CHORUS_DEPTH?&effect_controls[c][t.slot].depth:&effect_mixes[c][t.slot];
            }
            else if(id<=PARAM_CHANNEL_PITCH || id==PARAM_CHANNEL_MUTE || id==PARAM_PITCH_RANGE) { bindings[a]=&channel_controls[c][id==PARAM_CHANNEL_MUTE?3:id==PARAM_PITCH_RANGE?4:id-PARAM_CHANNEL_VOLUME]; channel_changed[c]=1; if(id==PARAM_CHANNEL_PITCH || id==PARAM_PITCH_RANGE) channel_pitch_changed[c]=1; }
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
        if(sequence && (!io || io->monitor_only) && p->frame>=end_frame) { p->frame=begin_frame; step=(int64_t)(p->frame/stepframes); p->last_step=-1; if(loop) memset(p->voices,0,sizeof p->voices); }
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
            for(int a=0;a<pr->automation_count;a++) if(bindings[a]) {
                const ParameterDescriptor *info=parameter_descriptor(pr->automations[a].target.parameter);
                if(info && info->kind==PARAMETER_INTEGER) *bindings[a]=roundf(*bindings[a]);
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
                            p->voices[v]=(Voice){.channel=c,.sampler={.position=audio_clip_position(pr,l,b,p->frame/(stepframes*96)),.speed=1},.remaining=fmin(length*stepframes*96-position,end_frame-p->frame),.gain=1,.lane=l,.audio_clip=1};
                        }
                    } else if(tick && local>=0 && local<length*96 && local+pr->clip_offsets[l][b]*96<pr->pattern_steps[pat-1]*96) trigger(p,pr,pat-1,local+(int64_t)llround(pr->clip_offsets[l][b]*96),fminf(length-local/96.f,end-step/96.f),l,pr->clip_offsets[l][b]>0 && (local==0 || entered),master_controls[4],speed,pitch_speed,channel_mutes,fm_controls);
                }
            } else trigger(p,pr,p->pattern,step,end-step/96.f,-1,0,master_controls[4],speed,pitch_speed,channel_mutes,fm_controls);
            count=active_voices(p,live,active); resync=tempo_changed=0;
        }
        for(int at=0;at<buses_count;at++) buses[order[at]][0]=buses[order[at]][1]=0;
        if(io && io->input) {
            io->input(io->context,buses);
            for(int id=0;id<=pr->insert_count;id++) if(!input_audible[id]) buses[id][0]=buses[id][1]=0;
        }
        for(int v=0;v<count;v++) {
            Voice *voice=active[v]; if(!voice->gain) continue;
            int c=voice->channel;
            if(c>=pr->channel_count || voice->instrument!=pr->instrument[c]) { voice->gain=0; continue; }
            int synth=voice->instrument==INSTRUMENT_FM;
            if(!synth && voice->sampler.position>=s[c].frames) { voice->gain=0; continue; }
            float gain=voice->instrument==INSTRUMENT_FM && voice->fm.engine==1?1:voice->gain;
            if(synth) {
                if(voice->remaining>=0 && --voice->remaining<=0) { fm_note_off(&voice->fm,fm_controls[c]); voice->remaining=-1; }
            } else {
                if(pr->sampler[c].fit_bpm) {
                    double rate=voice->sampler.tempo_rate?voice->sampler.tempo_rate:pr->bpm/pr->sampler[c].fit_bpm;
                    gain*=fmin(1,(s[c].frames-voice->sampler.position)/(rate*voice->sampler.speed*pitch_speed*speed[c]*RATE*.005));
                }
                if(voice->remaining>=0) {
                    gain*=fminf(1,voice->remaining/(RATE*.005f));
                    if(--voice->remaining<=0) voice->gain=0;
                }
            }
            if(sequence && p->song && voice->lane>=0 && voice->lane<LANES && !lane_enabled[voice->lane]) gain=0;
            if(gain>0 && !pr->master_mute && (gain_left[c]>0 || gain_right[c]>0)) p->channel_active[c]=1;
            int id=pr->route[c];
            float stereo[2];
            double rate=pr->sampler[c].fit_bpm?pr->bpm/pr->sampler[c].fit_bpm:1;
            if(synth) {
                fm_sample_stereo(&voice->fm,&fm_controls[c],pitch_speed*speed[c],stereo);
                if(!fm_active(&voice->fm)) voice->gain=0;
            } else sampler_voice_sample(&voice->sampler,s[c],voice->sampler.speed*pitch_speed*speed[c],rate,pr->sampler[c].fit_bpm && pr->sampler[c].stretch,stereo);
            for(unsigned side=0;side<2;side++) buses[id][side]+=stereo[side]*gain*(side?gain_right[c]:gain_left[c]);
        }
        for(int at=0;at<buses_count;at++) {
            int id=order[at]; float stereo[2]={buses[id][0],buses[id][1]};
            effects_process_eq(p->effects,id,pr->effect_type[id],effect_controls[id],eq_controls[id],effect_mixes[id],pr->effect_bypass[id],stereo);
            float l=effect_audible[id]?stereo[0]*bus_left[id]:0,r=effect_audible[id]?stereo[1]*bus_right[id]:0;
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
        const Voice *voice=&p->voices[v]; int l=voice->lane,c=voice->channel;
        if(voice->gain && l>=0 && l<LANES && c<pr->channel_count && (voice->instrument==INSTRUMENT_FM?fm_active(&voice->fm):voice->sampler.position<s[c].frames) && lane_enabled[l] && (gain_left[c]>0 || gain_right[c]>0)) p->lane_active[l]=1;
    }
    for(int c=0;c<pr->channel_count;c++) if(pr->master_mute || !(gain_left[c]>0 || gain_right[c]>0) || (pr->instrument[c]!=INSTRUMENT_FM && !s[c].frames)) p->channel_trigger[c]=0;
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
