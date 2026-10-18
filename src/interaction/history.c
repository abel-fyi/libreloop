// SPDX-License-Identifier: GPL-3.0-only
#include "history.h"
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
typedef struct { Sample sample; const float *identity; uint64_t stamp; unsigned refs; } HistorySample;
/* Keep the fixed project metadata, but omit all-zero note rows from undo.
   The live engine still receives a complete concrete Project. */
struct HistoryEntry { HistorySample *sources[CHANNELS];size_t bytes;uint64_t notes;unsigned char state[]; };
#define NOTE_OFFSET offsetof(Project,notes)
#define NOTE_BYTES sizeof(((Project *)0)->notes)
#define NOTE_END (NOTE_OFFSET+NOTE_BYTES)
#define BODY_BYTES (sizeof(Project)-NOTE_BYTES)
_Static_assert(PATTERNS<=64,"History note mask must cover every pattern");
static int row_zero(const void *row){
    const unsigned char *p=row;uint64_t any=0;
    for(size_t i=0;i<sizeof(((Project *)0)->notes[0]);i+=sizeof(uint64_t)){uint64_t v;memcpy(&v,p+i,sizeof v);any|=v;}
    return !any;
}
static int project_same(const HistoryEntry *e,const Project *p){
    size_t derived=offsetof(Project,audio_seconds),tail=derived+sizeof p->audio_seconds;
    const unsigned char *src=(const unsigned char *)p;
    if(memcmp(src,e->state,NOTE_OFFSET) || memcmp(src+NOTE_END,e->state+NOTE_OFFSET,derived-NOTE_END) ||
       memcmp(src+tail,e->state+tail-NOTE_BYTES,sizeof *p-tail))return 0;
    const unsigned char *notes=e->state+BODY_BYTES;
    for(int i=0;i<PATTERNS;i++){
        if(e->notes&(UINT64_C(1)<<i)){if(memcmp(p->notes[i],notes,sizeof p->notes[i]))return 0;notes+=sizeof p->notes[i];}
        else if(!row_zero(p->notes[i]))return 0;
    }
    return 1;
}
static void project_restore(const HistoryEntry *e,Project *p){
    memcpy(p,e->state,NOTE_OFFSET);memcpy((unsigned char *)p+NOTE_END,e->state+NOTE_OFFSET,sizeof *p-NOTE_END);
    memset(p->notes,0,sizeof p->notes);const unsigned char *notes=e->state+BODY_BYTES;
    for(int i=0;i<PATTERNS;i++)if(e->notes&(UINT64_C(1)<<i)){memcpy(p->notes[i],notes,sizeof p->notes[i]);notes+=sizeof p->notes[i];}
}
static size_t sample_bytes(Sample s) { return (size_t)s.frames*sample_channels(s)*sizeof(float); }
static void discard(History *h,HistoryEntry *e) {
    for(int c=0;c<CHANNELS;c++) if(e->sources[c] && !--e->sources[c]->refs) {
        h->bytes-=sizeof(HistorySample)+sample_bytes(e->sources[c]->sample);
        sample_free(e->sources[c]->sample); free(e->sources[c]);
    }
    h->bytes-=e->bytes; free(e);
}
void history_clear(History *h) { for(int i=0;i<h->count;i++) discard(h,h->entries[i]); *h=(History){0}; }
int history_capture(History *h,const Project *p,const Sample sources[CHANNELS],const uint64_t stamps[CHANNELS]) {
    HistoryEntry *previous=h->count?h->entries[h->cursor]:NULL;
    int same=previous && project_same(previous,p);
    for(int c=0;c<CHANNELS && same;c++) {
        HistorySample *s=previous->sources[c];
        same=s?(s->identity==sources[c].data && s->stamp==stamps[c]):!sources[c].frames;
    }
    if(same) return 1;
    uint64_t mask=0;size_t note_bytes=0;
    for(int i=0;i<PATTERNS;i++)if(!row_zero(p->notes[i])){mask|=UINT64_C(1)<<i;note_bytes+=sizeof p->notes[i];}
    size_t bytes=sizeof(HistoryEntry)+BODY_BYTES+note_bytes;
    HistoryEntry *e=calloc(1,bytes); if(!e) return 0;e->bytes=bytes;e->notes=mask;h->bytes+=bytes;
    memcpy(e->state,p,NOTE_OFFSET);memcpy(e->state+NOTE_OFFSET,(const unsigned char *)p+NOTE_END,sizeof *p-NOTE_END);
    unsigned char *notes=e->state+BODY_BYTES;
    for(int i=0;i<PATTERNS;i++)if(mask&(UINT64_C(1)<<i)){memcpy(notes,p->notes[i],sizeof p->notes[i]);notes+=sizeof p->notes[i];}
    for(int c=0;c<CHANNELS;c++)if(sources[c].frames || !p->paths[c][0] || !strcmp(p->paths[c],SAMPLE_EMPTY)){
        float zero=0;memcpy(e->state+offsetof(Project,audio_seconds)-NOTE_BYTES+c*sizeof(float),&zero,sizeof zero);
    }
    for(int c=0;c<CHANNELS;c++) if(sources[c].frames) {
        HistorySample *old=previous?previous->sources[c]:NULL;
        if(old && old->identity==sources[c].data && old->stamp==stamps[c]) { e->sources[c]=old; old->refs++; continue; }
        HistorySample *s=calloc(1,sizeof *s); if(!s) { discard(h,e); return 0; }
        if(!sample_clone(sources[c],&s->sample)) { free(s); discard(h,e); return 0; }
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
    project_restore(h->entries[index],p);
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
