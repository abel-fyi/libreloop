// SPDX-License-Identifier: GPL-3.0-only
#include "files.h"
#include "windows_utf.h"
#include "windows_path_math.h"
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <stdatomic.h>
struct PlatformDirectory { HANDLE handle;WIN32_FIND_DATAW item;int first;char path[LL_PATH_CAPACITY]; };
int platform_path_exists(const char *path){wchar_t *wide=windows_utf16(path);if(!wide)return 0;int ok=_waccess(wide,0)==0;free(wide);return ok;}
int platform_canonical_path(const char *path,char *out,size_t cap) {
    wchar_t *wide=windows_utf16(path);if(!wide)return 0;
    HANDLE file=CreateFileW(wide,FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
        NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,NULL);free(wide);
    if(file==INVALID_HANDLE_VALUE)return 0;
    DWORD count=GetFinalPathNameByHandleW(file,NULL,0,FILE_NAME_NORMALIZED);
    wchar_t *final=count?malloc(((size_t)count+1)*sizeof *final):NULL;int ok=0;
    DWORD actual=final?GetFinalPathNameByHandleW(file,final,count+1,FILE_NAME_NORMALIZED):0;
    if(actual && actual<count+1) {
        const wchar_t *plain=final;
        if(!wcsncmp(final,L"\\\\?\\UNC\\",8)){final[6]=L'\\';plain=final+6;}
        else if(!wcsncmp(final,L"\\\\?\\",4))plain=final+4;
        ok=windows_utf8(plain,out,cap);if(ok)for(char *p=out;*p;p++)if(*p=='\\')*p='/';
    }
    free(final);CloseHandle(file);return ok;
}
int platform_absolute_path(const char *p){return windows_path_absolute(p);}
const char *platform_basename(const char *p){return windows_path_basename(p);}
int platform_parent_path(const char *p,char *out,size_t cap){return windows_path_parent(p,out,cap);}
int platform_relative_path(const char *d,const char *a,char *out,size_t cap){return windows_path_relative(d,a,out,cap);}
int platform_join_path(const char *d,const char *n,char *out,size_t cap){return windows_path_join(d,n,out,cap);}
int platform_working_directory(char *out,size_t cap) {
    DWORD n=GetCurrentDirectoryW(0,NULL);wchar_t *wide=n?malloc((size_t)n*sizeof *wide):NULL;
    DWORD actual=wide?GetCurrentDirectoryW(n,wide):0;
    int ok=actual && actual<n && windows_utf8(wide,out,cap);free(wide);
    if(ok)for(char *p=out;*p;p++)if(*p=='\\')*p='/';return ok;
}
int platform_file_kind(const char *path,int *kind) {
    wchar_t *wide=windows_utf16(path);if(!wide)return 0;
    HANDLE file=CreateFileW(wide,FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
        NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,NULL);free(wide);
    if(file==INVALID_HANDLE_VALUE)return 0;
    BY_HANDLE_FILE_INFORMATION info;int ok=GetFileInformationByHandle(file,&info)!=0;
    if(ok)*kind=info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY?PLATFORM_DIRECTORY:GetFileType(file)==FILE_TYPE_DISK?PLATFORM_FILE:PLATFORM_OTHER;
    CloseHandle(file);return ok;
}
int platform_path_access(const char *path,int writable) {
    wchar_t *wide=windows_utf16(path);if(!wide)return 0;int ok=_waccess(wide,writable?2:4)==0;free(wide);return ok;
}
PlatformDirectory *platform_directory_open(const char *path) {
    char mask[LL_PATH_CAPACITY];if(!platform_join_path(path,"*",mask,sizeof mask))return NULL;
    wchar_t *wide=windows_utf16(mask);if(!wide)return NULL;
    PlatformDirectory *d=calloc(1,sizeof *d);if(!d){free(wide);return NULL;}
    d->handle=FindFirstFileW(wide,&d->item);free(wide);
    if(d->handle==INVALID_HANDLE_VALUE){free(d);return NULL;}d->first=1;snprintf(d->path,sizeof d->path,"%s",path);return d;
}
int platform_directory_next(PlatformDirectory *d,PlatformEntry *out) {
    for(;;) {
        if(d->first)d->first=0;
        else if(!FindNextFileW(d->handle,&d->item))return GetLastError()==ERROR_NO_MORE_FILES?0:-1;
        if(!windows_utf8(d->item.cFileName,out->name,sizeof out->name))continue;
        char full[LL_PATH_CAPACITY];if(!platform_join_path(d->path,out->name,full,sizeof full) || !platform_file_kind(full,&out->kind))continue;
        return 1;
    }
}
void platform_directory_close(PlatformDirectory *d){if(d){FindClose(d->handle);free(d);}}
static atomic_uint directory_nonce;
int platform_asset_directory(char *path) {
    size_t n=strlen(path);if(n<6 || strcmp(path+n-6,"XXXXXX"))return 0;
    for(int attempt=0;attempt<128;attempt++) {
        unsigned value=atomic_fetch_add(&directory_nonce,1)+GetCurrentProcessId()+(unsigned)GetTickCount64();
        for(int i=0;i<6;i++){path[n-1-i]="0123456789abcdefghijklmnopqrstuvwxyz"[value%36];value/=36;}
        wchar_t *wide=windows_utf16(path);if(!wide)return 0;int ok=CreateDirectoryW(wide,NULL)!=0;DWORD error=GetLastError();free(wide);
        if(ok)return 1;if(error!=ERROR_ALREADY_EXISTS)return 0;
    }
    return 0;
}
void platform_remove_directory(const char *path){wchar_t *wide=windows_utf16(path);if(wide){RemoveDirectoryW(wide);free(wide);}}
