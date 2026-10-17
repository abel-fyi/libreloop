// SPDX-License-Identifier: GPL-3.0-only
/* Draw-only extraction of frozen ui_piano.c plus timeline drawing. */
#include "piano_draw.h"
#include "theme.h"
#include "icons.h"
#include <math.h>
#include <stdio.h>
static void grid(DrawContext *draw,float start,float span,float gx,float y,float width,float height) {
    float pixels=width/span;TimelineGrid grid=timeline_grid_layout(pixels);float band=STEPS*4;
    if(grid.band_alpha>0)for(int i=0;i<span/band+2;i++) {
        float step=(floorf(start/band)+i)*band;if(step<0 || fmodf(step/band,2)==0)continue;
        float left=fmaxf(gx,gx+(step-start)*pixels),right=fminf(gx+width,gx+(step+band-start)*pixels);
        if(right>left)draw_rect(draw,(DrawRect){left,y,right-left,height},draw_color_fade((DrawColor){0,0,0,255},grid.band_alpha));
    }
    if(!grid.lines)return;
    for(int i=0;i<span/grid.lines+2;i++) {
        float step=(floorf(start/grid.lines)+i)*grid.lines,x=gx+(step-start)*pixels;
        int bar=fmodf(step,STEPS)==0,beat=fmodf(step,4)==0;
        if(step>=0 && x>=gx && x<=gx+width)draw_line(draw,(DrawPoint){x,y},(DrawPoint){x,y+height},1,bar?ui_theme.grid_major:draw_color_fade(ui_theme.grid_minor,beat?1:.65f));
    }
}
static void ruler(UIRender *render,const PianoDrawing *s,float gx,float y,float width) {
    DrawContext *draw=ui_render_draw(render);float start=s->view->pan,span=s->view->span,pixels=width/span;
    draw_rect(draw,(DrawRect){(int)gx,(int)y,(int)width,14},ui_theme.browser);
    if(s->loop_end>s->loop_start) {
        float left=fmaxf(gx,gx+(s->loop_start-start)*pixels),right=fminf(gx+width,gx+(s->loop_end-start)*pixels);
        if(right>left)draw_rect(draw,(DrawRect){left,y,right-left,14},draw_color_fade(ui_theme.loop,.6f));
    }
    float spacing=timeline_grid_layout(pixels).labels;char text[128];snprintf(text,sizeof text,"%.0f",fmaxf(0,start+span)/STEPS+1);
    float label_width=ui_render_text_width(render,text,11)+8;
    while(spacing*pixels<label_width && spacing<1e30f)spacing*=2;
    for(int i=0;i<width/(spacing*pixels)+2;i++) {
        float step=(floorf(start/spacing)+i)*spacing,x=gx+(step-start)*pixels;
        if(step>=0 && x>=gx && x<gx+width) {
            snprintf(text,sizeof text,"%.0f",step/STEPS+1);int major=fmodf(step,STEPS*4)==0,size=major?11:10;
            if(x+3+ui_render_text_width(render,text,size)<=gx+width)ui_render_label(render,text,x+3,y+1,size,major?ui_theme.text:ui_theme.secondary);
        }
    }
    float x=gx+(s->playback_start-start)*pixels;
    if(x>=gx && x<=gx+width) {
        DrawColor c=s->marker_drag?ui_theme.text:ui_theme.signal;
        DrawVertex v[]={{{x-5,y+1},0,0,c},{{x,y+12},0,0,c},{{x+5,y+1},0,0,c}};draw_triangles(draw,NULL,v,3);
    }
}
static int active(const PianoDrawing *s,int pitch) {
    return (s->keyboard_notes && s->keyboard_notes[pitch]) || (s->playing_keys && s->playing_keys[pitch]) || (s->keys->pitch==pitch && s->keys->channel==s->gesture->channel);
}
void piano_draw_toolbar(UIRender *render,const PianoDrawing *s) {
    DrawContext *draw=ui_render_draw(render);
    for(int i=0;i<3;i++) {
        int x=8+i*24,over=s->hovered_tool==i;
        DrawColor surface=s->tool==i?ui_theme.highlight:over?ui_theme.hover:ui_theme.control;
        DrawColor color=s->tool==i || over?theme_foreground(surface):ui_theme.text;
        draw_rect(draw,(DrawRect){x,28,20,20},surface);
        ui_render_icon(render,i==PIANO_TOOL_PENCIL?ICON_PENCIL:i==PIANO_TOOL_BRUSH?ICON_BRUSH:ICON_SELECT,x+10,38,20,color);
    }
    char caption[128],fitted[128];snprintf(caption,sizeof caption,"%s / %s",s->project->channel_names[s->gesture->channel],s->project->pattern_names[s->gesture->pattern]);
    ui_render_fit(render,fitted,sizeof fitted,caption,s->view->width-158,12);ui_render_label(render,fitted,88,30,12,ui_theme.secondary);
}
void piano_draw_content(UIRender *render,const PianoDrawing *s) {
    DrawContext *draw=ui_render_draw(render);const PianoViewport *v=s->view;
    int gx=60,gy=86,pattern=s->gesture->pattern,channel=s->gesture->channel;
    float gridw=v->width-84,rows=piano_rows(v),rh=(v->height-180)/rows,cw=gridw/v->span;
    float view=v->pan,end=s->project->pattern_steps[pattern],partial=ceilf(end/STEPS)*STEPS;
    draw_rect(draw,(DrawRect){4,gy,gx-6,rh*rows},ui_theme.piano_white);ui_render_clip(render,(DrawRect){4,gy,54,v->height-180});
    for(int black=0;black<2;black++)for(int row=-1;row<=(int)ceilf(rows);row++) {
        int pitch=v->top-row;if(pitch<0 || pitch>127 || piano_is_black_key(pitch)!=black)continue;
        PianoBounds bounds=piano_key_bounds(v,pitch);DrawRect key={bounds.x,bounds.y,bounds.width,bounds.height};
        float bottom=fminf(gy+rh*rows,key.y+key.height);key.y=fmaxf(gy,key.y);key.height=fmaxf(0,bottom-key.y);if(key.height<=0)continue;
        int on=active(s,pitch);DrawColor base=black?ui_theme.piano_black:pitch%12==0?ui_theme.piano_c:ui_theme.piano_white;
        draw_rect(draw,key,on?ui_theme.highlight:base);
        if(!on && s->hovered_key==pitch)draw_rect(draw,key,draw_color_fade(ui_theme.highlight,.22f));
        if(!black)draw_line(draw,(DrawPoint){key.x,key.y+key.height},(DrawPoint){key.x+key.width,key.y+key.height},1,draw_color_fade(ui_theme.piano_black,.15f));
        if(pitch%12==0) {
            char name[32];snprintf(name,sizeof name,"C%d",pitch/12-1);
            ui_render_label(render,name,gx-5-ui_render_text_width(render,name,10),fmaxf(gy,fminf(gy+rh*rows-10,gy+(row+.5f)*rh-5)),10,on?ui_theme.selected_text:ui_theme.piano_black);
        }
    }
    ui_render_clip(render,(DrawRect){gx,gy,gridw,rh*rows});
    for(int row=0;row<(int)ceilf(rows);row++) {
        int pc=(v->top-row)%12,black=pc==1 || pc==3 || pc==6 || pc==8 || pc==10;float y=gy+row*rh;
        draw_rect(draw,(DrawRect){gx,y,gridw,rh},ui_theme.piano_row[black]);int pitch=v->top-row;
        if(pitch>=0 && pitch<=127 && active(s,pitch))draw_rect(draw,(DrawRect){gx,y,gridw,rh},draw_color_fade(ui_theme.highlight,.15f));
        draw_line(draw,(DrawPoint){gx,y},(DrawPoint){gx+gridw,y},1,draw_color_fade(ui_theme.grid_minor,.45f));
        if(partial>end){float left=fmaxf(gx,gx+(end-view)*cw),right=fminf(gx+gridw,gx+(partial-view)*cw);if(right>left)draw_rect(draw,(DrawRect){left,y,right-left,rh-1},ui_theme.disabled);}
        if(pc==0)draw_line(draw,(DrawPoint){gx,y+rh},(DrawPoint){gx+gridw,y+rh},1,ui_theme.grid_major);
    }
    grid(draw,view,v->span,gx,gy,gridw,rh*rows);
    for(int i=0;i<NOTES;i++) {
        const Note *n=&s->project->notes[pattern][channel][i];
        if(!n->velocity || n->pitch<v->top-(int)ceilf(rows)+1 || n->pitch>v->top || n->start>=end || n->start+(n->length?n->length:1)<=view)continue;
        float note_end=fminf(n->start+(n->length?n->length:1),end),x=gx+(n->start-view)*cw,y=gy+(v->top-n->pitch)*rh,w=(note_end-n->start)*cw;
        float left=fmaxf(gx,x),right=fminf(gx+gridw,x+w);if(right<=left)continue;
        float inset=fminf(1,rh*.15f),note_height=rh-2*inset;int selected=s->gesture->selected[i];
        if(selected)draw_border(draw,(DrawRect){left,y,right-left,rh},fminf(1,rh*.25f),ui_theme.text);
        DrawRect body={left+1,y+inset,fmaxf(1,right-left-2),note_height};
        draw_rect(draw,body,n==s->gesture->drag || selected?ui_theme.note_drag:ui_theme.note);
        draw_border(draw,body,fminf(1,rh*.25f),draw_color_fade(ui_theme.signal,.45f));
        if(x+w>=gx && x+w<=gx+gridw)draw_rect(draw,(DrawRect){x+w-5,y+inset,3,note_height},(s->edges && s->edges[i]) || (n==s->gesture->drag && !s->gesture->moving)?ui_theme.text:draw_color_fade(ui_theme.background,.35f));
    }
    if(s->gesture->kind==PIANO_BOX) {
        float left=fminf(s->gesture->from.x,s->gesture->now.x),high=fmaxf(s->gesture->from.y,s->gesture->now.y);
        DrawRect box={gx+(left-view)*cw,gy+(v->top-high)*rh,fabsf(s->gesture->from.x-s->gesture->now.x)*cw,fabsf(s->gesture->from.y-s->gesture->now.y)*rh};
        draw_rect(draw,box,draw_color_fade(ui_theme.signal,.15f));draw_border(draw,box,1,ui_theme.signal);
    }
    ui_render_clip(render,(DrawRect){0,0,v->width,v->height});int vy=v->height-84;ui_render_label(render,"Velocity",6,vy+4,10,ui_theme.secondary);
    ui_render_clip(render,(DrawRect){gx,vy,gridw,62});draw_rect(draw,(DrawRect){gx,vy,(int)gridw,62},ui_theme.piano_row[0]);
    for(int line=1;line<5;line++)draw_hairline(draw,(DrawPoint){gx,vy+line*62/5},(DrawPoint){(int)(gx+gridw),vy+line*62/5},draw_color_fade(ui_theme.grid_minor,.6f));
    grid(draw,view,v->span,gx,vy,gridw,62);
    if(partial>end){float left=fmaxf(gx,gx+(end-view)*cw),right=fminf(gx+gridw,gx+(partial-view)*cw);if(right>left)draw_rect(draw,(DrawRect){(int)left,vy,(int)(right-left),62},ui_theme.disabled);}
    for(int i=0;i<NOTES;i++) {
        Note n=s->project->notes[pattern][channel][i];if(!n.velocity || n.start>=end)continue;
        float x=gx+(n.start-view)*cw;if(x<gx-10 || x>gx+gridw)continue;
        float top=vy+62-n.velocity*.48f;DrawColor color=s->gesture->selected[i]?ui_theme.note_drag:ui_theme.note;
        draw_rect(draw,(DrawRect){x,top,2,n.velocity*.48f},color);ui_render_circle(render,x+1,top,2.5f,color);ui_render_circle(render,x+1,top,1.25f,ui_theme.piano_row[0]);
    }
    if(s->velocity->ramp==1 && s->velocity->pattern==pattern && s->velocity->channel==channel)
        draw_line(draw,(DrawPoint){gx+(s->velocity->from.x-view)*cw,vy+62-s->velocity->from.y*.48f},(DrawPoint){gx+(s->velocity->now.x-view)*cw,vy+62-s->velocity->now.y*.48f},1,ui_theme.highlight);
    ui_render_clip(render,(DrawRect){0,0,v->width,v->height});
    float rail_top=gy-14,varea=rh*rows+14,vthumb=varea*rows/128,vypos=rail_top+(127-v->top)/128.f*varea;
    draw_rect(draw,(DrawRect){v->width-4-(16+16)/2,rail_top,16,varea},ui_theme.background);
    draw_rect(draw,(DrawRect){v->width-4-(16+16)/2,vypos,16,vthumb},s->bars->vertical || s->hover_vertical?ui_theme.highlight:ui_theme.secondary);
    float rail_width=v->width-4-16-gx,thumb=timeline_thumb(rail_width,v->span,s->range),travel=rail_width-thumb,maximum=s->range-v->span,x=gx+(maximum>0?fmaxf(0,v->pan)/maximum*travel:0);
    draw_rect(draw,(DrawRect){gx,gy-14-(16+16)/2,rail_width,16},ui_theme.background);
    draw_rect(draw,(DrawRect){x,gy-14-(16+16)/2,thumb,16},s->bars->horizontal || s->hover_horizontal?ui_theme.highlight:ui_theme.secondary);
    float corner_x=v->width-4-16,corner_y=gy-14-16;
    DrawColor corner=s->row_zoom_drag?(s->row_zoom_hover?ui_theme.active_hover:ui_theme.highlight):s->row_zoom_hover?ui_theme.hover:ui_theme.control;
    draw_rect(draw,(DrawRect){corner_x,corner_y,16,16},corner);
    ui_render_icon(render,ICON_ROW_ZOOM,corner_x+8,corner_y+8,16,s->row_zoom_drag || s->row_zoom_hover?theme_foreground(corner):ui_theme.secondary);
    ruler(render,s,gx,gy-14,gridw);
    if(s->playback>=0){float x=gx+(s->playback-view)*cw;if(x>=gx && x<=gx+gridw)draw_line(draw,(DrawPoint){x,gy},(DrawPoint){x,gy+rh*rows},1.5f,ui_theme.signal);}
}
