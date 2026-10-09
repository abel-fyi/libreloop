// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include "atomic_file.h"
#include "project_format.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>
#include <float.h>
#include <ctype.h>

static int project_valid(const Project *p) {
    if(!isfinite(p->bpm) || p->bpm<30 || p->bpm>300 || p->pattern_count<1 || p->pattern_count>PATTERNS) return 0;
    if(!isfinite(p->master_pitch) || fabsf(p->master_pitch)>12) return 0;
    if(!automation_valid(p) || p->midi_binding_count<0 || p->midi_binding_count>MIDI_BINDINGS) return 0;
    for(int i=0;i<p->midi_binding_count;i++){MidiBinding b=p->midi_bindings[i];if(b.channel>15 || b.controller>119 || !parameter_descriptor(b.target.parameter) || b.target.owner>INSERTS || b.target.slot>=EFFECT_SLOTS)return 0;}
    for(int c=0;c<CHANNELS;c++) if(p->instrument[c]>INSTRUMENT_FM || !fm_valid(p->fm[c]) || (p->instrument[c]==INSTRUMENT_FM && p->channel_audio[c])) return 0;
    for(int bus=0;bus<=INSERTS;bus++) for(int slot=0;slot<EFFECT_SLOTS;slot++)
        if(p->effect_type[bus][slot]>EFFECT_EQ || !chorus_valid(p->chorus[bus][slot]) || !equalizer_valid(p->eq[bus][slot])) return 0;
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
    for(int c=0;c<CHANNELS;c++) if(!isfinite(p->pan[c]) || fabsf(p->pan[c])>1 || p->route[c]>p->insert_count) return 0;
    for(int i=0;i<INSERTS;i++) if(!isfinite(p->insert_pan[i]) || fabsf(p->insert_pan[i])>1 || p->insert_mute[i]>3 || (p->insert_output[i]>p->insert_count && p->insert_output[i]!=255)) return 0;
    for(int i=1;i<=p->insert_count;i++) {
        uint8_t visited[INSERTS+1]={0}; int bus=i;
        while(bus && bus!=255) { if(bus>p->insert_count || visited[bus]) return 0; visited[bus]=1; bus=p->insert_output[bus-1]; }
    }
    for(int pat=0;pat<PATTERNS;pat++) {
        if(!isfinite(p->pattern_steps[pat]) || p->pattern_steps[pat]<STEPS || p->pattern_steps[pat]>1e15f) return 0;
        for(int c=0;c<CHANNELS;c++) for(int n=0;n<NOTES;n++) {
            Note v=p->notes[pat][c][n];
            if(v.pitch>127 || v.velocity>127 || !isfinite(v.start) || !isfinite(v.length) || v.start<0 || v.start>=1e15f || v.length<0 || v.length>1e15f-v.start) return 0;
        }
    }
    for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) {
        unsigned source=p->clips[l][b];
        if(source>SOURCES || (source && source<=PATTERNS && source>(unsigned)p->pattern_count)) return 0;
        if(!isfinite(p->clip_starts[l][b]) || p->clip_starts[l][b]<0 || p->clip_starts[l][b]>=1e15f ||
           !isfinite(p->clip_steps[l][b]) || p->clip_steps[l][b]<0 || p->clip_steps[l][b]>1e15f ||
           !isfinite(p->clip_offsets[l][b]) || p->clip_offsets[l][b]<0 || p->clip_offsets[l][b]>1e15f) return 0;
    }
    return 1;
}

_Static_assert(sizeof(float)==4 && sizeof(int)==4 && sizeof(unsigned)==4,
               "LLP scalar mappings require 32-bit float/int/unsigned");
