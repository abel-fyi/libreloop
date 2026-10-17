// SPDX-License-Identifier: GPL-3.0-only
#include "../src/ui/frontend/ui_internal.h"
#include "history.h"
UiState ui;static int checkpoints;static uint64_t stamps[CHANNELS];
int history_checkpoint(void){checkpoints++;return history_capture(&ui.edit_history,&ui.project,ui.originals,stamps);}
#include "../src/ui/frontend/ui_rack.c"
#include "../src/ui/frontend/ui_menus.c"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
static Project before,after,restored;static Sample sources[CHANNELS];
int main(void) {
    project_new(&ui.project);Note *original=note_add(&ui.project,0,0,0,60,0);original->velocity=73;
    begin_rack_fill(0);CHECK(ui.number_target==&ui.rack_fill_interval && ui.number_integer && ui.rack_fill_interval==4 && ui.pattern_popup==5);
    before=ui.project;
    CHECK(desktop_rack_fill(0,0,3)==5 && checkpoints==1);
    CHECK(original->velocity==73 && ui.project.pattern_steps[0]==before.pattern_steps[0]);
    for(int step=0;step<16;step++)CHECK(!!note_at(&ui.project,0,0,step,60)==(step%3==0));
    CHECK(desktop_rack_fill(0,0,3)==0 && checkpoints==1);
    after=ui.project;CHECK(history_capture(&ui.edit_history,&ui.project,ui.originals,stamps));
    CHECK(history_peek(&ui.edit_history,-1,&restored,sources));CHECK(!memcmp(&restored,&before,sizeof before));
    history_step(&ui.edit_history,-1);CHECK(history_peek(&ui.edit_history,1,&restored,sources));CHECK(!memcmp(&restored,&after,sizeof after));
    history_clear(&ui.edit_history);
    for(int spacing=1;spacing<=8;spacing++) {
        project_new(&ui.project);CHECK(rack_fill_steps(&ui.project,0,0,spacing,0)==(15/spacing+1));
        CHECK(rack_fill_steps(&ui.project,0,0,spacing,1)==15/spacing+1);
    }
    project_new(&ui.project);ui.project.pattern_steps[0]=256;
    for(int i=0;i<NOTES;i++)CHECK(note_add(&ui.project,0,0,i+.5f,60,0));
    before=ui.project;CHECK(desktop_rack_fill(0,0,1)==0);CHECK(!memcmp(&before,&ui.project,sizeof before));
    CHECK(rack_fill_steps(&ui.project,0,0,0,1)<0);
    return 0;
}
