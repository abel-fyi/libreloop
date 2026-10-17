// SPDX-License-Identifier: GPL-3.0-only
/* Original browser-facing directory behavior adapted from pinned raylib5.5
   rcore.c; retained zlib notice docs/licenses/raylib.txt. Native APIs stay here. */
#include "browser_files.h"
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum { BROWSER_LIST_PATH_CAPACITY=4096 };
int platform_browser_directory(const char *path) {
    DIR *dir=opendir(path);if(!dir)return 0;closedir(dir);return 1;
}
void platform_browser_config_base(char *out,size_t capacity) {
    const char *home=getenv("HOME"),*xdg=getenv("XDG_CONFIG_HOME");
    if(xdg && *xdg)snprintf(out,capacity,"%s",xdg);
    else if(home)snprintf(out,capacity,"%s/.config",home);
    else if(capacity)out[0]=0;
}
int platform_browser_make_directory(const char *directory) {
    if(!directory || !directory[0])return 1;
    if(platform_browser_directory(directory))return 0;
    int length=(int)strlen(directory)+1;char *copy=calloc((size_t)length,1);if(!copy)return 1;
    memcpy(copy,directory,(size_t)length);
    for(int i=0;i<length && copy[i];i++) {
        if(copy[i]==':')i++;
        else if(copy[i]=='\\' || copy[i]=='/') {
            copy[i]=0;if(!platform_browser_directory(copy))mkdir(copy,0777);copy[i]='/';
        }
    }
    if(!platform_browser_directory(copy))mkdir(copy,0777);free(copy);
    /* Preserve the original return convention (mkdir errors are ignored). */
    return 0;
}
BrowserFileList platform_browser_files(const char *path) {
    BrowserFileList files={0};DIR *directory=opendir(path);if(!directory)return files;
    struct dirent *entry;
    while((entry=readdir(directory)))if(strcmp(entry->d_name,".") && strcmp(entry->d_name,".."))files.capacity++;
    closedir(directory);
    files.paths=calloc(files.capacity,sizeof *files.paths);
    if(files.capacity && !files.paths){files.capacity=0;return files;}
    for(unsigned int i=0;i<files.capacity;i++) {
        files.paths[i]=malloc(BROWSER_LIST_PATH_CAPACITY);
        if(!files.paths[i]){platform_browser_files_free(files);return (BrowserFileList){0};}
    }
    directory=opendir(path);if(!directory)return files;
    while((entry=readdir(directory))) {
        if(!strcmp(entry->d_name,".") || !strcmp(entry->d_name,".."))continue;
        if(files.count>=files.capacity)break;
        if(snprintf(files.paths[files.count],BROWSER_LIST_PATH_CAPACITY,"%s/%s",path,entry->d_name)>=BROWSER_LIST_PATH_CAPACITY)continue;
        files.count++;
    }
    closedir(directory);return files;
}
void platform_browser_files_free(BrowserFileList files) {
    for(unsigned int i=0;i<files.capacity;i++)free(files.paths[i]);free(files.paths);
}
