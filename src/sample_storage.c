// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdatomic.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
struct SampleStorage { void *mapping; size_t bytes; atomic_uint refs; };
void sample_free(Sample sample) {
    if(!sample.storage) { free(sample.data); return; }
    if(atomic_fetch_sub(&sample.storage->refs,1)==1) {
        munmap(sample.storage->mapping,sample.storage->bytes); free(sample.storage);
    }
}
int sample_clone(Sample source,Sample *result) {
    if(source.frames && !source.data) return 0;
    if(source.storage) { atomic_fetch_add(&source.storage->refs,1); *result=source; return 1; }
    Sample copy={.frames=source.frames,.channels=source.channels};
    if(source.frames) {
        size_t bytes=(size_t)source.frames*sample_channels(source)*sizeof(float);
        copy.data=malloc(bytes); if(!copy.data) return 0;
        memcpy(copy.data,source.data,bytes);
    }
    *result=copy; return 1;
}
int sample_map_recording(const char *path,unsigned frames,Sample *result) {
    if(!frames) { *result=(Sample){0}; return 1; }
    /* Our writer emits a 44-byte, little-endian, float32 stereo WAV. */
    uint32_t endian=1; if(*(unsigned char *)&endian!=1 || sizeof(float)!=4) return 0;
    if(frames>SAMPLE_MAX_FRAMES) return 0;
    size_t bytes=44+(size_t)frames*8;
    int fd=open(path,O_RDONLY); if(fd<0) return 0;
    struct stat info;
    if(fstat(fd,&info) || info.st_size<(off_t)bytes) { close(fd); return 0; }
    void *mapping=mmap(NULL,bytes,PROT_READ,MAP_SHARED,fd,0); close(fd);
    if(mapping==MAP_FAILED) return 0;
    const unsigned char *header=mapping;
    if(memcmp(header,"RIFF",4) || memcmp(header+8,"WAVEfmt ",8) ||
       header[20]!=3 || header[21]!=0 || header[22]!=2 || header[23]!=0 ||
       header[24]!=(RATE&255) || header[25]!=((RATE>>8)&255) || header[26]!=0 || header[27]!=0 ||
       header[32]!=8 || header[33]!=0 || header[34]!=32 || header[35]!=0 || memcmp(header+36,"data",4)) {
        munmap(mapping,bytes); return 0;
    }
    SampleStorage *storage=malloc(sizeof *storage);
    if(!storage) { munmap(mapping,bytes); return 0; }
    storage->mapping=mapping; storage->bytes=bytes; atomic_init(&storage->refs,1);
    *result=(Sample){.data=(float *)((char *)mapping+44),.frames=frames,.channels=2,.storage=storage};
    return 1;
}

int sample_map_wav(const char *path,Sample *result) {
    FILE *file=fopen(path,"rb"); if(!file) return 0;
    unsigned char header[44]; size_t n=fread(header,1,sizeof header,file); fclose(file);
    if(n!=sizeof header) return 0;
    uint32_t bytes=(uint32_t)header[40] | (uint32_t)header[41]<<8 | (uint32_t)header[42]<<16 | (uint32_t)header[43]<<24;
    if(!bytes || bytes%8) return 0;
    return sample_map_recording(path,bytes/8,result);
}
