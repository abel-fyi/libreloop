// SPDX-License-Identifier: GPL-3.0-only
#include "window.h"
#include "input_sdl.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
struct PlatformWindow {
    SDL_Window *native;
    SDL_Renderer *renderer;
    DrawContext *draw;
    AppInput input;
    InputSDL adapter;
    Uint32 wake_event;
    int resized,close_requested;
    SDL_Cursor *horizontal_cursor,*vertical_cursor,*hand_cursor;
    int requested_cursor;
    WindowDrops pending,building;
};
PlatformWindow *platform_window_create(const char *title,int hidden) {
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS,"0");
    if(!SDL_Init(SDL_INIT_VIDEO))return NULL;
    PlatformWindow *window=calloc(1,sizeof *window);if(!window){SDL_QuitSubSystem(SDL_INIT_VIDEO);return NULL;}
    SDL_WindowFlags flags=SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIGH_PIXEL_DENSITY|(hidden?SDL_WINDOW_HIDDEN:0);
    int width=1200,height=675,minw=900,minh=506;
#ifdef _WIN32
    float content=SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());if(!isfinite(content) || content<=0)content=1;
    width=(int)roundf(width*content);height=(int)roundf(height*content);
    minw=(int)roundf(minw*content);minh=(int)roundf(minh*content);
#endif
    window->native=SDL_CreateWindow(title,width,height,flags);if(!window->native)goto fail;
    if(!SDL_SetWindowMinimumSize(window->native,minw,minh))goto fail;
    window->renderer=SDL_CreateRenderer(window->native,NULL);if(!window->renderer)goto fail;
    SDL_SetRenderVSync(window->renderer,0);
    window->draw=draw_create(window->renderer);if(!window->draw)goto fail;
    input_init(&window->input);
    window->input.focused=(SDL_GetWindowFlags(window->native)&SDL_WINDOW_INPUT_FOCUS)!=0;
    window->wake_event=SDL_RegisterEvents(1);if(!window->wake_event)goto fail;
    input_sdl_init(&window->adapter,&window->input);
    if(!SDL_StartTextInput(window->native))goto fail;
    window->horizontal_cursor=SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE);
    window->vertical_cursor=SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NS_RESIZE);
    window->hand_cursor=SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
    return window;
