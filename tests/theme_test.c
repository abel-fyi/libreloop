// SPDX-License-Identifier: GPL-3.0-only
#include "theme.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
static float luminance(DrawColor c) {
    float v[]={c.r/255.f,c.g/255.f,c.b/255.f};
    for(int i=0;i<3;i++) v[i]=v[i]<=.04045f?v[i]/12.92f:powf((v[i]+.055f)/1.055f,2.4f);
    return .2126f*v[0]+.7152f*v[1]+.0722f*v[2];
}
static float contrast(DrawColor a,DrawColor b) { float x=luminance(a),y=luminance(b); return (fmaxf(x,y)+.05f)/(fminf(x,y)+.05f); }
static int same(DrawColor a,DrawColor b) { return a.r==b.r && a.g==b.g && a.b==b.b && a.a==b.a; }
static SDL_Surface *surface;
static SDL_Renderer *renderer;
static DrawContext *draw;
static int check_render(void) {
    DrawTexture *target=draw_texture_create(draw,80,64,NULL,1);if(!target)return 0;
    draw_target(draw,target);draw_clear(draw,ui_theme.surface);
    ui_fader_handle(draw,(DrawRect){10,10,14,18},0);ui_fader_handle(draw,(DrawRect){40,10,14,18},1);
    unsigned char pixels[80*64*4];if(!draw_read_rgba(draw,pixels,80,64))return 0;
    int mark=0,selected=0;
    for(int y=0;y<64;y++)for(int x=12;x<23;x++){const unsigned char *p=pixels+(y*80+x)*4;mark+=same((DrawColor){p[0],p[1],p[2],p[3]},ui_theme.fader_mark);}
    for(int y=0;y<64;y++)for(int x=42;x<53;x++){const unsigned char *p=pixels+(y*80+x)*4;selected+=same((DrawColor){p[0],p[1],p[2],p[3]},ui_theme.selected_text);}
    draw_target(draw,NULL);draw_texture_destroy(target);return mark>=8 && selected>=8;
}
static int check_meter(void) {
    DrawTexture *target=draw_texture_create(draw,80,64,NULL,1);if(!target)return 0;
    draw_target(draw,target);draw_clear(draw,(DrawColor){0,0,0,255});
    ui_level_fill(draw,(DrawRect){4,4,70,8},1,.8f,1,0);
    ui_level_fill(draw,(DrawRect){4,20,70,8},.513f,.8f,1,0);
    ui_level_fill(draw,(DrawRect){4,36,70,8},0,.8f,1,0);
    unsigned char p[80*64*4];if(!draw_read_rgba(draw,p,80,64))return 0;
    int colors=0;for(int x=5;x<73;x++)if(memcmp(p+(7*80+x)*4,p+(7*80+x-1)*4,3))colors++;
    const unsigned char *red=p+(7*80+73)*4,*green=p+(7*80+5)*4,*empty=p+(39*80+5)*4,*beyond=p+(23*80+42)*4;
    int ok=colors>40 && red[0]>red[1]*2 && green[1]>green[0]*2 && !empty[0] && !empty[1] && !beyond[0] && !beyond[1];
    draw_target(draw,NULL);draw_texture_destroy(target);return ok;
}
static DrawColor skin_pixel(const unsigned char *p,int width,float scale,int x,int y) {
    int at=((int)((y+.5f)*scale)*width+(int)((x+.5f)*scale))*4;
    return (DrawColor){p[at],p[at+1],p[at+2],p[at+3]};
}
static int check_bitmap_knobs(void) {
    DrawTexture *target=draw_texture_create(draw,128,40,NULL,1);if(!target)return 0;
    draw_target(draw,target);draw_clear(draw,ui_theme.surface);
    if(!theme_draw_init(draw))return 0;
    const int radii[]={8,9,10,12};
    for(int k=0;k<4;k++)ui_knob_face(draw,16+k*32,16,radii[k],-1.5707963f,0);
    unsigned char pixels[128*40*4];if(!draw_read_rgba(draw,pixels,128,40))return 0;
    int ok=1;
    for(int k=0;k<4;k++) {
        int seen[256]={0},shades=0,ink=0;
        for(int y=2;y<radii[k]-2;y++)for(int x=-radii[k]+3;x<radii[k]-2;x++) {
            if(x*x+y*y>(radii[k]-2)*(radii[k]-2))continue;
            const unsigned char *p=pixels+((16+y)*128+16+k*32+x)*4;
            /* Decorative body uses a shared small palette, not continuous
               triangle shading. Pointer and signal curves remain continuous. */
            if(p[0]%16==0 && p[1]==p[0]+4 && p[2]==p[0]+8){ink++;if(!seen[p[0]]){seen[p[0]]=1;shades++;}}
        }
        ok&=ink>=10 && shades>=3;
    }
    draw_target(draw,NULL);draw_texture_destroy(target);theme_draw_close();return ok;
}
static int check_grips(float scale,int active) {
    int size=(int)(40*scale);
    DrawTexture *target=draw_texture_create(draw,size,size,NULL,1);if(!target)return 0;
    draw_target(draw,target);draw_transform(draw,scale,0,0);draw_clear(draw,(DrawColor){0,0,0,255});
    ui_divider_grip(draw,(DrawRect){0,2,40,6},0,active);
    DrawColor saved_text=ui_theme.text,saved_secondary=ui_theme.secondary;
    ui_theme.text=(DrawColor){255,0,255,255};ui_theme.secondary=(DrawColor){0,255,0,255};
    ui_divider_grip(draw,(DrawRect){2,0,6,40},1,active);
    ui_theme.text=saved_text;ui_theme.secondary=saved_secondary;
    unsigned char pixels[120*120*4];int ok=draw_read_rgba(draw,pixels,size,size);
    for(int i=0;i<5;i++) {
        int axis=(int)((11.5f+i*4)*scale),fixed=(int)(4.5f*scale);
        const unsigned char *h=pixels+(fixed*size+axis)*4,*v=pixels+(axis*size+fixed)*4;
        int dots=!memcmp(h,&ui_theme.border,3) && !memcmp(v,&ui_theme.border,3);
        if(!dots)fprintf(stderr,"grip %.2f dot %d: h %d,%d,%d v %d,%d,%d expected %d,%d,%d\n",scale,i,h[0],h[1],h[2],v[0],v[1],v[2],ui_theme.border.r,ui_theme.border.g,ui_theme.border.b);
        ok &= dots;
        if(i<4) {
            int gap=(int)((14.f+i*4)*scale);
            /* Gap centers retain the same rail fill in both orientations. */
            int empty=abs(pixels[(fixed*size+gap)*4]-ui_theme.control.r-(active?8:-8))<3 && abs(pixels[(gap*size+fixed)*4]-ui_theme.control.r-(active?8:-8))<3;
            if(!empty)fprintf(stderr,"grip %.2f gap %d: %d %d\n",scale,i,pixels[(fixed*size+gap)*4],pixels[(gap*size+fixed)*4]);
            ok &= empty;
        }
    }
    draw_transform(draw,1,0,0);draw_target(draw,NULL);draw_texture_destroy(target);return ok;
}
static int check_skin(float scale) {
    int w=(int)(80*scale),h=(int)(64*scale);
    DrawTexture *target=draw_texture_create(draw,w,h,NULL,1);if(!target)return 0;
    draw_target(draw,target);draw_transform(draw,scale,0,0);draw_clear(draw,ui_theme.surface);
    if(!theme_draw_init(draw))return 0;
    ui_button_face(draw,(DrawRect){2,2,22,22},ui_theme.control,0);
    ui_button_face(draw,(DrawRect){28,2,22,22},ui_theme.highlight,1);
    ui_dropdown_arrow(draw,(DrawRect){6,2,44,22},0);
    ui_titlebar(draw,(DrawRect){2,28,48,10},1);
    ui_graph_paper(draw,(DrawRect){2,42,48,18});
    ui_graph_area(draw,(DrawPoint){3,43},(DrawPoint){48,43},59);
    ui_graph_dots(draw,(DrawRect){2,42,48,18});
    ui_graph_handle(draw,(DrawPoint){40,55});
    ui_knob_face(draw,65,16,9,-1.5707963f,0);
    ui_knob_face(draw,65,44,9,0,1);
    ui_lamp(draw,(DrawPoint){65,60},3,(DrawColor){255,33,47,255});
    ui_text_well(draw,(DrawRect){54,26,24,9},1);
    /* A real curve is drawn above the paper, never clipped into dot cells. */
    draw_line(draw,(DrawPoint){3,50},(DrawPoint){48,50},2,ui_skin.graph_line);
    unsigned char pixels[240*192*4];if(!draw_read_rgba(draw,pixels,w,h))return 0;
    DrawColor top=skin_pixel(pixels,w,scale,10,7);
    /* Compare gradient brightness at the same physical Bayer phase. */
    int tx=(int)(10.5f*scale),ty=(int)(7.5f*scale),by=(int)(19.5f*scale);
    by-=(by-ty)%4;const unsigned char *bp=pixels+(by*w+tx)*4;
    DrawColor bottom={bp[0],bp[1],bp[2],bp[3]};
    DrawColor raised=skin_pixel(pixels,w,scale,2,10),sunken=skin_pixel(pixels,w,scale,28,10);
    DrawColor title=skin_pixel(pixels,w,scale,12,32),center=skin_pixel(pixels,w,scale,65,16),edge=skin_pixel(pixels,w,scale,72,21);
    /* Paper dots have a fixed four-physical-pixel period at every UI scale. */
    int dot=0,empty=255;
    for(int y=(int)ceilf(44*scale);y<(int)ceilf(44*scale)+4;y++)for(int x=(int)ceilf(4*scale);x<(int)ceilf(4*scale)+4;x++) {
        int blue=pixels[(y*w+x)*4+2];if(blue>dot)dot=blue;if(blue<empty)empty=blue;
    }
    const unsigned char *line=pixels+((int)(50*scale)*w+(int)(6*scale))*4;
    DrawColor curve={line[0],line[1],line[2],line[3]};
    DrawColor fill_top=skin_pixel(pixels,w,scale,11,44),fill_bottom=skin_pixel(pixels,w,scale,11,57);
    DrawColor handle=skin_pixel(pixels,w,scale,40,55),lamp=skin_pixel(pixels,w,scale,65,60);
    DrawColor arrow=skin_pixel(pixels,w,scale,42,13),arrow_face=skin_pixel(pixels,w,scale,42,6);
    DrawColor field=skin_pixel(pixels,w,scale,65,30);
    DrawColor knob_shadow=skin_pixel(pixels,w,scale,70,24),button_shadow=skin_pixel(pixels,w,scale,25,12);
    /* Faint paper remains visible without competing with the real curve. */
    int ok=top.r>bottom.r+15 && raised.r>sunken.r+80 && abs(title.b-title.r)<25 && dot>empty+8 && dot<empty+35 &&
        curve.g>150 && curve.b>220 && center.r>edge.r+20 && fill_top.b>fill_bottom.b+25 &&
        handle.r>180 && handle.g>200 && lamp.r>lamp.g+40 && arrow.r<25 && arrow_face.r>100 &&
        same(field,ui_skin.field) && contrast(theme_foreground(field),field)>=4.5f &&
        knob_shadow.r<ui_theme.surface.r-20 && button_shadow.r<ui_theme.surface.r-20;
    if(!ok)fprintf(stderr,"field=%d/%d shadow=%d/%d arrow=%d/%d lamp=%d/%d fill=%d/%d\n",field.r,ui_skin.field.r,knob_shadow.r,button_shadow.r,arrow.r,arrow_face.r,lamp.r,lamp.g,fill_top.b,fill_bottom.b);
    if(!ok)fprintf(stderr,"Skin scale %.2f: gradient %d/%d, bevel %d/%d, title %d/%d, dots %d/%d, curve %d/%d, knob %d/%d\n",scale,top.r,bottom.r,raised.r,sunken.r,title.r,title.b,dot,empty,curve.g,curve.b,center.r,edge.r);
    draw_transform(draw,1,0,0);draw_target(draw,NULL);draw_texture_destroy(target);theme_draw_close();return ok;
}
/* A pattern texel must stay on the same output pixel across scale/translation. */
static int check_pixel_pattern(void) {
    DrawColor tile[64];for(int y=0;y<8;y++)for(int x=0;x<8;x++)tile[y*8+x]=(DrawColor){20+x*20,30+y*20,80,255};
    DrawTexture *texture=draw_texture_create(draw,8,8,tile,0);if(!texture)return 0;
    const float scales[]={.75f,1,1.25f,1.5f,2,2.5f};unsigned char actual[80*64*4];
    for(unsigned i=0;i<sizeof scales/sizeof *scales;i++)for(int moved=0;moved<3;moved++) {
        draw_transform(draw,scales[i],moved==2?-7.37f:moved?.37f:0,moved==2?-3.83f:moved?.83f:0);
        draw_clear(draw,(DrawColor){0,0,0,255});DrawRect clip={8,8,56,48};draw_clip(draw,&clip);
        draw_texture_tiled_pixels(draw,texture,(DrawRect){0,0,8,8},(DrawRect){0,0,100,100},(DrawColor){255,255,255,255});
        draw_clip(draw,NULL);draw_flush(draw);
        if(!draw_read_rgba(draw,actual,80,64)){draw_texture_destroy(texture);return 0;}
        for(int y=10;y<54;y++)for(int x=10;x<62;x++) {
            DrawColor expected=tile[(y%8)*8+x%8];unsigned char *p=actual+(y*80+x)*4;
            if(p[0]!=expected.r || p[1]!=expected.g || p[2]!=expected.b){draw_texture_destroy(texture);return 0;}
        }
        if(actual[0] || actual[1]){draw_texture_destroy(texture);return 0;}
    }
    draw_transform(draw,1,0,0);draw_texture_destroy(texture);return 1;
}
int main(int argc,char **argv) {
    int render=argc>1 && !strcmp(argv[1],"--render");
    if(render) { CHECK(SDL_Init(0));surface=SDL_CreateSurface(80,64,SDL_PIXELFORMAT_RGBA32);CHECK(surface);renderer=SDL_CreateSoftwareRenderer(surface);CHECK(renderer);draw=draw_create(renderer);CHECK(draw); }
    if(render)CHECK(check_pixel_pattern());
    theme_init();
    CHECK(same(ui_theme.background,(DrawColor){9,7,15,255}));
    CHECK(contrast(ui_theme.selected_text,ui_theme.highlight)>=4.5f);
    CHECK(contrast(theme_foreground(ui_theme.active_hover),ui_theme.active_hover)>=4.5f);
    DrawColor surfaces[]={ui_theme.surface,ui_theme.browser,ui_theme.title,ui_theme.title_focus};
    for(unsigned i=0;i<sizeof surfaces/sizeof *surfaces;i++) {
        CHECK(contrast(ui_theme.text,surfaces[i])>=4.5f);
        CHECK(contrast(ui_theme.secondary,surfaces[i])>=4.5f);
    }
    CHECK(contrast(ui_theme.note,ui_theme.piano_row[0])>=3);
    CHECK(contrast(ui_theme.note,ui_theme.piano_row[1])>=3);
    CHECK(contrast(ui_theme.waveform,ui_theme.background)>=3);
    CHECK(contrast(theme_foreground(ui_theme.hover),ui_theme.hover)>=4.5f);
    CHECK(same(ui_theme.highlight,(DrawColor){179,154,221,255}));
    CHECK(same(ui_theme.hover,ui_theme.highlight));
    CHECK(contrast(ui_theme.border,ui_theme.control)>=3);
    CHECK(contrast(ui_theme.knob_track,ui_theme.control)>=3);
    float delta=fabsf(draw_color_hsv(ui_theme.pan_left).x-draw_color_hsv(ui_theme.pan_right).x);
    CHECK(fminf(delta,360-delta)>=90);
    DrawColor hints[]={ui_theme.pan_left,ui_theme.pan_right,ui_theme.stereo,ui_theme.mono};
    for(unsigned i=0;i<sizeof hints/sizeof *hints;i++) {
        DrawColor hint=draw_color_blend(ui_theme.knob_track,draw_color_fade(hints[i],.35f),((DrawColor){255,255,255,255}));
        CHECK(contrast(hint,ui_theme.control)>=3 && contrast(hint,ui_theme.surface)>=3);
    }
    if(render) { CHECK(check_render()); CHECK(check_meter()); }
    CHECK(contrast(ui_theme.fader_mark,ui_theme.fader)>=4.5f);
    CHECK(contrast(ui_theme.text,ui_theme.control)>=4.5f && contrast(ui_theme.secondary,ui_theme.control)>=4.5f);
    DrawColor indicators[]={ui_theme.pan_left,ui_theme.pan_right,ui_theme.stereo,ui_theme.mono};
    for(unsigned i=0;i<sizeof indicators/sizeof *indicators;i++) CHECK(contrast(indicators[i],ui_theme.control)>=3);
    DrawColor meters[]={ui_theme.meter_low,ui_theme.meter_mid,ui_theme.meter_high};
    for(unsigned i=0;i<sizeof meters/sizeof *meters;i++) CHECK(contrast(meters[i],ui_theme.background)>=3);
    theme_studio();
    for(int selected=0;selected<=1;selected++) {
        DrawColor previous=ui_note_velocity_color(1,selected),base=selected?ui_theme.note_drag:ui_theme.note;
        for(int velocity=2;velocity<=127;velocity++) {
            DrawColor c=ui_note_velocity_color(velocity,selected);
            CHECK(c.r>=previous.r && c.g>=previous.g && c.b>=previous.b && c.a==base.a);
            previous=c;
        }
        CHECK(same(previous,base));
        CHECK(!same(ui_note_velocity_color(1,selected),previous));
    }
    CHECK(!same(ui_note_velocity_color(64,0),ui_note_velocity_color(64,1)));
    CHECK(abs(ui_theme.title_focus.b-ui_theme.title_focus.r)<25);
    CHECK(contrast(ui_theme.text,ui_theme.control)>=4.5f);
    CHECK(same(ui_theme.surface,(DrawColor){131,136,146,255}));
    CHECK(contrast(ui_theme.text,ui_theme.surface)>=3);
    CHECK(contrast(ui_theme.signal,ui_skin.display)>=4.5f);
    if(render) {
        CHECK(check_bitmap_knobs());
        const float scales[]={1,1.25f,1.5f,2,3};
        for(unsigned i=0;i<sizeof scales/sizeof *scales;i++){CHECK(check_skin(scales[i]));CHECK(check_grips(scales[i],0));CHECK(check_grips(scales[i],1));}
        CHECK(check_meter());
    }
    if(render){draw_destroy(draw);SDL_DestroyRenderer(renderer);SDL_DestroySurface(surface);SDL_Quit();}
    puts("Fader marks, directional indicators and palette contrast passed."); return 0;
}
