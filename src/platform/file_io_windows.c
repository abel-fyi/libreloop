// SPDX-License-Identifier: GPL-3.0-only
#include "file_io.h"
#include "windows_utf.h"
FILE *platform_file_open(const char *path,const char *mode) {
    wchar_t *wide=windows_utf16(path),*flags=windows_utf16(mode);FILE *file=NULL;
    if(wide && flags)file=_wfopen(wide,flags);free(wide);free(flags);return file;
}
int platform_file_remove(const char *path){wchar_t *wide=windows_utf16(path);if(!wide)return -1;int result=_wremove(wide);free(wide);return result;}
int platform_file_rename(const char *from,const char *to) {
    wchar_t *source=windows_utf16(from),*destination=windows_utf16(to);int result=-1;
    if(source && destination && MoveFileExW(source,destination,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))result=0;
    if(result)errno=EIO;free(source);free(destination);return result;
}
