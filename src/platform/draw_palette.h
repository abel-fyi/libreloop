// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_DRAW_PALETTE_H
#define LIBRELOOP_DRAW_PALETTE_H
#include "draw.h"
/* Luice's fixed 64-color paint palette. Six shades per color family, plus
   sixteen neutral grays. No runtime palette extension. */
static DrawColor luice_palette_color(int index){
    static const unsigned char families[8][3]={{255,51,51},{255,136,34},{255,221,34},{153,221,51},{34,204,102},{0,187,238},{68,102,238},{187,68,221}};
    if(index<16){int v=index*17;return (DrawColor){v,v,v,255};}
    int family=(index-16)/6,shade=(index-16)%6;DrawColor c={0,0,0,255};
    unsigned char *p=&c.r;
    static const int shade_level[6]={64,153,255,89,166,210};
    for(int i=0;i<3;i++)p[i]=shade<3?families[family][i]*shade_level[shade]/255:families[family][i]+(255-families[family][i])*shade_level[shade]/255;
    return c;
}
static DrawColor luice_palette_map(DrawColor c,int x,int y,int dither){
    int first=0,second=0,best=2147483647,next=2147483647;
    int high=c.r>c.g?c.r:c.g;if(c.b>high)high=c.b;
    int low=c.r<c.g?c.r:c.g;if(c.b<low)low=c.b;
    /* Keep semantic track colors from collapsing into the neutral ramp. */
    int start=0,end=16;
    if(high-low>25){
        int family=0,distance=2147483647;
        int nr=((int)c.r-low)*255/(high-low),ng=((int)c.g-low)*255/(high-low),nb=((int)c.b-low)*255/(high-low);
        for(int f=0;f<8;f++){
            DrawColor p=luice_palette_color(16+f*6+2);
            int pl=p.r<p.g?p.r:p.g;if(p.b<pl)pl=p.b;
            int ph=p.r>p.g?p.r:p.g;if(p.b>ph)ph=p.b;
            int r=nr-((int)p.r-pl)*255/(ph-pl),g=ng-((int)p.g-pl)*255/(ph-pl),b=nb-((int)p.b-pl)*255/(ph-pl);
            int error=r*r+g*g+b*b;if(error<distance){distance=error;family=f;}
        }
        start=16+family*6;end=start+6;
    }
    for(int i=start;i<end;i++){
        DrawColor p=luice_palette_color(i);int r=(int)c.r-p.r,g=(int)c.g-p.g,b=(int)c.b-p.b;
        int error=r*r+g*g+b*b;
        if(error<best){next=best;second=first;best=error;first=i;}else if(error<next){next=error;second=i;}
    }
    DrawColor a=luice_palette_color(first),b=luice_palette_color(second);
    if(dither && best){
        static const unsigned char bayer[4][4]={{0,8,2,10},{12,4,14,6},{3,11,1,9},{15,7,13,5}};
        int r=(int)b.r-a.r,g=(int)b.g-a.g,blue=(int)b.b-a.b;
        int length=r*r+g*g+blue*blue;
        int projection=((int)c.r-a.r)*r+((int)c.g-a.g)*g+((int)c.b-a.b)*blue;
        if(length && projection>0 && projection*32>length*(2*bayer[y&3][x&3]+1))a=b;
    }
    a.a=c.a;return a;
}
#endif
