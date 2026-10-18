// SPDX-License-Identifier: GPL-3.0-only
/* Exercise the frontend's actual routing hit test, including scrolled bus IDs. */
#include "../src/ui/frontend/ui_internal.h"
UiState ui;
Theme ui_theme;
int host_check_collision_point_rec(DrawPoint p,DrawRect r) {
    return p.x>=r.x && p.x<r.x+r.width && p.y>=r.y && p.y<r.y+r.height;
}
#include "../src/ui/frontend/ui_mixer.c"
#define CHECK(x) do {if(!(x)){fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x);return 1;}} while(0)
int main(void) {
    project_new(&ui.project);ui.project.insert_count=INSERTS; /* Exercise high scrolled bus IDs. */
    ui.windows.editors[3].visible=1;
    ui.studio_style=1;ui.mixer_scroll=37;
    ui.windows.editors[3].rect=(Rect){244,854,1200,208};
    ui.windows.workspace.enabled=1;
    ui.mixer_offset=37*112+0.5f;
    Rect frame=desktop_editor_rect(3);
    MixerView view=mixer_view(frame.w,frame.h,ui.project.insert_count,ui.mixer_offset,0);
    DrawPoint master={frame.x+55,frame.y+view.route+6};
    DrawPoint insert={frame.x+view.insert_x+12.5f,master.y};
    CHECK(mixer_input_at(master)==0);
    CHECK(mixer_input_at(insert)==38);
    CHECK(mixer_input_at((DrawPoint){insert.x+6*112,insert.y})==44);
    CHECK(mixer_input_at((DrawPoint){frame.x+view.insert_x-1,insert.y})==-1);
    CHECK(mixer_input_at((DrawPoint){insert.x+7,insert.y})==-1);
    CHECK(mixer_input_at((DrawPoint){insert.x,frame.y+view.route-1})==-1);
    CHECK(mixer_input_at((DrawPoint){insert.x,frame.y+view.route+12})==-1);
    CHECK(insert_connect(&ui.project,38,mixer_input_at((DrawPoint){insert.x+6*112,insert.y})));
    CHECK(ui.project.insert_output[37]==44);
    ui.windows.editors[3].visible=0;
    CHECK(mixer_input_at(insert)==-1);
    ui.windows.editors[3].visible=1;ui.windows.workspace.enabled=0;
    ui.studio_style=0;ui.mixer_scroll=11;
    ui.windows.editors[3].rect=(Rect){180,351,720,300};
    CHECK(mixer_input_at((DrawPoint){306,611})==12);
    CHECK(mixer_input_at((DrawPoint){714,611})==-1);
    puts("Classic and legacy routing target margins and scrolled bus IDs passed.");
    return 0;
}
