// SPDX-License-Identifier: GPL-3.0-only
#include "widget_pixels.h"
#include <stdio.h>
static int capture(const char *name,WidgetPixels pixels){if(!pixels.pixels)return 0;FILE *file=fopen(name,"wb");if(!file){widget_pixels_free(pixels);return 0;}int ok=fwrite(pixels.pixels,4,(size_t)pixels.width*pixels.height,file)==(size_t)pixels.width*pixels.height;if(fclose(file))ok=0;widget_pixels_free(pixels);return ok;}
int main(void){return capture("circle.rgba",widget_circle_pixels()) && capture("arcs.rgba",widget_arc_pixels()) && capture("cable.rgba",widget_cable_pixels())?0:1;}
