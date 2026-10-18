// SPDX-License-Identifier: GPL-3.0-only
#include "theme.h"
#include "signal_palette.h"
#include <math.h>

Theme ui_theme;
DesktopSkin ui_skin;
static int classic_style;
static DrawTexture *skin_patterns,*skin_knobs;
static const unsigned char skin_bayer[4][4]={{0,8,2,10},{12,4,14,6},{3,11,1,9},{15,7,13,5}};
static const int skin_knob_radii[]={8,9,10,12};
int theme_draw_init(DrawContext *draw) {
    if(skin_patterns && skin_knobs)return 1;
    /* Left tile: ordered two-tone face dither. Right: cyan one-pixel dots.
       Generated once, nearest filtered, reused at logical UI scale. */
    DrawColor pixels[128*64]={0},knobs[128*32]={0};
    for(int y=0;y<64;y++)for(int x=0;x<128;x++) {
        int order=skin_bayer[y&3][x&3];
        if(x<64)pixels[y*128+x]=order<8?(DrawColor){0,0,0,18}:(DrawColor){255,255,255,18};
        else pixels[y*128+x]=!(x%4) && !(y%4)?(DrawColor){0,146,207,48}:(DrawColor){0,0,0,0};
    }
    /* Small pixel-shaded faces use their own compact atlas. Angle/pointer and
       interaction remain continuous; only the decorative body is a bitmap. */
    for(int k=0;k<4;k++)for(int y=0;y<32;y++)for(int x=0;x<32;x++) {
        float radius=skin_knob_radii[k],dx=x+.5f-14,dy=y+.5f-14;
        float distance=sqrtf(dx*dx+dy*dy),shadow=sqrtf((dx-2)*(dx-2)+(dy-3)*(dy-3));
        DrawColor c={0,0,0,(int)(145*fmaxf(0,fminf(1,radius+1.5f-shadow)))};
        float coverage=fmaxf(0,fminf(1,radius+.5f-distance));
        if(coverage>0) {
            float nx=dx/radius,ny=dy/radius,z=sqrtf(fmaxf(0,1-nx*nx-ny*ny));
            float light=distance>radius-1.2f?30+90*fmaxf(0,-(nx+ny)*.7f):140+58*z-40*(nx+ny);
            float band=light/16;int shade=(floorf(band)+(band-floorf(band)>(skin_bayer[y&3][x&3]+.5f)/16))*16;
            c=(DrawColor){fminf(247,shade),fminf(251,shade+4),fminf(255,shade+8),(int)(coverage*255)};
        }
        knobs[y*128+k*32+x]=c;
    }
    skin_patterns=draw_texture_create(draw,128,64,pixels,0);
    skin_knobs=draw_texture_create(draw,128,32,knobs,0);
    if(!skin_patterns || !skin_knobs){theme_draw_close();return 0;}
    draw_texture_filter(skin_patterns,0);draw_texture_filter(skin_knobs,0);
    return 1;
}
void theme_draw_close(void) { draw_texture_destroy(skin_patterns);draw_texture_destroy(skin_knobs);skin_patterns=skin_knobs=NULL; }
static void skin_pattern(DrawContext *draw,DrawRect r,int dots) {
    if(!skin_patterns || r.width<=0 || r.height<=0)return;
    for(float y=0;y<r.height;y+=64)for(float x=0;x<r.width;x+=64) {
        float w=fminf(64,r.width-x),h=fminf(64,r.height-y);
        draw_texture(draw,skin_patterns,(DrawRect){dots?64:0,0,w,h},(DrawRect){r.x+x,r.y+y,w,h},(DrawColor){255,255,255,255});
    }
}
void ui_graph_paper(DrawContext *draw,DrawRect r) {
    draw_rect(draw,r,ui_skin.graph);
    ui_graph_dots(draw,r);
}
void ui_graph_dots(DrawContext *draw,DrawRect r) { if(classic_style)skin_pattern(draw,r,1); }
void ui_graph_area(DrawContext *draw,DrawPoint a,DrawPoint b,float baseline) {
    DrawColor top=ui_skin.graph_fill,bottom={0,31,49,255};
    DrawVertex v[]={{a,0,0,top},{{a.x,baseline},0,0,bottom},{{b.x,baseline},0,0,bottom},
        {a,0,0,top},{{b.x,baseline},0,0,bottom},{b,0,0,top}};
    draw_triangles(draw,NULL,v,6);
}
void ui_graph_handle(DrawContext *draw,DrawPoint p) {
    draw_rect(draw,(DrawRect){p.x-4,p.y-4,8,8},(DrawColor){5,15,23,255});
    draw_rect(draw,(DrawRect){p.x-3,p.y-3,6,6},(DrawColor){208,223,226,255});
    draw_hairline(draw,(DrawPoint){p.x-3,p.y-3},(DrawPoint){p.x+2,p.y-3},(DrawColor){255,255,255,255});
    draw_hairline(draw,(DrawPoint){p.x+2,p.y-2},(DrawPoint){p.x+2,p.y+2},(DrawColor){103,125,134,255});
}
void ui_lamp(DrawContext *draw,DrawPoint center,float radius,DrawColor c) {
    DrawVertex v[40*9];DrawColor black={15,20,24,255};int n=0;
    for(int i=0;i<40;i++) {
        float a=6.2831853f*i/40,b=6.2831853f*(i+1)/40;
        DrawPoint p={center.x+cosf(a)*radius,center.y+sinf(a)*radius},q={center.x+cosf(b)*radius,center.y+sinf(b)*radius};
        DrawPoint inner={center.x+cosf(a)*(radius-1.4f),center.y+sinf(a)*(radius-1.4f)};
        DrawPoint next={center.x+cosf(b)*(radius-1.4f),center.y+sinf(b)*(radius-1.4f)};
        DrawColor rim={c.r*.62f,c.g*.42f,c.b*.42f,255};
        v[n++]=(DrawVertex){{center.x-radius*.18f,center.y-radius*.22f},0,0,c};
        v[n++]=(DrawVertex){inner,0,0,rim};v[n++]=(DrawVertex){next,0,0,rim};
        v[n++]=(DrawVertex){inner,0,0,rim};v[n++]=(DrawVertex){p,0,0,black};v[n++]=(DrawVertex){q,0,0,black};
        v[n++]=(DrawVertex){inner,0,0,rim};v[n++]=(DrawVertex){q,0,0,black};v[n++]=(DrawVertex){next,0,0,rim};
    }
    draw_triangles(draw,NULL,v,n);
}
/* Website colors: violet-black #09070f, lavender, pink and pale lime. */
static const Theme dark_theme={
        .background={9,7,15,255}, .surface={30,25,41,255},
        .control={20,16,29,255}, .text={238,238,255,255},
        .secondary={170,153,187,255}, .highlight={179,154,221,255},
        .hover={179,154,221,255}, .active_hover={204,187,238,255},
        .selected_text={35,24,49,255}, .border={119,102,136,255},
        .title={28,23,39,255}, .title_focus={43,34,57,255},
        .disabled={49,40,62,255}, .step_alt={43,34,57,255},
        .track={24,19,34,255}, .clip={{78,62,95,255},{69,55,87,255}},
        .note_drag={179,154,221,255}, .piano_white={218,204,234,255},
        .patt={179,154,221,255}, .grid_major={61,47,79,255},
        .grid_minor={40,31,53,255},
        .piano_black={49,40,62,255}, .piano_c={179,154,221,255},
        .piano_row={{23,19,33,255},{30,24,42,255}},
        .signal={187,255,85,255}, .loop={255,68,204,255},
        .meter_low={187,255,85,255}, .meter_mid={255,193,48,255}, .meter_high={255,74,100,255},
        .rack={30,25,41,255}, .mixer={30,25,41,255}, .effects={30,25,41,255},
        .browser={16,12,23,255}, .note={153,136,170,255},
        .step_on={{153,136,170,255},{140,160,122,255}}, .waveform={153,136,170,255},
        .fader={153,153,170,255}, .fader_mark={9,7,15,255},
        .knob={136,128,136,255}, .knob_track={112,104,112,255}, .swing={187,255,85,255},
        .pan_left={187,255,85,255}, .pan_right={255,68,204,255},
        .stereo={187,255,85,255}, .mono={255,68,204,255},
};

