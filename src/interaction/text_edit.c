// SPDX-License-Identifier: GPL-3.0-only
/* Extracted from frozen ui_menus.c:text_input/ui_widgets.c:backspace. */
#include "text_edit.h"
#include "utf8.h"
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
void text_backspace(char *text) {
    size_t n=strlen(text); if(!n) return;
    do { n--; } while(n && ((unsigned char)text[n]&0xc0)==0x80);
    text[n]=0;
}
void text_edit(char *text,size_t capacity,int *selected,TextEditInput event) {
    if(event.select_all) *selected=1;
    if(event.paste) {
        const char *paste=event.clipboard;
        if(paste) {
            if(*selected) { text[0]=0; *selected=0; }
            while(*paste) {
                int bytes,ch=utf8_next(paste,&bytes); size_t n=strlen(text);
                if(ch<32 || ch==127 || n+bytes>=capacity) break;
                memcpy(text+n,paste,bytes); text[n+bytes]=0; paste+=bytes;
            }
        }
    }
    if(event.backspace) { if(*selected) text[0]=0; else text_backspace(text); *selected=0; }
    if(event.arrow) *selected=0;
    int ch;
    while((ch=input_next_char(event.input))>0) {
        if(ch<32 || ch==127 || event.command_down) continue;
        if(*selected) { text[0]=0; *selected=0; }
        int bytes; const char *utf8=utf8_encode(ch,&bytes); size_t n=strlen(text);
        if(n+bytes<capacity) { memcpy(text+n,utf8,bytes); text[n+bytes]=0; }
    }
}
int text_number_parse(const char *text,float low,float high,int integer,float *target) {
    char value[64],*end; snprintf(value,sizeof value,"%s",text);
    for(char *c=value;*c;c++) if(*c==',') *c='.';
    float parsed=strtof(value,&end); int parsed_any=end!=value; while(isspace((unsigned char)*end)) end++;
    if(!parsed_any || *end || !isfinite(parsed) || parsed<low || parsed>high || (integer && parsed!=roundf(parsed))) return 0;
    *target=parsed; return 1;
}
const char *text_rename_name(char *text) {
    size_t n=strlen(text); while(n && text[n-1]==' ') text[--n]=0;
    char *name=text; while(*name==' ') name++;
    return *name?name:NULL;
}
