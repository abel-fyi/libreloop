// SPDX-License-Identifier: GPL-3.0-only
/* Original target-window Cocoa monitor. The own scroll accumulator retains
   frozen float arithmetic; SDL's differently-scaled wheel is excluded. */
#import <Cocoa/Cocoa.h>
#include <SDL3/SDL.h>
#include "input_macos.h"
#include <stdlib.h>
struct NativeInputMac { id monitor; ScrollInput pending; Uint32 wake_event; };
NativeInputMac *platform_macos_input_create(void *native_window,unsigned int wake_event) {
    if(!native_window)return NULL;
    NativeInputMac *state=calloc(1,sizeof *state);if(!state)return NULL;
    state->wake_event=wake_event;
    NSWindow *target=(NSWindow *)native_window;
    state->monitor=[[NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskScrollWheel|NSEventMaskMagnify
        handler:^NSEvent *(NSEvent *event) {
            if(event.window!=target)return event;
            if(event.type==NSEventTypeMagnify) {
                scroll_input_magnify(&state->pending,event.magnification);
            } else {
                scroll_input_native(&state->pending,event.scrollingDeltaX,event.scrollingDeltaY,event.hasPreciseScrollingDeltas);
            }
            /* Magnify may produce no SDL event. Wake an idle wait without
               suppressing the native event or changing its momentum phases. */
            SDL_Event wake={0};wake.type=state->wake_event;SDL_PushEvent(&wake);
            return event;
        }] retain];
    if(!state->monitor){free(state);return NULL;}return state;
}
void platform_macos_input_begin(NativeInputMac *state) {
    /* Only ordinary wheel resets before OS polling. Native precision/pinch
       stays pending until app consumption, exactly as the original monitor. */
    if(state)state->pending.wheel=(NavigationInput){0};
}
ScrollInput platform_macos_input_poll(NativeInputMac *state) {
    if(!state)return (ScrollInput){0};ScrollInput pending=state->pending;
    state->pending=(ScrollInput){0};return pending;
}
void platform_macos_input_destroy(NativeInputMac *state) {
    if(!state)return;
    [NSEvent removeMonitor:state->monitor];[state->monitor release];free(state);
}
