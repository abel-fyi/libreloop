// SPDX-License-Identifier: GPL-3.0-only
/* Inspect actual frontend digit/icon geometry; no window or event source. */
#include "../src/ui/frontend/ui_internal.h"
UiState ui;
Theme ui_theme;
DesktopSkin ui_skin;
static DrawRect rectangles[512],icon_source,icon_destination;
static int count;
void host_draw_rectangle_rec(DrawRect r,DrawColor c){(void)c;if(count<512)rectangles[count++]=r;}
void host_draw_texture_pro(HostTexture t,DrawRect source,DrawRect destination,DrawPoint origin,float angle,DrawColor color){(void)t;(void)origin;(void)angle;(void)color;icon_source=source;icon_destination=destination;}
static float test_scale=1;
float ui_scale(void){return test_scale;}
float text_pixel_position(float value,float origin,float layout_scale,float raster_scale){(void)origin;(void)layout_scale;(void)raster_scale;return value;}
int host_check_collision_point_rec(DrawPoint p,DrawRect r){return p.x>=r.x && p.x<r.x+r.width && p.y>=r.y && p.y<r.y+r.height;}
int host_is_mouse_button_down(int button){(void)button;return 0;}
#include "../src/ui/frontend/ui_widgets.c"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void) {
    ui.studio_style=1;
    bitmap_digits("00:00:",10,20,14,(DrawColor){255,255,255,255});
    int whole_count=count;count=0;
    lcd_time("00:00:00",10,20,80,(DrawColor){255,255,255,255});
    CHECK(count>whole_count);
    float large_top=100,small_top=100,large_bottom=0,small_bottom=0;
    for(int i=0;i<count;i++) {
        DrawRect r=rectangles[i];CHECK(r.x>=10 && r.x+r.width<=90 && r.y>=20 && r.y+r.height<=34.001f);
        if(i<whole_count){large_top=fminf(large_top,r.y);large_bottom=fmaxf(large_bottom,r.y+r.height);}
        else{small_top=fminf(small_top,r.y);small_bottom=fmaxf(small_bottom,r.y+r.height);}
    }
    CHECK(large_top==20 && small_top==23.5f && large_bottom==34 && small_bottom==34);
    count=0;lcd_time("123456:59:99",10,20,80,(DrawColor){255,255,255,255});
    for(int i=0;i<count;i++)CHECK(rectangles[i].x+rectangles[i].width<=90.001f);
    ui.font_scale=1;ui.icon_large=28;ui.icon_medium=20;ui.icon_small=16;
    for(int size=20;size<=42;size+=2) {
        icon_in_button(ICON_PLAY,(DrawRect){2,3,size,size},(DrawColor){255,255,255,255});
        CHECK(icon_source.width==16 && icon_source.height==16);
        CHECK(icon_destination.x>=4 && icon_destination.y>=5);
        CHECK(icon_destination.x+icon_destination.width<=size && icon_destination.y+icon_destination.height<=size+1);
        CHECK(icon_destination.width==fminf(32,size-4));
    }
    test_scale=.75f;count=0;lcd_number("120.00",10,20,100,14,(DrawColor){255,255,255,255});
    CHECK(count>0);for(int i=0;i<count;i++)CHECK(rectangles[i].width*test_scale>=.999f && rectangles[i].height*test_scale>=.999f);
    puts("Clock centiseconds are smaller and baseline-aligned; long times fit; icons fit button interiors.");return 0;
}
