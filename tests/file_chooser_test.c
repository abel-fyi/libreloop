// SPDX-License-Identifier: GPL-3.0-only
#include "file_chooser.h"
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
static int touch(const char *path) { FILE *f=fopen(path,"w"); return f && !fclose(f); }
int main(void) {
    char temporary[]="/tmp/libreloop-chooser-XXXXXX",root[PATH_MAX]; CHECK(mkdtemp(temporary) && realpath(temporary,root));
    char project[PATH_MAX],wave[PATH_MAX],hidden[PATH_MAX],folder[PATH_MAX],path[PATH_MAX];
    snprintf(project,sizeof project,"%s/My Song.hbt",root); CHECK(touch(project));
    snprintf(wave,sizeof wave,"%s/Audio.wav",root); CHECK(touch(wave));
    snprintf(hidden,sizeof hidden,"%s/.hidden.hbt",root); CHECK(touch(hidden));
    snprintf(folder,sizeof folder,"%s/Subfolder",root); CHECK(!mkdir(folder,0700));
    FileChooser c={0}; CHECK(file_chooser_begin(&c,project,"hbt",0));
    CHECK(c.count==2 && c.entries[0].directory && !strcmp(c.entries[1].name,"My Song.hbt"));
    CHECK(!strcmp(c.name,"")); CHECK(file_chooser_path(&c,path,sizeof path)==-1);
    snprintf(c.name,sizeof c.name,"My Song.hbt"); CHECK(file_chooser_path(&c,path,sizeof path)==1 && !strcmp(path,project));
    snprintf(c.name,sizeof c.name,"Audio.wav"); CHECK(file_chooser_path(&c,path,sizeof path)==-1);
    snprintf(c.name,sizeof c.name,"missing.hbt"); CHECK(file_chooser_path(&c,path,sizeof path)==-1);
    c.hidden=1; CHECK(file_chooser_folder(&c,c.directory) && c.count==3);
    CHECK(file_chooser_folder(&c,folder) && !c.count); CHECK(file_chooser_parent(&c));
    CHECK(!file_chooser_folder(&c,"/nonexistent/libreloop-folder") && c.count==3);
    CHECK(file_chooser_begin(&c,project,"hbt",1));
    CHECK(file_chooser_path(&c,path,sizeof path)==2 && !strcmp(path,project));
    snprintf(c.name,sizeof c.name,"Fresh Song"); CHECK(file_chooser_path(&c,path,sizeof path)==1);
    CHECK(strstr(path,"/Fresh Song.hbt") && access(path,F_OK)); /* Preparing a selection never writes it. */
    snprintf(c.name,sizeof c.name,"My Song"); CHECK(file_chooser_path(&c,path,sizeof path)==2); /* Confirm after adding extension. */
    snprintf(c.name,sizeof c.name,"Subfolder"); CHECK(file_chooser_path(&c,path,sizeof path)==0 && !strcmp(c.directory,folder));
    snprintf(c.name,sizeof c.name,"../Other.hbt"); CHECK(file_chooser_path(&c,path,sizeof path)==1 && strstr(path,"/Other.hbt"));
    CHECK(file_chooser_path(&c,path,8)==-1);
    CHECK(file_chooser_folder(&c,"/") && file_chooser_parent(&c) && !strcmp(c.directory,"/"));
    file_chooser_close(&c); CHECK(!c.entries && !c.count);
    CHECK(!remove(project) && !remove(wave) && !remove(hidden) && !rmdir(folder) && !rmdir(root));
    puts("Chooser filtering, spaces, hidden files, parent/root navigation, failures, extensions and overwrite checks passed."); return 0;
}
