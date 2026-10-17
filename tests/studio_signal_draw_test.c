// SPDX-License-Identifier: GPL-3.0-only
/* Inspect the actual frontend's draw commands for lost signal resolution. */
#include "../src/ui/frontend/ui_internal.h"
UiState ui;
Theme ui_theme;
DesktopSkin ui_skin;
static DrawRect rectangles[256];static DrawColor colors[256];static int count;
void host_draw_rectangle_rec(DrawRect r,DrawColor c){if(count<256){rectangles[count]=r;colors[count++]=c;}}
void host_draw_rectangle(int x,int y,int w,int h,DrawColor c){host_draw_rectangle_rec((DrawRect){x,y,w,h},c);}
void host_draw_line(int x,int y,int x2,int y2,DrawColor c){(void)x;(void)y;(void)x2;(void)y2;(void)c;}
static DrawVertex fill_vertices[6*(SPECTRUM_BINS+1)];static int fill_count;
void draw_triangles(DrawContext *d,DrawTexture *texture,const DrawVertex *v,int n){(void)d;(void)texture;fill_count=n;memcpy(fill_vertices,v,n*sizeof *v);}
void host_draw_line_ex(DrawPoint a,DrawPoint b,float width,DrawColor c){(void)a;(void)b;(void)width;(void)c;}
DrawColor host_color_from_h_s_v(float h,float s,float v){(void)h;(void)s;(void)v;return (DrawColor){0};}
int hover(float x,float y,float w,float h){(void)x;(void)y;(void)w;(void)h;return 0;}
const char *mixer_name(int id){(void)id;return "Test";}
/* Suppress only the cosmetic ramp, recording the requested fill extent. */
void ui_level_fill(DrawContext *d,DrawRect r,float level,float amber,float red,int vertical){
    (void)d;(void)amber;(void)red;
    if(level<=0)return;
    if(vertical){r.y+=r.height*(1-level);r.height*=level;}else r.width*=level;
    host_draw_rectangle_rec(r,ui_theme.meter_low);
}
DrawContext *host_draw(void){return NULL;}
#define ui_display_grid(...) ((void)0)
#define ui_bevel(...) ((void)0)
#include "../src/ui/frontend/ui_mixer.c"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void) {
    ui.studio_style=1;Spectrum s={0};
    for(int i=0;i<SPECTRUM_BINS;i++)s.db[i]=-78;
    s.db[11]=-32.5f;s.db[12]=-44.125f;s.db[63]=-12.25f;
    spectrum_draw(&s,(DrawRect){10,20,192,78},0);
    CHECK(fill_count==6*(SPECTRUM_BINS+1));
    for(int i=0;i<SPECTRUM_BINS;i++) {
        DrawPoint crest=fill_vertices[i*6+5].point;
        CHECK(fabsf(crest.x-(10+(i+.5f)*2))<.0001f);
        CHECK(fabsf((98-crest.y)-fmaxf(0,s.db[i]+78))<.0001f);
    }
    CHECK(fill_vertices[11*6+5].point.y!=fill_vertices[12*6+5].point.y);
    ui_theme.meter_low=(DrawColor){98,200,125,255};
    ui_theme.meter_mid=(DrawColor){224,190,88,255};
    ui_theme.meter_high=(DrawColor){231,89,83,255};
    int previous=0;
    for(int i=0;i<3;i++) {
        count=0;ui.meter_level[0][0]=powf(10,(-28.f+i)/20);
        ui.meter_level[0][1]=0;meter_draw(0,20,10,210,6);
        int fills=0,height=0;
        for(int r=0;r<count;r++)if(!memcmp(&colors[r],&ui_theme.meter_low,sizeof(DrawColor))){fills++;height=rectangles[r].height;}
        CHECK(fills==1 && height>previous);previous=height;
    }
    puts("All 96 FFT bins retain independent fractional heights; mixer fills respond continuously without segment steps.");return 0;
}
