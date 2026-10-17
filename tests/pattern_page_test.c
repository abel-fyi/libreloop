// SPDX-License-Identifier: GPL-3.0-only
#include "../src/ui/frontend/ui_internal.h"
UiState ui;static int preview_count;
void audio_note(int c,Note n){(void)c;(void)n;preview_count++;}
#include "../src/ui/frontend/ui_piano.c"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
static Project before;
int main(void) {
    project_new(&ui.project);ui.project.channel_count=3;
    windows_init(&ui.windows,1448,1086);studio_workspace_init(&ui.windows,1448,1086,238);windows_focus(&ui.windows,2);
    ui.piano_span[0]=91;ui.piano_pan[0]=7;ui.piano_top=80;ui.note_selected[0][0][1]=1;
    pattern_select_channel(2);CHECK(ui.piano_channel==2 && ui.channel==2);
    CHECK(ui.piano_span[0]==91 && ui.piano_pan[0]==7 && ui.note_selected[0][0][1] && ui.piano_top==80);
    pattern_select_channel(-1);CHECK(ui.piano_channel==2);
    windows_focus(&ui.windows,4);pattern_select_channel(1);CHECK(ui.instrument_channel==1);
    Rect r=desktop_editor_rect(2);
    CHECK(pattern_channel_at(r.x+50,r.y+81)==0 && pattern_channel_at(r.x+50,r.y+109)==1);
    CHECK(pattern_channel_at(r.x+170,r.y+81)==-1 && pattern_channel_at(r.x+50,r.y+r.h-100)==-1);
    windows_focus(&ui.windows,1);CHECK(pattern_channel_at(r.x+50,r.y+81)==-1);windows_focus(&ui.windows,2);
    ui.arrangement.source_pattern=0;ui.pattern_hit_step=2;pattern_hit_update(5.5f);
    for(int step=2;step<=5;step++){Note *n=note_at(&ui.project,0,1,step,60);CHECK(n && !n->length);}
    CHECK(preview_count==1);before=ui.project;pattern_hit_update(5);CHECK(!memcmp(&before,&ui.project,sizeof before));
    ui.pattern_hit_step=5;pattern_hit_update(17);CHECK(ui.project.pattern_steps[0]==32 && ui.arrangement.source_steps==32 && note_at(&ui.project,0,1,17,60));
    CHECK(note_add(&ui.project,0,1,3,64,8));ui.pattern_hit_erase=1;ui.pattern_hit_step=2;pattern_hit_update(5);
    CHECK(!note_at(&ui.project,0,1,3,60) && note_at(&ui.project,0,1,3,64));
    CHECK(note_add(&ui.project,0,2,0,66,2));before=ui.project;
    PianoViewport view={484,580,32,0,1,72};PianoPoint point={64,190};
    CHECK(!piano_pointer_note(&ui.project,0,1,&view,point)); /* Other channels are reference only. */
    Note stroke[NOTES];uint8_t selected[NOTES]={0},old_selected[NOTES]={0};
    PianoGesture gesture={.pattern=0,.channel=1,.before=stroke,.selected=selected,.selection_before=old_selected};
    CHECK(piano_press(&gesture,&ui.project,&view,(PianoPress){.pointer=point,.enabled=1,.right=1},NULL,NULL));
    CHECK(!memcmp(&before,&ui.project,sizeof before));
    project_new(&ui.project);ui.piano_channel=0;ui.pattern_hit_erase=0;ui.pattern_hit_step=0;before=ui.project;
    uint64_t stamps[CHANNELS]={0};Sample sources[CHANNELS];static Project restored;
    CHECK(history_capture(&ui.edit_history,&ui.project,ui.originals,stamps));
    pattern_hit_update(3);CHECK(history_capture(&ui.edit_history,&ui.project,ui.originals,stamps));
    CHECK(history_peek(&ui.edit_history,-1,&restored,sources) && !memcmp(&before,&restored,sizeof before));
    history_clear(&ui.edit_history);
    project_new(&ui.project);ui.project.pattern_steps[0]=256;for(int i=0;i<NOTES;i++)CHECK(note_add(&ui.project,0,0,i*2,70,2));before=ui.project;
    CHECK(pattern_hits_paint(&ui.project,0,0,0,8,0)<0 && !memcmp(&before,&ui.project,sizeof before));
    CHECK(pattern_hits_paint(&ui.project,0,0,NAN,0,0)<0 && pattern_hits_paint(&ui.project,0,0,0,100000,0)<0);
    puts("Pattern channel selection, drop hit areas and C4 hit strokes preserve existing notes and views.");return 0;
}
