// SPDX-License-Identifier: GPL-3.0-only
/* Original ui_widgets.c icon geometry; integer DrawRectangle/DrawCircle inputs
   are preserved. Circle tessellation follows pinned rshapes.c (zlib notice). */
#include "icons.h"
#include "shapes.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "classic_icons.h"
#define PI 3.14159265358979323846f
#define DEG2RAD (PI/180.0f)
static void rectangle(DrawContext *draw,int x,int y,int width,int height,DrawColor color){draw_rect(draw,(DrawRect){x,y,width,height},color);}
static void triangle(DrawContext *draw,DrawPoint a,DrawPoint b,DrawPoint c,DrawColor color){DrawVertex v[]={{a,0,0,color},{b,0,0,color},{c,0,0,color}};draw_triangles(draw,NULL,v,3);}
void icon_shapes(DrawContext *draw,int id) {
        if(id==ICON_RACK) for(int row=0;row<3;row++) {
            rectangle(draw,5,7+row*6,4,3,((DrawColor){255,255,255,255}));
            for(int step=0;step<3;step++) rectangle(draw,12+step*4,7+row*6,3,3,((DrawColor){255,255,255,255}));
        }
        if(id==ICON_PLAYLIST) for(int row=0;row<3;row++) {
            draw_line(draw,(DrawPoint){5,8+row*6},(DrawPoint){23,8+row*6},1,draw_color_fade(((DrawColor){255,255,255,255}),.45f));
            rectangle(draw,5+(row%2)*7,6+row*6,10,4,((DrawColor){255,255,255,255}));
        }
        if(id==ICON_PIANO) {
            draw_border(draw,(DrawRect){4,6,20,16},1.5f,((DrawColor){255,255,255,255}));
            for(int key=1;key<4;key++) draw_line(draw,(DrawPoint){4+key*5,6},(DrawPoint){4+key*5,22},1,((DrawColor){255,255,255,255}));
            for(int key=0;key<2;key++) rectangle(draw,7+key*5,6,4,9,((DrawColor){255,255,255,255}));
            rectangle(draw,21,6,3,9,((DrawColor){255,255,255,255})); /* Keep the partial F-sharp distinct from the border at small sizes. */
        }
        if(id==ICON_MIXER) for(int strip=0;strip<3;strip++) {
            int x=7+strip*7,y=9+strip*4;
            draw_line(draw,(DrawPoint){x,5},(DrawPoint){x,23},1.5f,((DrawColor){255,255,255,255}));
            rectangle(draw,x-3,y-2,6,4,((DrawColor){255,255,255,255}));
        }
        if(id==ICON_PENCIL) {
            draw_line(draw,(DrawPoint){8,20},(DrawPoint){21,7},3.5f,((DrawColor){255,255,255,255}));
            triangle(draw,(DrawPoint){5,23},(DrawPoint){10,21},(DrawPoint){7,18},((DrawColor){255,255,255,255}));
        }
        if(id==ICON_BRUSH) {
            draw_line(draw,(DrawPoint){14,14},(DrawPoint){21,6},3,((DrawColor){255,255,255,255}));
            shape_circle(draw,11,17,4,((DrawColor){255,255,255,255}));
            triangle(draw,(DrawPoint){5,23},(DrawPoint){13,21},(DrawPoint){8,15},((DrawColor){255,255,255,255}));
        }
        if(id==ICON_SELECT) for(int i=0;i<3;i++) {
            int p=6+i*6;
            draw_line(draw,(DrawPoint){p,6},(DrawPoint){p+3,6},1.5f,((DrawColor){255,255,255,255}));
            draw_line(draw,(DrawPoint){p,22},(DrawPoint){p+3,22},1.5f,((DrawColor){255,255,255,255}));
            draw_line(draw,(DrawPoint){6,p},(DrawPoint){6,p+3},1.5f,((DrawColor){255,255,255,255}));
            draw_line(draw,(DrawPoint){22,p},(DrawPoint){22,p+3},1.5f,((DrawColor){255,255,255,255}));
        }
        if(id==ICON_CUT) {
            shape_circle_lines(draw,8,19,4,((DrawColor){255,255,255,255})); shape_circle_lines(draw,20,19,4,((DrawColor){255,255,255,255}));
            draw_line(draw,(DrawPoint){10,16},(DrawPoint){22,5},2,((DrawColor){255,255,255,255}));
            draw_line(draw,(DrawPoint){18,16},(DrawPoint){6,5},2,((DrawColor){255,255,255,255}));
            shape_circle(draw,14,12,1.5f,((DrawColor){255,255,255,255}));
        }
        if(id==ICON_STRETCH) {
            draw_line(draw,(DrawPoint){5,14},(DrawPoint){23,14},2,((DrawColor){255,255,255,255}));
            draw_line(draw,(DrawPoint){5,14},(DrawPoint){10,9},2,((DrawColor){255,255,255,255}));
            draw_line(draw,(DrawPoint){5,14},(DrawPoint){10,19},2,((DrawColor){255,255,255,255}));
            draw_line(draw,(DrawPoint){23,14},(DrawPoint){18,9},2,((DrawColor){255,255,255,255}));
            draw_line(draw,(DrawPoint){23,14},(DrawPoint){18,19},2,((DrawColor){255,255,255,255}));
        }
        if(id==ICON_ROW_ZOOM) {
            draw_line(draw,(DrawPoint){14,5},(DrawPoint){14,23},1.75f,((DrawColor){255,255,255,255}));
            draw_line(draw,(DrawPoint){9,10},(DrawPoint){14,5},1.75f,((DrawColor){255,255,255,255}));
            draw_line(draw,(DrawPoint){19,10},(DrawPoint){14,5},1.75f,((DrawColor){255,255,255,255}));
            draw_line(draw,(DrawPoint){9,18},(DrawPoint){14,23},1.75f,((DrawColor){255,255,255,255}));
            draw_line(draw,(DrawPoint){19,18},(DrawPoint){14,23},1.75f,((DrawColor){255,255,255,255}));
        }
        if(id==ICON_METRO) {
            draw_line(draw,(DrawPoint){7,22},(DrawPoint){11,6},1.8f,((DrawColor){255,255,255,255}));
            draw_line(draw,(DrawPoint){11,6},(DrawPoint){16,22},1.8f,((DrawColor){255,255,255,255}));
            draw_line(draw,(DrawPoint){7,22},(DrawPoint){16,22},1.8f,((DrawColor){255,255,255,255}));
            draw_line(draw,(DrawPoint){12,19},(DrawPoint){21,8},1.8f,((DrawColor){255,255,255,255})); shape_circle(draw,21,8,2,((DrawColor){255,255,255,255}));
        }
        if(id==ICON_FOLLOW) {
            draw_line(draw,(DrawPoint){7,5},(DrawPoint){7,23},1.5f,((DrawColor){255,255,255,255}));
            draw_line(draw,(DrawPoint){12,14},(DrawPoint){23,14},1.8f,((DrawColor){255,255,255,255}));
            draw_line(draw,(DrawPoint){18,9},(DrawPoint){23,14},1.8f,((DrawColor){255,255,255,255}));
            draw_line(draw,(DrawPoint){18,19},(DrawPoint){23,14},1.8f,((DrawColor){255,255,255,255}));
        }
        if(id==ICON_KEYS) {
            draw_border(draw,(DrawRect){3,7,22,14},1.5f,((DrawColor){255,255,255,255}));
            for(int row=0;row<2;row++) for(int key=0;key<5;key++) rectangle(draw,6+key*3.5f,10+row*4,2,2,((DrawColor){255,255,255,255}));
            draw_rect(draw,(DrawRect){9,18,10,1.5f},((DrawColor){255,255,255,255}));
        }
        if(id==ICON_WAVE) {
            DrawPoint previous={4,14};
            for(int step=1;step<=48;step++) {
                float phase=step/48.f;
                DrawPoint next={4+20*phase,14-7*sinf(phase*2*PI)};
                draw_line(draw,previous,next,2,((DrawColor){255,255,255,255})); previous=next;
            }
        }
        if(id==ICON_AUTOMATION) { draw_line(draw,(DrawPoint){5,21},(DrawPoint){14,7},2,((DrawColor){255,255,255,255})); draw_line(draw,(DrawPoint){14,7},(DrawPoint){23,17},2,((DrawColor){255,255,255,255})); shape_circle(draw,5,21,3,((DrawColor){255,255,255,255})); shape_circle(draw,14,7,3,((DrawColor){255,255,255,255})); shape_circle(draw,23,17,3,((DrawColor){255,255,255,255})); }
        if(id==ICON_PLAY) triangle(draw,(DrawPoint){9,6},(DrawPoint){9,22},(DrawPoint){22,14},((DrawColor){255,255,255,255}));
        if(id==ICON_BACK) triangle(draw,(DrawPoint){19,6},(DrawPoint){6,14},(DrawPoint){19,22},((DrawColor){255,255,255,255}));
        if(id==ICON_STOP) rectangle(draw,7,7,14,14,((DrawColor){255,255,255,255}));
        if(id==ICON_PAUSE) { rectangle(draw,7,6,5,16,((DrawColor){255,255,255,255})); rectangle(draw,16,6,5,16,((DrawColor){255,255,255,255})); }
        if(id==ICON_CLOSE) {
            draw_line(draw,(DrawPoint){7,7},(DrawPoint){21,21},2,((DrawColor){255,255,255,255}));
            draw_line(draw,(DrawPoint){7,21},(DrawPoint){21,7},2,((DrawColor){255,255,255,255}));
        }
}
WidgetPixels icons_pack(const DrawColor *input,int high,int large,int medium,int small) {
    WidgetPixels image={ICON_COUNT*large,large+medium+small,NULL};
    image.pixels=calloc((size_t)image.width*image.height,sizeof *image.pixels);if(!image.pixels)return (WidgetPixels){0};
    int source_width=ICON_COUNT*high;DrawColor *output=image.pixels;
    for(int variant=0;variant<3;variant++) {
        int size=variant==2?small:variant==1?medium:large,row=variant==2?large+medium:variant==1?large:0;
        float ratio=high/(float)size;
        for(int id=0;id<ICON_COUNT;id++)for(int y=0;y<size;y++)for(int x=0;x<size;x++) {
            float left=x*ratio,top=y*ratio,right=(x+1)*ratio,bottom=(y+1)*ratio,alpha=0;
            for(int sy=floorf(top);sy<ceilf(bottom) && sy<high;sy++)for(int sx=floorf(left);sx<ceilf(right) && sx<high;sx++) {
                float weight=(fminf(right,sx+1)-fmaxf(left,sx))*(fminf(bottom,sy+1)-fmaxf(top,sy));
                alpha+=input[sy*source_width+id*high+sx].a*weight;
            }
            output[(row+y)*image.width+id*large+x]=(DrawColor){255,255,255,roundf(alpha/(ratio*ratio))};
        }
    }
    return image;
}
int icons_build(DrawContext *draw,float scale,UIIcons *icons) {
    *icons=(UIIcons){0};int large=fmaxf(1,roundf(28*scale)),small=fmaxf(1,roundf(16*scale)),medium=fmaxf(1,roundf(20*scale)),high=large*4;
    DrawTexture *target=draw_texture_create(draw,ICON_COUNT*high,high,NULL,1);if(!target)return 0;
    DrawColor *pixels=malloc((size_t)ICON_COUNT*high*high*sizeof *pixels);if(!pixels){draw_texture_destroy(target);return 0;}
    draw_target(draw,target);draw_clip(draw,NULL);draw_clear(draw,(DrawColor){255,255,255,0});
    for(int id=0;id<ICON_COUNT;id++){draw_transform(draw,high/28.f,id*high,0);icon_shapes(draw,id);}
    int ok=draw_read_rgba(draw,pixels,ICON_COUNT*high,high);draw_target(draw,NULL);draw_texture_destroy(target);
    if(!ok){free(pixels);return 0;}
    WidgetPixels packed=icons_pack(pixels,high,large,medium,small);free(pixels);if(!packed.pixels)return 0;
    icons->texture=draw_texture_create(draw,packed.width,packed.height,packed.pixels,0);widget_pixels_free(packed);
    if(!icons->texture)return 0;draw_texture_filter(icons->texture,0);icons->large=large;icons->medium=medium;icons->small=small;return 1;
}
/* Fixed, authored sprites. No image decoder, render target or readback needed. */
int icons_build_classic(DrawContext *draw,float scale,UIIcons *icons) {
    *icons=(UIIcons){0};
    int large=fmaxf(1,roundf(28*scale)),medium=fmaxf(1,roundf(20*scale)),small=fmaxf(1,roundf(16*scale));
    int width=ICON_CLASSIC_COUNT*large,height=large+medium+small;
    DrawColor *pixels=calloc((size_t)width*height,sizeof *pixels);if(!pixels)return 0;
    const char *codes=".#DWwsBbGgYyRr";
    const DrawColor palette[]={
        {0,0,0,0},{24,24,24,255},{80,80,80,255},{255,255,255,255},
        {224,224,224,255},{192,192,192,255},{114,179,239,255},{33,83,152,255},
        {143,229,114,255},{32,128,64,255},{255,241,140,255},{192,144,32,255},
        {255,143,114,255},{176,48,32,255}};
    for(int v=0;v<3;v++) {
        int size=v==0?large:v==1?medium:small,row=v==0?0:v==1?large:large+medium;
        /* A 16px glyph has 2px breathing room inside a 20px toolbar cell. */
        int glyph=fmaxf(1,roundf((v==0?24:16)*scale)),offset=(size-glyph)/2;
        for(int id=0;id<ICON_CLASSIC_COUNT;id++)for(int y=0;y<glyph;y++)for(int x=0;x<glyph;x++) {
            char code=classic_icon_pixels[id][y*16/glyph][x*16/glyph];int index=0;
            while(codes[index] && codes[index]!=code)index++;
            if(codes[index])pixels[(row+offset+y)*width+id*large+offset+x]=palette[index];
        }
    }
    icons->texture=draw_texture_create(draw,width,height,pixels,0);free(pixels);
    if(!icons->texture)return 0;
    draw_texture_filter(icons->texture,0);icons->large=large;icons->medium=medium;icons->small=small;return 1;
}
void icons_destroy(UIIcons *icons){draw_texture_destroy(icons->texture);*icons=(UIIcons){0};}

