// SPDX-License-Identifier: GPL-3.0-only
#include "file_chooser.h"
#include "atomic_file.h"
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
static char recent[FILE_PURPOSES][PATH_MAX],locations_path[PATH_MAX];
void file_chooser_locations(const char *browser_config) {
    memset(recent,0,sizeof recent); locations_path[0]=0;
    const char *slash=strrchr(browser_config,'/');
    if(!slash || snprintf(locations_path,sizeof locations_path,"%.*s/chooser-folders.txt",(int)(slash-browser_config),browser_config)>=(int)sizeof locations_path) { locations_path[0]=0; return; }
    FILE *f=fopen(locations_path,"r"); if(!f) return;
    char line[PATH_MAX+32];
    while(fgets(line,sizeof line,f)) {
        char *end,*newline=strchr(line,'\n'); if(!newline) continue;
        *newline=0; long purpose=strtol(line,&end,10);
        if(end!=line && *end=='\t' && end[1]=='/' && purpose>=0 && purpose<FILE_PURPOSES && strlen(end+1)<PATH_MAX)
            snprintf(recent[purpose],PATH_MAX,"%s",end+1);
    }
    fclose(f);
}
static void remember_folder(int purpose,const char *folder) {
    if(purpose<0 || purpose>=FILE_PURPOSES || !folder[0] || strlen(folder)>=PATH_MAX || !strcmp(recent[purpose],folder)) return;
    snprintf(recent[purpose],PATH_MAX,"%s",folder);
    AtomicFile out;
    if(!locations_path[0] || !atomic_file_open(&out,locations_path)) return;
    for(int i=0;i<FILE_PURPOSES;i++) if(recent[i][0] && !strchr(recent[i],'\n') && !strchr(recent[i],'\r')) fprintf(out.file,"%d\t%s\n",i,recent[i]);
    atomic_file_commit(&out);
}
void file_chooser_remember(FileChooser *c,const char *chosen) {
    if(!c->remember) return;
    char parent[PATH_MAX]; if(strlen(chosen)>=sizeof parent) return;
    snprintf(parent,sizeof parent,"%s",chosen); char *slash=strrchr(parent,'/');
    if(!slash) return; if(slash==parent) slash[1]=0; else *slash=0;
    remember_folder(c->purpose,parent); c->remember=0;
}
static void free_entries(FileEntry *entries,int count) {
    for(int i=0;i<count;i++) free(entries[i].name);
    free(entries);
}
void file_chooser_close(FileChooser *c) { if(c->remember) remember_folder(c->purpose,c->directory); free_entries(c->entries,c->count); *c=(FileChooser){.selected=-1}; }
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

int file_chooser_begin_recent(FileChooser *c,const char *initial,const char *extension,int save,int purpose) {
    file_chooser_close(c);
    if(!file_chooser_begin(c,initial,extension,save)) return 0;
    if(purpose>=0 && purpose<FILE_PURPOSES) {
        if(recent[purpose][0]) { file_chooser_folder(c,recent[purpose]); c->error[0]=0; }
        c->purpose=purpose; c->remember=1;
    }
    return 1;
}
