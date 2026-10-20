// SPDX-License-Identifier: GPL-3.0-only
#include "window.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <math.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Window check%d: %s (%s)\n",__LINE__,#x,platform_window_error());return 1;}}while(0)
static atomic_uint_fast64_t wake_time;
static int wake_thread(void *context){SDL_Delay(20);atomic_store(&wake_time,SDL_GetTicksNS());platform_window_wake(context);return 0;}
static void drop(Uint32 id,Uint32 type,const char *path){SDL_Event e={0};e.type=type;e.drop.windowID=id;e.drop.data=path;e.drop.x=321.5f;e.drop.y=123.25f;SDL_PushEvent(&e);}
int main(void) {
    PlatformWindow *window=platform_window_create("LibreLoop boundary check",1);CHECK(window);
    int count=0;SDL_Window **windows=SDL_GetWindows(&count);CHECK(windows && count==1);SDL_Window *native=windows[0];SDL_free(windows);Uint32 id=SDL_GetWindowID(native);
    WindowGeometry geometry=platform_window_geometry(window);CHECK(geometry.width==1200 && geometry.height==675 && geometry.layout_scale==1);
    platform_window_events(window,0);
    SDL_Event e={0};e.type=SDL_EVENT_MOUSE_BUTTON_DOWN;e.button.windowID=id;e.button.button=SDL_BUTTON_LEFT;CHECK(SDL_PushEvent(&e));e.type=SDL_EVENT_MOUSE_BUTTON_UP;CHECK(SDL_PushEvent(&e));
    CHECK(platform_window_events(window,0)>=2);AppInput *input=platform_window_input(window);CHECK(input_click(input,0));CHECK(!input_button_down(input,0));
    platform_window_events(window,0);CHECK(!input_click(input,0));
    CHECK(platform_clipboard_write("LibreLoop \xce\xbb"));char *clipboard=platform_clipboard_read();CHECK(clipboard && !strcmp(clipboard,"LibreLoop \xce\xbb"));free(clipboard);
    e=(SDL_Event){0};e.type=SDL_EVENT_PINCH_UPDATE;e.pinch.windowID=id+1;e.pinch.scale=1.08f;
    CHECK(SDL_PushEvent(&e));platform_window_events(window,0);CHECK(input->scroll.native.zoom==0);
    e.pinch.windowID=id;CHECK(SDL_PushEvent(&e));platform_window_events(window,0);
    CHECK(fabsf(scroll_input_navigation(&input->scroll).zoom-1)<.00001f);
    platform_window_events(window,0);CHECK(input->scroll.native.zoom==0);
    drop(id,SDL_EVENT_DROP_BEGIN,NULL);drop(id,SDL_EVENT_DROP_FILE,"/tmp/first.wav");drop(id,SDL_EVENT_DROP_FILE,"/tmp/second.llp");drop(id,SDL_EVENT_DROP_COMPLETE,NULL);
    platform_window_events(window,0);WindowDrops paths=platform_window_take_drops(window);CHECK(paths.count==2);CHECK(!strcmp(paths.paths[0],"/tmp/first.wav") && !strcmp(paths.paths[1],"/tmp/second.llp"));platform_window_drops_free(paths);CHECK(platform_window_take_drops(window).count==0);
    CHECK(input->pointer.x==321.5f && input->pointer.y==123.25f);
    drop(id,SDL_EVENT_DROP_BEGIN,NULL);drop(id,SDL_EVENT_DROP_FILE,"/tmp/old.wav");drop(id,SDL_EVENT_DROP_COMPLETE,NULL);drop(id,SDL_EVENT_DROP_BEGIN,NULL);drop(id,SDL_EVENT_DROP_FILE,"/tmp/new.wav");drop(id,SDL_EVENT_DROP_COMPLETE,NULL);
    platform_window_events(window,0);paths=platform_window_take_drops(window);CHECK(paths.count==1 && !strcmp(paths.paths[0],"/tmp/new.wav"));platform_window_drops_free(paths);
    int resize_width=2400,resize_height=1350;
#ifdef _WIN32
    float content=SDL_GetWindowDisplayScale(native);resize_width=(int)(2400*content+.5f);resize_height=(int)(1350*content+.5f);
#endif
    CHECK(SDL_SetWindowSize(native,resize_width,resize_height));platform_window_events(window,0);geometry=platform_window_geometry(window);CHECK(geometry.width==2400 && geometry.height==1350 && geometry.layout_scale==2);CHECK(platform_window_resized(window));
    platform_window_events(window,0);CHECK(!platform_window_resized(window));
    SDL_Thread *worker=SDL_CreateThread(wake_thread,"window-wake",window);CHECK(worker);CHECK(platform_window_events(window,500)>0);SDL_WaitThread(worker,NULL);CHECK(input->frame_timestamp_ns>=atomic_load(&wake_time));
    e=(SDL_Event){0};e.type=SDL_EVENT_WINDOW_CLOSE_REQUESTED;e.window.windowID=id;CHECK(SDL_PushEvent(&e));platform_window_events(window,0);CHECK(platform_window_take_close_request(window));CHECK(!platform_window_take_close_request(window));
    draw_clear(platform_window_draw(window),(DrawColor){9,7,15,255});CHECK(platform_window_present(window));platform_window_destroy(window);
    puts("SDL window geometry, deferred clicks, clipboard, grouped drops, bounded wait/wake and close request passed");return 0;
}
