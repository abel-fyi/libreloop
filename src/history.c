// SPDX-License-Identifier: GPL-3.0-only
#include "history.h"
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
typedef struct { Sample sample; const float *identity; uint64_t stamp; unsigned refs; } HistorySample;
struct HistoryEntry { Project project; HistorySample *sources[CHANNELS]; };
static size_t sample_bytes(Sample s) { return (size_t)s.frames*sample_channels(s)*sizeof(float); }
static void discard(History *h,HistoryEntry *e) {
    for(int c=0;c<CHANNELS;c++) if(e->sources[c] && !--e->sources[c]->refs) {
        h->bytes-=sizeof(HistorySample)+sample_bytes(e->sources[c]->sample);
        free(e->sources[c]->sample.data); free(e->sources[c]);
    }
    h->bytes-=sizeof *e; free(e);
}
void history_clear(History *h) { for(int i=0;i<h->count;i++) discard(h,h->entries[i]); *h=(History){0}; }
static Project canonical(const Project *p) { Project copy=*p; memset(copy.audio_seconds,0,sizeof copy.audio_seconds); return copy; }
int history_capture(History *h,const Project *p,const Sample sources[CHANNELS],const uint64_t stamps[CHANNELS]) {
    HistoryEntry *previous=h->count?h->entries[h->cursor]:NULL;
    size_t derived=offsetof(Project,audio_seconds),tail=derived+sizeof p->audio_seconds;
    int same=previous && !memcmp(p,&previous->project,derived)
        && !memcmp((const char *)p+tail,(const char *)&previous->project+tail,sizeof *p-tail);
    for(int c=0;c<CHANNELS && same;c++) {
        HistorySample *s=previous->sources[c];
        same=s?(s->identity==sources[c].data && s->stamp==stamps[c]):!sources[c].frames;
    }
    if(same) return 1;
    HistoryEntry *e=calloc(1,sizeof *e); if(!e) return 0;
    h->bytes+=sizeof *e; e->project=canonical(p);
    for(int c=0;c<CHANNELS;c++) if(sources[c].frames) {
        HistorySample *old=previous?previous->sources[c]:NULL;
        if(old && old->identity==sources[c].data && old->stamp==stamps[c]) { e->sources[c]=old; old->refs++; continue; }
        HistorySample *s=calloc(1,sizeof *s); if(!s) { discard(h,e); return 0; }
        s->sample=sources[c]; s->sample.data=malloc(sample_bytes(sources[c]));
        if(!s->sample.data) { free(s); discard(h,e); return 0; }
        memcpy(s->sample.data,sources[c].data,sample_bytes(sources[c]));
        s->identity=sources[c].data; s->stamp=stamps[c]; s->refs=1; e->sources[c]=s;
        h->bytes+=sizeof *s+sample_bytes(s->sample);
    }
    while(h->count && h->count-1>h->cursor) discard(h,h->entries[--h->count]);
    if(h->count==HISTORY_LIMIT) {
        discard(h,h->entries[0]); memmove(h->entries,h->entries+1,(--h->count)*sizeof h->entries[0]);
    }
    h->entries[h->count++]=e; h->cursor=h->count-1;
    while(h->count>1 && h->bytes>HISTORY_BYTES) {
        discard(h,h->entries[0]); memmove(h->entries,h->entries+1,(--h->count)*sizeof h->entries[0]); h->cursor--;
    }
    return 1;
}
int history_peek(const History *h,int direction,Project *p,Sample sources[CHANNELS]) {
    int index=h->cursor+direction;
    if((direction!=-1 && direction!=1) || index<0 || index>=h->count) return 0;
    *p=h->entries[index]->project;
    for(int c=0;c<CHANNELS;c++) sources[c]=h->entries[index]->sources[c]?h->entries[index]->sources[c]->sample:(Sample){0};
    return 1;
}
void history_step(History *h,int direction) { if((direction==-1 || direction==1) && h->cursor+direction>=0 && h->cursor+direction<h->count) h->cursor+=direction; }

void history_rebind(History *h,const Sample sources[CHANNELS],const uint64_t stamps[CHANNELS]) {
    if(!h->count) return;
    for(int c=0;c<CHANNELS;c++) {
        HistorySample *s=h->entries[h->cursor]->sources[c];
        if(s) { s->identity=sources[c].data; s->stamp=stamps[c]; }
    }
}

int history_source_matches(const History *h,int direction,int channel,Sample source,uint64_t stamp) {
    int index=h->cursor+direction;
    if(index<0 || index>=h->count || channel<0 || channel>=CHANNELS) return 0;
    HistorySample *s=h->entries[index]->sources[channel];
    return s?s->identity==source.data && s->stamp==stamp:!source.frames;
}
