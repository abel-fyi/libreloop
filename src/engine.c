// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *snap_names[SNAP_COUNT]={"Auto","Bar","Beat","1/2 beat","1/3 beat","Step","1/6 beat","1/2 step","1/3 step","1/4 step"};
float snap_interval(int mode,float pixels) {
    const float intervals[]={0,16,4,2,4.f/3,1,2.f/3,.5f,1.f/3,.25f};
    if(mode>SNAP_AUTO && mode<SNAP_COUNT) return intervals[mode];
    const float automatic[]={.25f,.5f,1,2,4,16};
    for(int i=0;i<6;i++) if(automatic[i]*pixels>=12) return automatic[i];
    return powf(2,ceilf(log2f(12/fmaxf(pixels,1e-30f))));
}
void timeline_zoom(float *span,float *start,float wheel,float anchor,float unit) {
    (void)unit;
    float next=*span*expf(-wheel*logf(1.08f));
    if(!isfinite(next) || next<=0 || !isfinite(1/next)) return;
    *start=fmaxf(0,*start+anchor*(*span-next)); *span=next;
}
float timeline_thumb(float width,float span,float range) { return fminf(width,fmaxf(24,width*span/range)); }
void project_default(Project *p) {
    memset(p, 0, sizeof *p); p->bpm=120; p->master=.7f; p->pattern_count=1; p->channel_count=4;
    for(int pat=0;pat<PATTERNS;pat++) { p->pattern_steps[pat]=STEPS; snprintf(p->pattern_names[pat],PATTERN_NAME,"Pattern %d",pat+1); }
    const char *names[]={"Kick","Snare","Hat","Tone"};
    for(int c=0;c<CHANNELS;c++) { p->volume[c]=.7f; p->sampler[c].time=p->sampler[c].length=1; if(c<4) snprintf(p->channel_names[c],PATTERN_NAME,"%s",names[c]); else snprintf(p->channel_names[c],PATTERN_NAME,"Channel %d",c+1); }
    for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) p->clip_starts[l][b]=b;
    p->insert_count=INSERTS;
    for(int i=0;i<INSERTS;i++) p->insert_volume[i]=1;
    for(int c=0;c<4;c++) p->route[c]=c+1;
    for(int pat=0;pat<1;pat++) for(int i=0;i<STEPS;i++) {
        if(i%4==0) p->notes[pat][0][i]=(Note){60,100,i,0};
        if(i%8==4) p->notes[pat][1][i]=(Note){60,100,i,0};
        if(i%2==0) p->notes[pat][2][i]=(Note){60,70,i,0};
        if(pat && i==15) p->notes[pat][1][i]=(Note){60,75,i,0};
    }
}
int channel_delete(Project *p,int c) {
    if(c<0 || c>=p->channel_count) return 0;
    for(int i=c;i<p->channel_count-1;i++) {
        p->sampler[i]=p->sampler[i+1];
        p->volume[i]=p->volume[i+1]; p->pan[i]=p->pan[i+1]; p->mute[i]=p->mute[i+1]; p->route[i]=p->route[i+1];
        memcpy(p->paths[i],p->paths[i+1],sizeof p->paths[i]);
        memmove(p->channel_names[i],p->channel_names[i+1],PATTERN_NAME);
        size_t length=strlen(p->channel_names[i]); memset(p->channel_names[i]+length,0,PATTERN_NAME-length);
        for(int pat=0;pat<PATTERNS;pat++) memcpy(p->notes[pat][i],p->notes[pat][i+1],sizeof p->notes[pat][i]);
    }
    int last=--p->channel_count; p->sampler[last]=(Sampler){.time=1,.length=1}; p->volume[last]=.7f; p->pan[last]=0; p->mute[last]=p->route[last]=0;
    memset(p->paths[last],0,sizeof p->paths[last]); memset(p->channel_names[last],0,PATTERN_NAME); snprintf(p->channel_names[last],PATTERN_NAME,"Channel %d",last+1);
    for(int pat=0;pat<PATTERNS;pat++) memset(p->notes[pat][last],0,sizeof p->notes[pat][last]);
    return 1;
}
int insert_reset(Project *p,int id) {
    if(id<1 || id>p->insert_count) return 0;
    p->insert_volume[id-1]=1; p->insert_pan[id-1]=0; p->insert_mute[id-1]=0; p->insert_output[id-1]=0;
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
    p->frame=(uint64_t)llround(p->start_step*RATE*60.0/pr->bpm/4); p->last_step=-1;
    memset(p->voices,0,sizeof p->voices);
}
float clip_length(const Project *p,int lane,int bar) { return p->clip_steps[lane][bar]?p->clip_steps[lane][bar]:STEPS; }
float song_steps(const Project *p) {
    float end=STEPS;
    for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) if(p->clips[l][b] && p->clip_starts[l][b]*STEPS+clip_length(p,l,b)>end) end=p->clip_starts[l][b]*STEPS+clip_length(p,l,b);
    return end;
}
static void trigger(Player *p, const Project *pr, int pat,int64_t tick,float remaining) {
    for(int c=0;c<pr->channel_count;c++) for(int i=0;i<NOTES;i++) {
        Note n=pr->notes[pat][c][i];
        if(!n.velocity || (int64_t)llround(n.start*96.0)!=tick || pr->mute[c]) continue;
        int v=0;
        while(v<127 && p->voices[v].gain!=0) v++;
        p->voices[v]=(Voice){c,0,pow(2,((int)n.pitch-60)/12.0),n.length?fmin(n.length,remaining)*RATE*60.0/pr->bpm/4:-1,n.velocity/127.f};
    }
}
static void render_audio(Player *p,const Project *pr,const Sample s[CHANNELS],float *out,unsigned frames,int sequence) {
    float end=p->song?song_steps(pr):pr->pattern_steps[p->pattern];
    if(end<=p->start_step) end=p->start_step+STEPS;
    double stepframes=RATE*60.0/pr->bpm/4/96,pitch_speed=pow(2,pr->master_pitch/12.0);
    float gain_left[CHANNELS],gain_right[CHANNELS];
    int solo=0; for(int i=0;i<pr->insert_count;i++) solo|=pr->insert_mute[i]&2;
    for(int c=0;c<pr->channel_count;c++) {
        float l=pr->mute[c]?0:pr->volume[c]*fminf(1,1-pr->pan[c]),r=pr->mute[c]?0:pr->volume[c]*fminf(1,1+pr->pan[c]);
        int id=pr->route[c],hops=0,audible=!solo;
        while(id && id!=255 && hops++<INSERTS) {
            int i=id-1; float gain=(pr->insert_mute[i]&1)?0:pr->insert_volume[i],pan=pr->insert_pan[i];
            audible|=pr->insert_mute[i]&2;
            l*=gain*fminf(1,1-pan); r*=gain*fminf(1,1+pan); id=pr->insert_output[i];
        }
        gain_left[c]=id || !audible?0:l; gain_right[c]=id || !audible?0:r;
    }
    for(unsigned f=0;f<frames;f++,p->frame++) {
        int64_t step=(int64_t)(p->frame/stepframes);
        if(sequence && step>=end*96) { p->frame=(uint64_t)llround(p->start_step*stepframes*96); step=(int64_t)(p->frame/stepframes); p->last_step=-1; }
        if(sequence && step!=p->last_step) {
            p->last_step=step;
            if(p->song) {
                for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) {
                    int pat=pr->clips[l][b]; int64_t local=step-(int64_t)llround(pr->clip_starts[l][b]*STEPS*96.0); float length=clip_length(pr,l,b);
                    if(pat && local>=0 && local<length*96 && local<pr->pattern_steps[pat-1]*96) trigger(p,pr,pat-1,local,length-local/96.f);
                }
            } else trigger(p,pr,p->pattern,step,end-step/96.f);
        }
        float left=0,right=0;
        for(int v=0;v<128;v++) {
            Voice *voice=&p->voices[v]; if(!voice->gain) continue;
            int c=voice->channel; if(c>=pr->channel_count) { voice->gain=0; continue; } unsigned i=(unsigned)voice->position;
            if(i>=s[c].frames) { voice->gain=0; continue; }
            float a=s[c].data[i], b=i+1<s[c].frames?s[c].data[i+1]:0;
            float x=(a+(b-a)*(voice->position-i))*voice->gain;
            if(voice->remaining>=0) {
                x*=fminf(1,voice->remaining/(RATE*.005f));
                if(--voice->remaining<=0) voice->gain=0;
            }
            left+=x*gain_left[c]; right+=x*gain_right[c];
            voice->position+=voice->speed*pitch_speed;
        }
        if(sequence) { out[f*2]=tanhf(left*pr->master); out[f*2+1]=tanhf(right*pr->master); }
        else if(left || right) { out[f*2]=tanhf(out[f*2]+left*pr->master); out[f*2+1]=tanhf(out[f*2+1]+right*pr->master); }
    }
}
void render(Player *p,const Project *pr,const Sample s[CHANNELS],float *out,unsigned frames) {
    render_audio(p,pr,s,out,frames,1);
}
void render_live(Player *p,const Project *pr,const Sample s[CHANNELS],float *out,unsigned frames) {
    render_audio(p,pr,s,out,frames,0);
}
/* Text format keeps projects inspectable and avoids ABI-dependent struct dumps. */
static int read_line(FILE *f,char *out,size_t capacity) {
    char line[1025];
    if(capacity>1024 || !fgets(line,(int)capacity+1,f)) return 0;
    char *newline=strchr(line,'\n'); if(!newline || strchr(line,'\r')) return 0;
    *newline=0; memset(out,0,capacity); memcpy(out,line,strlen(line)+1); return 1;
}
int project_save(const char *path,const Project *p) {
    if(p->channel_count<0 || p->channel_count>CHANNELS || p->insert_count<0 || p->insert_count>INSERTS) return 0;
    for(int c=0;c<CHANNELS;c++) if(!p->channel_names[c][0] || strchr(p->channel_names[c],'\n') || strchr(p->channel_names[c],'\r')) return 0;
    for(int c=0;c<CHANNELS;c++) if(strchr(p->paths[c],'\n') || strchr(p->paths[c],'\r')) return 0;
    for(int pat=0;pat<PATTERNS;pat++) if(!p->pattern_names[pat][0] || strchr(p->pattern_names[pat],'\n') || strchr(p->pattern_names[pat],'\r')) return 0;
    for(int c=0;c<CHANNELS;c++) if(!sampler_valid(p->sampler[c])) return 0;
    char tmp[4096]; if(snprintf(tmp,sizeof tmp,"%s.tmp",path)>=(int)sizeof tmp) return 0;
    FILE *f=fopen(tmp,"w"); if(!f) return 0;
    fprintf(f,"HOMEBEAT 15\n%.9g %.9g %d\n",p->bpm,p->master,p->channel_count);
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
    for(int c=0;c<CHANNELS;c++) q.volume[c]=.7f;
    for(int c=0;c<4;c++) q.route[c]=c+1;
    int ok=fscanf(f,"%31s %d",magic,&version)==2 && !strcmp(magic,"HOMEBEAT") && version>=1 && version<=15;
    ok=ok && fscanf(f,"%f %f",&q.bpm,&q.master)==2 && isfinite(q.bpm) && q.bpm>=30 && q.bpm<=300 && isfinite(q.master) && q.master>=0 && q.master<=1;
    if(version>=12) ok=ok && fscanf(f,"%d",&q.channel_count)==1 && q.channel_count>=0 && q.channel_count<=CHANNELS;
    int channels=version>=12?CHANNELS:4,inserts=version>=12?INSERTS:16,clips=version>=14?CLIPS:BARS;
    for(int c=0;ok && c<channels;c++) {
        ok=fscanf(f,"%f %f %u",&q.volume[c],&q.pan[c],&x)==3 && isfinite(q.volume[c]) && isfinite(q.pan[c]) && q.volume[c]>=0 && q.volume[c]<=1 && fabsf(q.pan[c])<=1 && x<=1;
        if(ok) q.mute[c]=x;
    }
    for(int a=0;ok && a<PATTERNS;a++) for(int c=0;ok && c<channels;c++) for(int i=0;ok && i<(version>=4?NOTES:STEPS);i++) {
        float start=i,length=0;
        ok=fscanf(f,"%u %u",&x,&y)==2 && x<=127 && y<=127;
        if(version>=4) { float limit=version>=14?1e15f:version==4?STEPS:BARS*STEPS; ok=ok && fscanf(f,"%f %f",&start,&length)==2 && isfinite(start) && isfinite(length) && start>=0 && length>=0 && start<limit && length<=limit-start; }
        if(ok) q.notes[a][c][i]=(Note){x,y,start,length};
    }
    for(int l=0;ok && l<(version>=10?LANES:4);l++) for(int b=0;ok && b<clips;b++) { ok=fscanf(f,"%u",&x)==1 && x<=PATTERNS; if(ok) q.clips[l][b]=x; }
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
            ok=fscanf(f,"%f %f %u",&q.insert_volume[i],&q.insert_pan[i],&x)==3 && isfinite(q.insert_volume[i]) && q.insert_volume[i]>=0 && q.insert_volume[i]<=1 && isfinite(q.insert_pan[i]) && fabsf(q.insert_pan[i])<=1 && x<=(version>=11?3u:1u);
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
        for(int l=0;ok && l<LANES;l++) for(int b=0;ok && b<clips;b++) ok=q.clips[l][b]<=q.pattern_count;
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
    for(uint32_t i=0;i<frames;) { unsigned n=frames-i>512?512:frames-i; render(&player,pr,s,block,n); for(unsigned j=0;j<n*2;j++) le(f,(uint16_t)(int16_t)(block[j]*32767),2); i+=n; }
    int ok=!ferror(f); if(fclose(f)) ok=0; return ok;
}
