// SPDX-License-Identifier: GPL-3.0-only
#include "edit_clipboard.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed %d: %s\n",__LINE__,#x); return 1; } } while(0)
static Project p,before;
static EditClipboard c;
static uint8_t selected[NOTES],clips[LANES][CLIPS];
int main(void) {
    project_new(&p); p.notes[0][0][0]=(Note){60,70,2,3}; p.notes[0][0][1]=(Note){64,110,4,1}; selected[0]=selected[1]=1;
    CHECK(clipboard_copy_notes(&c,&p,0,0,selected)==2);
    CHECK(clipboard_paste_notes(&c,&p,0,0,8,selected)==2);
    CHECK(p.notes[0][0][2].start==8 && p.notes[0][0][2].length==3 && p.notes[0][0][2].velocity==70);
    CHECK(p.notes[0][0][3].start==10 && p.notes[0][0][3].pitch==64 && selected[2] && selected[3] && !selected[0]);
    CHECK(clipboard_paste_notes(&c,&p,0,0,8,selected)==2);
    CHECK(p.notes[0][0][4].start==8 && p.notes[0][0][4].pitch==60 && selected[4]);
    CHECK(p.notes[0][0][2].velocity==70 && !selected[2]);
    CHECK(clipboard_paste_notes(&c,&p,0,0,40,selected)==2 && p.pattern_steps[0]>=43);
    for(int i=0;i<NOTES;i++) p.notes[0][0][i].velocity=100;
    before=p; CHECK(clipboard_paste_notes(&c,&p,0,0,80,selected)==-2 && !memcmp(&before,&p,sizeof p));
    project_new(&p); p.channel_count=2; p.channel_audio[1]=1; p.audio_seconds[1]=3;
    p.clips[2][0]=1; p.clip_starts[2][0]=1; p.clip_steps[2][0]=4; p.clip_offsets[2][0]=2;
    p.clips[3][0]=PATTERNS+2; p.clip_starts[3][0]=2; p.clip_steps[3][0]=.5f; p.clip_offsets[3][0]=.2f;
    clips[2][0]=clips[3][0]=1; CHECK(clipboard_copy_clips(&c,&p,(const uint8_t (*)[CLIPS])clips)==2);
    CHECK(clipboard_paste_clips(&c,&p,5,4,clips)==2);
    CHECK(p.clips[5][0]==1 && p.clip_starts[5][0]==4 && p.clip_steps[5][0]==4 && p.clip_offsets[5][0]==2);
    CHECK(p.clips[6][0]==PATTERNS+2 && p.clip_starts[6][0]==5 && p.clip_steps[6][0]==.5f && p.clip_offsets[6][0]==.2f && clips[5][0] && clips[6][0]);
    before=p; CHECK(clipboard_paste_clips(&c,&p,5,4,clips)==-3 && !memcmp(&before,&p,sizeof p));
    CHECK(clipboard_paste_clips(&c,&p,LANES-1,8,clips)==-4 && !memcmp(&before,&p,sizeof p));
    p.channel_count=1; CHECK(clipboard_paste_clips(&c,&p,8,8,clips)==-1);
    puts("Clipboard timing, velocity, trim/offset preservation, selection and atomic failure checks passed."); return 0;
}
