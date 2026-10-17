// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_FRONTEND_HOST_H
#define LIBRELOOP_FRONTEND_HOST_H
/* Concrete bridge for the carried desktop, using only application-owned types.
   SDL and OS headers are confined to the platform implementation. */
#include "draw.h"
#include "window.h"
#include "thread.h"
#include "text_fonts.h"
#include "font_layout.h"
#include "host_constants.h"
#define PI 3.14159265358979323846f
#define DEG2RAD (PI/180.0f)
#define RAD2DEG (180.0f/PI)
#define WHITE ((DrawColor){255,255,255,255})
#define BLACK ((DrawColor){0,0,0,255})
#define BLANK ((DrawColor){0,0,0,0})
typedef struct {DrawTexture *id;int width,height;} HostTexture;
typedef struct {DrawTexture *id;HostTexture texture;} HostTarget;
typedef struct {void *data;int width,height;} HostImage;
typedef struct {DrawPoint offset,target;float rotation,zoom;} HostCamera;
DrawContext *host_draw(void);
PlatformWindow *host_window(void);
AppInput *host_input(void);
TextFonts *host_fonts(void);
void host_fonts_clear(void);
void host_init_window(int width,int height,const char *title);
void host_close_window(void);
void host_set_config_flags(unsigned flags);
void host_set_window_min_size(int w,int h);
void host_set_exit_key(int key);
void host_set_target_f_p_s(int fps);
void host_set_window_icon(HostImage image);
void host_begin_drawing(void);
void host_end_drawing(void);
void host_enable_event_waiting(void);
void host_disable_event_waiting(void);
int host_window_should_close(void);
int host_get_screen_width(void);
int host_get_screen_height(void);
int host_is_window_focused(void);
int host_is_window_resized(void);
void host_framebuffer_size(PlatformWindow *window,int *w,int *h);
void host_window_size(PlatformWindow *window,int *w,int *h);
void host_wake(void);
double host_get_time(void);
float host_get_frame_time(void);
DrawPoint host_get_mouse_position(void);
DrawPoint host_get_mouse_delta(void);
DrawPoint host_get_mouse_wheel_move_v(void);
float host_get_mouse_wheel_move(void);
NavigationInput host_navigation(void);
int host_is_key_down(int key);
int host_is_key_pressed(int key);
int host_is_key_pressed_repeat(int key);
int host_is_key_released(int key);
int host_get_char_pressed(void);
int host_is_mouse_button_down(int button);
int host_is_mouse_button_pressed(int button);
int host_is_mouse_button_released(int button);
void host_set_mouse_cursor(int cursor);
const char *host_get_clipboard_text(void);
int host_is_file_dropped(void);
WindowDrops host_load_dropped_files(void);
void host_unload_dropped_files(WindowDrops files);
void host_begin_mode2_d(HostCamera camera);
void host_end_mode2_d(void);
void host_begin_scissor_mode(int x,int y,int w,int h);
void host_end_scissor_mode(void);
void host_clear_background(DrawColor color);
void host_draw_rectangle(int x,int y,int w,int h,DrawColor color);
void host_draw_rectangle_rec(DrawRect rect,DrawColor color);
void host_draw_rectangle_lines(int x,int y,int w,int h,DrawColor color);
void host_draw_rectangle_lines_ex(DrawRect rect,float thickness,DrawColor color);
void host_draw_line(int x,int y,int xx,int yy,DrawColor color);
void host_draw_line_ex(DrawPoint a,DrawPoint b,float width,DrawColor color);
void host_draw_triangle(DrawPoint a,DrawPoint b,DrawPoint c,DrawColor color);
void host_draw_circle(int x,int y,float radius,DrawColor color);
void host_draw_circle_lines(int x,int y,float radius,DrawColor color);
void host_draw_text_ex(DrawFont *font,const char *text,DrawPoint p,float size,float spacing,DrawColor color);
void host_draw_texture_pro(HostTexture texture,DrawRect source,DrawRect dest,DrawPoint origin,float rotation,DrawColor color);
DrawColor host_fade(DrawColor color,float alpha);
DrawColor host_color_alpha_blend(DrawColor dest,DrawColor src,DrawColor tint);
DrawColor host_color_from_h_s_v(float h,float s,float v);
DrawHSV host_color_to_h_s_v(DrawColor color);
DrawPoint host_get_spline_point_bezier_cubic(DrawPoint a,DrawPoint b,DrawPoint c,DrawPoint d,float t);
int host_check_collision_point_rec(DrawPoint p,DrawRect r);
HostImage host_gen_image_color(int w,int h,DrawColor color);
HostImage host_load_image(const char *path);
HostImage host_load_image_from_texture(HostTexture texture);
void host_image_flip_vertical(HostImage *image);
void host_unload_image(HostImage image);
HostTexture host_load_texture_from_image(HostImage image);
void host_unload_texture(HostTexture texture);
void host_set_texture_filter(HostTexture texture,int filter);
void host_set_texture_wrap(HostTexture texture,int wrap);
HostTarget host_load_render_texture(int w,int h);
void host_unload_render_texture(HostTarget target);
void host_begin_texture_mode(HostTarget target);
void host_end_texture_mode(void);
void host_take_screenshot(const char *path);
void host_mesh_settexture(DrawTexture *texture);
void host_mesh_begin(int mode);
void host_mesh_color4ub(unsigned char r,unsigned char g,unsigned char b,unsigned char a);
void host_mesh_texcoord2f(float u,float v);
void host_mesh_vertex2f(float x,float y);
void host_mesh_end(void);
int host_directory_exists(const char *path);
int host_path_absolute(const char *path);
int host_file_exists(const char *path);
int host_is_file_extension(const char *path,const char *extensions);
const char *host_get_working_directory(void);
int host_make_directory(const char *path);
int host_change_directory(const char *path);
const char *host_get_file_name(const char *path);
const char *host_get_file_name_without_ext(const char *path);
const char *host_get_application_directory(void);
char *host_realpath(const char *path,char *out);
int host_thread_create(PlatformThread **thread,const void *attributes,void *(*routine)(void *),void *context);
int host_thread_join(PlatformThread *thread,void **result);
const char *host_text_format(const char *format,...);
int host_get_codepoint_next(const char *text,int *size);
const char *host_codepoint_to_u_t_f8(int codepoint,int *size);
#endif
