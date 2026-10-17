// SPDX-License-Identifier: GPL-3.0-only
#include "file_io.h"
#include "file_chooser.h"
#include "atomic_file.h"
#include "files.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "portable.h"
#include <errno.h>
static char recent[FILE_PURPOSES][LL_PATH_CAPACITY],locations_path[LL_PATH_CAPACITY];
void file_chooser_locations(const char *browser_config) {
    memset(recent,0,sizeof recent); locations_path[0]=0;
    const char *slash=strrchr(browser_config,'/');
    if(!slash || snprintf(locations_path,sizeof locations_path,"%.*s/chooser-folders.txt",(int)(slash-browser_config),browser_config)>=(int)sizeof locations_path) { locations_path[0]=0; return; }
    FILE *f=platform_file_open(locations_path,"r"); if(!f) return;
    char line[LL_PATH_CAPACITY+32];
    while(fgets(line,sizeof line,f)) {
        char *end,*newline=strchr(line,'\n'); if(!newline) continue;
        *newline=0; long purpose=strtol(line,&end,10);
        if(end!=line && *end=='\t' && platform_absolute_path(end+1) && purpose>=0 && purpose<FILE_PURPOSES && strlen(end+1)<LL_PATH_CAPACITY)
            snprintf(recent[purpose],LL_PATH_CAPACITY,"%s",end+1);
    }
    fclose(f);
}
static void remember_folder(int purpose,const char *folder) {
    if(purpose<0 || purpose>=FILE_PURPOSES || !folder[0] || strlen(folder)>=LL_PATH_CAPACITY || !strcmp(recent[purpose],folder)) return;
    snprintf(recent[purpose],LL_PATH_CAPACITY,"%s",folder);
    AtomicFile out;
    if(!locations_path[0] || !atomic_file_open(&out,locations_path)) return;
    for(int i=0;i<FILE_PURPOSES;i++) if(recent[i][0] && !strchr(recent[i],'\n') && !strchr(recent[i],'\r')) fprintf(out.file,"%d\t%s\n",i,recent[i]);
    atomic_file_commit(&out);
}
void file_chooser_remember(FileChooser *c,const char *chosen) {
    if(!c->remember) return;
    char parent[LL_PATH_CAPACITY]; if(strlen(chosen)>=sizeof parent) return;
    if(platform_basename(chosen)==chosen || !platform_parent_path(chosen,parent,sizeof parent)) return;
    remember_folder(c->purpose,parent); c->remember=0;
}
static void free_entries(FileEntry *entries,int count) {
    for(int i=0;i<count;i++) free(entries[i].name);
    free(entries);
}
void file_chooser_close(FileChooser *c) { if(c->remember) remember_folder(c->purpose,c->directory); free_entries(c->entries,c->count); *c=(FileChooser){.selected=-1}; }
static int compare(const void *a,const void *b) {
    const FileEntry *x=a,*y=b;
    return x->directory!=y->directory?y->directory-x->directory:ll_strcasecmp(x->name,y->name);
}
static int matches(const char *name,const char *extensions) {
    const char *dot=strrchr(name,'.'); if(!dot) return 0;
    size_t length=strlen(dot+1);
    for(const char *at=extensions;*at;) {
        const char *end=strchr(at,';'); size_t n=end?(size_t)(end-at):strlen(at);
        if(n==length && !ll_strncasecmp(dot+1,at,n)) return 1;
        if(!end) break;
        at=end+1;
    }
    return 0;
}
int file_chooser_folder(FileChooser *c,const char *path) {
    char resolved[LL_PATH_CAPACITY];
    if(!platform_canonical_path(path,resolved,sizeof resolved)) { snprintf(c->error,sizeof c->error,"Cannot open folder: %s",strerror(errno)); return 0; }
    PlatformDirectory *dir=platform_directory_open(resolved);
    if(!dir) { snprintf(c->error,sizeof c->error,"Cannot read folder: %s",strerror(errno)); return 0; }
    FileEntry *entries=NULL; int count=0,capacity=0,failed=0; PlatformEntry entry;
    for(;;) {
        int result=platform_directory_next(dir,&entry);
        if(result<=0) { if(result<0) failed=1; break; }
        if(!strcmp(entry.name,".") || !strcmp(entry.name,"..") || (!c->hidden && entry.name[0]=='.')) continue;
        int directory=entry.kind==PLATFORM_DIRECTORY;
        if(!directory && (entry.kind!=PLATFORM_FILE || !matches(entry.name,c->extension))) continue;
        if(count==10000) { failed=1; break; }
        if(count==capacity) {
            int next=capacity?capacity*2:64; FileEntry *grown=realloc(entries,next*sizeof *grown);
            if(!grown) { failed=1; break; } entries=grown; capacity=next;
        }
        char *name=ll_strdup(entry.name); if(!name) { failed=1; break; }
        entries[count++]=(FileEntry){name,directory};
    }
    platform_directory_close(dir);
    if(failed) { free_entries(entries,count); snprintf(c->error,sizeof c->error,"Could not list folder (memory, read error or too many files)."); return 0; }
    if(count>1) qsort(entries,count,sizeof *entries,compare);
    free_entries(c->entries,c->count); c->entries=entries; c->count=count; c->selected=-1; c->scroll=0;
    snprintf(c->directory,sizeof c->directory,"%s",resolved); c->error[0]=0; return 1;
}
int file_chooser_parent(FileChooser *c) {
    char path[LL_PATH_CAPACITY];
    if(!platform_parent_path(c->directory,path,sizeof path)) return 0;
    return file_chooser_folder(c,path);
}
int file_chooser_begin(FileChooser *c,const char *initial,const char *extension,int save) {
    file_chooser_close(c); c->save=save; snprintf(c->extension,sizeof c->extension,"%s",extension);
    char path[LL_PATH_CAPACITY];
    if(strlen(initial)>=sizeof path) { snprintf(c->error,sizeof c->error,"Path is too long."); return 0; }
    const char *name=platform_basename(initial);
    snprintf(c->name,sizeof c->name,"%s",name);
    if(name!=initial) { if(!platform_parent_path(initial,path,sizeof path)) return 0; }
    else if(!platform_working_directory(path,sizeof path)) return 0;
    if(!file_chooser_folder(c,path)) {
        if(!platform_working_directory(path,sizeof path) || !file_chooser_folder(c,path)) return 0;
    }
    if(!save) c->name[0]=0;
    return 1;
}
int file_chooser_path(FileChooser *c,char *path,size_t capacity) {
    if(!c->name[0]) { snprintf(c->error,sizeof c->error,"Choose a file or enter a filename."); return -1; }
    int n=platform_absolute_path(c->name)?snprintf(path,capacity,"%s",c->name):snprintf(path,capacity,"%s/%s",c->directory,c->name);
    if(n<0 || (size_t)n>=capacity) { snprintf(c->error,sizeof c->error,"Path is too long."); return -1; }
    int kind;
    if(platform_file_kind(path,&kind) && kind==PLATFORM_DIRECTORY) return file_chooser_folder(c,path)?0:-1;
    if(c->save && !matches(path,c->extension)) {
        size_t length=strlen(path);
        n=snprintf(path+length,capacity-length,".%s",c->extension);
        if(n<0 || (size_t)n>=capacity-length) { snprintf(c->error,sizeof c->error,"Path is too long."); return -1; }
    }
    char parent[LL_PATH_CAPACITY],resolved[LL_PATH_CAPACITY];
    if(strlen(path)>=sizeof parent) { snprintf(c->error,sizeof c->error,"Path is too long."); return -1; }
    if(platform_basename(path)==path || !platform_parent_path(path,parent,sizeof parent)) return -1;
    if(!platform_canonical_path(parent,resolved,sizeof resolved) || !platform_path_access(resolved,c->save)) { snprintf(c->error,sizeof c->error,"Folder is unavailable or access is denied."); return -1; }
    const char *name=platform_basename(path); char final[LL_PATH_CAPACITY];
    if(!platform_join_path(resolved,name,final,sizeof final) || strlen(final)>=capacity) { snprintf(c->error,sizeof c->error,"Path is too long."); return -1; }
    memcpy(path,final,strlen(final)+1);
    if(platform_file_kind(path,&kind)) {
        if(kind!=PLATFORM_FILE) { snprintf(c->error,sizeof c->error,"Choose a regular file."); return -1; }
        if(!c->save && (!matches(path,c->extension) || !platform_path_access(path,0))) { snprintf(c->error,sizeof c->error,"Choose a readable .%s file.",c->extension); return -1; }
        return c->save?2:1;
    }
    if(c->save && errno==ENOENT) return 1;
    snprintf(c->error,sizeof c->error,"File is unavailable: %s",strerror(errno)); return -1;
}

int file_chooser_begin_recent(FileChooser *c,const char *initial,const char *extension,int save,int purpose) {
    file_chooser_close(c);
    if(!file_chooser_begin(c,initial,extension,save)) return 0;
    if(purpose>=0 && purpose<FILE_PURPOSES) {
        if(recent[purpose][0]) { file_chooser_folder(c,recent[purpose]); c->error[0]=0; }
        c->purpose=purpose; c->remember=1;
    }
    return 1;
}
