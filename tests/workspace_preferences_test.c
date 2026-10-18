// SPDX-License-Identifier: GPL-3.0-only
#include "../src/ui/frontend/ui_internal.h"
#define LIBRELOOP_SAMPLES ""
#define LIBRELOOP_LOGO ""
#define LIBRELOOP_FONT ""
UiState ui;
#include "../src/ui/frontend/app_project.c"
#include "../src/ui/frontend/ui_shell.c"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void) {
    snprintf(ui.browser.config,sizeof ui.browser.config,"workspace-prefs/browser.txt");
    windows_init(&ui.windows,1448,1086);studio_workspace_init(&ui.windows,1448,1086,238);
    ui.ui_scale_preference=1.5f;CHECK(ui_scale_save());ui.ui_scale_preference=0;ui_scale_load();CHECK(ui.ui_scale_preference==1.5f);
    FILE *scale_file=fopen("workspace-prefs/ui-scale.txt","w");CHECK(scale_file);fputs("1 nan\n",scale_file);fclose(scale_file);
    ui_scale_load();CHECK(ui.ui_scale_preference==1.5f);
    scale_file=fopen("workspace-prefs/ui-scale.txt","w");CHECK(scale_file);fputs("1 9\n",scale_file);fclose(scale_file);
    ui_scale_load();CHECK(ui.ui_scale_preference==1.5f);
    ui.ui_scale_preference=0;CHECK(ui_scale_save());ui.ui_scale_preference=1;ui_scale_load();CHECK(ui.ui_scale_preference==0);
    remove("workspace-prefs/ui-scale.txt");
    ui.windows.workspace.editor_right=1;ui.windows.workspace.editor_width=.57f;ui.windows.workspace.mixer=.31f;ui.windows.workspace.piano=.4f;workspace_save();
    ui.windows.workspace.mixer=.2f;ui.windows.workspace.piano=.3f;workspace_load();
    CHECK(ui.windows.workspace.mixer==.31f && ui.windows.workspace.piano==.4f && ui.windows.workspace.editor_right && ui.windows.workspace.editor_width==.57f);
    CHECK(ui.windows.editors[4].rect.w==440 && ui.windows.editors[4].rect.h==420);
    ui.windows.editors[4].rect.w=376;ui.windows.editors[4].rect.h=802;workspace_save();workspace_load();
    CHECK(ui.windows.editors[4].rect.h==420);
    ui.windows.editors[4].rect.h=720;workspace_save();workspace_load();
    CHECK(ui.windows.editors[4].rect.h==420);
    ui.song=1;ui.reset=0;desktop_playback_page(0);
    CHECK(!ui.song && ui.reset && ui.windows.editors[2].visible && !ui.windows.editors[1].visible);
    ui.reset=0;desktop_playback_page(0);CHECK(!ui.reset);
    desktop_playback_page(1);
    CHECK(ui.song && ui.reset && ui.windows.editors[1].visible && !ui.windows.editors[2].visible);
    /* A valid old three-pane preference file migrates to exclusive pages. */
    FILE *old=fopen("workspace-prefs/workspace.txt","w");CHECK(old);
    fputs("2 .31 .4 168 334 4 1 .57\n",old);
    for(int i=0;i<WINDOW_CAPACITY;i++){Rect r=ui.windows.editors[i].rect;fprintf(old,"1 0 %g %g %g %g\n",r.x,r.y,r.w,r.h);}
    fclose(old);workspace_load();studio_workspace_layout(&ui.windows,1448,1086,238);
    CHECK(ui.windows.editors[1].visible && !ui.windows.editors[0].visible && !ui.windows.editors[2].visible && !ui.windows.editors[4].visible);
    CHECK(ui.windows.workspace.floating[4] && ui.windows.editors[1].rect.w==1200);
    CHECK(fabsf(ui.windows.workspace.mixer-(.31f-88.f/1002))<.00001f);
    /* Migrate the former stock split, while the earlier .31 custom size stays unchanged. */
    ui.windows.workspace.mixer=272.f/1002;workspace_save();ui.windows.workspace.mixer=.2f;workspace_load();
    CHECK(fabsf(ui.windows.workspace.mixer-studio_workspace_default_mixer(ui.windows.workspace.height))<.00001f);
    Windows expected=ui.windows;FILE *f=fopen("workspace-prefs/workspace.txt","w");CHECK(f);
    fputs("1 nan .5 168 334 4\n",f);fclose(f);workspace_load();CHECK(!memcmp(&expected,&ui.windows,sizeof expected));
    remove("workspace-prefs/workspace.txt");
    float aw=0,pw=0;ui.studio_style=1;ui.arrangement.zoom=1;
    for(int i=0;i<PATTERNS;i++)ui.piano_span[i]=64;
    workspace_view_scale(&aw,&pw);float old_a=aw*ui.arrangement.zoom,old_p=pw/ui.piano_span[0];
    studio_workspace_layout(&ui.windows,1600,1200,238);workspace_view_scale(&aw,&pw);
    CHECK(fabsf(aw*ui.arrangement.zoom-old_a)<.001f);
    for(int i=0;i<PATTERNS;i++)CHECK(fabsf(pw/ui.piano_span[i]-old_p)<.001f);
    float pan=ui.piano_pan[0]=11,span=ui.piano_span[0],zoom=ui.arrangement.zoom;
    ui.track_scroll=45;ui.note_selected[0][0][2]=1;
    for(int i=0;i<10;i++){windows_focus(&ui.windows,2);workspace_view_scale(&aw,&pw);windows_focus(&ui.windows,1);workspace_view_scale(&aw,&pw);}
    CHECK(ui.piano_pan[0]==pan && ui.piano_span[0]==span && ui.arrangement.zoom==zoom && ui.track_scroll==45 && ui.note_selected[0][0][2]);
    return 0;
}
