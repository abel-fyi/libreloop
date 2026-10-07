// SPDX-License-Identifier: GPL-3.0-only
#include "preset.h"
#include "atomic_file.h"
#include "project_assets.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
static int valid(const DevicePreset *p) {
    if(p->kind==PRESET_SAMPLER) return sampler_valid(p->sampler) && strnlen(p->sample_path,sizeof p->sample_path)<sizeof p->sample_path && !strchr(p->sample_path,'\n') && !strchr(p->sample_path,'\r');
    if(p->kind==PRESET_FM) return fm_valid(p->fm);
    return ((p->kind==PRESET_CHORUS && chorus_valid(p->chorus)) || (p->kind==PRESET_EQ && equalizer_valid(p->eq))) && isfinite(p->mix) && p->mix>=0 && p->mix<=1;
}
int preset_save(const char *path,const DevicePreset *p) {
    if(!valid(p)) return 0;
    char reference[1024]={0};
    if(p->kind==PRESET_SAMPLER) {
        if(p->sample_path[0] && strcmp(p->sample_path,SAMPLE_EMPTY)) {
            if(!project_relative_reference(path,p->sample_path,reference,sizeof reference)) return 0;
        } else snprintf(reference,sizeof reference,"%s",p->sample_path);
    }
    AtomicFile out; if(!atomic_file_open(&out,path)) return 0;
    fprintf(out.file,"LIBRELOOP_PRESET 6 %d\n",p->kind);
    if(p->kind==PRESET_FM) {
        FMSettings s=p->fm; fprintf(out.file,"%.9g %.9g %.9g %.9g %.9g %.9g\n",s.ratio,s.depth,s.attack,s.decay,s.sustain,s.release);
        fprintf(out.file,"%.9g %.9g %.9g %.9g %.9g %.9g\n",s.mod_decay,s.mod_sustain,s.velocity,s.lfo_rate,s.vibrato,s.tremolo);
        for(unsigned id=PARAM_FM_CARRIER_RATIO;id<=PARAM_FM_BODY_PITCH;id++) fprintf(out.file,"%.9g ",*fm_parameter_pointer(&s,id));
        fputc('\n',out.file);
        for(unsigned id=PARAM_FM_ENGINE;id<=PARAM_FM_LAST;id++) fprintf(out.file,"%.9g ",*fm_parameter_pointer(&s,id));
        for(int i=0;i<DX7_PARAMETERS;i++) fprintf(out.file,"%.9g ",s.dx7.value[i]);
        fputc('\n',out.file);
    } else if(p->kind==PRESET_CHORUS) fprintf(out.file,"%.9g %.9g %.9g\n",p->chorus.rate,p->chorus.depth,p->mix);
    else if(p->kind==PRESET_EQ) {
        for(int b=0;b<EQ_BANDS;b++) { EQBand v=p->eq.bands[b]; fprintf(out.file,"%.9g %.9g %.9g %u\n",v.frequency,v.gain,v.q,v.shape); }
        fprintf(out.file,"%.9g\n",p->mix);
    } else {
        Sampler s=p->sampler;
        fprintf(out.file,"%.9g %.9g %.9g %.9g %.9g %u %u %.9g\n%s\n",s.pitch,s.time,s.start,s.length,s.trim,s.flags,s.stretch,s.fit_bpm,reference);
    }
    return atomic_file_commit(&out);
}
int preset_load(const char *path,DevicePreset *p) {
    FILE *f=fopen(path,"r"); if(!f) return 0;
    DevicePreset next={0}; char magic[32]; int version;
    int ok=fscanf(f,"%31s %d %d",magic,&version,&next.kind)==3 && !strcmp(magic,"LIBRELOOP_PRESET") && (version>=1 && version<=6);
    if(ok && next.kind==PRESET_FM) {
        next.fm=fm_legacy(); FMSettings *s=&next.fm; ok=fscanf(f,"%f %f %f %f %f %f",&s->ratio,&s->depth,&s->attack,&s->decay,&s->sustain,&s->release)==6;
        if(ok && version>=3) ok=fscanf(f,"%f %f %f %f %f %f",&s->mod_decay,&s->mod_sustain,&s->velocity,&s->lfo_rate,&s->vibrato,&s->tremolo)==6;
        if(ok && version>=4) for(unsigned id=PARAM_FM_CARRIER_RATIO;ok && id<=PARAM_FM_BODY_PITCH;id++)
            ok=fscanf(f,"%f",(float *)fm_parameter_pointer(s,id))==1;
        if(ok && version>=5) {
            for(unsigned id=PARAM_FM_ENGINE;ok && id<=PARAM_FM_LAST;id++) ok=fscanf(f,"%f",(float *)fm_parameter_pointer(s,id))==1;
            for(int i=0;ok && i<(version>=6?DX7_PARAMETERS:DX7_NATIVE_PARAMETERS);i++) ok=fscanf(f,"%f",&s->dx7.value[i])==1;
            if(version<6) dx7_legacy_tracking(&s->dx7);
        }
    } else if(ok && next.kind==PRESET_CHORUS) ok=fscanf(f,"%f %f %f",&next.chorus.rate,&next.chorus.depth,&next.mix)==3;
    else if(ok && next.kind==PRESET_EQ) {
        next.eq=version==1?equalizer_legacy():equalizer_default();
        for(int b=0;ok && b<(version==1?4:EQ_BANDS);b++) {
            EQBand *v=&next.eq.bands[b]; ok=fscanf(f,"%f %f %f",&v->frequency,&v->gain,&v->q)==3;
            if(ok && version>=2) ok=fscanf(f,"%u",&v->shape)==1;
        }
        ok=ok && fscanf(f,"%f",&next.mix)==1;
    }
    else if(ok && next.kind==PRESET_SAMPLER) {
        unsigned flags,mode; Sampler *s=&next.sampler;
        ok=fscanf(f,"%f %f %f %f %f %u %u %f",&s->pitch,&s->time,&s->start,&s->length,&s->trim,&flags,&mode,&s->fit_bpm)==8 && flags<=7 && mode<=1;
        if(ok) { s->flags=flags; s->stretch=mode; ok=fgetc(f)=='\n' && fgets(next.sample_path,sizeof next.sample_path,f)!=NULL; }
        if(ok) {
            char *newline=strchr(next.sample_path,'\n'); ok=newline!=NULL; if(newline) *newline=0;
            if(ok && next.sample_path[0] && strcmp(next.sample_path,SAMPLE_EMPTY)) {
                char resolved[1024]; ok=project_sample_path(path,next.sample_path,resolved,sizeof resolved);
                if(ok) memcpy(next.sample_path,resolved,strlen(resolved)+1);
            }
        }
    } else ok=0;
    ok=ok && valid(&next);
    fclose(f); if(ok) *p=next; return ok;
}
