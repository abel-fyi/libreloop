// SPDX-License-Identifier: GPL-3.0-only
/* Actual envelope capture/update with drawing disabled, at the overview size. */
#include "../src/ui/frontend/ui_internal.h"
#include "../src/ui/frontend/ui_state.c"
Theme ui_theme;
DesktopSkin ui_skin;
static int pressed,blue_triangles,grid_lines;
void host_draw_rectangle_rec(DrawRect r,DrawColor c){(void)r;(void)c;}
void host_draw_line_ex(DrawPoint a,DrawPoint b,float w,DrawColor c){(void)a;(void)b;(void)w;(void)c;grid_lines++;}
void smooth_line(DrawPoint a,DrawPoint b,float w,DrawColor c){(void)a;(void)b;(void)w;(void)c;}
void circle(float x,float y,float r,DrawColor c){(void)x;(void)y;(void)r;(void)c;}
void label(const char *s,float x,float y,int n,DrawColor c){(void)s;(void)x;(void)y;(void)n;(void)c;}
void request_cursor(int cursor){(void)cursor;}
int mouse_pressed(int button){return pressed && button==HOST_MOUSE_BUTTON_LEFT;}
int hover(float x,float y,float w,float h){return ui.input_enabled && ui.mouse.x>=x && ui.mouse.x<x+w && ui.mouse.y>=y && ui.mouse.y<y+h;}
void host_draw_triangle(DrawPoint a,DrawPoint b,DrawPoint c,DrawColor color){(void)a;(void)b;(void)c;if(color.r==ui_skin.graph_fill.r && color.g==ui_skin.graph_fill.g && color.b==ui_skin.graph_fill.b)blue_triangles++;}
#define ui_bevel(...) ((void)0)
#define ui_graph_dots(...) ((void)0)
static void fixture_graph_area(DrawPoint a,DrawPoint b,float base) {
    host_draw_triangle(a,(DrawPoint){a.x,base},(DrawPoint){b.x,base},ui_skin.graph_fill);
    host_draw_triangle(a,(DrawPoint){b.x,base},b,ui_skin.graph_fill);
}
#define ui_graph_area(draw,a,b,base) fixture_graph_area(a,b,base)
#define ui_graph_handle(...) ((void)0)
#include "../src/ui/frontend/ui_instruments.c"
#define CHECK(x) do {if(!(x)){fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x);return 1;}} while(0)
int main(void) {
    ui_skin.graph_fill=(DrawColor){0,92,139,255};
    project_new(&ui.project);
    FMSettings initial=fm_default();
    ui.fm_graph_channel=ui.instrument_channel=0;
    ui.fm_graph_area=(DrawRect){22,514,290,64};
    for(int tab=0;tab<2;tab++)for(int drag=0;drag<3;drag++) {
        ui.project.fm[0]=initial;ui.studio_style=0;ui.fm_tab=tab;ui.fm_graph_drag=drag;
        fm_graph_update_at(90+drag*80,540);
        FMSettings expected=ui.project.fm[0];
        CHECK(memcmp(&expected,&initial,sizeof expected));
        ui.project.fm[0]=initial;ui.studio_style=1;ui.fm_tab=1-tab;ui.fm_graph_tab=tab;
        fm_graph_update_at(90+drag*80,540);
        CHECK(!memcmp(&expected,&ui.project.fm[0],sizeof expected));
    }
    ui.project.fm[0]=initial;ui.studio_style=1;ui.fm_tab=0;ui.fm_graph_tab=-1;
    ui.fm_graph_drag=-1;ui.input_enabled=1;pressed=1;
    const ParameterDescriptor *info=parameter_descriptor(PARAM_FM_ATTACK);
    float slot=(ui.fm_graph_area.width-24)/3;
    float t=log1pf(*(float *)fm_parameter_pointer(&initial,PARAM_FM_ATTACK)*100)/log1pf(info->high*100);
    ui.mouse=(DrawPoint){ui.fm_graph_area.x+8+.9f*slot*t,ui.fm_graph_area.y+12};
    fm_envelope_graph(&ui.project.fm[0],ui.fm_graph_area);
    CHECK(ui.fm_graph_drag==0 && ui.fm_graph_tab==0 && !ui.input_enabled);
    ui.fm_tab=1;pressed=0;
    fm_graph_update_at(90,540);
    CHECK(*(float *)fm_parameter_pointer(&ui.project.fm[0],PARAM_FM_ATTACK)!=*(float *)fm_parameter_pointer(&initial,PARAM_FM_ATTACK));
    CHECK(*(float *)fm_parameter_pointer(&ui.project.fm[0],PARAM_FM_MOD_ATTACK)==*(float *)fm_parameter_pointer(&initial,PARAM_FM_MOD_ATTACK));
    /* Every six-op operator and its pitch envelope have their own renderer. */
    for(int base=0;base<=126;base+=21)for(int stage=0;stage<4;stage++) {
        ui.project.fm[0]=initial;
        for(int i=0;i<4;i++){ui.project.fm[0].dx7.value[base+i]=40+i*10;ui.project.fm[0].dx7.value[base+4+i]=90-i*20;}
        FMSettings saved=ui.project.fm[0];
        DrawRect plot={12,336,310,250};float slot=(plot.width-16)/4;
        float x=plot.x+8+slot*(stage+.1f+.8f*(1-saved.dx7.value[base+stage]/99));
        float y=plot.y+6+(plot.height-12)*(1-saved.dx7.value[base+4+stage]/99);
        ui.studio_style=1;ui.input_enabled=1;ui.fm_graph_drag=-1;pressed=1;ui.mouse=(DrawPoint){x,y};
        blue_triangles=grid_lines=0;
        dx7_envelope_graph(&ui.project.fm[0],base,plot);
        CHECK(blue_triangles==8 && grid_lines==9);
        CHECK(ui.fm_graph_drag==4+stage && ui.fm_dx_drag_base==base && !ui.input_enabled);
        CHECK(!memcmp(&saved,&ui.project.fm[0],sizeof saved));
        fm_graph_update_at(x+5,y+10);FMSettings actual=ui.project.fm[0];
        ui.project.fm[0]=saved;ui.studio_style=0;
        fm_graph_update_at(x+5,y+10);
        CHECK(!memcmp(&actual,&ui.project.fm[0],sizeof actual));
    }
    ui.studio_style=1;ui.windows.workspace.enabled=1;
    ui.windows.editors[4].rect=(Rect){1060,84,376,802};
    ui.fm_graph_area=(DrawRect){22,480,332,96};ui.fm_graph_drag=1;ui.fm_graph_tab=0;
    ui.project.fm[0]=initial;fm_graph_update_at(100,540);FMSettings translated=ui.project.fm[0];
    ui.project.fm[0]=initial;
    Rect content=windows_content_frame(&ui.windows,4);
    ui.mouse=(DrawPoint){content.x+100,content.y+540};fm_graph_update();
    CHECK(!memcmp(&translated,&ui.project.fm[0],sizeof translated));
    puts("Envelope capture retains its parameter group, original drag formulas and global/body mapping beneath larger captions.");
    return 0;
}
