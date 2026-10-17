// SPDX-License-Identifier: GPL-3.0-only
#include "render.h"
#include "text_fonts.h"
#include "font_layout.h"
#include "widget_pixels.h"
#include "icons.h"
#include "path.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
struct UIRender {
    DrawContext *draw;TextFonts *fonts;
    DrawTexture *circle,*arcs,*cable;
    WindowGeometry geometry;DrawRect editor;
    char font_path[LL_PATH_CAPACITY];
    UIIcons icons;float icon_density;
};
static DrawTexture *upload(DrawContext *draw,WidgetPixels pixels) {
    if(!pixels.pixels)return NULL;
    DrawTexture *texture=draw_texture_create(draw,pixels.width,pixels.height,pixels.pixels,0);
    widget_pixels_free(pixels);draw_texture_filter(texture,1);return texture;
}
UIRender *ui_render_create(DrawContext *draw,const char *path) {
    UIRender *render=calloc(1,sizeof *render);if(!render)return NULL;render->draw=draw;
    snprintf(render->font_path,sizeof render->font_path,"%s",path);
    render->fonts=text_fonts_create(draw);render->circle=upload(draw,widget_circle_pixels());
    render->arcs=upload(draw,widget_arc_pixels());render->cable=upload(draw,widget_cable_pixels());
    if(!render->fonts || !render->circle || !render->arcs || !render->cable){ui_render_destroy(render);return NULL;}return render;
}
void ui_render_destroy(UIRender *render) {
    if(!render)return;
    text_fonts_close(render->fonts);icons_destroy(&render->icons);draw_texture_destroy(render->circle);draw_texture_destroy(render->arcs);draw_texture_destroy(render->cable);free(render);
}
void ui_render_view(UIRender *render,WindowGeometry geometry,DrawRect editor) {
    render->geometry=geometry;render->editor=editor;
    if(render->icon_density!=geometry.raster_scale) {
        draw_flush(render->draw);icons_destroy(&render->icons);
        if(icons_build(render->draw,geometry.raster_scale,&render->icons))render->icon_density=geometry.raster_scale;
    }
    text_fonts_update(render->fonts,render->font_path,geometry.raster_scale);
    draw_transform(render->draw,geometry.raster_scale,editor.x*geometry.raster_scale,editor.y*geometry.raster_scale);
}
DrawRect ui_render_scissor(WindowGeometry g,int x,int y,int width,int height) {
    int left=x,bottom=g.pixel_height-(y+height),w=width,h=height;
    if(g.dpi_scissor) {
        left=(int)(x*g.dpi_x);bottom=(int)(g.height*g.dpi_y-((y+height)*g.dpi_y));
        w=(int)(width*g.dpi_x);h=(int)(height*g.dpi_y);
    }
    return (DrawRect){left,g.pixel_height-bottom-h,w,h};
}
void ui_render_clip(UIRender *render,DrawRect local) {
    WindowGeometry g=render->geometry;DrawRect editor=render->editor;
    DrawRect clip=ui_render_scissor(g,(int)((editor.x+local.x)*g.layout_scale),(int)((editor.y+local.y)*g.layout_scale),(int)(local.width*g.layout_scale),(int)(local.height*g.layout_scale));
    draw_clip(render->draw,&clip);
}
void ui_render_unclip(UIRender *render){draw_clip(render->draw,NULL);}
void ui_render_label(UIRender *render,const char *text,float x,float y,int size,DrawColor color) {
    WindowGeometry g=render->geometry;DrawRect editor=render->editor;
    DrawFont *font=text_font(render->fonts,text,size);
    DrawPoint at={text_pixel_position(x,editor.x*g.layout_scale,g.layout_scale,g.raster_scale),text_pixel_position(y,editor.y*g.layout_scale,g.layout_scale,g.raster_scale)};
    draw_text(render->draw,font,text,at,draw_font_layout(font)->size/g.raster_scale,0,color);
}
int ui_render_text_width(UIRender *render,const char *text,int size){return (int)ceilf(text_font_width(render->fonts,text,size));}
void ui_render_fit(UIRender *render,char *out,size_t capacity,const char *text,float width,int size){text_fit(render->fonts,out,capacity,text,width,size);}
void ui_render_circle(UIRender *render,float x,float y,float radius,DrawColor color) {
    draw_texture(render->draw,render->circle,(DrawRect){0,0,64,64},(DrawRect){x-radius,y-radius,radius*2,radius*2},color);
}
void ui_render_arc(UIRender *render,int bank,int level,DrawRect destination,DrawColor color) {
    draw_texture(render->draw,render->arcs,(DrawRect){bank*512+level%16*32,level/16*32,32,32},destination,color);
}
void ui_render_arc_pro(UIRender *render,int bank,int level,DrawRect destination,DrawPoint origin,float rotation,DrawColor color) {
    draw_texture_pro(render->draw,render->arcs,(DrawRect){bank*512+level%16*32,level/16*32,32,32},destination,origin,rotation,color);
}
DrawContext *ui_render_draw(UIRender *render){return render->draw;}
DrawTexture *ui_render_cable_texture(UIRender *render){return render->cable;}
void ui_render_icon(UIRender *render,int id,float x,float y,float size,DrawColor color) {
    int variant=size<=16?2:size<=20?1:0,pixels=variant==2?render->icons.small:variant==1?render->icons.medium:render->icons.large;
    WindowGeometry g=render->geometry;DrawRect editor=render->editor;
    float left=text_pixel_position(x-pixels/g.raster_scale/2,editor.x*g.layout_scale,g.layout_scale,g.raster_scale);
    float top=text_pixel_position(y-pixels/g.raster_scale/2,editor.y*g.layout_scale,g.layout_scale,g.raster_scale);
    draw_texture(render->draw,render->icons.texture,(DrawRect){id*render->icons.large,variant==2?render->icons.large+render->icons.medium:variant==1?render->icons.large:0,pixels,pixels},(DrawRect){left,top,pixels/g.raster_scale,pixels/g.raster_scale},color);
}
