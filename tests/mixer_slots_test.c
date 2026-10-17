// SPDX-License-Identifier: GPL-3.0-only
#include "../src/ui/frontend/ui_internal.h"
UiState ui;Theme ui_theme;
#include "../src/ui/frontend/ui_mixer.c"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void) {
    project_new(&ui.project);windows_init(&ui.windows,1448,1086);studio_workspace_init(&ui.windows,1448,1086,238);
    int saved_count=ui.project.insert_count;float saved_width=ui.windows.editors[3].rect.w;
    CHECK(mixer_strip_width()==51);
    ui.studio_style=1;
    for(int width=652;width<=1800;width+=37)for(int count=1;count<=INSERTS;count++) {
        ui.windows.editors[3].rect.w=width;ui.project.insert_count=count;
        MixerView view=mixer_view(width,360,count,0,0);
        CHECK(mixer_strip_width()==112 && view.viewport==width-162);
        CHECK(view.maximum==fmaxf(0,count*112-view.viewport));

    }
    ui.windows.editors[3].rect.w=saved_width;ui.project.insert_count=saved_count;ui.studio_style=0;
    int counts[INSERTS+1];studio_fx_slots(counts);CHECK(counts[1]==9);
    ui.project.midi_binding_count=1;ui.project.midi_bindings[0].target=(ParameterTarget){PARAM_CHANNEL_VOLUME,1,9};
    studio_fx_slots(counts);CHECK(counts[1]==9);
    ui.project.midi_bindings[0].target=(ParameterTarget){PARAM_EQ_FIRST,100,9};
    studio_fx_slots(counts);CHECK(counts[100]==10 && counts[1]==9);ui.project.midi_binding_count=0;
    mixer_effect_add(1,9,EFFECT_CHORUS);
    studio_fx_slots(counts);CHECK(counts[1]==10 && counts[0]==9);
    CHECK(ui.eq_bus==1 && ui.eq_slot==9 && ui.effect_slot==9 && ui.windows.workspace.inspector==4 && ui.windows.workspace.floating[5]);
    CHECK(ui.windows.editors[5].visible && !ui.windows.editors[4].visible);
    CHECK(ui.project.effect_type[1][9]==EFFECT_CHORUS && ui.project.effect_mix[1][9]==.5f);
    mixer_effect_open(1,9,1);CHECK(!ui.windows.editors[5].visible);
    mixer_effect_open(1,9,1);CHECK(ui.windows.editors[5].visible && ui.windows.editors[5].rect.h==280);
    ChorusSettings chorus=chorus_default();CHECK(!memcmp(&chorus,&ui.project.chorus[1][9],sizeof chorus));
    CHECK(automation_create(&ui.project,(ParameterTarget){PARAM_CHORUS_RATE,1,9},"target",16)>=0);
    mixer_effect_add(1,8,EFFECT_CHORUS);
    CHECK(automation_create(&ui.project,(ParameterTarget){PARAM_CHORUS_RATE,1,8},"other slot",16)>=0);
    CHECK(automation_create(&ui.project,(ParameterTarget){PARAM_CHANNEL_VOLUME,0,0},"channel",16)>=0);
    ui.project.effect_type[1][9]=EFFECT_EMPTY;studio_fx_slots(counts);CHECK(counts[1]==10);
    ui.project.effect_type[1][9]=EFFECT_CHORUS;
    mixer_effect_remove(1,9);
    studio_fx_slots(counts);CHECK(counts[1]==9);
    CHECK(ui.project.effect_type[1][9]==EFFECT_EMPTY && ui.project.automation_count==2);
    CHECK(ui.project.automations[0].target.slot==8);
    ui.project.effect_bypass[1][9]=1;mixer_effect_add(1,9,EFFECT_EQ);
    EQSettings eq=equalizer_default();CHECK(!memcmp(&eq,&ui.project.eq[1][9],sizeof eq));
    CHECK(ui.project.effect_mix[1][9]==1 && !ui.project.effect_bypass[1][9]);
    for(int id=5;id<WINDOW_CAPACITY;id++)studio_workspace_hide(&ui.windows,id);
    mixer_effect_add(1,1,EFFECT_CHORUS);mixer_effect_add(1,2,EFFECT_CHORUS);mixer_effect_add(1,3,EFFECT_EQ);
    int a=mixer_effect_window(1,1),b=mixer_effect_window(1,2),c=mixer_effect_window(1,3);
    CHECK(a>=5 && b>=5 && c>=5 && a!=b && a!=c && b!=c);
    CHECK(ui.effect_windows[a].slot==1 && ui.effect_windows[b].slot==2 && ui.effect_windows[c].slot==3);
    windows_focus(&ui.windows,a);CHECK(ui.windows.order[WINDOW_CAPACITY-1]==a);
    windows_focus(&ui.windows,1);CHECK(ui.windows.focused==1 && ui.windows.order[WINDOW_CAPACITY-1]==a);
    mixer_effect_open(1,2,1);CHECK(!ui.windows.editors[b].visible && ui.windows.editors[a].visible && ui.windows.editors[c].visible);
    mixer_effect_open(1,2,1);CHECK(mixer_effect_window(1,2)>=5);
    return 0;
}