void theme_init(void) {
    classic_style=0;
    ui_theme=dark_theme;
    ui_skin=(DesktopSkin){.display={20,16,29,255},.graph={9,7,15,255},.graph_line={179,154,221,255},.graph_fill={78,62,95,255}};
    ui_theme.selected_text=theme_foreground(ui_theme.highlight);
}
void theme_visual_preview(void) {
    classic_style=0;
    ui_skin=(DesktopSkin){.display={7,16,20,255},.graph={0,14,24,255},.graph_line={0,210,250,255},.graph_fill={0,92,139,255}};
    ui_theme=(Theme){
        .background={17,19,22,255},.surface={35,39,44,255},
        .control={25,28,32,255},.text={235,239,242,255},
        .secondary={170,181,189,255},.highlight={71,205,225,255},
        .hover={71,205,225,255},.active_hover={112,223,237,255},
        .border={90,101,110,255},.title={29,33,38,255},.title_focus={43,52,59,255},
        .disabled={44,49,55,255},.step_alt={42,48,54,255},
        .track={28,32,37,255},.clip={{56,75,83,255},{61,69,79,255}},
        .note_drag={71,205,225,255},.piano_white={215,220,225,255},
        .patt={71,205,225,255},.grid_major={65,75,84,255},.grid_minor={37,43,49,255},
        .piano_black={37,43,49,255},.piano_c={148,192,204,255},
        .piano_row={{23,27,31,255},{30,35,40,255}},
        .signal={71,205,225,255},.loop={235,178,88,255},
        .meter_low={104,211,153,255},.meter_mid={235,187,88,255},.meter_high={239,108,116,255},
        .rack={35,39,44,255},.mixer={35,39,44,255},.effects={35,39,44,255},
        .browser={23,26,30,255},.note={132,176,188,255},
        .step_on={{132,176,188,255},{136,189,158,255}},.waveform={141,194,205,255},
        .fader={170,184,193,255},.fader_mark={22,26,30,255},
        .knob={150,168,179,255},.knob_track={93,108,120,255},.swing={71,205,225,255},
        .pan_left={104,211,153,255},.pan_right={235,178,88,255},
        .stereo={71,205,225,255},.mono={235,178,88,255}
    };
    ui_theme.selected_text=theme_foreground(ui_theme.highlight);
}
/* 90s.png: steel desktop faces, cobalt captions and cyan instrument glass.
   Keep this palette and the skin primitives below shared by every editor. */
