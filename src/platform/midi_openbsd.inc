// SPDX-License-Identifier: GPL-3.0-only
/* OpenBSD midi(4) presents raw input bytes through /dev/rmidiN. */
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <poll.h>
#include <pthread.h>
#include <unistd.h>

static int midi_fd=-1;
static pthread_t worker;
static atomic_int running;
static int started;
static MidiParser parser;
static char current_id[128];

int midi_input_devices(MidiDevice *devices,int capacity) {
    glob_t paths={0};
    if(capacity<=0 || glob("/dev/rmidi[0-9]*",0,NULL,&paths))return 0;
    int count=0;
    for(size_t i=0;i<paths.gl_pathc && count<capacity;i++) {
        const char *path=paths.gl_pathv[i];
        const char *number=path+strlen("/dev/rmidi");
        if(!*number || strspn(number,"0123456789")!=strlen(number))continue;
        if(strcmp(path,current_id)) {
            int probe=open(path,O_RDONLY|O_NONBLOCK);
            if(probe<0)continue;
            close(probe);
        }
        snprintf(devices[count].id,sizeof devices[count].id,"%s",path);
        snprintf(devices[count].name,sizeof devices[count].name,"MIDI input %s",number);
        count++;
    }
    globfree(&paths);
    return count;
}

static void *receive(void *unused) {
    (void)unused;
    struct pollfd descriptor={.fd=midi_fd,.events=POLLIN};
    unsigned char bytes[256];
    while(atomic_load(&running)) {
        int result=poll(&descriptor,1,50);
        if(result<0 && errno==EINTR)continue;
        if(result<0 || (result>0 && (descriptor.revents&(POLLERR|POLLHUP|POLLNVAL))))break;
        if(result<=0 || !(descriptor.revents&POLLIN))continue;
        ssize_t count=read(midi_fd,bytes,sizeof bytes);
        if(count>0)midi_parse(&parser,bytes,(unsigned)count,midi_input_time(),emit,NULL);
        else if(count==0 || (errno!=EAGAIN && errno!=EINTR))break;
    }
    if(atomic_load(&running)){atomic_store(&failed,2);notify_change();}
    return NULL;
}

void midi_input_close(void) {
    if(started){atomic_store(&running,0);pthread_join(worker,NULL);started=0;}
    if(midi_fd>=0){close(midi_fd);midi_fd=-1;}
    current_id[0]=0;
    memset(&parser,0,sizeof parser);
    midi_input_flush();
}

int midi_input_open(const char *id,char error[256]) {
    midi_input_close();
    if(!*id)return 1;
    if(strncmp(id,"/dev/rmidi",10) || !id[10] || strspn(id+10,"0123456789")!=strlen(id+10)) {
        snprintf(error,256,"Invalid OpenBSD MIDI device");return 0;
    }
    midi_fd=open(id,O_RDONLY|O_NONBLOCK);
    if(midi_fd<0){snprintf(error,256,"Cannot open %s: %s",id,strerror(errno));return 0;}
    snprintf(current_id,sizeof current_id,"%s",id);
    atomic_store(&running,1);
    if(pthread_create(&worker,NULL,receive,NULL)) {
        midi_input_close();snprintf(error,256,"Cannot start MIDI input");return 0;
    }
    started=1;
    return 1;
}
