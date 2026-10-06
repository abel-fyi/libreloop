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
    CHECK(file_chooser_begin(&c,wave,"wav;flac;mp3",0));
    snprintf(c.name,sizeof c.name,"Audio.wav"); CHECK(file_chooser_path(&c,path,sizeof path)==1);
    file_chooser_close(&c); CHECK(!c.entries && !c.count);
    char config[PATH_MAX],locations[PATH_MAX];
    snprintf(config,sizeof config,"%s/folders.txt",root);
    snprintf(locations,sizeof locations,"%s/chooser-folders.txt",root);
    file_chooser_locations(config);
    CHECK(file_chooser_begin_recent(&c,project,"hbt",0,FILE_OPEN));
    CHECK(file_chooser_folder(&c,folder)); file_chooser_close(&c); /* Cancel remembers navigation. */
    file_chooser_locations(config); /* Simulate an app restart. */
    CHECK(file_chooser_begin_recent(&c,project,"hbt",0,FILE_OPEN) && !strcmp(c.directory,folder) && !c.name[0]);
    file_chooser_close(&c);
    CHECK(file_chooser_begin_recent(&c,project,"hbt",1,FILE_SAVE) && !strcmp(c.directory,root));
    CHECK(file_chooser_folder(&c,folder)); file_chooser_close(&c);
    CHECK(file_chooser_begin_recent(&c,project,"hbt",1,FILE_SAVE) && !strcmp(c.directory,folder) && !strcmp(c.name,"My Song.hbt"));
    file_chooser_remember(&c,project); file_chooser_close(&c); /* Absolute filename uses its actual parent. */
    file_chooser_locations(config);
    CHECK(file_chooser_begin_recent(&c,project,"hbt",1,FILE_SAVE) && !strcmp(c.directory,root)); file_chooser_close(&c);
    CHECK(file_chooser_begin_recent(&c,wave,"wav",1,FILE_EXPORT) && !strcmp(c.directory,root)); file_chooser_close(&c);
    CHECK(!rmdir(folder));
    CHECK(file_chooser_begin_recent(&c,project,"hbt",0,FILE_OPEN) && !strcmp(c.directory,root) && !c.error[0]); file_chooser_close(&c);
    CHECK(!remove(locations));
    CHECK(!remove(project) && !remove(wave) && !remove(hidden) && !rmdir(root));
    puts("Chooser filtering, spaces, hidden files, parent/root navigation, failures, extensions and overwrite checks passed."); return 0;
}