void theme_studio(void) {
    theme_visual_preview();
    classic_style=1;
    ui_theme.background=(DrawColor){76,83,89,255};
    ui_theme.surface=ui_theme.rack=ui_theme.mixer=ui_theme.effects=(DrawColor){131,136,146,255};
    ui_theme.control=(DrawColor){154,161,173,255};
    ui_theme.text=(DrawColor){0,0,0,255};
    ui_theme.secondary=(DrawColor){9,12,15,255};
    ui_theme.title=(DrawColor){100,105,111,255};
    ui_theme.title_focus=(DrawColor){137,142,148,255};
    ui_theme.border=(DrawColor){29,33,37,255};
    ui_theme.browser=(DrawColor){7,47,73,255};
    ui_theme.track=(DrawColor){0,30,37,255};
    ui_theme.piano_row[0]=(DrawColor){45,142,156,255};
    ui_theme.piano_row[1]=(DrawColor){34,125,141,255};
    ui_theme.piano_black=(DrawColor){7,10,14,255};
    ui_theme.piano_white=(DrawColor){195,201,204,255};
    ui_theme.piano_c=(DrawColor){182,188,191,255};
    ui_theme.grid_major=(DrawColor){74,136,145,255};
    ui_theme.grid_minor=(DrawColor){43,92,103,255};
    ui_theme.highlight=ui_theme.hover=ui_theme.note_drag=(DrawColor){45,187,239,255};
    ui_theme.signal=(DrawColor){45,244,162,255};
    ui_theme.patt=(DrawColor){151,55,245,255};
    ui_theme.active_hover=(DrawColor){86,211,251,255};
    ui_theme.note=(DrawColor){145,230,221,255};
    ui_theme.waveform=(DrawColor){98,223,216,255};
    ui_theme.fader=(DrawColor){188,193,196,255};
    ui_theme.knob=(DrawColor){171,177,181,255};
    ui_theme.knob_track=(DrawColor){30,34,39,255};
    ui_skin.display=(DrawColor){2,8,10,255};
    ui_skin.field=(DrawColor){195,201,205,255};
    ui_theme.selected_text=theme_foreground(ui_theme.highlight);
}
/* Two crisp logical-pixel edges. Everything stays inside the existing bounds. */
void ui_bevel(DrawContext *draw,DrawRect r,int inset) {
    if(r.width<4 || r.height<4)return;
    DrawColor light=classic_style?(DrawColor){199,206,213,255}:(DrawColor){255,255,255,255};
    DrawColor face=classic_style?(DrawColor){166,174,182,255}:(DrawColor){223,223,223,255};
    DrawColor shadow=classic_style?(DrawColor){78,86,96,255}:(DrawColor){128,128,128,255};
    DrawColor dark=classic_style?(DrawColor){40,47,58,255}:(DrawColor){32,32,32,255};
    DrawColor top=inset?shadow:light,bottom=inset?light:dark;
    draw_hairline(draw,(DrawPoint){r.x,r.y},(DrawPoint){r.x+r.width-1,r.y},top);
    draw_hairline(draw,(DrawPoint){r.x,r.y},(DrawPoint){r.x,r.y+r.height-1},top);
    draw_hairline(draw,(DrawPoint){r.x,r.y+r.height-1},(DrawPoint){r.x+r.width-1,r.y+r.height-1},bottom);
    draw_hairline(draw,(DrawPoint){r.x+r.width-1,r.y},(DrawPoint){r.x+r.width-1,r.y+r.height-1},bottom);
    /* Direction matters: a uniform inner outline reads as a frame, not a
       raised button. Inset wells reverse both pairs of edges. */
    DrawRect inner={r.x+1,r.y+1,r.width-2,r.height-2};
    top=inset?dark:face;bottom=inset?face:shadow;
    draw_hairline(draw,(DrawPoint){inner.x,inner.y},(DrawPoint){inner.x+inner.width-1,inner.y},top);
    draw_hairline(draw,(DrawPoint){inner.x,inner.y},(DrawPoint){inner.x,inner.y+inner.height-1},top);
    draw_hairline(draw,(DrawPoint){inner.x,inner.y+inner.height-1},(DrawPoint){inner.x+inner.width-1,inner.y+inner.height-1},bottom);
    draw_hairline(draw,(DrawPoint){inner.x+inner.width-1,inner.y},(DrawPoint){inner.x+inner.width-1,inner.y+inner.height-1},bottom);
}
/* Two interpolated triangles, independent of pixel size: no bitmap or strip loop. */
void ui_gradient(DrawContext *draw,DrawRect r,DrawColor base,float strength,int horizontal) {
    if(r.width<=0 || r.height<=0)return;
    DrawColor light={(int)(base.r+(255-base.r)*strength),(int)(base.g+(255-base.g)*strength),(int)(base.b+(255-base.b)*strength),base.a};
    DrawColor dark={(int)(base.r*(1-strength)),(int)(base.g*(1-strength)),(int)(base.b*(1-strength)),base.a};
    /* Keep the original foreground contrast throughout dynamic source colors. */
    if(theme_foreground(base).r==0)dark=base;else light=base;
    DrawColor tr=horizontal?dark:light,bl=horizontal?light:dark;
    DrawVertex vertices[]={
        {{r.x,r.y},0,0,light},{{r.x,r.y+r.height},0,0,bl},{{r.x+r.width,r.y+r.height},0,0,dark},
        {{r.x,r.y},0,0,light},{{r.x+r.width,r.y+r.height},0,0,dark},{{r.x+r.width,r.y},0,0,tr}};
    draw_triangles(draw,NULL,vertices,6);
}
/* Explicit endpoints for the skin: unlike source-colour clip gradients these
   are not contrast-clamped. Text is chosen against the darkest endpoint. */
