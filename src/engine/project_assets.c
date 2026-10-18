// SPDX-License-Identifier: GPL-3.0-only
#include "file_io.h"
#include "project_assets.h"
#include "atomic_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "files.h"
#include <math.h>
static int project_directory(const char *path,char directory[LL_PATH_CAPACITY]) {
    char parent[LL_PATH_CAPACITY]; if(strlen(path)>=sizeof parent) return 0;
    if(!platform_parent_path(path,parent,sizeof parent)) return 0;
    return platform_canonical_path(parent,directory,LL_PATH_CAPACITY);
}
int project_sample_path(const char *path,const char *reference,char *out,size_t capacity) {
    char directory[LL_PATH_CAPACITY];
    if(platform_absolute_path(reference)) return snprintf(out,capacity,"%s",reference)<(int)capacity;
    if(!project_directory(path,directory)) return 0;
    return snprintf(out,capacity,"%s/%s",directory,reference)<(int)capacity;
}
int project_relative_reference(const char *path,const char *absolute,char *out,size_t capacity) {
    char directory[LL_PATH_CAPACITY],canonical[LL_PATH_CAPACITY];
    if(!platform_absolute_path(absolute) || !project_directory(path,directory)) return 0;
    return platform_relative_path(directory,platform_canonical_path(absolute,canonical,sizeof canonical)?canonical:absolute,out,capacity);
}
static void le(FILE *f,uint32_t value,int bytes) { for(int i=0;i<bytes;i++) fputc((value>>(8*i))&255,f); }
static int save_sample(const char *path,Sample sample) {
    unsigned channels=sample_channels(sample);
    if(channels>2 || !sample.frames || !sample.data || (uint64_t)sample.frames*channels*4>UINT32_MAX-36) return 0;
    AtomicFile out; if(!atomic_file_open(&out,path)) return 0;
    FILE *f=out.file; uint32_t bytes=sample.frames*channels*4;
    fwrite("RIFF",1,4,f); le(f,36+bytes,4); fwrite("WAVEfmt ",1,8,f);
    le(f,16,4); le(f,3,2); le(f,channels,2); le(f,RATE,4); le(f,RATE*channels*4,4);
    le(f,channels*4,2); le(f,32,2); fwrite("data",1,4,f); le(f,bytes,4);
    unsigned char block[4096];
    size_t count=(size_t)sample.frames*channels;
    for(size_t at=0;at<count;) {
        size_t n=count-at; if(n>1024) n=1024;
        for(size_t i=0;i<n;i++) {
            float value=sample.data[at+i]; if(!isfinite(value)) value=0;
            uint32_t bits; memcpy(&bits,&value,4);
            for(int b=0;b<4;b++) block[i*4+b]=(bits>>(b*8))&255;
        }
        if(fwrite(block,4,n,f)!=n) { atomic_file_abort(&out); return 0; } at+=n;
    }
    return atomic_file_commit(&out);
}
int project_save_assets(const char *path,Project *project,const Sample originals[CHANNELS],int collect) {
    char directory[LL_PATH_CAPACITY],asset_dir[LL_PATH_CAPACITY]={0},created[CHANNELS][LL_PATH_CAPACITY]={{0}};
    if(!project_directory(path,directory)) return 0;
    Project *saved=malloc(sizeof *saved); if(!saved) return 0; *saved=*project;
    int ok=1;
    if(collect) {
        const char *name=platform_basename(path);
        if(snprintf(asset_dir,sizeof asset_dir,"%s/%s.samples-XXXXXX",directory,name)>=(int)sizeof asset_dir || !platform_asset_directory(asset_dir)) ok=0;
    }
    for(int c=0;c<CHANNELS && ok;c++) {
        if(collect && c<project->channel_count && originals[c].frames) {
            ok=snprintf(created[c],sizeof created[c],"%s/channel-%02d.wav",asset_dir,c+1)<(int)sizeof created[c] &&
                strlen(created[c])<sizeof project->paths[c] && save_sample(created[c],originals[c]) && platform_relative_path(directory,created[c],saved->paths[c],sizeof saved->paths[c]);
        } else if(project->paths[c][0] && strcmp(project->paths[c],SAMPLE_EMPTY)) {
            if(collect && c<project->channel_count) { ok=0; break; } /* Do not silently omit missing audio. */
            char absolute[LL_PATH_CAPACITY];
            if(platform_absolute_path(project->paths[c])) snprintf(absolute,sizeof absolute,"%s",project->paths[c]);
            else if(!platform_canonical_path(project->paths[c],absolute,sizeof absolute)) { ok=0; break; }
            ok=platform_relative_path(directory,absolute,saved->paths[c],sizeof saved->paths[c]);
        }
    }
    if(ok) ok=project_save(path,saved);
    if(ok && collect) for(int c=0;c<CHANNELS;c++) if(created[c][0]) {
        size_t length=strlen(created[c]);
        memcpy(project->paths[c],created[c],length+1); /* Length checked before the project is committed. */
    }
    if(!ok && asset_dir[0]) { for(int c=0;c<CHANNELS;c++) if(created[c][0]) platform_file_remove(created[c]); platform_remove_directory(asset_dir); }
    free(saved); return ok;
}

int project_load_assets(const char *path,Project *project,Sample originals[CHANNELS],Sample processed[CHANNELS],ProjectSampleLoad load,int *missing) {
    Project *next=malloc(sizeof *next);
    memset(originals,0,CHANNELS*sizeof *originals); memset(processed,0,CHANNELS*sizeof *processed);
    if(!next || !load || !project_load(path,next)){free(next);return 0;}
    samples_default(originals); int absent=0;
    for(int c=0;c<CHANNELS;c++) {
        if(!strcmp(next->paths[c],SAMPLE_EMPTY)) { sample_free(originals[c]); originals[c]=(Sample){0}; }
        else if(next->paths[c][0]) {
            char resolved[LL_PATH_CAPACITY]; Sample decoded={0};
            if(!project_sample_path(path,next->paths[c],resolved,sizeof resolved) || strlen(resolved)>=sizeof next->paths[c]) goto failed;
            memcpy(next->paths[c],resolved,strlen(resolved)+1);
            sample_free(originals[c]); originals[c]=(Sample){0};
            if(load(resolved,&decoded)) originals[c]=decoded;
            else if(c<next->channel_count) absent++;
        } else if(c<next->channel_count && c<4 && !originals[c].data) goto failed;
    }
    for(int c=0;c<CHANNELS;c++) if(!sample_process(originals[c],next->sampler[c],&processed[c])) goto failed;
    *project=*next;free(next); if(missing) *missing=absent; return 1;
failed:
    for(int c=0;c<CHANNELS;c++) { sample_free(originals[c]); sample_free(processed[c]); originals[c]=processed[c]=(Sample){0}; }
    free(next);return 0;
}
