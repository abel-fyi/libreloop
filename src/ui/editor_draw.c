// SPDX-License-Identifier: GPL-3.0-only
/* Original ui_shell.c editor chrome; input/capture stays in its own model. */
#include "editor_draw.h"
#include "theme.h"
#include "icons.h"
#include <stdio.h>
static void rectangle_lines(DrawContext *draw,float camera_zoom,int x,int y,int width,int height,DrawColor color) {
    /* Exact pinned DrawRectangleLines geometry, whose endpoint extension is
       .5/modelview.m0, not the filled-border path. Camera is scale-only. */
    float zoom=.5f/camera_zoom;
    draw_hairline(draw,(DrawPoint){(float)x-zoom,(float)y},(DrawPoint){(float)x+(float)width+zoom,(float)y},color);
    draw_hairline(draw,(DrawPoint){(float)x+(float)width,(float)y-zoom},(DrawPoint){(float)x+(float)width,(float)y+(float)height+zoom},color);
    draw_hairline(draw,(DrawPoint){(float)x+(float)width+zoom,(float)y+(float)height},(DrawPoint){(float)x-zoom,(float)y+(float)height},color);
    draw_hairline(draw,(DrawPoint){(float)x,(float)y+(float)height+zoom},(DrawPoint){(float)x,(float)y-zoom},color);
}

void editor_draw_begin(UIRender *render,WindowGeometry geometry,const EditorDrawing *state) {
    const Windows *windows=state->windows;const Editor *e=&windows->editors[state->id];Rect r=e->rect;int top=EDITORS-1;
    while(top>0 && !windows->editors[windows->order[top]].visible)top--;
    int focused=(windows->editors[windows->focused].visible?windows->focused:windows->order[top])==state->id;
    ui_render_view(render,geometry,(DrawRect){r.x,r.y,r.w,r.h});ui_render_clip(render,(DrawRect){0,0,r.w,r.h});DrawContext *draw=ui_render_draw(render);
    draw_rect(draw,(DrawRect){0,0,r.w,r.h},state->id==0?ui_theme.rack:state->id==3?ui_theme.mixer:ui_theme.surface);
    draw_rect(draw,(DrawRect){0,0,r.w,TITLE},focused?ui_theme.title_focus:ui_theme.title);
    static const char *titles[]={"Channel Rack","Arrangement","Piano Roll","Mixer","Sampler","Equalizer"};char title[128];
    snprintf(title,sizeof title,"%s%s",state->id==4 && state->fm_instrument?"FM Synth":titles[state->id],e->pinned?" (on top)":"");ui_render_label(render,title,8,3,13,ui_theme.text);
    int hover=state->hover_slot;
    if(hover>=0)draw_rect(draw,(DrawRect){(int)(r.w-54+hover*18),1,17,TITLE-2},ui_theme.hover);
    DrawColor hover_ink=theme_foreground(ui_theme.hover);
    draw_line(draw,(DrawPoint){r.w-49,9},(DrawPoint){r.w-41,9},2,hover==0?hover_ink:ui_theme.text);
    int mx=r.w-27;DrawColor maximize=hover==1?hover_ink:ui_theme.text;
    if(e->maximized){rectangle_lines(draw,geometry.layout_scale,mx-2,4,7,7,maximize);draw_rect(draw,(DrawRect){mx-4,6,7,7},hover==1?ui_theme.hover:ui_theme.control);rectangle_lines(draw,geometry.layout_scale,mx-4,6,7,7,maximize);}
    else rectangle_lines(draw,geometry.layout_scale,mx-4,5,8,8,maximize);
    ui_render_icon(render,ICON_CLOSE,(int)(r.w-9),9,16,hover==2?hover_ink:ui_theme.text);
}
void editor_draw_end(UIRender *render,const EditorDrawing *state) {
    Rect r=state->windows->editors[state->id].rect;DrawContext *draw=ui_render_draw(render);
    draw_hairline(draw,(DrawPoint){(int)(r.w-9),(int)(r.h-2)},(DrawPoint){(int)(r.w-2),(int)(r.h-9)},ui_theme.secondary);ui_render_unclip(render);
}