static void skin_gradient(DrawContext *draw,DrawRect r,DrawColor a,DrawColor b) {
    if(r.width<=0 || r.height<=0)return;
    DrawVertex v[]={{{r.x,r.y},0,0,a},{{r.x,r.y+r.height},0,0,b},{{r.x+r.width,r.y+r.height},0,0,b},
        {{r.x,r.y},0,0,a},{{r.x+r.width,r.y+r.height},0,0,b},{{r.x+r.width,r.y},0,0,a}};
    draw_triangles(draw,NULL,v,6);
}
static DrawColor skin_offset(DrawColor c,int amount) {
    return (DrawColor){fmaxf(0,fminf(255,c.r+amount)),fmaxf(0,fminf(255,c.g+amount)),fmaxf(0,fminf(255,c.b+amount)),c.a};
}
void ui_panel(DrawContext *draw,DrawRect r,DrawColor color) {
    if(!classic_style){draw_rect(draw,r,color);return;}
    skin_gradient(draw,r,skin_offset(color,9),skin_offset(color,-12));
    skin_pattern(draw,r,0);
}
void ui_button_face(DrawContext *draw,DrawRect r,DrawColor color,int pressed) {
    if(!classic_style){draw_rect(draw,r,color);return;}
    if(!pressed) {
        DrawColor shadow={0,0,0,125};
        draw_rect(draw,(DrawRect){r.x+2,r.y+r.height,r.width,2},shadow);
        draw_rect(draw,(DrawRect){r.x+r.width,r.y+2,2,r.height-2},shadow);
    }
    skin_gradient(draw,r,skin_offset(color,pressed?-13:23),skin_offset(color,pressed?4:-22));
    skin_pattern(draw,r,0);
    ui_bevel(draw,r,pressed);
    if(r.width>8 && r.height>8 && !pressed)
        draw_hairline(draw,(DrawPoint){r.x+3,r.y+3},(DrawPoint){r.x+r.width-4,r.y+3},skin_offset(color,40));
}
/* Velocity changes brightness continuously while retaining selection hue. */
DrawColor ui_note_velocity_color(int velocity,int selected) {
    DrawColor base=selected?ui_theme.note_drag:ui_theme.note;
    float value=fmaxf(1,fminf(127,velocity))/127.f;
    float strength=selected?.75f+.25f*value:.45f+.55f*value;
    return (DrawColor){roundf(base.r*strength),roundf(base.g*strength),roundf(base.b*strength),base.a};
}
void ui_divider_grip(DrawContext *draw,DrawRect rail,int vertical,int active) {
    float x=rail.x+rail.width/2,y=rail.y+rail.height/2;
    draw_rect(draw,rail,skin_offset(ui_theme.control,active?8:-8));
    DrawColor color=ui_theme.border;
    for(int i=0;i<5;i++) {
        float offset=-9+i*4;
        draw_rect(draw,(DrawRect){vertical?x-1:x+offset,vertical?y+offset:y-1,2,2},color);
    }
}
/* Checkboxes and record-arm keys share the same desktop button material. */
void ui_check_face(DrawContext *draw,DrawRect r,int active,DrawColor mark) {
    ui_button_face(draw,r,ui_theme.control,active);
    DrawRect well={r.x+4,r.y+4,r.width-8,r.height-8};
    draw_rect(draw,well,active?mark:(DrawColor){40,46,50,255});
    if(active && r.width>=18) {
        DrawColor ink={12,20,24,255};
        draw_line(draw,(DrawPoint){r.x+5,r.y+r.height/2},(DrawPoint){r.x+r.width/2-1,r.y+r.height-6},2,ink);
        draw_line(draw,(DrawPoint){r.x+r.width/2-1,r.y+r.height-6},(DrawPoint){r.x+r.width-5,r.y+5},2,ink);
    }
}
void ui_dropdown_arrow(DrawContext *draw,DrawRect r,int pressed) {
    float w=fminf(22,r.width/3);
    if(w<8 || r.height<10)return;
    DrawRect cell={r.x+r.width-w,r.y,w,r.height};
    ui_button_face(draw,cell,ui_theme.control,pressed);
    float x=cell.x+w/2+(pressed?1:0),y=cell.y+r.height/2+(pressed?1:0);
    DrawColor edge={222,229,232,255},ink={5,9,13,255};
    DrawVertex v[]={{{x-4,y-2},0,0,ink},{{x,y+3},0,0,ink},{{x+4,y-2},0,0,ink}};
    draw_line(draw,(DrawPoint){x-3,y-3},(DrawPoint){x+4,y-3},1,edge);
    draw_triangles(draw,NULL,v,3);
}
void ui_text_well(DrawContext *draw,DrawRect r,int focused) {
    draw_rect(draw,r,classic_style?ui_skin.field:ui_theme.control);
    if(classic_style) {
        ui_bevel(draw,r,1);
        if(focused && r.width>6 && r.height>6)
            draw_border(draw,(DrawRect){r.x+2,r.y+2,r.width-4,r.height-4},1,ui_theme.highlight);
    } else draw_border(draw,r,1,focused?ui_theme.highlight:ui_theme.border);
}
void ui_folder_icon(DrawContext *draw,float x,float y) {
    draw_rect(draw,(DrawRect){x,y,7,3},(DrawColor){248,230,122,255});
    skin_gradient(draw,(DrawRect){x,y+2,13,9},(DrawColor){250,229,101,255},(DrawColor){219,184,46,255});
    draw_hairline(draw,(DrawPoint){x,y+2},(DrawPoint){x+12,y+2},(DrawColor){255,246,168,255});
    draw_hairline(draw,(DrawPoint){x+12,y+3},(DrawPoint){x+12,y+10},(DrawColor){127,103,29,255});
    draw_hairline(draw,(DrawPoint){x,y+10},(DrawPoint){x+12,y+10},(DrawColor){127,103,29,255});
}
void ui_titlebar(DrawContext *draw,DrawRect r,int focused) {
    DrawColor c=focused?ui_theme.title_focus:ui_theme.title;
    if(!classic_style){draw_rect(draw,r,c);return;}
    skin_gradient(draw,r,skin_offset(c,23),skin_offset(c,-10));
    skin_pattern(draw,r,0);
    draw_hairline(draw,(DrawPoint){r.x+2,r.y+1},(DrawPoint){r.x+r.width-3,r.y+1},(DrawColor){184,189,195,255});
    draw_hairline(draw,(DrawPoint){r.x,r.y+r.height-1},(DrawPoint){r.x+r.width-1,r.y+r.height-1},(DrawColor){44,48,53,255});
}
/* Cached pixel-shaded bodies for the established sizes; geometry fallback
   for other radii. Pointer angle and parameter input remain continuous. */
