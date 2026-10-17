// SPDX-License-Identifier: GPL-3.0-only
#include "theme.h"
#include <stdint.h>
#include <stdio.h>
#define COLOR DrawColor
#define HSV DrawHSV
#define BLEND draw_color_blend
#define TO_HSV draw_color_hsv
#define FADE draw_color_fade
#define FROM_HSV draw_color_from_hsv

int main(void) {
    theme_init();FILE *palette=fopen("palette.bin","wb"),*out=fopen("colors.txt","w");if(!palette || !out)return 1;
    fwrite(&ui_theme,1,sizeof ui_theme,palette);fclose(palette);
    uint32_t seed=0x12345678u;const float alphas[]={-.25f,0,.35f,.5f,1,1.25f};
    for(int i=0;i<512;i++) {
        COLOR colors[3];for(int c=0;c<3;c++){unsigned char *p=(unsigned char *)&colors[c];for(int k=0;k<4;k++){seed=seed*1664525u+1013904223u;p[k]=(unsigned char)(seed>>24);}}
        COLOR foreground=theme_foreground(colors[0]),blend=BLEND(colors[0],colors[1],colors[2]);HSV hsv=TO_HSV(colors[0]);
        fprintf(out,"%d %d %d %d %d %d %d %d %.9g %.9g %.9g\n",foreground.r,foreground.g,foreground.b,foreground.a,blend.r,blend.g,blend.b,blend.a,hsv.x,hsv.y,hsv.z);
        for(int a=0;a<6;a++){COLOR fade=FADE(colors[0],alphas[a]);fprintf(out,"%d %d %d %d\n",fade.r,fade.g,fade.b,fade.a);}
    }
    fclose(out);out=fopen("from-hsv.txt","w");
    const float hues[]={-360,-30,0,90,180,235,320,360,720};const float saturation[]={0,.25f,.5f,1};const float value[]={0,.35f,.98f,1};
    for(int h=0;h<9;h++)for(int s=0;s<4;s++)for(int v=0;v<4;v++){COLOR c=FROM_HSV(hues[h],saturation[s],value[v]);fprintf(out,"%d %d %d %d\n",c.r,c.g,c.b,c.a);}
    fclose(out);return 0;
}
