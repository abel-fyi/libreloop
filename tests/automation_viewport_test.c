// SPDX-License-Identifier: GPL-3.0-only
#include "../src/ui/frontend/ui_internal.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
UiState ui;
static int focused=-1;
void windows_focus(Windows *windows,int id){(void)windows;focused=id;}
#include "../src/ui/frontend/ui_menus.c"
int main(void) {
    project_new(&ui.project);
    ui.automation_target=(ParameterTarget){PARAM_CHANNEL_VOLUME,0,0};
    strcpy(ui.menu_name,"Channel volume");ui.arrangement.view_start=7.25f;ui.track_scroll=333.5f;
    ui.playlist_start=16*19;ui.browser_focus=1;
    /* Occupied lanes force creation well below the viewport. */
    for(int lane=0;lane<12;lane++)assert(arrangement_place(&ui.project,lane,19,1,ui.project.pattern_steps[0])>=0);
    create_automation();
    assert(ui.project.automation_count==1 && ui.automation_selected==0 && ui.picker_tab==2);
    assert(ui.arrangement.view_start==7.25f && ui.track_scroll==333.5f);
    assert(focused==1 && !ui.browser_focus);
    int slot=arrangement_hit(&ui.project,12,19);
    assert(slot>=0 && ui.project.clips[12][slot]==AUTOMATION_SOURCE+1 && ui.arrangement.selected[12][slot]);
    assert(ui.project.clip_starts[12][slot]==19);
    return 0;
}
