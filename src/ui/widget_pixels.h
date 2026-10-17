// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_WIDGET_PIXELS_H
#define LIBRELOOP_WIDGET_PIXELS_H
#include "draw.h"
typedef struct { int width,height; DrawColor *pixels; } WidgetPixels;
WidgetPixels widget_circle_pixels(void);
WidgetPixels widget_arc_pixels(void);
WidgetPixels widget_cable_pixels(void);
void widget_pixels_free(WidgetPixels image);
#endif
