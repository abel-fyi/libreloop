// SPDX-License-Identifier: GPL-3.0-only
#include "browser_files.h"
#include "files.h"
#include "windows_utf.h"
#include <stdio.h>
#include <string.h>
int platform_browser_directory(const char *path) {
    wchar_t *wide=windows_utf16(path);if(!wide)return 0;
    HANDLE handle=CreateFileW(wide,FILE_LIST_DIRECTORY,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
        NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,NULL);free(wide);
    if(handle==INVALID_HANDLE_VALUE)return 0;CloseHandle(handle);return 1;
}
void platform_browser_config_base(char *out,size_t capacity) {
    DWORD n=GetEnvironmentVariableW(L"APPDATA",NULL,0);wchar_t *wide=n?malloc((size_t)n*sizeof *wide):NULL;
    DWORD actual=wide?GetEnvironmentVariableW(L"APPDATA",wide,n):0;
    if(!actual || actual>=n || !windows_utf8(wide,out,capacity))if(capacity)out[0]=0;
    free(wide);
}
int platform_browser_make_directory(const char *path) {
    if(!path || !*path)return 1;
    wchar_t *wide=windows_utf16(path);if(!wide)return 1;
    /* Root prefixes are not directories to create. Errors retain the original
       Browser convention; callers that require success check existence. */
    size_t start=0;
    if(wide[0] && wide[1]==L':' && (wide[2]==L'/' || wide[2]==L'\\'))start=3;
    else if((wide[0]==L'\\' || wide[0]==L'/') && (wide[1]==L'\\' || wide[1]==L'/')) {
        start=2;while(wide[start] && wide[start]!=L'\\' && wide[start]!=L'/')start++;
        if(wide[start])start++;while(wide[start] && wide[start]!=L'\\' && wide[start]!=L'/')start++;
        if(wide[start])start++;
    }
    for(size_t i=start;wide[i];i++)if(wide[i]==L'\\' || wide[i]==L'/') {
        wchar_t old=wide[i];wide[i]=0;CreateDirectoryW(wide,NULL);wide[i]=old;
    }
    CreateDirectoryW(wide,NULL);free(wide);return 0;
}
BrowserFileList platform_browser_files(const char *path) {
    BrowserFileList files={0};PlatformDirectory *dir=platform_directory_open(path);if(!dir)return files;
    PlatformEntry entry;
    while(platform_directory_next(dir,&entry)>0)if(strcmp(entry.name,".") && strcmp(entry.name,".."))files.capacity++;
    platform_directory_close(dir);
    files.paths=calloc(files.capacity,sizeof *files.paths);if(files.capacity && !files.paths)return (BrowserFileList){0};
    for(unsigned i=0;i<files.capacity;i++)if(!(files.paths[i]=malloc(4096))){platform_browser_files_free(files);return (BrowserFileList){0};}
    dir=platform_directory_open(path);if(!dir)return files;
    while(files.count<files.capacity && platform_directory_next(dir,&entry)>0) {
        if(!strcmp(entry.name,".") || !strcmp(entry.name,".."))continue;
        if(platform_join_path(path,entry.name,files.paths[files.count],4096))files.count++;
    }
    platform_directory_close(dir);return files;
}
void platform_browser_files_free(BrowserFileList files){for(unsigned i=0;i<files.capacity;i++)free(files.paths[i]);free(files.paths);}
