// SPDX-License-Identifier: GPL-3.0-only
#include "../ui/frontend/host.h"
#include "files.h"
#include "browser_files.h"
#include "render.h"
#include "shapes.h"
#include "utf8.h"
#include "text_legacy.h"
#include "frame_wait.h"
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#ifdef _WIN32
#include "windows_utf.h"
#include "browser_files.h"
#define STBI_WINDOWS_UTF8
#define STBIW_WINDOWS_UTF8
#else
#include <sys/stat.h>
#include <unistd.h>
#endif
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
static PlatformWindow *window;
static DrawContext *draw;
static TextFonts *fonts;
static DrawTexture *target;
static WindowDrops drops;
static char *clipboard;
static double previous,update,frame,target_time=1.0/60;
static int waiting;
static float camera_zoom=1;
static DrawTexture *mesh_texture;
static DrawColor mesh_color={255,255,255,255};
static float mesh_u,mesh_v;
static int mesh_mode,mesh_count;
static DrawVertex mesh[4];
DrawContext *host_draw(void){return draw;}
PlatformWindow *host_window(void){return window;}
AppInput *host_input(void){return platform_window_input(window);}
TextFonts *host_fonts(void){return fonts;}
void host_fonts_clear(void){fonts=NULL;}
void host_init_window(int width,int height,const char *title){
    window=platform_window_create(title,0);
    if(!window){fprintf(stderr,"Cannot create LibreLoop window: %s\n",platform_window_error());exit(1);}
    if(width!=1200 || height!=675)platform_window_set_size(window,width,height);
    draw=platform_window_draw(window);fonts=text_fonts_create(draw);previous=host_get_time();
}
void host_close_window(void){platform_window_drops_free(drops);free(clipboard);platform_window_destroy(window);window=NULL;draw=NULL;}
void host_set_config_flags(unsigned flags){(void)flags;}
void host_set_window_min_size(int w,int h){platform_window_set_min_size(window,w,h);}
void host_set_exit_key(int key){(void)key;/* Original disables the implicit escape exit. */}
void host_set_target_f_p_s(int fps){target_time=fps<1?0:1.0/fps;}
void host_set_window_icon(HostImage image){platform_window_icon(window,image.data,image.width,image.height);}
void host_begin_drawing(void){double now=host_get_time();update=now-previous;previous=now;host_begin_mode2_d((HostCamera){.zoom=1});}
void host_end_drawing(void){
    platform_window_present(window);double now=host_get_time();frame=update+now-previous;previous=now;
    if(frame<target_time){platform_frame_wait(target_time-frame);
        now=host_get_time();frame+=now-previous;previous=now;}
    platform_window_events(window,waiting?-1:0);
    WindowDrops next=platform_window_take_drops(window);if(next.count){platform_window_drops_free(drops);drops=next;}
}
void host_enable_event_waiting(void){waiting=1;}
void host_disable_event_waiting(void){waiting=0;}
int host_window_should_close(void){return platform_window_take_close_request(window);}
int host_get_screen_width(void){return platform_window_geometry(window).width;}
int host_get_screen_height(void){return platform_window_geometry(window).height;}
int host_is_window_focused(void){return host_input()->focused;}
int host_is_window_resized(void){return platform_window_resized(window);}
void host_framebuffer_size(PlatformWindow *w,int *x,int *y){WindowGeometry g=platform_window_geometry(w);*x=g.pixel_width;*y=g.pixel_height;}
void host_window_size(PlatformWindow *w,int *x,int *y){WindowGeometry g=platform_window_geometry(w);*x=g.width;*y=g.height;}
void host_wake(void){if(window)platform_window_wake(window);}
double host_get_time(void){return platform_window_time();}
float host_get_frame_time(void){return (float)frame;}
DrawPoint host_get_mouse_position(void){InputPoint p=input_pointer(host_input());return (DrawPoint){p.x,p.y};}
DrawPoint host_get_mouse_delta(void){InputPoint p=input_delta(host_input());
#ifdef _WIN32
    p.x*=host_input()->scale.x;p.y*=host_input()->scale.y;
#endif
    return (DrawPoint){p.x,p.y};}