typedef enum { F32,I32,U32,U8,STRING } FieldType;
typedef struct { const char *name; FieldType type; size_t offset,count,inner,outer_stride,inner_stride,width; } Field;
#define FLAT(m,t,k) {#m,k,offsetof(Project,m),sizeof(((Project *)0)->m)/sizeof(t),1,sizeof(t),0,sizeof(t)}
#define SCALAR(m,t,k) {#m,k,offsetof(Project,m),1,1,sizeof(t),0,sizeof(t)}
#define TEXT(m,n,w) {#m,STRING,offsetof(Project,m),n,1,w,0,w}
#define MEMBER(m,t,f,k,n) {#m "." #f,k,offsetof(Project,m)+offsetof(t,f),n,1,sizeof(t),0,sizeof(((t *)0)->f)}
#define SUB(m,t,s,u,f,k,n) {#m "." #s "." #f,k,offsetof(Project,m)+offsetof(t,s)+offsetof(u,f),n,1,sizeof(t),0,sizeof(((u *)0)->f)}
/* Stable file schema; offsets/strides map logical values without saving padding. */
static const Field fields[]={
    SCALAR(bpm,float,F32),
    SCALAR(master,float,F32),
    SCALAR(master_pitch,float,F32),
    SCALAR(master_width,float,F32),
    SCALAR(swing,float,F32),
    SCALAR(pattern_count,int,I32),
    SCALAR(channel_count,int,I32),
    SCALAR(insert_count,int,I32),
    SCALAR(automation_count,int,I32),
    SCALAR(midi_binding_count,int,I32),
    SCALAR(master_mute,uint8_t,U8),
    FLAT(volume,float,F32),
    FLAT(pan,float,F32),
    FLAT(channel_pitch,float,F32),
    FLAT(pitch_range,float,F32),
    FLAT(clip_steps,float,F32),
    FLAT(clip_starts,float,F32),
    FLAT(clip_offsets,float,F32),
    FLAT(audio_seconds,float,F32),
    FLAT(pattern_steps,float,F32),
    FLAT(insert_volume,float,F32),
    FLAT(insert_pan,float,F32),
    FLAT(insert_width,float,F32),
    FLAT(effect_mix,float,F32),
    FLAT(mute,uint8_t,U8),
    FLAT(clips,uint8_t,U8),
    FLAT(channel_audio,uint8_t,U8),
    FLAT(route,uint8_t,U8),
    FLAT(insert_mute,uint8_t,U8),
    FLAT(insert_output,uint8_t,U8),
    FLAT(effect_type,uint8_t,U8),
    FLAT(effect_bypass,uint8_t,U8),
    FLAT(lane_mute,uint8_t,U8),
    FLAT(instrument,uint8_t,U8),
    FLAT(pattern_colors,uint32_t,U32),
    FLAT(channel_colors,uint32_t,U32),
    TEXT(paths,CHANNELS,1024),
    TEXT(channel_names,CHANNELS,PATTERN_NAME),
    TEXT(pattern_names,PATTERNS,PATTERN_NAME),
    TEXT(audio_io,(INSERTS+1)*2,128),
    TEXT(track_names,LANES,PATTERN_NAME),
    TEXT(insert_names,INSERTS,PATTERN_NAME),
    MEMBER(notes,Note,pitch,U8,PATTERNS*CHANNELS*NOTES),
    MEMBER(notes,Note,velocity,U8,PATTERNS*CHANNELS*NOTES),
    MEMBER(notes,Note,start,F32,PATTERNS*CHANNELS*NOTES),
    MEMBER(notes,Note,length,F32,PATTERNS*CHANNELS*NOTES),
    MEMBER(sampler,Sampler,pitch,F32,CHANNELS),
    MEMBER(sampler,Sampler,time,F32,CHANNELS),
    MEMBER(sampler,Sampler,start,F32,CHANNELS),
    MEMBER(sampler,Sampler,length,F32,CHANNELS),
    MEMBER(sampler,Sampler,trim,F32,CHANNELS),
    MEMBER(sampler,Sampler,fit_bpm,F32,CHANNELS),
    MEMBER(sampler,Sampler,flags,U8,CHANNELS),
    MEMBER(sampler,Sampler,stretch,U8,CHANNELS),
    MEMBER(fm,FMSettings,ratio,F32,CHANNELS),
    MEMBER(fm,FMSettings,depth,F32,CHANNELS),
    MEMBER(fm,FMSettings,attack,F32,CHANNELS),
    MEMBER(fm,FMSettings,decay,F32,CHANNELS),
    MEMBER(fm,FMSettings,sustain,F32,CHANNELS),
    MEMBER(fm,FMSettings,release,F32,CHANNELS),
    MEMBER(fm,FMSettings,mod_decay,F32,CHANNELS),
    MEMBER(fm,FMSettings,mod_sustain,F32,CHANNELS),
    MEMBER(fm,FMSettings,velocity,F32,CHANNELS),
    MEMBER(fm,FMSettings,lfo_rate,F32,CHANNELS),
    MEMBER(fm,FMSettings,vibrato,F32,CHANNELS),
    MEMBER(fm,FMSettings,tremolo,F32,CHANNELS),
    MEMBER(fm,FMSettings,carrier_ratio,F32,CHANNELS),
    MEMBER(fm,FMSettings,carrier_detune,F32,CHANNELS),
    MEMBER(fm,FMSettings,body_detune,F32,CHANNELS),
    MEMBER(fm,FMSettings,mod_attack,F32,CHANNELS),
    MEMBER(fm,FMSettings,mod_release,F32,CHANNELS),
    MEMBER(fm,FMSettings,attack_ratio,F32,CHANNELS),
    MEMBER(fm,FMSettings,attack_detune,F32,CHANNELS),
    MEMBER(fm,FMSettings,attack_depth,F32,CHANNELS),
    MEMBER(fm,FMSettings,attack_attack,F32,CHANNELS),
    MEMBER(fm,FMSettings,attack_decay,F32,CHANNELS),
    MEMBER(fm,FMSettings,attack_sustain,F32,CHANNELS),
    MEMBER(fm,FMSettings,attack_release,F32,CHANNELS),
    MEMBER(fm,FMSettings,routing,F32,CHANNELS),
    MEMBER(fm,FMSettings,lfo_shape,F32,CHANNELS),
    MEMBER(fm,FMSettings,lfo_fade,F32,CHANNELS),
    MEMBER(fm,FMSettings,body_pitch,F32,CHANNELS),
    MEMBER(fm,FMSettings,engine,F32,CHANNELS),
    SUB(fm,FMSettings,analog,AnalogSettings,wave,F32,CHANNELS),
    SUB(fm,FMSettings,analog,AnalogSettings,detune,F32,CHANNELS),
    SUB(fm,FMSettings,analog,AnalogSettings,mix,F32,CHANNELS),
    SUB(fm,FMSettings,analog,AnalogSettings,sub,F32,CHANNELS),
    SUB(fm,FMSettings,analog,AnalogSettings,noise,F32,CHANNELS),
    SUB(fm,FMSettings,analog,AnalogSettings,pulse,F32,CHANNELS),
    SUB(fm,FMSettings,analog,AnalogSettings,pwm,F32,CHANNELS),
    SUB(fm,FMSettings,analog,AnalogSettings,cutoff,F32,CHANNELS),
    SUB(fm,FMSettings,analog,AnalogSettings,resonance,F32,CHANNELS),
    SUB(fm,FMSettings,analog,AnalogSettings,filter_env,F32,CHANNELS),
    SUB(fm,FMSettings,analog,AnalogSettings,chorus,F32,CHANNELS),
    {"fm.dx7.value",F32,offsetof(Project,fm)+offsetof(FMSettings,dx7)+offsetof(DX7Settings,value),CHANNELS*DX7_PARAMETERS,DX7_PARAMETERS,sizeof(FMSettings),sizeof(float),sizeof(float)},
    MEMBER(chorus,ChorusSettings,rate,F32,(INSERTS+1)*EFFECT_SLOTS),
    MEMBER(chorus,ChorusSettings,depth,F32,(INSERTS+1)*EFFECT_SLOTS),
    {"eq.bands.frequency",F32,offsetof(Project,eq)+offsetof(EQSettings,bands)+offsetof(EQBand,frequency),(INSERTS+1)*EFFECT_SLOTS*EQ_BANDS,EQ_BANDS,sizeof(EQSettings),sizeof(EQBand),sizeof(((EQBand *)0)->frequency)},
    {"eq.bands.gain",F32,offsetof(Project,eq)+offsetof(EQSettings,bands)+offsetof(EQBand,gain),(INSERTS+1)*EFFECT_SLOTS*EQ_BANDS,EQ_BANDS,sizeof(EQSettings),sizeof(EQBand),sizeof(((EQBand *)0)->gain)},
    {"eq.bands.q",F32,offsetof(Project,eq)+offsetof(EQSettings,bands)+offsetof(EQBand,q),(INSERTS+1)*EFFECT_SLOTS*EQ_BANDS,EQ_BANDS,sizeof(EQSettings),sizeof(EQBand),sizeof(((EQBand *)0)->q)},
    {"eq.bands.shape",U32,offsetof(Project,eq)+offsetof(EQSettings,bands)+offsetof(EQBand,shape),(INSERTS+1)*EFFECT_SLOTS*EQ_BANDS,EQ_BANDS,sizeof(EQSettings),sizeof(EQBand),sizeof(((EQBand *)0)->shape)},
    SUB(automations,Automation,target,ParameterTarget,parameter,U32,AUTOMATIONS),
    SUB(automations,Automation,target,ParameterTarget,owner,U32,AUTOMATIONS),
    SUB(automations,Automation,target,ParameterTarget,slot,U32,AUTOMATIONS),
    MEMBER(automations,Automation,steps,F32,AUTOMATIONS),
    MEMBER(automations,Automation,count,I32,AUTOMATIONS),
    MEMBER(automations,Automation,color,U32,AUTOMATIONS),
    {"automations.name",STRING,offsetof(Project,automations)+offsetof(Automation,name),AUTOMATIONS,1,sizeof(Automation),0,48},
    {"automations.points.step",F32,offsetof(Project,automations)+offsetof(Automation,points)+offsetof(AutomationPoint,step),AUTOMATIONS*AUTOMATION_POINTS,AUTOMATION_POINTS,sizeof(Automation),sizeof(AutomationPoint),sizeof(float)},
    {"automations.points.value",F32,offsetof(Project,automations)+offsetof(Automation,points)+offsetof(AutomationPoint,value),AUTOMATIONS*AUTOMATION_POINTS,AUTOMATION_POINTS,sizeof(Automation),sizeof(AutomationPoint),sizeof(float)},
    MEMBER(midi_bindings,MidiBinding,channel,U32,MIDI_BINDINGS),
    MEMBER(midi_bindings,MidiBinding,controller,U32,MIDI_BINDINGS),
    SUB(midi_bindings,MidiBinding,target,ParameterTarget,parameter,U32,MIDI_BINDINGS),
    SUB(midi_bindings,MidiBinding,target,ParameterTarget,owner,U32,MIDI_BINDINGS),
    SUB(midi_bindings,MidiBinding,target,ParameterTarget,slot,U32,MIDI_BINDINGS),
};
#undef FLAT
#undef SCALAR
#undef TEXT
#undef MEMBER
#undef SUB
#define FIELD_COUNT (sizeof fields/sizeof *fields)
#define MAX_FIELDS (FIELD_COUNT+64)
static const char *type_names[]={"f32","i32","u32","u8","str"};
static unsigned char *address(Project *p,const Field *f,size_t i) {
    return (unsigned char *)p+f->offset+(i/f->inner)*f->outer_stride+(i%f->inner)*f->inner_stride;
}
static int structure_valid(const Project *p) {
    if(!p) return 0;
    for(size_t f=0;f<FIELD_COUNT;f++)
        for(size_t i=0;i<fields[f].count;i++) {
            const char *s=(const char *)address((Project *)p,&fields[f],i);
            if(fields[f].type==STRING && (!memchr(s,0,fields[f].width) || strchr(s,'\n') || strchr(s,'\r'))) return 0;
            if(fields[f].type==F32) { float v; memcpy(&v,s,sizeof v); if(!isfinite(v)) return 0; }
        }
    return project_valid(p);
}
static int equal_value(const unsigned char *a,const unsigned char *b,const Field *f) {
    return f->type==STRING?!strcmp((const char *)a,(const char *)b):!memcmp(a,b,f->width);
}
int project_save(const char *path,const Project *p) {
    if(!structure_valid(p)) return 0;
    AtomicFile out; if(!atomic_file_open(&out,path)) return 0;
    FILE *file=out.file;
    fprintf(file,PROJECT_FORMAT_MAGIC " %d\n",PROJECT_FORMAT_VERSION);
    for(size_t n=0;n<FIELD_COUNT;n++) {
        const Field *f=&fields[n]; fprintf(file,"%s %s %zu\n",f->name,type_names[f->type],f->count);
        for(size_t i=0;i<f->count;) {
            const unsigned char *value=address((Project *)p,f,i); size_t run=1;
            while(i+run<f->count && equal_value(value,address((Project *)p,f,i+run),f)) run++;
            fprintf(file,"%zu ",run);
            if(f->type==STRING) {
                size_t length=strlen((const char *)value);
                fprintf(file,"%zu:",length); fwrite(value,1,length,file); fputc('\n',file);
            } else if(f->type==F32) { float v; memcpy(&v,value,sizeof v); fprintf(file,"%.9g\n",v); }
            else if(f->type==I32) { int v; memcpy(&v,value,sizeof v); fprintf(file,"%d\n",v); }
            else if(f->type==U32) { unsigned v; memcpy(&v,value,sizeof v); fprintf(file,"%u\n",v); }
            else fprintf(file,"%u\n",(unsigned)*value);
            i+=run;
        }
    }
    fputs("end\n",file);
    return atomic_file_commit(&out);
}
static int line(FILE *f,char *out,size_t size) {
    size_t used=0; int ch;
    while((ch=fgetc(f))!=EOF && ch!='\n') {
        if(!ch || used+1>=size) return 0;
        out[used++]=(char)ch;
    }
    if(ch!='\n') return 0;
    if(used && out[used-1]=='\r') used--;
    out[used]=0; return 1;
}
static int blank(const char *s) { while(*s && isspace((unsigned char)*s)) s++; return !*s; }
/* Parse bounded counts without scanf's undefined behavior on integer overflow. */
static int count_token(const char *text,size_t *value,size_t limit) {
    if(!isdigit((unsigned char)*text)) return 0;
    char *end; errno=0; unsigned long long n=strtoull(text,&end,10);
    if(errno || !blank(end) || n>limit) return 0;
    *value=(size_t)n; return 1;
}
static int number_prefix(FILE *file,size_t *value,int delimiter,size_t limit) {
    size_t n=0; int ch=fgetc(file),digits=0;
    while(ch>='0' && ch<='9') {
        unsigned d=ch-'0'; if(n>limit/10 || (n==limit/10 && d>limit%10)) return 0;
        n=n*10+d; digits++; ch=fgetc(file);
    }
    if(!digits || ch!=delimiter) return 0;
    *value=n; return 1;
}
static int eol(FILE *f) { int ch=fgetc(f); return ch=='\n' || (ch=='\r' && fgetc(f)=='\n'); }
static int read_values(FILE *file,Project *p,const Field *field,FieldType type,size_t count) {
    size_t used=0;
    while(used<count) {
        size_t run; if(!number_prefix(file,&run,' ',count-used) || !run) return 0;
        unsigned char value[1024]={0};
        if(type==STRING) {
            size_t length;
            if(!number_prefix(file,&length,':',sizeof value-1) || (field && length>=field->width) ||
               fread(value,1,length,file)!=length || !eol(file) || memchr(value,0,length) ||
               memchr(value,'\n',length) || memchr(value,'\r',length)) return 0;
        } else {
            char text[128],*end; if(!line(file,text,sizeof text) || !text[0]) return 0;
            errno=0;
            if(type==F32) {
                /* Parse directly to binary32: %.9g of FLT_MAX can be a little
                   above its exact double value, while still rounding to it. */
                float v=strtof(text,&end);
                if(end==text || !blank(end) || !isfinite(v) || (errno && (errno!=ERANGE || v==0))) return 0;
                memcpy(value,&v,sizeof v);
            } else if(type==I32) {
                long long v=strtoll(text,&end,10); if(end==text || errno || !blank(end) || v<INT32_MIN || v>INT32_MAX) return 0;
                int i=(int)v; memcpy(value,&i,sizeof i);
            } else {
                if(text[0]=='-') return 0;
                unsigned long long v=strtoull(text,&end,10);
                if(end==text || errno || !blank(end) || v>(type==U8?UINT8_MAX:UINT32_MAX)) return 0;
                if(type==U8) *value=(uint8_t)v; else { unsigned u=(unsigned)v; memcpy(value,&u,sizeof u); }
            }
        }
        if(field) for(size_t i=0;i<run;i++) memcpy(address(p,field,used+i),value,field->width);
        used+=run;
    }
    return 1;
}
int project_load(const char *path,Project *p) {
    FILE *file=fopen(path,"rb"); if(!file) return 0;
    if(fseek(file,0,SEEK_END) || ftell(file)<0 || ftell(file)>32*1024*1024 || fseek(file,0,SEEK_SET)) { fclose(file); return 0; }
    char text[256],magic[64]; size_t version; int consumed=0;
    int ok=p && line(file,text,sizeof text) && sscanf(text,"%63s %n",magic,&consumed)==1 &&
        !strcmp(magic,PROJECT_FORMAT_MAGIC) && count_token(text+consumed,&version,PROJECT_FORMAT_VERSION) && version==PROJECT_FORMAT_VERSION;
    Project *next=ok?calloc(1,sizeof *next):NULL;
    char (*names)[64]=ok?calloc(MAX_FIELDS,sizeof *names):NULL;
    if(ok && (!next || !names)) ok=0;
    unsigned char seen[FIELD_COUNT]={0}; size_t records=0; int ended=0;
    while(ok && line(file,text,sizeof text)) {
        if(!strcmp(text,"end")) { ended=1; break; }
        char name[64],kind[8]; size_t count; consumed=0;
        ok=records<MAX_FIELDS && sscanf(text,"%63s %7s %n",name,kind,&consumed)==2 &&
            count_token(text+consumed,&count,1048576) && count>0;
        if(!ok) break;
        for(size_t i=0;ok && i<records;i++) if(!strcmp(names[i],name)) ok=0;
        if(!ok) break;
        strcpy(names[records++],name);
        int type=-1; for(int t=0;t<=STRING;t++) if(!strcmp(kind,type_names[t])) type=t;
        const Field *field=NULL;
        for(size_t i=0;i<FIELD_COUNT;i++) if(!strcmp(name,fields[i].name)) { field=&fields[i]; seen[i]=1; break; }
        ok=type>=0 && (field?field->type==(FieldType)type && field->count==count:!strncmp(name,"x-",2));
        if(ok) ok=read_values(file,next,field,(FieldType)type,count);
    }
    ok=ok && ended && !ferror(file);
    for(size_t i=0;ok && i<FIELD_COUNT;i++) if(!seen[i]) ok=0;
    for(int ch;ok && (ch=fgetc(file))!=EOF;) if(!isspace((unsigned char)ch)) ok=0;
    ok=ok && !ferror(file) && structure_valid(next);
    if(ok) *p=*next;
    free(next); free(names); fclose(file); return ok;
}


