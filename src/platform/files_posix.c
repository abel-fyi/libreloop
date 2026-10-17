// SPDX-License-Identifier: GPL-3.0-only
#include "files.h"
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
struct PlatformDirectory { DIR *handle; char path[LL_PATH_CAPACITY]; };
int platform_canonical_path(const char *path,char *out,size_t capacity) {
    char resolved[LL_PATH_CAPACITY]; if(!realpath(path,resolved)) return 0;
    size_t n=strlen(resolved); if(n>=capacity) { errno=ENAMETOOLONG; return 0; }
    memcpy(out,resolved,n+1); return 1;
}
int platform_absolute_path(const char *path) { return path[0]=='/'; }
int platform_working_directory(char *out,size_t capacity) { return getcwd(out,capacity)!=NULL; }
int platform_file_kind(const char *path,int *kind) {
    struct stat info; if(stat(path,&info)) return 0;
    *kind=S_ISDIR(info.st_mode)?PLATFORM_DIRECTORY:S_ISREG(info.st_mode)?PLATFORM_FILE:PLATFORM_OTHER; return 1;
}
int platform_path_access(const char *path,int writable) { return access(path,writable?W_OK:R_OK)==0; }
int platform_path_exists(const char *path) { return access(path,F_OK)==0; }
PlatformDirectory *platform_directory_open(const char *path) {
    DIR *handle=opendir(path); if(!handle) return NULL;
    PlatformDirectory *directory=malloc(sizeof *directory);
    if(!directory) { closedir(handle); return NULL; }
    directory->handle=handle; snprintf(directory->path,sizeof directory->path,"%s",path); return directory;
}
int platform_directory_next(PlatformDirectory *directory,PlatformEntry *out) {
    for(;;) {
        errno=0; struct dirent *entry=readdir(directory->handle);
        if(!entry) return errno?-1:0;
        char full[LL_PATH_CAPACITY]; int kind;
        if(snprintf(full,sizeof full,"%s/%s",directory->path,entry->d_name)>=(int)sizeof full || !platform_file_kind(full,&kind)) continue;
        snprintf(out->name,sizeof out->name,"%s",entry->d_name); out->kind=kind; return 1;
    }
}
void platform_directory_close(PlatformDirectory *directory) { if(directory) { closedir(directory->handle); free(directory); } }
int platform_asset_directory(char *path) { return mkdtemp(path)!=NULL; }
void platform_remove_directory(const char *path) { rmdir(path); }

int platform_parent_path(const char *path,char *out,size_t capacity) {
    size_t n=strlen(path); if(n>=capacity) return 0;
    memmove(out,path,n+1); char *slash=strrchr(out,'/');
    if(slash) { if(slash==out) slash[1]=0; else *slash=0; }
    else { if(capacity<2) return 0; strcpy(out,"."); }
    return 1;
}
int platform_relative_path(const char *directory,const char *absolute,char *out,size_t capacity) {
    size_t common=0;
    for(size_t i=0;directory[i] && absolute[i] && directory[i]==absolute[i];i++) if(directory[i]=='/') common=i+1;
    size_t n=strlen(directory);
    if(!strncmp(directory,absolute,n) && absolute[n]=='/') common=n+1;
    size_t used=0;
    if(common<=n) for(size_t i=common;i<=n;i++) if(directory[i]=='/' || directory[i]==0) {
        if(used+3>=capacity) return 0;
        memcpy(out+used,"../",3); used+=3;
    }
    if(strlen(absolute+common)>=capacity-used) return 0;
    strcpy(out+used,absolute+common); return 1;
}

const char *platform_basename(const char *path) {
    const char *slash=strrchr(path,'/'); return slash?slash+1:path;
}
int platform_join_path(const char *directory,const char *name,char *out,size_t capacity) {
    return snprintf(out,capacity,"%s%s%s",directory,!strcmp(directory,"/")?"":"/",name)<(int)capacity;
}
