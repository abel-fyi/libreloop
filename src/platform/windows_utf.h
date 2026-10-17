// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_WINDOWS_UTF_H
#define LIBRELOOP_WINDOWS_UTF_H
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <wchar.h>
static inline wchar_t *windows_utf16(const char *text) {
    if(!text){errno=EINVAL;return NULL;}
    int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text,-1,NULL,0);
    if(!count){errno=EILSEQ;return NULL;}
    wchar_t *wide=malloc((size_t)count*sizeof *wide);
    if(!wide)return NULL;
    if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text,-1,wide,count)){free(wide);errno=EILSEQ;return NULL;}
    return wide;
}
static inline int windows_utf8(const wchar_t *wide,char *out,size_t capacity) {
    if(capacity>INT_MAX)return 0;
    return WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,wide,-1,out,(int)capacity,NULL,NULL)>0;
}
#endif
