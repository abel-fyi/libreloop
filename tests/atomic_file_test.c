// SPDX-License-Identifier: GPL-3.0-only
#include "atomic_file.h"
#include "engine.h"
#include <unistd.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
static Project project;
static int unchanged(const char *path) {
    FILE *f=fopen(path,"rb"); if(!f) return 0;
    char text[64]={0}; size_t n=fread(text,1,sizeof text,f); fclose(f);
    return n==8 && !memcmp(text,"original",8);
}
int main(void) {
    char directory[]="/tmp/libreloop-atomic-XXXXXX",path[PATH_MAX]; CHECK(mkdtemp(directory));
    snprintf(path,sizeof path,"%s/song.wav",directory);
    FILE *f=fopen(path,"wb"); CHECK(f && fwrite("original",1,8,f)==8 && !fclose(f));
    AtomicFile output; CHECK(atomic_file_open(&output,path));
    CHECK(fwrite("new",1,3,output.file)==3); atomic_file_abort(&output); CHECK(unchanged(path));
    project_demo(&project);
    for(int save=0;save<2;save++) {
        pid_t child=fork(); CHECK(child>=0);
        if(!child) {
            signal(SIGXFSZ,SIG_IGN); struct rlimit limit={8,8};
            if(setrlimit(RLIMIT_FSIZE,&limit)) _exit(2);
            Sample samples[CHANNELS]={0};
            int ok=save?project_save(path,&project):export_wav(path,&project,samples);
            _exit(ok?1:0);
        }
        int status; CHECK(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
        CHECK(unchanged(path));
        DIR *d=opendir(directory); CHECK(d); struct dirent *entry;
        while((entry=readdir(d))) CHECK(!strstr(entry->d_name,".tmp-")); closedir(d);
    }
    CHECK(atomic_file_open(&output,path)); CHECK(fwrite("replacement",1,11,output.file)==11);
    CHECK(atomic_file_commit(&output));
    f=fopen(path,"rb"); char data[12]={0}; CHECK(f && fread(data,1,11,f)==11 && !memcmp(data,"replacement",11)); fclose(f);
    unlink(path); CHECK(!rmdir(directory));
    puts("Failed export/save preserve the destination and clean up unique temporary files."); return 0;
}
