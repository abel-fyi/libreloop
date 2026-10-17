// SPDX-License-Identifier: GPL-3.0-only
#include "atomic_file.h"
#include "windows_utf.h"
#include "file_io.h"
#include <stdatomic.h>
#include <io.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
static atomic_uint nonce;
int atomic_file_open(AtomicFile *out,const char *path) {
    *out=(AtomicFile){0};if(!path || strlen(path)>=sizeof out->destination)return 0;
    strcpy(out->destination,path);
    for(int attempt=0;attempt<128;attempt++) {
        unsigned id=atomic_fetch_add(&nonce,1)+(unsigned)GetTickCount64()+GetCurrentProcessId();
        if(snprintf(out->temporary,sizeof out->temporary,"%s.tmp-%06x",path,id)>=(int)sizeof out->temporary)return 0;
        wchar_t *wide=windows_utf16(out->temporary);if(!wide)return 0;
        HANDLE file=CreateFileW(wide,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
        DWORD error=GetLastError();free(wide);
        if(file==INVALID_HANDLE_VALUE){if(error==ERROR_FILE_EXISTS || error==ERROR_ALREADY_EXISTS)continue;return 0;}
        int fd=_open_osfhandle((intptr_t)file,_O_BINARY|_O_RDWR);
        if(fd<0){CloseHandle(file);platform_file_remove(out->temporary);return 0;}
        out->file=_fdopen(fd,"wb");if(!out->file){_close(fd);platform_file_remove(out->temporary);return 0;}
        return 1;
    }
    return 0;
}
void atomic_file_abort(AtomicFile *out) {
    if(out->file)fclose(out->file);out->file=NULL;
    if(out->temporary[0])platform_file_remove(out->temporary);
}
int atomic_file_commit(AtomicFile *out) {
    if(!out->file)return 0;int ok=!ferror(out->file);
    if(fflush(out->file) || _commit(_fileno(out->file)))ok=0;
    if(fclose(out->file))ok=0;out->file=NULL;
    if(ok && !platform_file_rename(out->temporary,out->destination))return 1;
    atomic_file_abort(out);return 0;
}
