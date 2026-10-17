// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_UI_RENDER_H
#define LIBRELOOP_UI_RENDER_H
#include "window.h"
typedef struct UIRender UIRender;
UIRender *ui_render_create(DrawContext *draw,const char *font_path);
void ui_render_destroy(UIRender *render);
void ui_render_view(UIRender *render,WindowGeometry geometry,DrawRect editor);
void ui_render_clip(UIRender *render,DrawRect local);
void ui_render_unclip(UIRender *render);
void ui_render_label(UIRender *render,const char *text,float x,float y,int size,DrawColor color);
int ui_render_text_width(UIRender *render,const char *text,int size);
void ui_render_fit(UIRender *render,char *out,size_t capacity,const char *text,float width,int size);
void ui_render_circle(UIRender *render,float x,float y,float radius,DrawColor color);
void ui_render_arc(UIRender *render,int bank,int level,DrawRect destination,DrawColor color);
void ui_render_arc_pro(UIRender *render,int bank,int level,DrawRect destination,DrawPoint origin,float rotation,DrawColor color);
void ui_render_icon(UIRender *render,int id,float x,float y,float size,DrawColor color);
DrawContext *ui_render_draw(UIRender *render);
DrawTexture *ui_render_cable_texture(UIRender *render);
/* Reproduce original integer scissor calls and convert GL bottom-origin to
   SDL top-origin. FBO callers pass dpi_scissor0 and their framebuffer height. */
DrawRect ui_render_scissor(WindowGeometry geometry,int x,int y,int width,int height);
#endif
