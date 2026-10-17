// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_EDITOR_NAVIGATION_H
#define LIBRELOOP_EDITOR_NAVIGATION_H
#include "editor_windows.h"
#include "arrangement.h"
#include "navigation.h"
typedef struct { float x,y; } NavigationPoint;
typedef struct {
    Windows *windows;
    Arrangement *arrangement;
    int pattern,drag,active,browser_focus,pan_cursor;
    NavigationPoint last;
    float track_scroll,track_zoom,track_heights[LANES];
    float piano_span[PATTERNS],piano_pan[PATTERNS],piano_zoom,piano_remainder;
    int piano_top;
    int rack_enabled,rack_count;
    float rack_scroll,rack_width[PATTERNS],rack_view[PATTERNS];
} EditorNavigation;
/* Boundary-normalized logical pointer; native scroll remains in original
   Cocoa point units for navigation_motion's exact scale conversion. */
typedef struct {
    NavigationPoint pointer;
    NavigationInput scroll;
    float layout_scale;
    int blocked,captured,middle_pressed,middle_held,shift,zoom_modifier;
    /* Optional concrete content rectangle; zero width retains legacy geometry. */
    Rect piano_content,playlist_content;
    float piano_reference_area;
    float playlist_grid_x,playlist_grid_top,playlist_header_x;
} NavigationFrame;
void editor_navigate(EditorNavigation *state,NavigationFrame input);
#endif
