// SPDX-License-Identifier: GPL-3.0-only
/* Windows lexical paths stay separate from POSIX rules; no OS/UI dependency. */
#include "windows_path_math.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
static int separator(char c){return c=='/' || c=='\\';}
static size_t root_size(const char *p) {
    size_t n=strlen(p);
    if(n>=3 && isalpha((unsigned char)p[0]) && p[1]==':' && separator(p[2]))return 3;
    if(n>=2 && separator(p[0]) && separator(p[1])) {
        size_t i=2;while(p[i] && !separator(p[i]))i++;
        if(!p[i])return i;
        i++;while(p[i] && !separator(p[i]))i++;
        return i;
    }
    return n && separator(p[0])?1:0;
}
int windows_path_absolute(const char *p){return root_size(p)>0;}
const char *windows_path_basename(const char *p){const char *name=p;for(;*p;p++)if(separator(*p))name=p+1;return name;}
int windows_path_parent(const char *path,char *out,size_t cap) {
    size_t n=strlen(path),root=root_size(path);if(n>=cap)return 0;
    memmove(out,path,n+1);while(n>root && separator(out[n-1]))out[--n]=0;
    if(n<=root)return 1;
    while(n>root && !separator(out[n-1]))n--;
    if(!n){if(cap<2)return 0;strcpy(out,".");return 1;}
    if(n>root)n--;out[n]=0;return 1;
}
static int root_equal(const char *a,const char *b,size_t n) {
    for(size_t i=0;i<n;i++)if(separator(a[i]) && separator(b[i]))continue;
    else if(tolower((unsigned char)a[i])!=tolower((unsigned char)b[i]))return 0;
    return 1;
}
int windows_path_relative(const char *directory,const char *absolute,char *out,size_t cap) {
    size_t aroot=root_size(directory),broot=root_size(absolute),n=strlen(directory);
    if(aroot!=broot || !root_equal(directory,absolute,aroot)) {
        size_t bytes=strlen(absolute);if(bytes>=cap)return 0;memmove(out,absolute,bytes+1);return 1;
    }
    size_t common=aroot;
    for(size_t i=aroot;directory[i] && absolute[i];i++) {
        if(directory[i]!=absolute[i] && !(separator(directory[i]) && separator(absolute[i])))break;
        if(separator(directory[i]))common=i+1;
    }
    if(n>=aroot && !strncmp(directory+aroot,absolute+broot,n-aroot) && separator(absolute[n]))common=n+1;
    size_t used=0;
    if(common<n)for(size_t i=common;i<=n;i++)if(separator(directory[i]) || directory[i]==0) {
        if(used+3>=cap)return 0;memcpy(out+used,"../",3);used+=3;
    }
    size_t rest=strlen(absolute+common);if(rest>=cap-used)return 0;
    memmove(out+used,absolute+common,rest+1);return 1;
}
int windows_path_join(const char *directory,const char *name,char *out,size_t cap) {
    size_t n=strlen(directory);return snprintf(out,cap,"%s%s%s",directory,n && separator(directory[n-1])?"":"/",name)<(int)cap;
}
