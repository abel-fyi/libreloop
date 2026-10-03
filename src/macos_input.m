// SPDX-License-Identifier: GPL-3.0-only
#import <Cocoa/Cocoa.h>
#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include "navigation.h"
#include <math.h>
static id monitor;
static NavigationInput pending;
void macos_navigation_init(void *window) {
    NSWindow *target=glfwGetCocoaWindow((GLFWwindow *)window);
    monitor=[[NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskScrollWheel|NSEventMaskMagnify
        handler:^NSEvent *(NSEvent *event) {
            if(event.window!=target) return event;
            if(event.type==NSEventTypeMagnify) {
                float factor=1+event.magnification;
                if(isfinite(factor) && factor>0) pending.zoom+=logf(factor)/logf(1.08f);
            } else if(event.hasPreciseScrollingDeltas) {
                pending.x+=event.scrollingDeltaX; pending.y+=event.scrollingDeltaY; pending.precise=1;
            }
            return event;
        }] retain];
}
NavigationInput macos_navigation_poll(void) {
    NavigationInput input=pending; pending=(NavigationInput){0}; return input;
}
void macos_navigation_close(void) {
    if(monitor) { [NSEvent removeMonitor:monitor]; [monitor release]; monitor=nil; }
    pending=(NavigationInput){0};
}