void ui_knob_face(DrawContext *draw,float x,float y,float radius,float angle,int active) {
    if(classic_style && skin_knobs)for(int k=0;k<4;k++)if(radius==skin_knob_radii[k]) {
        draw_texture(draw,skin_knobs,(DrawRect){k*32,0,32,32},(DrawRect){x-14,y-14,32,32},(DrawColor){255,255,255,255});
        DrawPoint start={x+cosf(angle)*radius*.47f,y+sinf(angle)*radius*.47f};
        DrawPoint tip={x+cosf(angle)*(radius+2),y+sinf(angle)*(radius+2)};
        draw_line(draw,start,tip,2,active?ui_theme.highlight:(DrawColor){9,13,17,255});
        return;
    }
    enum { SEGMENTS=40 };
    DrawVertex v[SEGMENTS*12];int n=0;
    const float tau=6.2831853071795864769f;
    DrawColor black={9,13,17,255},center={202,207,210,255};
    /* Hard cast shadow shares the face batch; no blur, texture or timer. */
    DrawColor shadow={0,0,0,145};
    for(int i=0;i<SEGMENTS;i++) {
        float a=tau*i/SEGMENTS,b=tau*(i+1)/SEGMENTS;
        v[n++]=(DrawVertex){{x+2,y+3},0,0,shadow};
        v[n++]=(DrawVertex){{x+2+cosf(a)*(radius+1),y+3+sinf(a)*(radius+1)},0,0,shadow};
        v[n++]=(DrawVertex){{x+2+cosf(b)*(radius+1),y+3+sinf(b)*(radius+1)},0,0,shadow};
    }
    for(int i=0;i<SEGMENTS;i++) {
        float a=tau*i/SEGMENTS,b=tau*(i+1)/SEGMENTS;
        DrawPoint p={cosf(a),sinf(a)},q={cosf(b),sinf(b)};
        DrawColor ca=skin_offset(ui_theme.knob,-42-42*(p.x+p.y)),cb=skin_offset(ui_theme.knob,-42-42*(q.x+q.y));
        DrawPoint outer={x+p.x*radius,y+p.y*radius},next={x+q.x*radius,y+q.y*radius};
        DrawPoint inner={x+p.x*(radius-1.5f),y+p.y*(radius-1.5f)},inext={x+q.x*(radius-1.5f),y+q.y*(radius-1.5f)};
        v[n++]=(DrawVertex){{x,y},0,0,center};v[n++]=(DrawVertex){inner,0,0,ca};v[n++]=(DrawVertex){inext,0,0,cb};
        v[n++]=(DrawVertex){inner,0,0,ca};v[n++]=(DrawVertex){outer,0,0,black};v[n++]=(DrawVertex){next,0,0,black};
        v[n++]=(DrawVertex){inner,0,0,ca};v[n++]=(DrawVertex){next,0,0,black};v[n++]=(DrawVertex){inext,0,0,cb};
    }
    draw_triangles(draw,NULL,v,n);
    DrawPoint tip={x+cosf(angle)*(radius+2),y+sinf(angle)*(radius+2)};
    DrawPoint start={x+cosf(angle)*radius*.47f,y+sinf(angle)*radius*.47f};
    draw_line(draw,start,tip,2,active?ui_theme.highlight:black);
}
static DrawColor meter_mix(DrawColor a,DrawColor b,float t) {
    return (DrawColor){a.r+(b.r-a.r)*t,a.g+(b.g-a.g)*t,a.b+(b.b-a.b)*t,255};
}
/* Continuous, fixed-position color ramp: no cells, amplitude buckets or timers. */
void ui_level_fill(DrawContext *draw,DrawRect bar,float level,float amber,float red,int vertical) {
    level=fmaxf(0,fminf(1,level));if(level<=0)return;
    const float stops[]={0,amber*.55f,amber,amber+(red-amber)*.5f,amber+(red-amber)*.8f,red};
    const DrawColor *colors=signal_colors;
    for(int i=0;i<5;i++) {
        float low=stops[i],high=fminf(level,stops[i+1]);if(high<=low)continue;
        DrawColor a=colors[i],b=meter_mix(colors[i],colors[i+1],(high-low)/(stops[i+1]-low));
        DrawRect r=vertical?(DrawRect){bar.x,bar.y+(1-high)*bar.height,bar.width,(high-low)*bar.height}:
            (DrawRect){bar.x+low*bar.width,bar.y,(high-low)*bar.width,bar.height};
        DrawColor tl=vertical?b:a,br=vertical?a:b;
        DrawVertex vertices[]={{{r.x,r.y},0,0,tl},{{r.x,r.y+r.height},0,0,a},{{r.x+r.width,r.y+r.height},0,0,br},
            {{r.x,r.y},0,0,tl},{{r.x+r.width,r.y+r.height},0,0,br},{{r.x+r.width,r.y},0,0,b}};
        draw_triangles(draw,NULL,vertices,6);
    }
    if(level>red) draw_rect(draw,vertical?(DrawRect){bar.x,bar.y+(1-level)*bar.height,bar.width,(level-red)*bar.height}:(DrawRect){bar.x+red*bar.width,bar.y,(level-red)*bar.width,bar.height},colors[5]);
    DrawRect fill=vertical?(DrawRect){bar.x,bar.y+(1-level)*bar.height,bar.width,bar.height*level}:
        (DrawRect){bar.x,bar.y,bar.width*level,bar.height};
    DrawColor sheen={216,255,196,105},shade={0,24,12,110};
    if(vertical) {
        draw_hairline(draw,(DrawPoint){fill.x,fill.y},(DrawPoint){fill.x,fill.y+fill.height},sheen);
        draw_hairline(draw,(DrawPoint){fill.x+fill.width-1,fill.y},(DrawPoint){fill.x+fill.width-1,fill.y+fill.height},shade);
    } else {
        draw_hairline(draw,(DrawPoint){fill.x,fill.y},(DrawPoint){fill.x+fill.width,fill.y},sheen);
        draw_hairline(draw,(DrawPoint){fill.x,fill.y+fill.height-1},(DrawPoint){fill.x+fill.width,fill.y+fill.height-1},shade);
    }
}

