// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_WINDOW_H
#define LIBRELOOP_WINDOW_H
#include "draw.h"
#include "input.h"
typedef struct PlatformWindow PlatformWindow;
typedef struct {
    int width,height,pixel_width,pixel_height;
    float layout_scale,raster_scale,dpi_x,dpi_y;
    int dpi_scissor;
} WindowGeometry;
typedef struct { char **paths; int count; } WindowDrops;
/* Original desktop defaults:1200x675, minimum900x506, resizable. */
PlatformWindow *platform_window_create(const char *title,int hidden);
/* Release UI textures/fonts before destroying the window. */
void platform_window_destroy(PlatformWindow *window);
int platform_window_set_size(PlatformWindow *window,int width,int height);
int platform_window_set_min_size(PlatformWindow *window,int width,int height);
int platform_window_fit_display(PlatformWindow *window);
DrawContext *platform_window_draw(PlatformWindow *window);
AppInput *platform_window_input(PlatformWindow *window);
WindowGeometry platform_window_geometry(PlatformWindow *window);
/* Reset raw frame state, wait/poll, drain events, then consume deferred clicks.
   timeout0 polls; negative blocks; positive permits animation deadlines. */
int platform_window_events(PlatformWindow *window,int timeout_ms);
int platform_window_resized(const PlatformWindow *window);
int platform_window_take_close_request(PlatformWindow *window);
int platform_window_present(PlatformWindow *window);
int platform_window_icon(PlatformWindow *window,const void *rgba,int width,int height);
enum { WINDOW_CURSOR_DEFAULT=0,WINDOW_CURSOR_HAND=4,WINDOW_CURSOR_EW=5,WINDOW_CURSOR_NS=6 };
/* Original flush maps all other requests (including resize-all) to arrow. */
void platform_window_cursor(PlatformWindow *window,int requested);
/* Worker/MIDI notification only: never call this from the audio callback. */
void platform_window_wake(PlatformWindow *window);
WindowDrops platform_window_take_drops(PlatformWindow *window);
void platform_window_drops_free(WindowDrops drops);
/* NULL means unavailable/failed text; an owned "" means available empty text.
   Caller frees successful reads with ordinary free(). */
char *platform_clipboard_read(void);
int platform_clipboard_write(const char *text);
double platform_window_time(void);
const char *platform_window_error(void);
#endif