fail:
    platform_window_destroy(window);return NULL;
}
void platform_window_drops_free(WindowDrops drops) {
    for(int i=0;i<drops.count;i++)free(drops.paths[i]);free(drops.paths);
}
void platform_window_destroy(PlatformWindow *window) {
    if(!window)return;
    platform_window_drops_free(window->pending);platform_window_drops_free(window->building);
    SDL_DestroyCursor(window->horizontal_cursor);SDL_DestroyCursor(window->vertical_cursor);SDL_DestroyCursor(window->hand_cursor);
    draw_destroy(window->draw);SDL_DestroyRenderer(window->renderer);SDL_DestroyWindow(window->native);
    free(window);SDL_QuitSubSystem(SDL_INIT_VIDEO);
}
DrawContext *platform_window_draw(PlatformWindow *window){return window->draw;}
AppInput *platform_window_input(PlatformWindow *window){return &window->input;}
int platform_window_set_size(PlatformWindow *window,int width,int height) {
#ifdef _WIN32
    float content=SDL_GetWindowDisplayScale(window->native);
    if(!isfinite(content) || content<=0)content=1;
    width=(int)roundf(width*content);height=(int)roundf(height*content);
#endif
    return SDL_SetWindowSize(window->native,width,height);
}
int platform_window_set_min_size(PlatformWindow *window,int width,int height) {
#ifdef _WIN32
    float content=SDL_GetWindowDisplayScale(window->native);
    if(!isfinite(content) || content<=0)content=1;
    width=(int)roundf(width*content);height=(int)roundf(height*content);
#endif
    return SDL_SetWindowMinimumSize(window->native,width,height);
}
int platform_window_fit_display(PlatformWindow *window) {
    SDL_DisplayID display=SDL_GetDisplayForWindow(window->native);SDL_Rect usable;
    if(!display || !SDL_GetDisplayUsableBounds(display,&usable))return 0;
    int width,height,top=0,left=0,bottom=0,right=0;
    if(!SDL_GetWindowSize(window->native,&width,&height))return 0;
    SDL_GetWindowBordersSize(window->native,&top,&left,&bottom,&right);
    int max_width=usable.w-left-right,max_height=usable.h-top-bottom;
    if(max_width<=0 || max_height<=0)return 0;
    if(width>max_width || height>max_height)
        if(!SDL_SetWindowSize(window->native,width<max_width?width:max_width,height<max_height?height:max_height))return 0;
    return SDL_SetWindowPosition(window->native,SDL_WINDOWPOS_CENTERED_DISPLAY(display),SDL_WINDOWPOS_CENTERED_DISPLAY(display));
}
WindowGeometry platform_window_geometry(PlatformWindow *window) {
    WindowGeometry geometry={0};SDL_GetWindowSize(window->native,&geometry.width,&geometry.height);
    SDL_GetWindowSizeInPixels(window->native,&geometry.pixel_width,&geometry.pixel_height);
#ifdef _WIN32
    float content=SDL_GetWindowDisplayScale(window->native);if(!isfinite(content) || content<=0)content=1;
    geometry.width=(int)roundf(geometry.width/content);geometry.height=(int)roundf(geometry.height/content);
#endif
    geometry.layout_scale=fmaxf(1,fminf(geometry.width/1200.f,geometry.height/675.f));
    geometry.raster_scale=geometry.layout_scale*(geometry.width>0 && geometry.pixel_width>0?geometry.pixel_width/(float)geometry.width:1);
    geometry.dpi_x=geometry.dpi_y=SDL_GetWindowDisplayScale(window->native);
    geometry.dpi_scissor=geometry.pixel_width!=geometry.width || geometry.pixel_height!=geometry.height;
    return geometry;
}
static void event(PlatformWindow *window,const SDL_Event *native) {
    Uint32 id=SDL_GetWindowID(window->native);
    if(native->type==SDL_EVENT_QUIT){window->close_requested=1;return;}
    if(native->type>=SDL_EVENT_WINDOW_FIRST && native->type<=SDL_EVENT_WINDOW_LAST) {
        if(native->window.windowID!=id)return;
        if(native->type==SDL_EVENT_WINDOW_CLOSE_REQUESTED)window->close_requested=1;
        /* Original resized flag comes from the logical GLFW window-size
           callback. A pixel-density event wakes/redraws but is not that flag. */
        if(native->type==SDL_EVENT_WINDOW_RESIZED)window->resized=1;
    }
    Uint32 input_window=0;
    switch(native->type) {
    case SDL_EVENT_KEY_DOWN:case SDL_EVENT_KEY_UP:input_window=native->key.windowID;break;
    case SDL_EVENT_TEXT_INPUT:input_window=native->text.windowID;break;
    case SDL_EVENT_MOUSE_MOTION:input_window=native->motion.windowID;break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:case SDL_EVENT_MOUSE_BUTTON_UP:input_window=native->button.windowID;break;
    case SDL_EVENT_MOUSE_WHEEL:input_window=native->wheel.windowID;break;
    case SDL_EVENT_PINCH_BEGIN:case SDL_EVENT_PINCH_UPDATE:case SDL_EVENT_PINCH_END:
        input_window=native->pinch.windowID;
        /* Some desktop backends deliver gestures without a window ID. */
        if(!input_window && SDL_GetKeyboardFocus()!=window->native)return;
        break;
    default:break;
    }
    if(input_window && input_window!=id)return;
    switch(native->type) {
    case SDL_EVENT_DROP_BEGIN:
        if(native->drop.windowID!=id)return;
        platform_window_drops_free(window->building);window->building=(WindowDrops){0};return;
    case SDL_EVENT_DROP_FILE: {
        if(native->drop.windowID!=id || !native->drop.data)return;
        size_t length=strlen(native->drop.data);char *copy=malloc(length+1);if(!copy)return;
        memcpy(copy,native->drop.data,length+1);
        char **paths=realloc(window->building.paths,(size_t)(window->building.count+1)*sizeof *paths);
        if(!paths){free(copy);return;}window->building.paths=paths;paths[window->building.count++]=copy;
        /* SDL supplies drop coordinates on every desktop backend. */
        InputEvent pointer={.kind=INPUT_POINTER,.data.pointer={native->drop.x,native->drop.y}};input_event(&window->input,&pointer);
        return;
    }
    case SDL_EVENT_DROP_COMPLETE:
        if(native->drop.windowID!=id)return;
        if(window->building.count){platform_window_drops_free(window->pending);window->pending=window->building;window->building=(WindowDrops){0};}return;
    default:break;
    }
    input_sdl_event(&window->adapter,native);
}
int platform_window_events(PlatformWindow *window,int timeout_ms) {
    input_begin(&window->input,SDL_GetTicksNS());window->resized=0;
    SDL_Event native;int count=0;
    if(timeout_ms!=0) {
        int received=timeout_ms<0?SDL_WaitEvent(&native):SDL_WaitEventTimeout(&native,timeout_ms);
        if(received){event(window,&native);count++;}
    }
    while(SDL_PollEvent(&native)){event(window,&native);count++;}
    /* UI timers observe the completed poll/wait, not its earlier start. */
    window->input.frame_timestamp_ns=SDL_GetTicksNS();
#ifdef _WIN32
    float content=SDL_GetWindowDisplayScale(window->native);if(!isfinite(content) || content<=0)content=1;
    window->input.scale=(InputPoint){1/content,1/content};
    SDL_SetWindowMinimumSize(window->native,(int)roundf(900*content),(int)roundf(506*content));
#endif
    input_clicks_begin(&window->input);return count;
}
int platform_window_resized(const PlatformWindow *window){return window->resized;}
int platform_window_take_close_request(PlatformWindow *window){int close=window->close_requested;window->close_requested=0;return close;}
int platform_window_present(PlatformWindow *window){return draw_present(window->draw);}
void platform_window_cursor(PlatformWindow *window,int requested) {
    if(requested==window->requested_cursor)return;
    SDL_Cursor *cursor=requested==WINDOW_CURSOR_EW?window->horizontal_cursor:requested==WINDOW_CURSOR_NS?window->vertical_cursor:requested==WINDOW_CURSOR_HAND?window->hand_cursor:NULL;
    SDL_SetCursor(cursor?cursor:SDL_GetDefaultCursor());window->requested_cursor=requested;
}
void platform_window_wake(PlatformWindow *window){SDL_Event wake={0};wake.type=window->wake_event;SDL_PushEvent(&wake);}
WindowDrops platform_window_take_drops(PlatformWindow *window){WindowDrops drops=window->pending;window->pending=(WindowDrops){0};return drops;}
char *platform_clipboard_read(void) {
    /* GetClipboardText collapses unavailable text and read failure to "".
       GLFW returns NULL for those states, while available empty text is "". */
    const char *mime="text/plain;charset=utf-8";
    if(!SDL_HasClipboardData(mime))return NULL;
    SDL_ClearError();size_t length=0;
    void *native=SDL_GetClipboardData(mime,&length);if(!native)return NULL;
    if(length==SIZE_MAX || (!length && *SDL_GetError())){SDL_free(native);return NULL;}
    char *copy=malloc(length+1);if(copy){memcpy(copy,native,length);copy[length]=0;}
    SDL_free(native);return copy;
}
int platform_clipboard_write(const char *text){return SDL_SetClipboardText(text);}
double platform_window_time(void){return SDL_GetTicksNS()/1000000000.0;}
const char *platform_window_error(void){return SDL_GetError();}

int platform_window_icon(PlatformWindow *window,const void *rgba,int width,int height) {
    if(!rgba || width<1 || height<1)return 0;
    SDL_Surface *image=SDL_CreateSurfaceFrom(width,height,SDL_PIXELFORMAT_RGBA32,(void *)rgba,width*4);
    if(!image)return 0;int ok=SDL_SetWindowIcon(window->native,image);SDL_DestroySurface(image);return ok;
}
