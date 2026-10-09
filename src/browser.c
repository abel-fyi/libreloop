// SPDX-License-Identifier: GPL-3.0-only
#include "browser.h"
#include "project_format.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int order_nodes(const void *a,const void *b) {
    const BrowserNode *x=a,*y=b;
    return x->dir!=y->dir?y->dir-x->dir:strcmp(GetFileName(x->path),GetFileName(y->path));
}
void browser_select(Browser *b,int entry) {
    if(entry<0 || entry>=b->items) return;
    b->selected=entry; b->follow=1;
    const char *path=b->nodes[entry].path;
    snprintf(b->path,sizeof b->path,"%s",b->nodes[entry].dir?path:GetDirectoryPath(path));
}
int browser_toggle(Browser *b,int entry) {
    if(entry<0 || entry>=b->items || !b->nodes[entry].dir) return 0;
    int depth=b->nodes[entry].depth;
    if(b->nodes[entry].open) {
        int end=entry+1;
        while(end<b->items && b->nodes[end].depth>depth) free(b->nodes[end++].path);
        int removed=end-entry-1;
        if(b->selected>entry && b->selected<end) browser_select(b,entry);
        else if(b->selected>=end) b->selected-=removed;
        memmove(b->nodes+entry+1,b->nodes+end,(b->items-end)*sizeof *b->nodes);
        b->items-=removed; b->nodes[entry].open=0; return 1;
    }
    if(depth>=64 || !DirectoryExists(b->nodes[entry].path)) return 0;
    FilePathList files=LoadDirectoryFiles(b->nodes[entry].path);
    BrowserNode *children=calloc(files.count?files.count:1,sizeof *children); int n=0,ok=children!=NULL;
    for(unsigned i=0;ok && i<files.count;i++) {
        int dir=DirectoryExists(files.paths[i]);
        if(!dir && !IsFileExtension(files.paths[i],".wav;.flac;.mp3;" PROJECT_FILE_SUFFIX)) continue;
        char *path=strdup(files.paths[i]); if(!path) { ok=0; break; }
        children[n++]=(BrowserNode){path,depth+1,dir,0};
    }
    UnloadDirectoryFiles(files);
    if(b->items+n>10000) ok=0;
    BrowserNode *nodes=ok?realloc(b->nodes,(b->items+n)*sizeof *nodes):NULL;
    if(!nodes) { for(int i=0;i<n;i++) free(children[i].path); free(children); return 0; }
    qsort(children,n,sizeof *children,order_nodes); b->nodes=nodes;
    memmove(nodes+entry+1+n,nodes+entry+1,(b->items-entry-1)*sizeof *nodes);
    memcpy(nodes+entry+1,children,n*sizeof *children); free(children);
    b->items+=n; nodes[entry].open=1;
    if(b->selected>entry) { b->selected+=n; b->follow=1; }
    return 1;
}
void browser_left(Browser *b) {
    int i=b->selected; if(i<0 || i>=b->items) return;
    if(b->nodes[i].dir && b->nodes[i].open) { browser_toggle(b,i); return; }
    int depth=b->nodes[i].depth;
    while(--i>=0) if(b->nodes[i].depth<depth) { browser_select(b,i); break; }
}
void browser_right(Browser *b) {
    int i=b->selected; if(i<0 || i>=b->items || !b->nodes[i].dir) return;
    if(!b->nodes[i].open) browser_toggle(b,i);
    else if(i+1<b->items && b->nodes[i+1].depth>b->nodes[i].depth) browser_select(b,i+1);
}
static int root_add(Browser *b,const char *path) {
    char *copy=strdup(path); if(!copy) return -1;
    BrowserNode *nodes=realloc(b->nodes,(b->items+1)*sizeof *nodes);
    if(!nodes) { free(copy); return -1; }
    b->nodes=nodes; int entry=b->items++; nodes[entry]=(BrowserNode){copy,0,1,0}; return entry;
}
void browser_init(Browser *b,const char *samples) {
    memset(b,0,sizeof *b); b->selected=-1;
    const char *home=getenv("HOME"),*xdg=getenv("XDG_CONFIG_HOME"); char base[PATH_MAX],dir[PATH_MAX];
    if(xdg && *xdg) snprintf(base,sizeof base,"%s",xdg);
    else if(home) snprintf(base,sizeof base,"%s/.config",home);
    else base[0]=0;
    if(base[0] && snprintf(dir,sizeof dir,"%s/libreloop",base)<(int)sizeof dir) {
        MakeDirectory(base); MakeDirectory(dir);
        if(snprintf(b->config,sizeof b->config,"%s/folders.txt",dir)>=(int)sizeof b->config) b->config[0]=0;
    }
    if(!realpath(samples,b->samples)) snprintf(b->samples,sizeof b->samples,"%s",samples);
    root_add(b,b->samples); browser_select(b,0); browser_toggle(b,0);
    FILE *f=b->config[0]?fopen(b->config,"r"):NULL;
    if(!f && base[0] && snprintf(dir,sizeof dir,"%s/homebeat/folders.txt",base)<(int)sizeof dir) f=fopen(dir,"r");
    if(f) {
        char line[PATH_MAX];
        while(b->count<FOLDERS && fgets(line,sizeof line,f)) {
            char *newline=strchr(line,'\n'); if(!newline) continue; *newline=0;
            if(*line && strcmp(line,b->samples) && root_add(b,line)>=0) snprintf(b->folders[b->count++],PATH_MAX,"%s",line);
        }
        fclose(f);
    }
}
int browser_add(Browser *b,const char *path) {
    char full[PATH_MAX];
    if(!realpath(path,full) || !DirectoryExists(full) || strchr(full,'\n')) return 0;
    for(int i=0;i<b->items;i++) if(b->nodes[i].depth==0 && !strcmp(full,b->nodes[i].path)) {
        browser_select(b,i); if(!b->nodes[i].open) browser_toggle(b,i); return 1;
    }
    if(b->count==FOLDERS || !b->config[0]) return 0;
    FILE *f=fopen(b->config,"w"); if(!f) return 0;
    for(int i=0;i<b->count;i++) fprintf(f,"%s\n",b->folders[i]);
    fprintf(f,"%s\n",full); int ok=!ferror(f); if(fclose(f)) ok=0;
    if(!ok) return 0;
    int entry=root_add(b,full); if(entry<0) return 0;
    snprintf(b->folders[b->count++],PATH_MAX,"%s",full);
    browser_select(b,entry); browser_toggle(b,entry); return 1;
}
void browser_close(Browser *b) {
    for(int i=0;i<b->items;i++) free(b->nodes[i].path);
    free(b->nodes); b->nodes=NULL; b->items=0;
}
