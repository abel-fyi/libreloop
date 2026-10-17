// SPDX-License-Identifier: GPL-3.0-only
#include "browser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const char *root_path;
static FILE *out;
static void path(const char *value){size_t n=strlen(root_path);if(!strncmp(value,root_path,n))fprintf(out,"$ROOT%s",value+n);else fprintf(out,"%s",value);}
static void snapshot(Browser *b,const char *tag) {
    fprintf(out,"%s %d %d %d %d %d\n",tag,b->count,b->items,b->scroll,b->selected,b->follow);
    path(b->path);fprintf(out,"\n");path(b->samples);fprintf(out,"\n");path(b->config);fprintf(out,"\n");
    for(int i=0;i<b->count;i++){path(b->folders[i]);fprintf(out,"\n");}
    for(int i=0;i<b->items;i++){fprintf(out,"%d %d %d %d ",i,b->nodes[i].depth,b->nodes[i].dir,b->nodes[i].open);path(b->nodes[i].path);fprintf(out,"\n");}
}
static int find(Browser *b,const char *suffix){for(int i=0;i<b->items;i++){const char *p=b->nodes[i].path;size_t a=strlen(p),n=strlen(suffix);if(a>=n && !strcmp(p+a-n,suffix))return i;}return -1;}
int main(int argc,char **argv) {
    if(argc!=2)return 1;char resolved[4096];if(!realpath(argv[1],resolved))return 2;root_path=resolved;char samples[4096],config[4096],sub[4096];
    snprintf(samples,sizeof samples,"%s/samples",root_path);snprintf(config,sizeof config,"%s/config",root_path);snprintf(sub,sizeof sub,"%s/samples/Sub\\samples",root_path);
    if(setenv("XDG_CONFIG_HOME",config,1))return 2;out=fopen("browser.txt","w");if(!out)return 2;
    Browser b;browser_init(&b,samples);snapshot(&b,"initial");
    browser_select(&b,find(&b,"/Sub\\samples"));browser_right(&b);snapshot(&b,"expanded");
    browser_right(&b);snapshot(&b,"first-child");browser_left(&b);snapshot(&b,"parent");browser_left(&b);snapshot(&b,"collapsed");
    browser_select(&b,find(&b,"/broken.wav"));snapshot(&b,"broken-file-selected");
    fprintf(out,"add %d\n",browser_add(&b,sub));snapshot(&b,"favorite");
    fprintf(out,"duplicate %d\n",browser_add(&b,sub));snapshot(&b,"duplicate");
    browser_select(&b,0);browser_toggle(&b,0);snapshot(&b,"root-collapsed");browser_right(&b);snapshot(&b,"root-expanded");
    browser_close(&b);browser_init(&b,samples);snapshot(&b,"reload");browser_close(&b);fclose(out);return 0;
}
