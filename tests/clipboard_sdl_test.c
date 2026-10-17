// SPDX-License-Identifier: GPL-3.0-only
#include "../src/ui/frontend/host.h"
#include "text_edit.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Failed line%d: %s: %s\n",__LINE__,#x,SDL_GetError());return 1;}}while(0)
static const void * SDLCALL provide(void *user,const char *mime,size_t *size){(void)mime;*size=user?strlen(user)+1:0;return user;}
static int paste(const char *expected,int expected_selected){
    char text[32]="keep";int selected=1;AppInput input;input_init(&input);
    text_edit(text,sizeof text,&selected,(TextEditInput){.paste=1,.clipboard=host_get_clipboard_text(),.input=&input});
    return !strcmp(text,expected) && selected==expected_selected;
}
int main(void){
    host_init_window(1200,675,"Clipboard boundary check");CHECK(host_window());
    CHECK(SDL_ClearClipboardData());CHECK(platform_clipboard_read()==NULL);CHECK(host_get_clipboard_text()==NULL);CHECK(paste("keep",1));
    const char *text_type[]={"text/plain;charset=utf-8"},*binary_type[]={"application/octet-stream"};
    CHECK(SDL_SetClipboardData(provide,NULL,"",text_type,1));char *text=platform_clipboard_read();CHECK(text && !*text);free(text);CHECK(paste("",0));
    CHECK(SDL_SetClipboardData(provide,NULL,"\xce\xbb text",text_type,1));text=platform_clipboard_read();CHECK(text && !strcmp(text,"\xce\xbb text"));free(text);CHECK(paste("\xce\xbb text",0));
    CHECK(SDL_SetClipboardData(provide,NULL,"binary",binary_type,1));CHECK(platform_clipboard_read()==NULL);CHECK(paste("keep",1));
    CHECK(SDL_SetClipboardData(provide,NULL,NULL,text_type,1));CHECK(platform_clipboard_read()==NULL);CHECK(paste("keep",1));
    for(int i=0;i<20;i++){CHECK(SDL_SetClipboardData(provide,NULL,i%2?"":"again",text_type,1));CHECK(paste(i%2?"":"again",0));}
    CHECK(SDL_ClearClipboardData());host_close_window();
    puts("Unavailable, empty, UTF8, nontext and failed clipboard data preserve original paste selection behavior");return 0;
}