/* Only action captions map to pictograms. Data names, values and choices stay
   readable. The frontend retains the caption for hover/keyboard descriptions. */
int icon_action(const char *caption) {
    static const struct { const char *caption;int icon; } actions[]={
        {"DX7",ICON_KEYS},{"Analog",ICON_WAVE},{"Custom FM",ICON_SETTINGS},{"Oscs",ICON_WAVE},
        {"FILE",ICON_FILE},
        {"EDIT",ICON_PENCIL},
        {"VIEW",ICON_VIEW},
        {"HELP",ICON_HELP},
        {"Keybindings",ICON_HELP},
        {"New",ICON_NEW},
        {"Demo",ICON_PLAY},
        {"Save",ICON_SAVE},
        {"Save As...",ICON_SAVE},
        {"Save preset",ICON_SAVE},
        {"Open...",ICON_FILE},
        {"Open",ICON_FILE},
        {"Load",ICON_FILE},
        {"Load preset",ICON_FILE},
        {"Export...",ICON_WAVE},
        {"Export",ICON_WAVE},
        {"Collect samples and save...",ICON_LIBRARY},
        {"Undo",ICON_UNDO},
        {"Redo",ICON_REDO},
        {"Cut",ICON_CUT},
        {"Copy",ICON_COPY},
        {"Paste",ICON_PASTE},
        {"Select all",ICON_SELECT},
        {"Octave up",ICON_UP},
        {"Octave down",ICON_ROW_ZOOM},
        {"Arrange",ICON_PLAYLIST},
        {"Pattern",ICON_PIANO},
        {"Back to Arrangement",ICON_PLAYLIST},
        {"Back to Rack",ICON_RACK},
        {"Channel Rack",ICON_RACK},
        {"Go to Piano Roll",ICON_PIANO},
        {"Mixer (collapsed)  ^",ICON_MIXER},
        {"+ Add channel",ICON_ADD},
        {"+ Add folder",ICON_FILE},
        {"+",ICON_ADD},
        {"-",ICON_DELETE},
        {"...",ICON_SETTINGS},
        {"Fill X...",ICON_RACK},
        {"Fill every X steps...",ICON_RACK},
        {"Factory",ICON_LIBRARY},
        {"Voice",ICON_SETTINGS},
        {"Envelope",ICON_AUTOMATION},
        {"Sound",ICON_SPEAKER},{"Body",ICON_WAVE},{"Attack",ICON_PENCIL},
        {"Amplitude",ICON_AUTOMATION},{"Filter",ICON_MIXER},
        {"Audio",ICON_WAVE},{"Notes",ICON_PIANO},{"Automation",ICON_AUTOMATION},
        {"Keyboard",ICON_KEYS},
        {"Ratio",ICON_FX},
        {"Hz at C4",ICON_WAVE},
        {"Global",ICON_SETTINGS},
        {"Motion",ICON_WAVE},
        {"Pitch",ICON_ROW_ZOOM},
        {"Pitch envelope",ICON_AUTOMATION},
        {"Volume",ICON_MIXER},
        {"Play C4",ICON_PLAY},
        {"Release C4",ICON_STOP},
        {"Apply",ICON_CHECK},
        {"Cancel",ICON_CLOSE},
        {"Discard",ICON_DELETE},
        {"Reset",ICON_RESET},
        {"Reset Master volume",ICON_RESET},
        {"Reset mixer channel",ICON_RESET},
        {"Enter value",ICON_PENCIL},
        {"Rename",ICON_PENCIL},
        {"Delete",ICON_DELETE},
        {"Delete channel",ICON_DELETE},
        {"Close editor",ICON_CLOSE},
        {"Hide window",ICON_CLOSE},
        {"Replace",ICON_CHECK},
        {"Replace instrument...",ICON_SETTINGS},
        {"Relink sample...",ICON_FILE},
        {"Replace with sample...",ICON_FILE},
        {"Color",ICON_BRUSH},
        {"Up",ICON_UP},
        {"^",ICON_UP},
        {"v",ICON_ROW_ZOOM},
        {"Home",ICON_HOME},
        {"Back",ICON_BACK},
        {"Hidden",ICON_VIEW},
        {"MIDI / Recording",ICON_KEYS},
        {"Refresh inputs",ICON_RESET},
        {"Clear MIDI link",ICON_DELETE},
        {"Clear MIDI links",ICON_DELETE},
        {"MIDI Learn",ICON_KEYS},
        {"Create automation",ICON_AUTOMATION},
        {"Bypass",ICON_FOLLOW},
        {"Add Chorus",ICON_FX},
        {"Add Equalizer",ICON_MIXER},
        {"+ Chorus",ICON_FX},
        {"+ Equalizer",ICON_MIXER},
        {"Open effect",ICON_FX},
        {"Open Equalizer",ICON_MIXER},
        {"Remove effect",ICON_DELETE},
        {"Mute channel",ICON_SPEAKER},
        {"Mute insert",ICON_SPEAKER},
        {"Solo insert",ICON_SOLO},
        {"M",ICON_SPEAKER},
        {"S",ICON_SOLO},
        {"Stay on top",ICON_UP},
        {"Dark",ICON_VIEW},
        {"Light",ICON_VIEW},
    };
    if(!caption)return -1;
    for(unsigned i=0;i<sizeof actions/sizeof *actions;i++)
        if(!strcmp(caption,actions[i].caption))return actions[i].icon;
    if(!strncmp(caption,"Count-in:",9))return ICON_METRO;
    if(!strncmp(caption,"Hits:",5))return ICON_RACK;
    return -1;
}
