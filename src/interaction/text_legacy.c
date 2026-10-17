// SPDX-License-Identifier: GPL-3.0-only
/* Pinned raylib5.5 rcore/rtext behavior adapted as application-owned helpers.
   Copyright Ramon Santamaria; zlib notice retained in docs/licenses/raylib.txt. */
#include "text_legacy.h"
#include "path.h"
#include <stdio.h>
#include <string.h>
enum { LEGACY_TEXT_BUFFER=1024,LEGACY_SPLIT_COUNT=128,LEGACY_EXTENSION_LENGTH=16 };
const char *legacy_file_name(const char *path){if(!path)return NULL;const char *name=path;for(const char *p=path;*p;p++)if(*p=='/' || *p=='\\')name=p+1;return name;}
const char *legacy_file_stem(const char *path){
    /* Retain the port's bounded path storage; original valid basenames fit255 bytes. */
    static char name[LL_PATH_CAPACITY];name[0]=0;
    if(path){snprintf(name,sizeof name,"%s",legacy_file_name(path));char *dot=strrchr(name,'.');if(dot && dot>name)*dot=0;}
    return name;
}
static const char **split_extensions(const char *text,int *count){
    static const char *result[LEGACY_SPLIT_COUNT];static char buffer[LEGACY_TEXT_BUFFER];
    memset(buffer,0,sizeof buffer);result[0]=buffer;int counter=0;
    if(text){counter=1;for(int i=0;i<LEGACY_TEXT_BUFFER;i++){
        buffer[i]=text[i];if(buffer[i]=='\0')break;
        else if(buffer[i]==';'){buffer[i]='\0';result[counter]=buffer+i+1;counter++;if(counter==LEGACY_SPLIT_COUNT)break;}
    }}
    *count=counter;return result;
}
static const char *lower_text(const char *text){
    static char buffer[LEGACY_TEXT_BUFFER];memset(buffer,0,sizeof buffer);
    if(text)for(int i=0;i<LEGACY_TEXT_BUFFER-1 && text[i];i++)buffer[i]=text[i]>='A' && text[i]<='Z'?text[i]+32:text[i];
    return buffer;
}
int legacy_file_extension(const char *path,const char *extensions){
    const char *ext=strrchr(path,'.');if(!ext || ext==path)return 0;
    int count=0;const char **checks=split_extensions(extensions,&count);
    char file_lower[LEGACY_EXTENSION_LENGTH+1]={0};strncpy(file_lower,lower_text(ext),LEGACY_EXTENSION_LENGTH);
    for(int i=0;i<count;i++)if(!strcmp(file_lower,lower_text(checks[i])))return 1;
    return 0;
}
const char *legacy_text_formatv(const char *format,va_list args){
    static char buffers[4][LEGACY_TEXT_BUFFER];static int index;
    char *current=buffers[index];memset(current,0,LEGACY_TEXT_BUFFER);
    int required=vsnprintf(current,LEGACY_TEXT_BUFFER,format,args);
    if(required>=LEGACY_TEXT_BUFFER)memcpy(current+LEGACY_TEXT_BUFFER-4,"...",4);
    index++;if(index>=4)index=0;return current;
}
