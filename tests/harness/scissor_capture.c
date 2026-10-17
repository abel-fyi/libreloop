// SPDX-License-Identifier: GPL-3.0-only
#include "render.h"
#include <stdio.h>
static FILE *out;
static void emit(int mode,int height,int pixels,float scale,int x,int y,int w,int h){
    WindowGeometry g={.height=height,.pixel_height=pixels,.dpi_x=scale,.dpi_y=scale,.dpi_scissor=mode==1};
    DrawRect r=ui_render_scissor(g,x,y,w,h);fprintf(out,"%d %d %d %d\n",(int)r.x,(int)r.y,(int)r.width,(int)r.height);
}

int main(void){out=fopen("scissors.txt","w");if(!out)return 1;
    const int clips[][4]={{0,0,1200,675},{7,13,101,83},{-7,-13,120,120},{19,400,54,62},{1000,650,300,80}};
    const int heights[]={675,777,675};const int pixels[]={1350,971,675};const float scales[]={2,1.25f,1};
    for(int mode=0;mode<3;mode++)for(int g=0;g<3;g++)for(int c=0;c<5;c++){const int *r=clips[c];emit(mode,heights[g],pixels[g],scales[g],r[0],r[1],r[2],r[3]);}
    fclose(out);return 0;}
