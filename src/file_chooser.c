// SPDX-License-Identifier: GPL-3.0-only
#include "file_chooser.h"
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
static void free_entries(FileEntry *entries,int count) {
    for(int i=0;i<count;i++) free(entries[i].name);
    free(entries);
}
void file_chooser_close(FileChooser *c) { free_entries(c->entries,c->count); *c=(FileChooser){.selected=-1}; }
static int compare(const void *a,const void *b) {
    const FileEntry *x=a,*y=b;
    return x->directory!=y->directory?y->directory-x->directory:strcasecmp(x->name,y->name);
}
static int matches(const char *name,const char *extensions) {
    const char *dot=strrchr(name,'.'); if(!dot) return 0;
    size_t length=strlen(dot+1);
    for(const char *at=extensions;*at;) {
        const char *end=strchr(at,';'); size_t n=end?(size_t)(end-at):strlen(at);
        if(n==length && !strncasecmp(dot+1,at,n)) return 1;
        if(!end) break;
        at=end+1;
    }
    return 0;
}
int file_chooser_folder(FileChooser *c,const char *path) {
    char resolved[PATH_MAX];
    if(!realpath(path,resolved)) { snprintf(c->error,sizeof c->error,"Cannot open folder: %s",strerror(errno)); return 0; }
    DIR *dir=opendir(resolved);
    if(!dir) { snprintf(c->error,sizeof c->error,"Cannot read folder: %s",strerror(errno)); return 0; }
    FileEntry *entries=NULL; int count=0,capacity=0,failed=0; struct dirent *entry;
    for(;;) {
        errno=0; entry=readdir(dir);
        if(!entry) { if(errno) failed=1; break; }
        if(!strcmp(entry->d_name,".") || !strcmp(entry->d_name,"..") || (!c->hidden && entry->d_name[0]=='.')) continue;
        char full[PATH_MAX]; struct stat info;
        if(snprintf(full,sizeof full,"%s/%s",resolved,entry->d_name)>=(int)sizeof full || stat(full,&info)) continue;
        int directory=S_ISDIR(info.st_mode);
        if(!directory && (!S_ISREG(info.st_mode) || !matches(entry->d_name,c->extension))) continue;
        if(count==10000) { failed=1; break; }
        if(count==capacity) {
            int next=capacity?capacity*2:64; FileEntry *grown=realloc(entries,next*sizeof *grown);
            if(!grown) { failed=1; break; } entries=grown; capacity=next;
        }
        char *name=strdup(entry->d_name); if(!name) { failed=1; break; }
        entries[count++]=(FileEntry){name,directory};
    }
    closedir(dir);
    if(failed) { free_entries(entries,count); snprintf(c->error,sizeof c->error,"Could not list folder (memory, read error or too many files)."); return 0; }
    if(count>1) qsort(entries,count,sizeof *entries,compare);
    free_entries(c->entries,c->count); c->entries=entries; c->count=count; c->selected=-1; c->scroll=0;
    snprintf(c->directory,sizeof c->directory,"%s",resolved); c->error[0]=0; return 1;
}
int file_chooser_parent(FileChooser *c) {
    char path[PATH_MAX]; snprintf(path,sizeof path,"%s",c->directory);
    char *slash=strrchr(path,'/'); if(slash==path) slash[1]=0; else if(slash) *slash=0;
    return file_chooser_folder(c,path);
}
int file_chooser_begin(FileChooser *c,const char *initial,const char *extension,int save) {
    file_chooser_close(c); c->save=save; snprintf(c->extension,sizeof c->extension,"%s",extension);
    char path[PATH_MAX];
    if(strlen(initial)>=sizeof path) { snprintf(c->error,sizeof c->error,"Path is too long."); return 0; }
    snprintf(path,sizeof path,"%s",initial); char *slash=strrchr(path,'/');
    if(slash) { snprintf(c->name,sizeof c->name,"%s",slash+1); if(slash==path) slash[1]=0; else *slash=0; }
    else { snprintf(c->name,sizeof c->name,"%s",initial); if(!getcwd(path,sizeof path)) return 0; }
    if(!file_chooser_folder(c,path)) {
        if(!getcwd(path,sizeof path) || !file_chooser_folder(c,path)) return 0;
    }
    if(!save) c->name[0]=0;
    return 1;
}
int file_chooser_path(FileChooser *c,char *path,size_t capacity) {
    if(!c->name[0]) { snprintf(c->error,sizeof c->error,"Choose a file or enter a filename."); return -1; }
    int n=c->name[0]=='/'?snprintf(path,capacity,"%s",c->name):snprintf(path,capacity,"%s/%s",c->directory,c->name);
    if(n<0 || (size_t)n>=capacity) { snprintf(c->error,sizeof c->error,"Path is too long."); return -1; }
    struct stat info;
    if(!stat(path,&info) && S_ISDIR(info.st_mode)) return file_chooser_folder(c,path)?0:-1;
    if(c->save && !matches(path,c->extension)) {
        size_t length=strlen(path);
        n=snprintf(path+length,capacity-length,".%s",c->extension);
        if(n<0 || (size_t)n>=capacity-length) { snprintf(c->error,sizeof c->error,"Path is too long."); return -1; }
    }
    char parent[PATH_MAX],resolved[PATH_MAX];
    if(strlen(path)>=sizeof parent) { snprintf(c->error,sizeof c->error,"Path is too long."); return -1; }
    snprintf(parent,sizeof parent,"%s",path); char *slash=strrchr(parent,'/');
    if(!slash) return -1;
    if(slash==parent) slash[1]=0; else *slash=0;
    if(!realpath(parent,resolved) || access(resolved,c->save?W_OK:R_OK)) { snprintf(c->error,sizeof c->error,"Folder is unavailable or access is denied."); return -1; }
    const char *name=strrchr(path,'/')+1; char final[PATH_MAX];
    if(snprintf(final,sizeof final,"%s%s%s",resolved,!strcmp(resolved,"/")?"":"/",name)>=(int)sizeof final || strlen(final)>=capacity) { snprintf(c->error,sizeof c->error,"Path is too long."); return -1; }
    memcpy(path,final,strlen(final)+1);
    if(!stat(path,&info)) {
        if(!S_ISREG(info.st_mode)) { snprintf(c->error,sizeof c->error,"Choose a regular file."); return -1; }
        if(!c->save && (!matches(path,c->extension) || access(path,R_OK))) { snprintf(c->error,sizeof c->error,"Choose a readable .%s file.",c->extension); return -1; }
        return c->save?2:1;
    }
    if(c->save && errno==ENOENT) return 1;
    snprintf(c->error,sizeof c->error,"File is unavailable: %s",strerror(errno)); return -1;
}