/* Quiet phosphor grid: no animation and no invented signal when idle. */
void ui_display_grid(DrawContext *draw,DrawRect r,int columns,int rows) {
    if(classic_style)ui_graph_paper(draw,r);
    else draw_rect(draw,r,(DrawColor){23,35,32,255});
    DrawColor grid=classic_style?(DrawColor){18,61,83,255}:(DrawColor){42,58,48,255};
    for(int i=1;i<columns;i++)draw_hairline(draw,(DrawPoint){r.x+r.width*i/columns,r.y+2},(DrawPoint){r.x+r.width*i/columns,r.y+r.height-3},grid);
    for(int i=1;i<rows;i++)draw_hairline(draw,(DrawPoint){r.x+2,r.y+r.height*i/rows},(DrawPoint){r.x+r.width-3,r.y+r.height*i/rows},grid);
    ui_bevel(draw,r,1);
}
void ui_scrollbar(DrawContext *draw,DrawRect rail,DrawRect thumb,int active) {
    draw_rect(draw,rail,classic_style?(DrawColor){72,81,89,255}:ui_theme.background);
    if(classic_style)ui_button_face(draw,thumb,ui_theme.control,active);
    else draw_rect(draw,thumb,active?ui_theme.highlight:ui_theme.secondary);
}
void ui_surface(DrawContext *draw,DrawRect rect,DrawColor color) { draw_rect(draw,rect,color); }
void ui_frame(DrawContext *draw,DrawRect rect) {
    ui_panel(draw,rect,ui_theme.surface);
    if(classic_style)draw_border(draw,rect,1,ui_theme.border);
    else draw_border(draw,rect,1,ui_theme.border);
}

DrawColor theme_foreground(DrawColor background) {
    float channels[]={background.r/255.f,background.g/255.f,background.b/255.f};
    for(int i=0;i<3;i++) channels[i]=channels[i]<=.04045f?channels[i]/12.92f:powf((channels[i]+.055f)/1.055f,2.4f);
    float luminance=.2126f*channels[0]+.7152f*channels[1]+.0722f*channels[2];
    return (luminance+.05f)/.05f >= 1.05f/(luminance+.05f)?(DrawColor){0,0,0,255}:(DrawColor){255,255,255,255};
}
void ui_fader_handle(DrawContext *draw,DrawRect rect,int selected) {
    if(classic_style)ui_button_face(draw,rect,ui_theme.fader,0);
    else draw_rect(draw,rect,selected?ui_theme.highlight:ui_theme.fader);
    draw_hairline(draw,(DrawPoint){(int)(rect.x+2),(int)(rect.y+rect.height/2)},(DrawPoint){(int)(rect.x+rect.width-2),(int)(rect.y+rect.height/2)},
        selected?ui_theme.selected_text:ui_theme.fader_mark);
}
