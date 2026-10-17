// SPDX-License-Identifier: GPL-3.0-only
#include "atomic_file.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
int atomic_file_open(AtomicFile *out,const char *path) {
    *out=(AtomicFile){0};
    if(!path || strlen(path)>=sizeof out->destination ||
       snprintf(out->temporary,sizeof out->temporary,"%s.tmp-XXXXXX",path)>=(int)sizeof out->temporary) return 0;
    strcpy(out->destination,path);
    int fd=mkstemp(out->temporary); if(fd<0) return 0;
    struct stat info;
    if(!stat(path,&info)) fchmod(fd,info.st_mode&0777);
    out->file=fdopen(fd,"wb");
    if(!out->file) { close(fd); unlink(out->temporary); return 0; }
    return 1;
}
void atomic_file_abort(AtomicFile *out) {
    if(out->file) fclose(out->file);
    if(out->temporary[0]) unlink(out->temporary);
    out->file=NULL;
}
int atomic_file_commit(AtomicFile *out) {
    if(!out->file) return 0;
    int ok=!ferror(out->file);
    if(fflush(out->file) || fsync(fileno(out->file))) ok=0;
    if(fclose(out->file)) ok=0;
    out->file=NULL;
    if(ok && !rename(out->temporary,out->destination)) return 1;
    atomic_file_abort(out); return 0;
}
