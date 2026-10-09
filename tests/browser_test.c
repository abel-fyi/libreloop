// SPDX-License-Identifier: GPL-3.0-only
#include "browser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
int main(void) {
    char temporary[]="/tmp/libreloop-browser-XXXXXX",root[PATH_MAX];
    CHECK(mkdtemp(temporary)); CHECK(realpath(temporary,root));
    char config[PATH_MAX],child[PATH_MAX],file[PATH_MAX],ignored[PATH_MAX],nested[PATH_MAX];
    snprintf(config,sizeof config,"%s/config",root); CHECK(!setenv("XDG_CONFIG_HOME",config,1));
    snprintf(child,sizeof child,"%s/My samples",root); CHECK(MakeDirectory(child)==0);
    snprintf(nested,sizeof nested,"%s/My samples/chord.wav",root); FILE *f=fopen(nested,"w"); CHECK(f); fclose(f);
    snprintf(file,sizeof file,"%s/test.wav",root); f=fopen(file,"w"); CHECK(f); fclose(f);
    snprintf(ignored,sizeof ignored,"%s/README.txt",root); f=fopen(ignored,"w"); CHECK(f); fclose(f);
    Browser b; browser_init(&b,root); CHECK(!strcmp(b.path,root));
    CHECK(b.items==4 && b.nodes[0].open && b.nodes[1].dir && b.nodes[2].dir && !b.nodes[3].dir);
    browser_select(&b,1); browser_right(&b); CHECK(b.nodes[1].open && b.items==5);
    browser_right(&b); CHECK(b.selected==2 && !strcmp(b.nodes[2].path,nested));
    browser_left(&b); CHECK(b.selected==1); browser_left(&b); CHECK(!b.nodes[1].open && b.items==4);
    CHECK(browser_add(&b,child)); CHECK(b.count==1 && !strcmp(b.path,child));
    int added=b.selected; CHECK(b.nodes[added].depth==0 && b.nodes[added].open);
    CHECK(browser_add(&b,child) && b.count==1 && b.selected==added);
    CHECK(browser_add(&b,root) && b.count==1 && b.selected==0);
    CHECK(!browser_add(&b,"/libreloop-directory-that-does-not-exist"));
    browser_select(&b,b.items-1); CHECK(browser_toggle(&b,0));
    CHECK(b.items==3 && b.selected==2 && !strcmp(b.nodes[b.selected].path,nested));
    browser_select(&b,0); browser_left(&b); CHECK(b.selected==0);
    CHECK(browser_toggle(&b,0)); CHECK(b.nodes[0].open && b.selected==0);
    browser_close(&b); browser_init(&b,root);
    CHECK(b.count==1 && !strcmp(b.folders[0],child));
    CHECK(b.nodes[b.items-1].depth==0 && !b.nodes[b.items-1].open);
    /* Native projects are visible; unsupported HBT files are filtered out. */
    char projectfile[PATH_MAX],oldfile[PATH_MAX];
    snprintf(projectfile,sizeof projectfile,"%s/session.llp",root); f=fopen(projectfile,"w"); CHECK(f); fclose(f);
    snprintf(oldfile,sizeof oldfile,"%s/session.hbt",root); f=fopen(oldfile,"w"); CHECK(f); fclose(f);
    CHECK(browser_add(&b,root));
    if(b.nodes[0].open) CHECK(browser_toggle(&b,0));
    CHECK(browser_toggle(&b,0)); int found=0;
    for(int i=0;i<b.items;i++) { CHECK(strcmp(b.nodes[i].path,oldfile)); if(!strcmp(b.nodes[i].path,projectfile)) found++; }
    CHECK(found==1); remove(projectfile); remove(oldfile);
    remove(b.config); char configdir[PATH_MAX]; snprintf(configdir,sizeof configdir,"%s/libreloop",config);
    browser_close(&b); remove(file); remove(ignored); remove(nested); rmdir(child); rmdir(configdir); rmdir(config); rmdir(root);
    puts("Tree expansion/collapse, parent navigation, folders-first sorting, root selection, duplicate roots and persistence passed."); return 0;
}