DrawPoint host_get_mouse_wheel_move_v(void){NavigationInput s=host_input()->scroll.wheel;return (DrawPoint){s.x,s.y};}
float host_get_mouse_wheel_move(void){return scroll_input_scalar(&host_input()->scroll);}
NavigationInput host_navigation(void){return host_input()->scroll.native;}
int host_is_key_down(int k){return input_key_down(host_input(),k);}
int host_is_key_pressed(int k){return input_key_pressed(host_input(),k);}
int host_is_key_pressed_repeat(int k){return input_key_repeat(host_input(),k);}
int host_is_key_released(int k){return input_key_released(host_input(),k);}
int host_get_char_pressed(void){return input_next_char(host_input());}
int host_is_mouse_button_down(int b){return input_button_down(host_input(),b);}
int host_is_mouse_button_pressed(int b){return input_click(host_input(),b);}
int host_is_mouse_button_released(int b){return input_button_released(host_input(),b);}
void host_set_mouse_cursor(int c){platform_window_cursor(window,c);}
const char *host_get_clipboard_text(void){free(clipboard);clipboard=platform_clipboard_read();return clipboard;}
int host_is_file_dropped(void){return drops.count>0;}
WindowDrops host_load_dropped_files(void){WindowDrops result=drops;drops=(WindowDrops){0};return result;}
void host_unload_dropped_files(WindowDrops d){platform_window_drops_free(d);}
void host_begin_mode2_d(HostCamera c){camera_zoom=c.zoom;WindowGeometry g=platform_window_geometry(window);float density=target?1:g.width>0?g.pixel_width/(float)g.width:1;draw_pattern_density(draw,density);draw_transform(draw,c.zoom*density,(c.offset.x-c.target.x*c.zoom)*density,(c.offset.y-c.target.y*c.zoom)*density);}
void host_end_mode2_d(void){host_begin_mode2_d((HostCamera){.zoom=1});}
void host_begin_scissor_mode(int x,int y,int w,int h){DrawRect r=ui_render_scissor(platform_window_geometry(window),x,y,w,h);draw_clip(draw,&r);}
void host_end_scissor_mode(void){draw_clip(draw,NULL);}
void host_clear_background(DrawColor c){draw_clear(draw,c);}
void host_draw_rectangle(int x,int y,int w,int h,DrawColor c){draw_rect(draw,(DrawRect){x,y,w,h},c);}
void host_draw_rectangle_rec(DrawRect r,DrawColor c){draw_rect(draw,r,c);}
void host_draw_rectangle_lines_ex(DrawRect r,float width,DrawColor c){draw_border(draw,r,width,c);}
void host_draw_line(int x,int y,int xx,int yy,DrawColor c){draw_hairline(draw,(DrawPoint){x,y},(DrawPoint){xx,yy},c);}
void host_draw_line_ex(DrawPoint a,DrawPoint b,float width,DrawColor c){draw_line(draw,a,b,width,c);}
void host_draw_rectangle_lines(int x,int y,int w,int h,DrawColor c){float z=.5f/camera_zoom;draw_hairline(draw,(DrawPoint){x-z,y},(DrawPoint){x+w+z,y},c);draw_hairline(draw,(DrawPoint){x+w,y-z},(DrawPoint){x+w,y+h+z},c);draw_hairline(draw,(DrawPoint){x+w+z,y+h},(DrawPoint){x-z,y+h},c);draw_hairline(draw,(DrawPoint){x,y+h+z},(DrawPoint){x,y-z},c);}
void host_draw_triangle(DrawPoint a,DrawPoint b,DrawPoint c,DrawColor color){DrawVertex v[]={{a,0,0,color},{b,0,0,color},{c,0,0,color}};draw_triangles(draw,NULL,v,3);}
void host_draw_circle(int x,int y,float radius,DrawColor c){shape_circle(draw,x,y,radius,c);}
void host_draw_circle_lines(int x,int y,float radius,DrawColor c){shape_circle_lines(draw,x,y,radius,c);}
void host_draw_text_ex(DrawFont *f,const char *t,DrawPoint p,float size,float spacing,DrawColor c){draw_text(draw,f,t,p,size,spacing,c);}
void host_draw_texture_pro(HostTexture t,DrawRect s,DrawRect d,DrawPoint origin,float rotation,DrawColor c){draw_texture_pro(draw,t.id,s,d,origin,rotation,c);}
DrawColor host_fade(DrawColor c,float a){return draw_color_fade(c,a);}
DrawColor host_color_alpha_blend(DrawColor d,DrawColor s,DrawColor t){return draw_color_blend(d,s,t);}
DrawColor host_color_from_h_s_v(float h,float s,float v){return draw_color_from_hsv(h,s,v);}
DrawHSV host_color_to_h_s_v(DrawColor c){return draw_color_hsv(c);}
DrawPoint host_get_spline_point_bezier_cubic(DrawPoint startPos,DrawPoint startControlPos,DrawPoint endControlPos,DrawPoint endPos,float t) {
    float a=powf(1.0f-t,3),b=3.0f*powf(1.0f-t,2)*t;
    float c=3.0f*(1.0f-t)*powf(t,2),d=powf(t,3);
    DrawPoint point;
    point.y=a*startPos.y+b*startControlPos.y+c*endControlPos.y+d*endPos.y;
    point.x=a*startPos.x+b*startControlPos.x+c*endControlPos.x+d*endPos.x;
    return point;
}
int host_check_collision_point_rec(DrawPoint p,DrawRect r){return p.x>=r.x && p.x<r.x+r.width && p.y>=r.y && p.y<r.y+r.height;}
HostImage host_gen_image_color(int w,int h,DrawColor c){HostImage i={calloc((size_t)w*h,4),w,h};if(i.data)for(int n=0;n<w*h;n++)((DrawColor*)i.data)[n]=c;return i;}
HostImage host_load_image(const char *p){HostImage i={0};int channels;i.data=stbi_load(p,&i.width,&i.height,&channels,4);return i;}
HostImage host_load_image_from_texture(HostTexture t){HostImage i=host_gen_image_color(t.width,t.height,BLANK);DrawTexture *old=target;draw_target(draw,t.id);if(!draw_read_rgba(draw,i.data,t.width,t.height)){free(i.data);i.data=NULL;}draw_target(draw,old);return i;}
void host_image_flip_vertical(HostImage *i){if(!i->data)return;unsigned char *p=i->data;for(int y=0;y<i->height/2;y++)for(int x=0;x<i->width*4;x++){unsigned char a=p[y*i->width*4+x];p[y*i->width*4+x]=p[(i->height-1-y)*i->width*4+x];p[(i->height-1-y)*i->width*4+x]=a;}}
void host_unload_image(HostImage i){free(i.data);}
HostTexture host_load_texture_from_image(HostImage i){return (HostTexture){draw_texture_create(draw,i.width,i.height,i.data,0),i.width,i.height};}
void host_unload_texture(HostTexture t){draw_texture_destroy(t.id);}
void host_set_texture_filter(HostTexture t,int filter){draw_texture_filter(t.id,filter==HOST_TEXTURE_FILTER_BILINEAR);}
void host_set_texture_wrap(HostTexture t,int wrap){(void)t;(void)wrap;/* SDL texture sampling clamps. */}
HostTarget host_load_render_texture(int w,int h){DrawTexture *t=draw_texture_create(draw,w,h,NULL,1);return (HostTarget){t,{t,w,h}};}
void host_unload_render_texture(HostTarget t){draw_texture_destroy(t.id);}
void host_begin_texture_mode(HostTarget t){target=t.id;draw_target(draw,target);draw_pattern_density(draw,1);draw_transform(draw,1,0,0);}
void host_end_texture_mode(void){target=NULL;draw_target(draw,NULL);draw_transform(draw,1,0,0);}
void host_take_screenshot(const char *path){WindowGeometry g=platform_window_geometry(window);void *p=malloc((size_t)g.pixel_width*g.pixel_height*4);if(p && draw_read_rgba(draw,p,g.pixel_width,g.pixel_height))stbi_write_png(path,g.pixel_width,g.pixel_height,4,p,g.pixel_width*4);free(p);}
void host_mesh_settexture(DrawTexture *t){mesh_texture=t;}
void host_mesh_begin(int mode){mesh_mode=mode;mesh_count=0;}
void host_mesh_color4ub(unsigned char r,unsigned char g,unsigned char b,unsigned char a){mesh_color=(DrawColor){r,g,b,a};}
void host_mesh_texcoord2f(float u,float v){mesh_u=u;mesh_v=v;}
void host_mesh_vertex2f(float x,float y){mesh[mesh_count++]=(DrawVertex){{x,y},mesh_u,mesh_v,mesh_color};int count=mesh_mode==HOST_RL_QUADS?4:3;if(mesh_count==count){if(count==4){DrawVertex v[]={mesh[0],mesh[1],mesh[2],mesh[0],mesh[2],mesh[3]};draw_triangles(draw,mesh_texture,v,6);}else draw_triangles(draw,mesh_texture,mesh,3);mesh_count=0;}}
void host_mesh_end(void){draw_flush(draw);}
int host_path_absolute(const char *path){return platform_absolute_path(path);}
int host_directory_exists(const char *p){return platform_browser_directory(p);}
int host_file_exists(const char *p){return platform_path_exists(p);}
int host_is_file_extension(const char *p,const char *extensions){return legacy_file_extension(p,extensions);}
const char *host_get_working_directory(void){static char path[LL_PATH_CAPACITY];platform_working_directory(path,sizeof path);return path;}
#ifdef _WIN32
int host_make_directory(const char *path){return platform_browser_make_directory(path);}
int host_change_directory(const char *path){wchar_t *wide=windows_utf16(path);if(!wide)return 0;int ok=SetCurrentDirectoryW(wide)!=0;free(wide);return ok;}
#else
int host_make_directory(const char *path){return platform_browser_make_directory(path);}
int host_change_directory(const char *p){return chdir(p)==0;}
#endif
const char *host_get_file_name(const char *p){return legacy_file_name(p);}
const char *host_get_file_name_without_ext(const char *p){return legacy_file_stem(p);}
const char *host_get_application_directory(void){const char *p=SDL_GetBasePath();return p?p:"";}
char *host_realpath(const char *p,char *out){return platform_canonical_path(p,out,LL_PATH_CAPACITY)?out:NULL;}
int host_thread_create(PlatformThread **t,const void *attributes,void *(*routine)(void *),void *context){(void)attributes;*t=platform_thread_start("LibreLoop sampler",routine,context);return *t?0:1;}
int host_thread_join(PlatformThread *t,void **result){(void)result;platform_thread_join(t);return 0;}
const char *host_text_format(const char *format,...){va_list args;va_start(args,format);const char *text=legacy_text_formatv(format,args);va_end(args);return text;}
int host_get_codepoint_next(const char *t,int *size){return utf8_next(t,size);}
const char *host_codepoint_to_u_t_f8(int c,int *size){return utf8_encode(c,size);}
