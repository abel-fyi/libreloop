// SPDX-License-Identifier: GPL-3.0-only
#ifndef NAVIGATION_H
#define NAVIGATION_H
typedef struct { float x,y,zoom; int precise; } NavigationInput;
typedef struct { float x,y,zoom; } NavigationMotion;
NavigationMotion navigation_motion(NavigationInput input,int shift,int zoom_modifier,float scale);
#ifdef __APPLE__
void macos_navigation_init(void *window);
NavigationInput macos_navigation_poll(void);
void macos_navigation_close(void);
#endif
#endif
