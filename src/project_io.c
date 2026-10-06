// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include "atomic_file.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int read_line(FILE *f,char *out,size_t capacity) {
    char line[1025];
    if(capacity>1024 || !fgets(line,(int)capacity+1,f)) return 0;
    char *newline=strchr(line,'\n'); if(!newline || strchr(line,'\r')) return 0;
    *newline=0; memset(out,0,capacity); memcpy(out,line,strlen(line)+1); return 1;
}
int project_save(const char *path,const Project *p) {
    if(!automation_valid(p)) return 0;
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
    AtomicFile output; if(!atomic_file_open(&output,path)) return 0;
    FILE *f=output.file;
    for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) if(!isfinite(p->clip_offsets[l][b]) || p->clip_offsets[l][b]<0 || p->clip_offsets[l][b]>1e15f) { atomic_file_abort(&output); return 0; }
    fprintf(f,"HOMEBEAT 38\n%.9g %.9g %d\n",p->bpm,p->master,p->channel_count);
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
    for(int bus=0;bus<=INSERTS;bus++) for(int slot=0;slot<EFFECT_SLOTS;slot++)
        fprintf(f,"%u %.9g %.9g\n",p->effect_type[bus][slot],p->chorus[bus][slot].rate,p->chorus[bus][slot].depth);
    for(int c=0;c<CHANNELS;c++) {
        FMSettings v=p->fm[c]; fprintf(f,"%u %.9g %.9g %.9g %.9g %.9g %.9g\n",p->instrument[c],v.ratio,v.depth,v.attack,v.decay,v.sustain,v.release);
    }
    for(int c=0;c<CHANNELS;c++) { FMSettings v=p->fm[c]; fprintf(f,"%.9g %.9g %.9g %.9g %.9g %.9g\n",v.mod_decay,v.mod_sustain,v.velocity,v.lfo_rate,v.vibrato,v.tremolo); }
    for(int c=0;c<CHANNELS;c++) {
        for(unsigned id=PARAM_FM_CARRIER_RATIO;id<=PARAM_FM_LAST;id++) fprintf(f,"%.9g ",*fm_parameter_pointer(&p->fm[c],id));
        fputc('\n',f);
    }
    for(int bus=0;bus<=INSERTS;bus++) for(int slot=0;slot<EFFECT_SLOTS;slot++) for(int b=0;b<EQ_BANDS;b++) {
        EQBand v=p->eq[bus][slot].bands[b]; fprintf(f,"%.9g %.9g %.9g %u\n",v.frequency,v.gain,v.q,v.shape);
    }
    return atomic_file_commit(&output);
}
int project_load(const char *path,Project *p) {
    FILE *f=fopen(path,"r"); if(!f) return 0;
    Project q; project_default(&q); memset(q.notes,0,sizeof q.notes); q.pattern_count=PATTERNS; char magic[32]; int version=0; unsigned x,y;
    for(int pat=0;pat<PATTERNS;pat++) { q.pattern_steps[pat]=STEPS; snprintf(q.pattern_names[pat],PATTERN_NAME,"Pattern %d",pat+1); }
    q.insert_count=4;
    for(int i=0;i<INSERTS;i++) q.insert_volume[i]=1;
    for(int c=0;c<4;c++) q.route[c]=c+1;
    for(int c=0;c<CHANNELS;c++) q.fm[c]=fm_legacy();
    int ok=fscanf(f,"%31s %d",magic,&version)==2 && !strcmp(magic,"HOMEBEAT") && version>=1 && version<=38;
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
    if(version>=33) for(int bus=0;ok && bus<=INSERTS;bus++) for(int slot=0;ok && slot<EFFECT_SLOTS;slot++) {
        ok=fscanf(f,"%u %f %f",&x,&q.chorus[bus][slot].rate,&q.chorus[bus][slot].depth)==3 && x<=(version>=35?EFFECT_EQ:EFFECT_CHORUS) && chorus_valid(q.chorus[bus][slot]);
        if(ok) q.effect_type[bus][slot]=x;
    }
    if(version>=34) for(int c=0;ok && c<CHANNELS;c++) {
        FMSettings *v=&q.fm[c];
        ok=fscanf(f,"%u %f %f %f %f %f %f",&x,&v->ratio,&v->depth,&v->attack,&v->decay,&v->sustain,&v->release)==7 && x<=INSTRUMENT_FM && fm_valid(*v) && !(x==INSTRUMENT_FM && q.channel_audio[c]);
        if(ok) q.instrument[c]=x;
    }
    if(version>=37) for(int c=0;ok && c<CHANNELS;c++) {
        FMSettings *v=&q.fm[c];
        ok=fscanf(f,"%f %f %f %f %f %f",&v->mod_decay,&v->mod_sustain,&v->velocity,&v->lfo_rate,&v->vibrato,&v->tremolo)==6 && fm_valid(*v);
    }
    if(version>=38) for(int c=0;ok && c<CHANNELS;c++) {
        for(unsigned id=PARAM_FM_CARRIER_RATIO;ok && id<=PARAM_FM_LAST;id++)
            ok=fscanf(f,"%f",(float *)fm_parameter_pointer(&q.fm[c],id))==1;
        ok=ok && fm_valid(q.fm[c]);
    }
    if(version>=35) for(int bus=0;ok && bus<=INSERTS;bus++) for(int slot=0;ok && slot<EFFECT_SLOTS;slot++) {
        if(version==35) q.eq[bus][slot]=equalizer_legacy();
        for(int b=0;ok && b<(version==35?4:EQ_BANDS);b++) {
            EQBand *v=&q.eq[bus][slot].bands[b]; ok=fscanf(f,"%f %f %f",&v->frequency,&v->gain,&v->q)==3;
            if(ok && version>=36) ok=fscanf(f,"%u",&v->shape)==1;
        }
        ok=ok && equalizer_valid(q.eq[bus][slot]);
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
