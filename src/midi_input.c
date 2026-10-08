// SPDX-License-Identifier: GPL-3.0-only
#include "midi_input.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define MIDI_QUEUE 2048
static MidiEvent events[MIDI_QUEUE];
static atomic_uint read_pos,write_pos;
static atomic_int failed,changed;
typedef void (*Wake)(void);
static _Atomic(Wake) wake;
void midi_input_wake(Wake fn) { atomic_store(&wake,fn); }
double midi_input_time(void) {struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;}
static void emit(void *context,MidiEvent event) {
    (void)context;if(event.status<0x80 || event.status>=0xf0 || event.data1>127 || event.data2>127)return;unsigned w=atomic_load_explicit(&write_pos,memory_order_relaxed),r=atomic_load_explicit(&read_pos,memory_order_acquire);
    if(w-r>=MIDI_QUEUE)atomic_store(&failed,1);
    else {events[w%MIDI_QUEUE]=event;atomic_store_explicit(&write_pos,w+1,memory_order_release);}
    Wake fn=atomic_load(&wake);if(fn)fn();
}
int midi_input_poll(MidiEvent *event) {
    unsigned r=atomic_load_explicit(&read_pos,memory_order_relaxed),w=atomic_load_explicit(&write_pos,memory_order_acquire);
    if(r==w)return 0;*event=events[r%MIDI_QUEUE];atomic_store_explicit(&read_pos,r+1,memory_order_release);return 1;
}
int midi_input_failed(void) {return atomic_exchange(&failed,0);}
int midi_input_changed(void) {return atomic_exchange(&changed,0);}
static void notify_change(void) {atomic_store(&changed,1);Wake fn=atomic_load(&wake);if(fn)fn();}
void midi_input_flush(void) {atomic_store(&read_pos,atomic_load(&write_pos));}
#ifdef __APPLE__
#include <CoreMIDI/CoreMIDI.h>
static MIDIClientRef client;static MIDIPortRef port;static MIDIEndpointRef source;static MidiParser parser;
static void receive(const MIDIPacketList *list,void *context,void *connection) {
    (void)context;(void)connection;const MIDIPacket *p=&list->packet[0];
    for(UInt32 i=0;i<list->numPackets;i++) {midi_parse(&parser,p->data,p->length,midi_input_time(),emit,NULL);p=MIDIPacketNext(p);}
}
static void notify(const MIDINotification *message,void *context) {(void)message;(void)context;notify_change();}
static int prepare(void) {return client || MIDIClientCreate(CFSTR("LibreLoop"),notify,NULL,&client)==noErr;}
static void source_id(MIDIEndpointRef endpoint,char id[128]) {SInt32 unique=0;MIDIObjectGetIntegerProperty(endpoint,kMIDIPropertyUniqueID,&unique);snprintf(id,128,"%d",(int)unique);}
int midi_input_devices(MidiDevice *devices,int capacity) {
    if(!prepare())return 0;int n=0;
    for(ItemCount i=0;i<MIDIGetNumberOfSources() && n<capacity;i++) {
        MIDIEndpointRef e=MIDIGetSource(i);if(!e)continue;source_id(e,devices[n].id);
        CFStringRef name=NULL;if(MIDIObjectGetStringProperty(e,kMIDIPropertyDisplayName,&name)==noErr && name) {
            if(!CFStringGetCString(name,devices[n].name,128,kCFStringEncodingUTF8))snprintf(devices[n].name,128,"MIDI input %lu",(unsigned long)i+1);CFRelease(name);
        } else snprintf(devices[n].name,128,"MIDI input %lu",(unsigned long)i+1);n++;
    }return n;
}
void midi_input_close(void) {
    if(port){if(source)MIDIPortDisconnectSource(port,source);MIDIPortDispose(port);}if(client){MIDIClientDispose(client);client=0;}port=0;source=0;memset(&parser,0,sizeof parser);midi_input_flush();
}
int midi_input_open(const char *id,char error[256]) {
    midi_input_close();if(!prepare()){snprintf(error,256,"CoreMIDI is unavailable");return 0;}
    if(!*id)return 1;
    for(ItemCount i=0;i<MIDIGetNumberOfSources();i++){MIDIEndpointRef e=MIDIGetSource(i);char candidate[128];source_id(e,candidate);if(!strcmp(id,candidate)){source=e;break;}}
    if(!source || MIDIInputPortCreate(client,CFSTR("Keyboard"),receive,NULL,&port)!=noErr || MIDIPortConnectSource(port,source,NULL)!=noErr) {
        midi_input_close();snprintf(error,256,"Cannot connect to this MIDI input");return 0;
    }return 1;
}
#else
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <poll.h>
#include <errno.h>
static snd_seq_t *seq;static int port=-1;static pthread_t worker;static atomic_int running;static int started;
static int prepare(void) {if(seq)return 1;if(snd_seq_open(&seq,"default",SND_SEQ_OPEN_INPUT,SND_SEQ_NONBLOCK)<0)return 0;snd_seq_set_client_name(seq,"LibreLoop");return 1;}
int midi_input_devices(MidiDevice *devices,int capacity) {
    snd_seq_t *seq=NULL;if(snd_seq_open(&seq,"default",SND_SEQ_OPEN_INPUT,SND_SEQ_NONBLOCK)<0)return 0;int n=0;snd_seq_client_info_t *client;snd_seq_port_info_t *info;
    snd_seq_client_info_alloca(&client);snd_seq_port_info_alloca(&info);snd_seq_client_info_set_client(client,-1);
    while(snd_seq_query_next_client(seq,client)>=0 && n<capacity){int c=snd_seq_client_info_get_client(client);if(c==snd_seq_client_id(seq) || c==0)continue;
        snd_seq_port_info_set_client(info,c);snd_seq_port_info_set_port(info,-1);
        while(snd_seq_query_next_port(seq,info)>=0 && n<capacity) {
            unsigned caps=snd_seq_port_info_get_capability(info);if((caps&(SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ))!=(SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ))continue;
            snprintf(devices[n].id,128,"%d:%d",c,snd_seq_port_info_get_port(info));snprintf(devices[n].name,128,"%s / %s",snd_seq_client_info_get_name(client),snd_seq_port_info_get_name(info));n++;
        }
    }snd_seq_close(seq);return n;
}
static void *receive(void *unused) {
    (void)unused;int count=snd_seq_poll_descriptors_count(seq,POLLIN);struct pollfd *fds=count>0?calloc(count,sizeof *fds):NULL;
    if(count<=0 || !fds){free(fds);atomic_store(&failed,2);notify_change();return NULL;}snd_seq_poll_descriptors(seq,fds,count,POLLIN);
    while(atomic_load(&running)) {
        poll(fds,count,50);snd_seq_event_t *e;int result;
        while((result=snd_seq_event_input(seq,&e))>=0) {
            MidiEvent event={.time=midi_input_time()};
            switch(e->type) {
            case SND_SEQ_EVENT_NOTEON:case SND_SEQ_EVENT_NOTEOFF:event.status=(e->type==SND_SEQ_EVENT_NOTEON?0x90:0x80)|e->data.note.channel;event.data1=e->data.note.note;event.data2=e->data.note.velocity;break;
            case SND_SEQ_EVENT_CONTROLLER:event.status=0xb0|e->data.control.channel;event.data1=e->data.control.param;event.data2=e->data.control.value;break;
            case SND_SEQ_EVENT_PITCHBEND:{int v=e->data.control.value+8192;event.status=0xe0|e->data.control.channel;event.data1=v&127;event.data2=(v>>7)&127;break;}
            case SND_SEQ_EVENT_PORT_START:case SND_SEQ_EVENT_PORT_EXIT:case SND_SEQ_EVENT_PORT_CHANGE:if(e->data.addr.client!=snd_seq_client_id(seq))notify_change();continue;
            default:continue;
            }emit(NULL,event);
        }
        if(result!=-EAGAIN){atomic_store(&failed,result==-ENOSPC?1:2);notify_change();if(result!=-ENOSPC)atomic_store(&running,0);}
    }free(fds);return NULL;
}
void midi_input_close(void) {
    if(started){atomic_store(&running,0);pthread_join(worker,NULL);}started=0;
    if(seq){snd_seq_close(seq);seq=NULL;}port=-1;midi_input_flush();
}
int midi_input_open(const char *id,char error[256]) {
    midi_input_close();int c,p;
    if((*id && sscanf(id,"%d:%d",&c,&p)!=2) || !prepare() || (port=snd_seq_create_simple_port(seq,"Keyboard",SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,SND_SEQ_PORT_TYPE_APPLICATION))<0 || (*id && snd_seq_connect_from(seq,port,c,p)<0)) {
        midi_input_close();snprintf(error,256,"Cannot connect to the ALSA MIDI input");return 0;
    }
    snd_seq_connect_from(seq,port,SND_SEQ_CLIENT_SYSTEM,SND_SEQ_PORT_SYSTEM_ANNOUNCE);
    atomic_store(&running,1);if(pthread_create(&worker,NULL,receive,NULL)){midi_input_close();snprintf(error,256,"Cannot start MIDI input");return 0;}started=1;return 1;
}
#endif
