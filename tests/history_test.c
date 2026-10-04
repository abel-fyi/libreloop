// SPDX-License-Identifier: GPL-3.0-only
#include "history.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
static Project p,restored;
int main(void) {
    History h={0}; Sample sources[CHANNELS]={0},saved[CHANNELS]; uint64_t stamps[CHANNELS]={0};
    project_new(&p); CHECK(history_capture(&h,&p,sources,stamps)); CHECK(h.count==1);
    CHECK(history_capture(&h,&p,sources,stamps) && h.count==1);
    CHECK(!history_peek(&h,-1,&restored,saved));
    p.audio_seconds[0]=2; CHECK(history_capture(&h,&p,sources,stamps) && h.count==1);
    float pcm[]={.1f,.2f,.3f,.4f}; sources[0]=(Sample){pcm,2,2}; stamps[0]=1;
    p.paths[0][0]='a'; CHECK(history_capture(&h,&p,sources,stamps));
    size_t with_pcm=h.bytes;
    p.volume[0]=.4f; CHECK(history_capture(&h,&p,sources,stamps));
    CHECK(h.bytes-with_pcm<sizeof(Project)+CHANNELS*sizeof(void *)+128); /* PCM is shared across parameter edits. */
    float replacement[]={.8f,.9f}; sources[0]=(Sample){replacement,2,1}; stamps[0]++;
    CHECK(history_capture(&h,&p,sources,stamps)); pcm[0]=9; /* History owns its original PCM. */
    CHECK(history_peek(&h,-1,&restored,saved) && sample_channels(saved[0])==2 && saved[0].data[0]==.1f && restored.volume[0]==.4f);
    history_step(&h,-1);
    CHECK(history_peek(&h,1,&restored,saved) && saved[0].data[0]==.8f);
    CHECK(history_peek(&h,-1,&restored,saved) && restored.volume[0]==1);
    history_step(&h,-1); p=restored; sources[0]=saved[0]; stamps[0]=10;
    history_rebind(&h,sources,stamps); CHECK(history_capture(&h,&p,sources,stamps) && h.count==4);
    p.volume[0]=.7f; CHECK(history_capture(&h,&p,sources,stamps)); CHECK(!history_peek(&h,1,&restored,saved));
    for(int i=0;i<HISTORY_LIMIT+10;i++) { p.bpm=100+i; CHECK(history_capture(&h,&p,sources,stamps)); }
    CHECK(h.count==HISTORY_LIMIT && h.cursor==HISTORY_LIMIT-1);
    history_clear(&h); CHECK(!h.count && !h.bytes);
    puts("Undo/redo branching, retained stereo PCM, shared buffers, derived fields, restoration and bounds passed."); return 0;
}
