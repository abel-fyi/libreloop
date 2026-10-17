// SPDX-License-Identifier: GPL-3.0-only
#include "icons.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
static void pack(const DrawColor *input,int high,int large,int medium,int small){WidgetPixels image=icons_pack(input,high,large,medium,small);FILE *out=fopen("icons.rgba","wb");fwrite(image.pixels,4,image.width*image.height,out);fclose(out);widget_pixels_free(image);}
#define COLOR DrawColor
#define PACK pack

int main(int argc,char **argv){if(argc!=2)return 1;float scale=strtof(argv[1],NULL);int large=fmaxf(1,roundf(28*scale)),medium=fmaxf(1,roundf(20*scale)),small=fmaxf(1,roundf(16*scale)),high=large*4;
    COLOR *input=malloc((size_t)ICON_COUNT*high*high*sizeof *input);if(!input)return 2;
    for(int y=0;y<high;y++)for(int x=0;x<ICON_COUNT*high;x++)input[y*ICON_COUNT*high+x]=(COLOR){255,255,255,(x*73+y*29+(x/high)*17)%256};
    PACK(input,high,large,medium,small);free(input);return 0;}
