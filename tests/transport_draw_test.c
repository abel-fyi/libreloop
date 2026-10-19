// SPDX-License-Identifier: GPL-3.0-only
/* Inspect actual frontend transport icon geometry; no window or event source. */
#include "../src/ui/frontend/ui_internal.h"
UiState ui;
Theme ui_theme;
DesktopSkin ui_skin;
static DrawRect rectangles[512],icon_source,icon_destination;
static int count,held;
static DrawColor icon_color;
void host_draw_rectangle_rec(DrawRect r,DrawColor c){(void)c;if(count<512)rectangles[count++]=r;}
void host_draw_texture_pro(HostTexture t,DrawRect source,DrawRect destination,DrawPoint origin,float angle,DrawColor color){(void)t;(void)origin;(void)angle;(void)color;icon_source=source;icon_destination=destination;icon_color=color;}
static float test_scale=1;
float ui_scale(void){return test_scale;}
float text_pixel_position(float value,float origin,float layout_scale,float raster_scale){(void)origin;(void)layout_scale;(void)raster_scale;return value;}
int host_check_collision_point_rec(DrawPoint p,DrawRect r){return p.x>=r.x && p.x<r.x+r.width && p.y>=r.y && p.y<r.y+r.height;}
int host_is_mouse_button_down(int button){return button==HOST_MOUSE_BUTTON_LEFT && held;}
#include "../src/ui/frontend/ui_widgets.c"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void) {
    ui.studio_style=1;
    ui.font_scale=1;ui.icon_large=28;ui.icon_medium=20;ui.icon_small=16;
    for(int size=20;size<=42;size+=2) {
        icon_in_button(ICON_PLAY,(DrawRect){2,3,size,size},(DrawColor){255,255,255,255});
        CHECK(icon_source.width==16 && icon_source.height==16);
        CHECK(icon_destination.x>=4 && icon_destination.y>=5);
        CHECK(icon_destination.x+icon_destination.width<=size && icon_destination.y+icon_destination.height<=size+1);
        CHECK(icon_destination.width==fminf(32,size-4));
    }
    DrawRect bounds={2,3,40,40};DrawColor white={255,255,255,255};
    icon_button_state(ICON_PLAY,bounds,white,0,1);DrawRect idle=icon_destination;
    icon_button_state(ICON_PLAY,bounds,white,1,1);
    CHECK(icon_destination.x==idle.x+1 && icon_destination.y==idle.y+1 && icon_color.a==255);
    icon_button_state(ICON_PLAY,bounds,white,1,0);
    CHECK(icon_destination.x==idle.x && icon_destination.y==idle.y && icon_color.a==102);
    ui.input_enabled=1;ui.mouse=(DrawPoint){10,10};held=1;
    icon_button_state(ICON_PLAY,bounds,white,0,1);
    CHECK(icon_destination.x==idle.x+1 && icon_destination.y==idle.y+1);
    held=0;
    puts("Transport icons fit button interiors and preserve pressed/disabled states.");return 0;
}
