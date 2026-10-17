// SPDX-License-Identifier: GPL-3.0-only
#include "controls_draw.h"
#include "theme.h"
#include <stdio.h>
static FILE *out;
static void texture(unsigned id,DrawRect s,DrawRect d,DrawPoint o,float rotation,DrawColor c){fprintf(out,"%u %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %d %d %d %d\n",id,s.x,s.y,s.width,s.height,d.x,d.y,d.width,d.height,o.x,o.y,rotation,c.r,c.g,c.b,c.a);}
void ui_render_circle(UIRender *r,float x,float y,float radius,DrawColor c){(void)r;texture(2,(DrawRect){0,0,64,64},(DrawRect){x-radius,y-radius,radius*2,radius*2},(DrawPoint){0},0,c);}
void ui_render_arc_pro(UIRender *r,int bank,int level,DrawRect d,DrawPoint o,float rotation,DrawColor c){(void)r;texture(1,(DrawRect){bank*512+level%16*32,level/16*32,32,32},d,o,rotation,c);}
static void draw(int style,int mode,float low,float high,float initial,float shown){KnobDrawing s={60,70,style,mode%2,mode==2,low,high,initial,shown};knob_draw(NULL,&s);}
#define DRAW draw

int main(void){out=fopen("knobs.txt","w");if(!out)return 1;theme_init();for(int style=0;style<8;style++)for(int mode=0;mode<5;mode++) {
    float low=style==2 || style==4?-1:style==6?20:0,high=style==5?1.25f:style==3?2:style==6?20000:style==7?31:1;
    float initial=style==2 || style==4?0:style==5 || style==3?1:style==6?5000:style==7?8:.5f;
    float shown=mode==0?low:mode==1?initial:mode==2?(low+high)/2:mode==3?high:style==7?31:initial;
    fprintf(out,"case %d %d\n",style,mode);DRAW(style,mode,low,high,initial,shown);
}fclose(out);return 0;}
