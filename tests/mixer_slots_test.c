// SPDX-License-Identifier: GPL-3.0-only
#include "../src/ui/frontend/ui_internal.h"
UiState ui;Theme ui_theme;
static int held=1,cancel;
float ui_scale(void){return 1;}
int host_is_key_pressed(int key){return cancel && key==HOST_KEY_ESCAPE;}
int host_is_mouse_button_pressed(int button){(void)button;return 0;}
int host_is_mouse_button_down(int button){return button==HOST_MOUSE_BUTTON_LEFT && held;}

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
        CHECK(mixer_strip_width()==112 && view.viewport==width-204);
        CHECK(view.maximum==fmaxf(0,count*112-view.viewport));

    }
    ui.windows.editors[3].rect.w=saved_width;ui.project.insert_count=saved_count;ui.studio_style=0;
    EffectChainList chain=effect_chain_list(&ui.project,1);CHECK(chain.count==0 && chain.add_slot==0);
    for(int slot=0;slot<STUDIO_FX_SLOTS;slot++) {
        ui.project.effect_type[2][slot]=EFFECT_CHORUS;chain=effect_chain_list(&ui.project,2);
        CHECK(chain.count==slot+1 && chain.add_slot==(slot<8?slot+1:-1));
    }
    ui.project.effect_type[2][3]=EFFECT_EMPTY;chain=effect_chain_list(&ui.project,2);CHECK(chain.count==8 && chain.add_slot==3);
    memset(ui.project.effect_type[2],0,sizeof ui.project.effect_type[2]);
    mixer_effect_add(1,9,EFFECT_CHORUS);
    chain=effect_chain_list(&ui.project,1);CHECK(chain.count==1 && chain.slots[0]==9 && chain.add_slot==0);
    CHECK(ui.eq_bus==1 && ui.eq_slot==9 && ui.effect_slot==9 && ui.windows.workspace.inspector==4 && ui.windows.workspace.floating[5]);
    CHECK(ui.windows.editors[5].visible && !ui.windows.editors[4].visible);
    CHECK(ui.project.effect_type[1][9]==EFFECT_CHORUS && ui.project.effect_mix[1][9]==.5f);
    mixer_effect_open(1,9,1);CHECK(!ui.windows.editors[5].visible);
    mixer_effect_open(1,9,1);CHECK(ui.windows.editors[5].visible && ui.windows.editors[5].rect.h==164);
    ChorusSettings chorus=chorus_default();CHECK(!memcmp(&chorus,&ui.project.chorus[1][9],sizeof chorus));
    CHECK(automation_create(&ui.project,(ParameterTarget){PARAM_CHORUS_RATE,1,9},"target",16)>=0);
    mixer_effect_add(1,8,EFFECT_CHORUS);
    CHECK(automation_create(&ui.project,(ParameterTarget){PARAM_CHORUS_RATE,1,8},"other slot",16)>=0);
    CHECK(automation_create(&ui.project,(ParameterTarget){PARAM_CHANNEL_VOLUME,0,0},"channel",16)>=0);
    ui.project.effect_type[1][9]=EFFECT_EMPTY;chain=effect_chain_list(&ui.project,1);CHECK(chain.count==1 && chain.slots[0]==8);
    ui.project.effect_type[1][9]=EFFECT_CHORUS;
    mixer_effect_remove(1,9);
    chain=effect_chain_list(&ui.project,1);CHECK(chain.count==1 && chain.slots[0]==8);
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
    ui.studio_style=1;ui.windows.workspace.mixer_fx_rows=3;
    Rect root=desktop_editor_rect(3);MixerView view=desktop_mixer_view(root.w,root.h);
    ui.midi_learn_target=(ParameterTarget){PARAM_CHORUS_RATE,1,1};ui.automation_target=(ParameterTarget){PARAM_EFFECT_MIX,1,3};
    ui.fx_press=1;ui.fx_slot=1;ui.fx_bus=1;ui.fx_press_point=(DrawPoint){root.x+view.insert_x+50,root.y+view.fx_top+33};
    ui.mouse=(DrawPoint){ui.fx_press_point.x,root.y+view.fx_top+77};
    desktop_fx_drag_update();CHECK(ui.fx_dragging && ui.fx_drop==3 && ui.project.effect_type[1][3]==EFFECT_EQ);
    held=0;desktop_fx_drag_update();CHECK(!ui.fx_press && ui.project.effect_type[1][2]==EFFECT_EQ && ui.project.effect_type[1][3]==EFFECT_CHORUS);
    CHECK(ui.midi_learn_target.slot==3 && ui.automation_target.slot==2);
    CHECK(ui.effect_windows[a].slot==3 && ui.effect_windows[b].slot==1 && ui.effect_windows[c].slot==2);
    ui.fx_press=1;ui.fx_dragging=1;cancel=1;desktop_fx_drag_update();CHECK(!ui.fx_press && ui.project.effect_type[1][2]==EFFECT_EQ);
    mixer_effect_add(3,0,EFFECT_CHORUS);int original=mixer_effect_window(3,0);CHECK(original>=5);
    ui.eq_bus=3;ui.eq_slot=1;ui.fx_add_pending=1;ui.mixer_fx_scroll[3]=44;ui.midi_learn_target=(ParameterTarget){PARAM_CHORUS_RATE,3,0};
    mixer_effect_add(3,1,EFFECT_EQ);
    CHECK(!ui.fx_add_pending && ui.mixer_fx_scroll[3]==0 && ui.project.effect_type[3][0]==EFFECT_EQ && ui.project.effect_type[3][1]==EFFECT_CHORUS);
    CHECK(ui.effect_windows[original].slot==1 && ui.midi_learn_target.slot==1 && mixer_effect_window(3,0)>=5);
    mixer_effect_add(4,0,EFFECT_DELAY);int delay_window=mixer_effect_window(4,0);
    CHECK(delay_window>=5 && ui.project.effect_type[4][0]==EFFECT_DELAY && delay_valid(ui.project.delay[4][0]));
    CHECK(ui.windows.editors[delay_window].minw==620 && ui.windows.editors[delay_window].minh==184 && ui.project.effect_mix[4][0]==1);
    int delay_automation=automation_create(&ui.project,(ParameterTarget){PARAM_DELAY_FEEDBACK,4,0},"Delay feedback",16);CHECK(delay_automation>=0);
    int before_remove=ui.project.automation_count;mixer_effect_remove(4,0);
    CHECK(!ui.windows.editors[delay_window].visible && !ui.project.effect_type[4][0] && ui.project.automation_count==before_remove-1);
    mixer_effect_add(4,0,EFFECT_TUBE);int tube_window=mixer_effect_window(4,0);
    CHECK(tube_window>=5 && tube_valid(ui.project.tube[4][0]) && ui.project.tube[4][0].natural==0 && ui.project.effect_mix[4][0]==1);
    CHECK(ui.windows.editors[tube_window].minw==440 && ui.windows.editors[tube_window].minh==176);
    int tube_automation=automation_create(&ui.project,(ParameterTarget){PARAM_TUBE_DRIVE,4,0},"Tube drive",16);CHECK(tube_automation>=0);
    before_remove=ui.project.automation_count;mixer_effect_remove(4,0);
    CHECK(!ui.windows.editors[tube_window].visible && !ui.project.effect_type[4][0] && ui.project.automation_count==before_remove-1);
    mixer_effect_add(4,0,EFFECT_REVERB);int reverb_window=mixer_effect_window(4,0);
    CHECK(reverb_window>=5 && reverb_valid(ui.project.reverb[4][0]) && ui.project.effect_mix[4][0]==.25f);
    CHECK(ui.windows.editors[reverb_window].minw==640 && ui.windows.editors[reverb_window].minh==176);
    int reverb_automation=automation_create(&ui.project,(ParameterTarget){PARAM_REVERB_SIZE,4,0},"Reverb size",16);CHECK(reverb_automation>=0);
    before_remove=ui.project.automation_count;mixer_effect_remove(4,0);
    CHECK(!ui.windows.editors[reverb_window].visible && !ui.project.effect_type[4][0] && ui.project.automation_count==before_remove-1);
    for(int effect=0;effect<2;effect++){
        int type=effect?EFFECT_FLANGER:EFFECT_PHASER;unsigned parameter=effect?PARAM_FLANGER_FEEDBACK:PARAM_PHASER_FEEDBACK;
        mixer_effect_add(4,0,type);int window=mixer_effect_window(4,0);
        CHECK(window>=5 && ui.project.effect_type[4][0]==type && ui.project.effect_mix[4][0]==.5f);
        CHECK(ui.windows.editors[window].minw==600 && ui.windows.editors[window].minh==176);
        CHECK(effect?flanger_valid(ui.project.flanger[4][0]):phaser_valid(ui.project.phaser[4][0]));
        CHECK(automation_create(&ui.project,(ParameterTarget){parameter,4,0},"Modulation feedback",16)>=0);
        before_remove=ui.project.automation_count;mixer_effect_remove(4,0);
        CHECK(!ui.windows.editors[window].visible && !ui.project.effect_type[4][0] && ui.project.automation_count==before_remove-1);
    }
    mixer_effect_add(4,0,EFFECT_TUBE);
    CHECK(automation_create(&ui.project,(ParameterTarget){PARAM_TUBE_DRIVE,4,0},"Old Tube drive",16)>=0);
    CHECK(midi_bind(&ui.project,0,7,(ParameterTarget){PARAM_EFFECT_MIX,4,0}));
    CHECK(midi_bind(&ui.project,0,8,(ParameterTarget){PARAM_CHANNEL_VOLUME,0,0}));
    int old_bindings=ui.project.midi_binding_count;
    ui.midi_learning=1;ui.midi_learn_target=(ParameterTarget){PARAM_TUBE_DRIVE,4,0};
    int old_count=ui.project.automation_count;
    ui.project.effect_bypass[4][0]=1;ui.fx_add_pending=1;ui.eq_bus=4;ui.eq_slot=0;
    mixer_effect_replace(4,0,EFFECT_REVERB);
    CHECK(ui.project.effect_type[4][0]==EFFECT_REVERB && !ui.project.effect_bypass[4][0]);
    CHECK(ui.project.automation_count==old_count-1 && !ui.fx_add_pending);
    CHECK(ui.project.midi_binding_count==old_bindings-1 && !ui.midi_learning);
    CHECK(reverb_valid(ui.project.reverb[4][0]) && ui.project.effect_mix[4][0]==.25f);
    int replacement=mixer_effect_window(4,0);CHECK(replacement>=5 && ui.windows.editors[replacement].visible);
    CHECK(ui.project.effect_type[3][0]==EFFECT_EQ && ui.project.effect_type[3][1]==EFFECT_CHORUS);
    mixer_effect_replace(4,0,999);CHECK(ui.project.effect_type[4][0]==EFFECT_REVERB);
    int old_inserts=ui.project.insert_count;
    mixer_add_inserts(2);CHECK(ui.project.insert_count==old_inserts+2 && ui.mixer_selected==old_inserts+1);
    CHECK(ui.mixer_offset>=0 && ui.project.insert_volume[old_inserts]==1);
    mixer_add_inserts(INSERTS);CHECK(ui.project.insert_count==old_inserts+2);
    return 0;
}