static void le(FILE *f,uint32_t x,int n) { for(int i=0;i<n;i++) fputc((x>>(i*8))&255,f); }
int export_wav(const char *path,const Project *pr,const Sample s[CHANNELS]) {
    double total=ceil(song_steps(pr)*RATE*60.0/pr->bpm/4);
    if(total<0 || total>(UINT32_MAX-36)/4 || !isfinite(total)) return 0;
    uint32_t frames=(uint32_t)total;
    AtomicFile output; if(!atomic_file_open(&output,path)) return 0;
    FILE *f=output.file;
    fwrite("RIFF",1,4,f); le(f,36+frames*4,4); fwrite("WAVEfmt ",1,8,f); le(f,16,4); le(f,1,2); le(f,2,2); le(f,RATE,4); le(f,RATE*4,4); le(f,4,2); le(f,16,2); fwrite("data",1,4,f); le(f,frames*4,4);
    Player player; player_reset(&player); player.song=1; float block[1024];
    player.effects=effects_create(INSERTS+1);
    if(!player.effects) { atomic_file_abort(&output); return 0; }
    for(uint32_t i=0;i<frames;) {
        unsigned n=frames-i>512?512:frames-i; render(&player,pr,s,block,n);
        for(unsigned j=0;j<n*2;j++) {
            float x=isfinite(block[j])?fmaxf(-1,fminf(1,block[j])):0;
            le(f,(uint16_t)(int16_t)(x*32767),2);
        }
        i+=n;
    }
    effects_free(player.effects);
    return atomic_file_commit(&output);
}
