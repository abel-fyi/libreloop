// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_PORTABLE_H
#define LIBRELOOP_PORTABLE_H
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
/* C11 equivalents of the few nonstandard string helpers used by the source. */
static inline size_t ll_strnlen(const char *s,size_t limit) {
    size_t n=0; while(n<limit && s[n]) n++; return n;
}
static inline char *ll_strdup(const char *s) {
    size_t n=strlen(s)+1; char *copy=malloc(n); if(copy) memcpy(copy,s,n); return copy;
}
static inline int ll_strcasecmp(const char *a,const char *b) {
    for(;;a++,b++) { int x=tolower((unsigned char)*a),y=tolower((unsigned char)*b); if(x!=y || !x) return x-y; }
}
static inline int ll_strncasecmp(const char *a,const char *b,size_t n) {
    for(size_t i=0;i<n;i++) { int x=tolower((unsigned char)a[i]),y=tolower((unsigned char)b[i]); if(x!=y || !x) return x-y; } return 0;
}
#endif
