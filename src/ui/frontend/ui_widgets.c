// SPDX-License-Identifier: GPL-3.0-only
#include "ui_internal.h"
#include "control.h"
#include "automation_view.h"
#include "text_edit.h"

void fonts_update(float scale) {
    if(ui.font_scale==scale) return;
    if(ui.studio_style){text_fonts_large_captions(host_fonts());text_fonts_pixel_sampling(host_fonts());text_fonts_em_metrics(host_fonts());}
    ui.font_scale=scale; icons_init(scale); text_fonts_update(host_fonts(),ui.font_path,scale);
}

static int caption_size(int size) {
    if(!ui.studio_style)return size;
    return UI_TEXT_SIZE;
}
int text_width(const char *text,int size) {
    return (int)ceilf(text_font_width(host_fonts(),text,caption_size(size)));
}

float raster_position(float value,float origin) {
    return text_pixel_position(value,origin,ui_scale(),ui.font_scale);
}

void label_moving(const char *text,float x,float y,int size,DrawColor color) {
    DrawFont * font=text_font(host_fonts(),text,caption_size(size));
    host_draw_text_ex(font,text,(DrawPoint){x,y},draw_font_layout(font)->size/ui.font_scale,0,color);
}

void label(const char *text,float x,float y,int size,DrawColor color) {
    label_moving(text,raster_position(x,ui.text_origin.x),raster_position(y,ui.text_origin.y),size,color);
}

/* Original procedural seven-segment numerals, only for numeric readouts. */
static float lcd_units(const char *text) {
    float units=0;
    for(const char *p=text;*p;p++)units+=*p=='.' || *p==':'?.32f:.70f;
    return units;
}
static void lcd_digits(const char *text,float x,float y,float h,DrawColor color) {
    static const unsigned char digits[]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};
    float w=h*.50f,t=fmaxf(1,roundf(h*.09f));
    for(const char *p=text;*p;p++) {
        if(*p==':' || *p=='.') {
            host_draw_rectangle_rec((DrawRect){x,y+(*p==':'?h*.25f:h-t),t,t},color);
            if(*p==':')host_draw_rectangle_rec((DrawRect){x,y+h*.70f,t,t},color);
            x+=h*.32f;continue;
        }
        unsigned mask=*p>='0' && *p<='9'?digits[*p-'0']:*p=='-'?0x40:0;
        DrawRect segments[]={
            {x+t,y,w-2*t,t},{x+w-t,y+t,t,h/2-t},
            {x+w-t,y+h/2,t,h/2-t},{x+t,y+h-t,w-2*t,t},
            {x,y+h/2,t,h/2-t},{x,y+t,t,h/2-t},
            {x+t,y+h/2-t/2,w-2*t,t}};
        for(int i=0;i<7;i++)if(mask&(1u<<i))host_draw_rectangle_rec(segments[i],color);
        x+=h*.70f;
    }
}

void lcd_number(const char *text,float x,float y,float width,float height,DrawColor color) {
    lcd_digits(text,x,y,ui.studio_style?UI_TEXT_SIZE:fminf(height,width/fmaxf(.1f,lcd_units(text))),color);
}
void lcd_time(const char *text,float x,float y,float width,DrawColor color) {
    const char *last=strrchr(text,':');
    if(!last){lcd_number(text,x,y,width,UI_TEXT_SIZE,color);return;}
    char whole[32];size_t count=(size_t)(last-text)+1;
    if(count>=sizeof whole)count=sizeof whole-1;
    memcpy(whole,text,count);whole[count]=0;
    float large=UI_TEXT_SIZE,small=11;
    float used=lcd_units(whole)*large+lcd_units(last+1)*small;
    float fit=fminf(1,width/fmaxf(1,used));large*=fit;small*=fit;
    lcd_digits(whole,x,y,large,color);
    lcd_digits(last+1,x+lcd_units(whole)*large,y+large-small,small,color);
}

void backspace(char *text) {
    text_backspace(text);
}

const char *fit_text(const char *text,int width,int size) {
    static char fitted[LL_PATH_CAPACITY]; text_fit(host_fonts(),fitted,sizeof fitted,text,width,caption_size(size)); return fitted;
}

void circles_init(void) {
    HostImage image=host_gen_image_color(64,64,WHITE); DrawColor *pixels=image.data;
    for(int y=0;y<64;y++) for(int x=0;x<64;x++) {
        float dx=x+.5f-32,dy=y+.5f-32;
        pixels[y*64+x].a=255*fmaxf(0,fminf(1,(32-sqrtf(dx*dx+dy*dy))/3));
    }
    ui.circle_texture=host_load_texture_from_image(image); host_unload_image(image);
    host_set_texture_filter(ui.circle_texture,HOST_TEXTURE_FILTER_BILINEAR);
}

void circle(float x,float y,float radius,DrawColor color) {
    host_draw_texture_pro(ui.circle_texture,(DrawRect){0,0,64,64},(DrawRect){x-radius,y-radius,radius*2,radius*2},(DrawPoint){0},0,color);
}

void icons_init(float scale) {
    UIIcons icons={0}; if(ui.icons.id)host_unload_texture(ui.icons);
    if(!(ui.studio_style?icons_build_classic(host_draw(),scale,&icons):icons_build(host_draw(),scale,&icons))){ui.icons=(HostTexture){0};return;}
    ui.icons=(HostTexture){icons.texture,(ui.studio_style?ICON_CLASSIC_COUNT:ICON_COUNT)*icons.large,icons.large+icons.medium+icons.small};
    ui.icon_large=icons.large;ui.icon_medium=icons.medium;ui.icon_small=icons.small;
}

static void paint_icon(int id,float x,float y,int variant,float size,DrawColor color) {
    if(ui.studio_style)color=(DrawColor){255,255,255,color.a};
    int pixels=variant==2?ui.icon_small:variant==1?ui.icon_medium:ui.icon_large;
    if(size<=0)size=pixels/ui.font_scale;
    float left=raster_position(x-size/2,ui.text_origin.x),top=raster_position(y-size/2,ui.text_origin.y);
    host_draw_texture_pro(ui.icons,(DrawRect){id*ui.icon_large,variant==2?ui.icon_large+ui.icon_medium:variant==1?ui.icon_large:0,pixels,pixels},(DrawRect){left,top,size,size},(DrawPoint){0},0,color);
}
void icon(int id,float x,float y,float size,DrawColor color) {
    paint_icon(id,x,y,size<=16?2:size<=20?1:0,0,color);
}
void icon_in_button(int id,DrawRect button,DrawColor color) {
    /* Use the original 16px sprite directly: 32px transport icons stay a
       clean 2x enlargement, without the padded large-atlas cell. */
    if(button.width<=4 || button.height<=4)return;
    float size=fminf(32,fminf(button.width,button.height)-4);
    int pressed=hover(button.x,button.y,button.width,button.height) && host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT);
    paint_icon(id,button.x+button.width/2+pressed,button.y+button.height/2+pressed,2,size,color);
}

void arcs_init(void) {
    /* Swing horseshoe, centered full turn, and minimum-to-value full turn. */
    HostImage image=host_gen_image_color(1536,288,BLANK);
    DrawColor *pixels=image.data;
    for(int bank=0;bank<3;bank++) for(int level=0;level<=128;level++) for(int y=0;y<32;y++) for(int x=0;x<32;x++) {
        float dx=x+.5f-16,dy=y+.5f-16,distance=sqrtf(dx*dx+dy*dy);
        float origin=bank==0?2.4f:bank==1?PI/2:-PI/2;
        float angle=atan2f(dy,dx)-origin; if(angle<0) angle+=2*PI;
        float end=level/128.f*(bank==0?4.6f:2*PI),start=bank==1?PI:0;
        float alpha=fmaxf(0,fminf(1,fminf(16-distance,distance-(16-48.f/KNOB_RADIUS))/1.4f));
        if(bank!=2 || level!=128) alpha*=fmaxf(0,fminf(1,fminf(angle-fminf(start,end),fmaxf(start,end)-angle)*distance));
        pixels[(level/16*32+y)*1536+bank*512+level%16*32+x]=(DrawColor){255,255,255,255*alpha};
    }
    ui.knob_arcs=host_load_texture_from_image(image); host_unload_image(image);
    host_set_texture_filter(ui.knob_arcs,HOST_TEXTURE_FILTER_BILINEAR);
}

void cables_init(void) {
    HostImage image=host_gen_image_color(1,16,WHITE); DrawColor *pixels=image.data;
    for(int y=0;y<16;y++) pixels[y].a=255*fminf(1,(8-fabsf(y+.5f-8))/4);
    ui.cable_texture=host_load_texture_from_image(image); host_unload_image(image);
    host_set_texture_filter(ui.cable_texture,HOST_TEXTURE_FILTER_BILINEAR);
}

void cable(DrawPoint points[4],DrawColor color) {
    DrawPoint previous[2]={{0}},normal={2,0};
    host_mesh_settexture(ui.cable_texture.id); host_mesh_begin(HOST_RL_QUADS); host_mesh_color4ub(color.r,color.g,color.b,color.a);
    for(int i=0;i<=64;i++) {
        float t=i/64.f,u=1-t;
        DrawPoint p=host_get_spline_point_bezier_cubic(points[0],points[1],points[2],points[3],t);
        float dx=3*u*u*(points[1].x-points[0].x)+6*u*t*(points[2].x-points[1].x)+3*t*t*(points[3].x-points[2].x);
        float dy=3*u*u*(points[1].y-points[0].y)+6*u*t*(points[2].y-points[1].y)+3*t*t*(points[3].y-points[2].y);
        float length=hypotf(dx,dy); if(length>.001f) normal=(DrawPoint){-dy/length*2,dx/length*2};
        DrawPoint edge[2]={{p.x+normal.x,p.y+normal.y},{p.x-normal.x,p.y-normal.y}};
        if(i) {
            host_mesh_texcoord2f(.5f,0); host_mesh_vertex2f(previous[0].x,previous[0].y);
            host_mesh_texcoord2f(.5f,0); host_mesh_vertex2f(edge[0].x,edge[0].y);
            host_mesh_texcoord2f(.5f,1); host_mesh_vertex2f(edge[1].x,edge[1].y);
            host_mesh_texcoord2f(.5f,1); host_mesh_vertex2f(previous[1].x,previous[1].y);
        }
        previous[0]=edge[0]; previous[1]=edge[1];
    }
    host_mesh_end(); host_mesh_settexture(0);
}

void smooth_line(DrawPoint start,DrawPoint end,float width,DrawColor color) {
    float dx=end.x-start.x,dy=end.y-start.y,length=hypotf(dx,dy);
    if(length<.001f) return;
    float radius=width*2/3;
    DrawPoint normal={-dy/length*radius,dx/length*radius};
    host_mesh_settexture(ui.cable_texture.id); host_mesh_begin(HOST_RL_QUADS); host_mesh_color4ub(color.r,color.g,color.b,color.a);
    host_mesh_texcoord2f(.5f,0); host_mesh_vertex2f(start.x+normal.x,start.y+normal.y);
    host_mesh_texcoord2f(.5f,0); host_mesh_vertex2f(end.x+normal.x,end.y+normal.y);
    host_mesh_texcoord2f(.5f,1); host_mesh_vertex2f(end.x-normal.x,end.y-normal.y);
    host_mesh_texcoord2f(.5f,1); host_mesh_vertex2f(start.x-normal.x,start.y-normal.y);
    host_mesh_end(); host_mesh_settexture(0);
}

int hover(float x,float y,float w,float h) { return ui.input_enabled && host_check_collision_point_rec(ui.mouse,(DrawRect){x,y,w,h}); }

int menu_key(int key) { return host_is_key_pressed(key) || host_is_key_pressed_repeat(key); }

void menu_keys_begin(int id,int opened,int default_item,int *scroll,int total) {
    menu_navigation_begin(&ui.menu_keys,id,opened,default_item,scroll,total,(MenuNavigationInput){
        .pointer={ui.mouse.x,ui.mouse.y},.mouse_pressed=host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT),
        .next=menu_key(HOST_KEY_DOWN) || menu_key(HOST_KEY_J) || menu_key(HOST_KEY_RIGHT) || menu_key(HOST_KEY_L),
        .previous=menu_key(HOST_KEY_UP) || menu_key(HOST_KEY_K) || menu_key(HOST_KEY_LEFT) || menu_key(HOST_KEY_H),
        .enter=host_is_key_pressed(HOST_KEY_ENTER) || host_is_key_pressed(HOST_KEY_KP_ENTER)});
}

void menu_keys_end(void) { menu_navigation_end(&ui.menu_keys); }

int symbol(const char *text,int x,int y,DrawColor c) {
    int id;
    if(!strcmp(text,"x")) id=ICON_CLOSE;
    else if(!strcmp(text,"[]")) id=ICON_STOP;
    else if(!strcmp(text,"||")) id=ICON_PAUSE;
    else if(!strcmp(text,">")) id=ICON_PLAY;
    else if(!strcmp(text,"<")) id=ICON_BACK;
    else return 0;
    icon(id,x,y,16,c);
    return 1;
}

int button_color(const char *text,int x,int y,int w,int h,int active,DrawColor base) {
    int over=hover(x,y,w,h);
    int menu_item=ui.menu_keys.active && ui.input_enabled && strcmp(text,"x")?ui.menu_keys.items++:-1;
    if(menu_item>=0 && menu_item<128) ui.menu_keys.bounds[menu_item]=(MenuBounds){x,y,w,h};
    int focused=menu_item>=0 && menu_item==ui.menu_keys.selected;
    int activate=focused && ui.menu_keys.enter;
    if(activate) ui.menu_keys.enter=0;
    if(over) host_set_mouse_cursor(HOST_MOUSE_CURSOR_POINTING_HAND);
    if((menu_item>=0?focused:over) && *text) {
        const char *description=text;
        if(!strcmp(text,"New")) description="Start an empty project with one unloaded Sampler";
        else if(!strcmp(text,"Demo")) description="Load the built-in drum and bass demo";
        else if(!strcmp(text,"Save")) description="Save the current project";
        else if(!strcmp(text,"Open...")) description="Choose a project to open";
        else if(!strcmp(text,"Export...")) description="Choose where to export the Playlist as a WAV file";
        else if(!strcmp(text,"HELP")) description="Show all current keybindings";
        else if(!strcmp(text,"Dark")) description="Use dark colors; saved automatically for next launch";
        else if(!strcmp(text,"Light")) description="Use light colors; saved automatically for next launch";
        else if(!strcmp(text,"+ Add folder")) description="Add a folder as a new Browser tree root";
        else if(!strcmp(text,"PAT") || !strcmp(text,"SONG")) description="Switch playback between the current pattern and Playlist";
        else if(!strcmp(text,"Rack")) description="Show/focus or hide the Channel Rack";
        else if(!strcmp(text,"List")) description="Show/focus or hide the Playlist";
        else if(!strcmp(text,"Piano")) description="Show/focus or hide the Piano Roll";
        else if(!strcmp(text,"Mix")) description="Show/focus or hide the Mixer";
        else if(!strcmp(text,"x")) description="Close this dialog";
        else if(!strcmp(text,"<")) description="Previous";
        else if(!strcmp(text,">")) description="Next";
        else if(!strcmp(text,"[]")) description="Stop playback and return to the beginning";
        else if(!strcmp(text,"||")) description="Pause playback";
        else if(!strcmp(text,"Reset")) description="Restore this control's default value";
        else if(!strcmp(text,"Apply")) description="Apply the entered value";
        else if(!strcmp(text,"Cancel")) description="Cancel without changing the value";
        snprintf(ui.status,sizeof ui.status,"%s",description);
    }
    int highlighted=menu_item>=0?focused:over && !ui.menu_keys.active;
    DrawColor surface=menu_item>=0?(focused?ui_theme.hover:base):
        active?(highlighted?ui_theme.active_hover:accent):highlighted?ui_theme.hover:base;
    int pressed=active || (over && host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT));
    /* Desktop push buttons retain their neutral face on hover. Selection is
       sunken; only menu choices use the selection fill. Hit bounds unchanged. */
    if(ui.studio_style && menu_item<0)surface=active?ui_theme.highlight:base;
    ui_surface(host_draw(),(DrawRect){x,y,w,h},surface);
    int list_row=ui.studio_style &&
        ((!memcmp(&base,&ui_theme.browser,sizeof base)) || (!memcmp(&base,&ui_skin.field,sizeof base)));
    if(ui.studio_style && !ui.profile_flat && menu_item<0 && !list_row) {
        ui_button_face(host_draw(),(DrawRect){x,y,w,h},surface,pressed);
    }
    DrawColor foreground=(menu_item>=0?focused:active || highlighted)?theme_foreground(surface):ui.studio_style?theme_foreground(surface):ink;
    int offset=ui.studio_style && pressed?1:0;
    int action=ui.studio_style && !list_row?icon_action(text):-1;
    if(action>=0 && w>=56) {
        icon(action,x+12+offset,y+h/2+offset,16,foreground);
        const char *caption=fit_text(!strncmp(text,"+ ",2)?text+2:text,w-28,UI_TEXT_SIZE);
        label(caption,x+24+offset,y+(h-UI_TEXT_SIZE)/2+offset,UI_TEXT_SIZE,foreground);
    }
    else if(action>=0)icon_in_button(action,(DrawRect){x,y,w,h},foreground);
    else if(w>52 || !symbol(text,x+w/2+offset,y+h/2+offset,foreground)) {
        int available=w-(w<=72?8:16);
        int size=ui.studio_style?UI_TEXT_SIZE:text_button_size(host_fonts(),text,available,h);
        const char *caption=fit_text(text,available,size);
        label(caption,x+((w<=72 || !strcmp(text,"+"))?(w-text_width(caption,size))/2:8)+offset,y+(h-size)/2+offset,size,foreground);
    }
    if(menu_item>=0 && over && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) ui.menu_keys.selected=menu_item;
    return activate || (over && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT));
}

int button(const char *text,int x,int y,int w,int h,int active) {
    return button_color(text,x,y,w,h,active,cell);
}

int choice_button(const char *text,int x,int y,int w,int h) {
    if(!ui.studio_style)return button(text,x,y,w,h,0);
    int clicked=button("",x,y,w,h,0);
    int pressed=hover(x,y,w,h) && host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT);
    int size=10,available=w-(w<66?14:22)-12;
    const char *caption=fit_text(text,available,size);
    label(caption,x+6+pressed,y+(h-caption_size(size))/2+pressed,size,ink);
    ui_dropdown_arrow(host_draw(),(DrawRect){x,y,w,h},pressed);
    return clicked;
}

int drag_button(const char *text,int x,int y,int w,int h,int active) {
    int clicked=button(text,x,y,w,h,active);
    if(hover(x,y,w,h) || active || clicked) host_set_mouse_cursor(HOST_MOUSE_CURSOR_RESIZE_NS);
    return clicked;
}

int color_picker(int x,int y,int width,uint32_t selected) {
    int chosen=-1,spacing=width/COLOR_HUES;
    int hover_index=-1;
    for(int i=0;i<COLOR_COUNT;i++)
        if(hover(x+(i%COLOR_HUES)*spacing,y+(i/COLOR_HUES)*28,spacing-4,24)) hover_index=i;
    for(int i=0;i<COLOR_COUNT;i++) {
        unsigned rgb=pattern_palette[i]; DrawColor color=source_rgb(rgb);
        DrawRect swatch={x+(i%COLOR_HUES)*spacing,y+(i/COLOR_HUES)*28,spacing-4,24};
        int over=hover(swatch.x,swatch.y,swatch.width,swatch.height);
        ui_surface(host_draw(),swatch,color);
        if(hover_index>=0?i==hover_index:selected==rgb) host_draw_rectangle_lines_ex((DrawRect){swatch.x+2,swatch.y+2,swatch.width-4,swatch.height-4},2,clip_foreground(color));
        if(over) { host_set_mouse_cursor(HOST_MOUSE_CURSOR_POINTING_HAND); snprintf(ui.status,sizeof ui.status,"Choose color #%06X",rgb); if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) chosen=i; }
    }
    return chosen;
}

int picker_button(int y,DrawColor color,int selected) {
    int over=hover(4,y,picker_width(),50);
    if(over) host_set_mouse_cursor(HOST_MOUSE_CURSOR_POINTING_HAND);
    if(ui.studio_style && !ui.profile_flat)ui_gradient(host_draw(),(DrawRect){4,y,picker_width(),50},color,.45f,0);
    else ui_surface(host_draw(),(DrawRect){4,y,picker_width(),50},color);
    if(over || selected) host_draw_rectangle_lines_ex((DrawRect){4,y,picker_width(),50},2,WHITE);
    return over && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT);
}

static AutomationView desktop_automation_view(void) {
    AutomationView view;memcpy(view.shown,ui.automation_shown,sizeof view.shown);
    memcpy(view.visible,ui.automation_visible,sizeof view.visible);return view;
}
void automation_display_update(void) {
    AutomationView view=desktop_automation_view();automation_view_update(&view,&ui.project,ui.playing,ui.song,ui.visual_step);
    memcpy(ui.automation_shown,view.shown,sizeof view.shown);memcpy(ui.automation_visible,view.visible,sizeof view.visible);
}
float displayed_value(const void *pointer,float manual) {
    AutomationView view=desktop_automation_view();return automation_displayed_value(&view,&ui.project,ui.playing,ui.song,pointer,manual);
}

void mute_light(float x,float y,uint8_t *states,int count,int selected,const char *name) {
    uint8_t state=states[selected]; if(displayed_value(&states[selected],!!(state&1))>=.5f) state|=1; else state&=~1; int solo=count?solo_any(states,count):0;
    int over=hover(x-8,y-8,16,16);
    if(over) host_set_mouse_cursor(HOST_MOUSE_CURSOR_POINTING_HAND);
    int enabled=!(state&1) && (!solo || (state&2));
    DrawColor color=enabled?(state&2?ui_theme.meter_mid:ui_theme.signal):cell;
    if(enabled && !(state&2) && !over)
        color=host_color_alpha_blend(cell,host_fade(color,.48f),WHITE);
    if(ui.studio_style) {
        ui_surface(host_draw(),(DrawRect){x-7,y-7,14,14},color);
        if(!ui.profile_flat)host_draw_rectangle_lines(x-7,y-7,14,14,over?ink:ui_theme.border);
        icon(state&2?ICON_SOLO:ICON_SPEAKER,x,y,16,enabled?WHITE:host_fade(WHITE,.5f));
    } else { circle(x,y,6,over?ink:state&2?ui_theme.meter_mid:ui_theme.border); circle(x,y,5,color); }
    if(over) {
        snprintf(ui.status,sizeof ui.status,"%s: %s | Left-click: mute/unmute; right-click: %s",name,state&1?"Muted":state&2?"Solo":solo?"Silenced by solo":"Enabled",states==ui.project.lane_mute?"add/remove solo":"automation / solo menu");
        if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) { states[selected]^=1; ui.input_enabled=0; }
        if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)) {
            if(parameter_from_pointer(&ui.project,&states[selected],&ui.automation_target)) {
                snprintf(ui.menu_name,sizeof ui.menu_name,"Mute");
                DrawPoint position=ui.mouse; if(ui.knob_context>=0) { position.x+=desktop_editor_rect(ui.knob_context).x; position.y+=desktop_editor_rect(ui.knob_context).y; }
                open_context(10,0,position); return;
            }
            if(count) solo_toggle(states,count,selected);
            else {
                for(int i=0;i<INSERTS;i++) ui.project.insert_mute[i]&=~2;
                for(int c=0;c<CHANNELS;c++) ui.project.mute[c]&=~2;
                for(int l=0;l<LANES;l++) ui.project.lane_mute[l]&=~2;
            }
            ui.input_enabled=0;
        }
    }
}

void mute_solo_buttons(float x,float y,uint8_t *states,int count,int selected,const char *name) {
    int is_muted=displayed_value(&states[selected],!!(states[selected]&1))>=.5f;
    int solo=count?!!(states[selected]&2):solo_any(ui.project.insert_mute,ui.project.insert_count) || solo_any(ui.project.mute,ui.project.channel_count) || solo_any(ui.project.lane_mute,LANES);
    if(button("",x,y,18,20,is_muted)){states[selected]^=1;ui.input_enabled=0;}
    label("M",x+5,y+2,UI_TEXT_SIZE,ink);
    if(is_muted)host_draw_rectangle(x+3,y+17,12,2,ui_theme.meter_high);
    if(hover(x,y,18,20)) {
        snprintf(ui.status,sizeof ui.status,"%s: mute / unmute; right-click for automation",name);
        if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT) && parameter_from_pointer(&ui.project,&states[selected],&ui.automation_target)){
            snprintf(ui.menu_name,sizeof ui.menu_name,"Mute");Rect r=desktop_editor_rect(ui.knob_context);
            open_context(10,0,(DrawPoint){ui.mouse.x+r.x,ui.mouse.y+r.y});
        }
    }
    if(button("",x+20,y,18,20,solo)) {
        if(count)solo_toggle(states,count,selected);
        else {
            for(int i=0;i<INSERTS;i++)ui.project.insert_mute[i]&=~2;
            for(int i=0;i<CHANNELS;i++)ui.project.mute[i]&=~2;
            for(int i=0;i<LANES;i++)ui.project.lane_mute[i]&=~2;
        }
        ui.input_enabled=0;
    }
    label("S",x+25,y+2,UI_TEXT_SIZE,ink);
    if(solo)host_draw_rectangle(x+23,y+17,12,2,ui_theme.meter_mid);
    if(hover(x+20,y,18,20))snprintf(ui.status,sizeof ui.status,count?"%s: add / remove solo":"%s: clear all solos",name);
}

int editor_button(int id,int x) {
    static const char *names[]={"Channel Rack","Playlist","Piano Roll","Mixer"};
    int front=0;
    for(int i=windows_count(&ui.windows)-1;i>=0;i--) {
        int other=ui.windows.order[i];
        if(ui.windows.editors[other].visible && ui.windows.editors[other].pinned==ui.windows.editors[id].pinned) { front=other==id; break; }
    }
    StudioToolbar toolbar=studio_toolbar_layout(host_get_screen_width()/ui_scale());
    int y=ui.studio_style?toolbar.editors[0].y:8,h=ui.studio_style?toolbar.editors[0].h:22,w=h;
    int clicked=button("",x,y,w,h,0);
    if(clicked) { if(ui.windows.workspace.enabled) {windows_focus(&ui.windows,id);}else if(front) ui.windows.editors[id].visible=0; else windows_focus(&ui.windows,id); }
    icon(id,x+w/2,y+h/2,20,ink);
    if(hover(x,y,w,h)) snprintf(ui.status,sizeof ui.status,"%s: %s",names[id],ui.windows.workspace.enabled?"show / focus this view":front?"hide this window":"show / bring this window to the front");
    return clicked;
}

void open_context(int kind,int target,DrawPoint position) {
    if(ui.windows.workspace.enabled && (kind==14 || kind==16))ui.effect_context_window=ui.knob_context;
    ui.context_kind=kind; ui.context_target=target; ui.context_position=position; ui.context_opened=1; ui.input_enabled=0;
}

int preset_directory(char *out,size_t capacity,int kind) {
    const char *slash=strrchr(ui.browser.config,'/');
    if(!slash) return 0;
    char root[LL_PATH_CAPACITY];
    if(snprintf(root,sizeof root,"%.*s/presets",(int)(slash-ui.browser.config),ui.browser.config)>=(int)sizeof root || host_make_directory(root)) return 0;
    const char *device=kind==PRESET_TUBE?"Tube":kind==PRESET_DELAY?"Delay":kind==PRESET_FM?"FM Synth":kind==PRESET_EQ?"Equalizer":kind==PRESET_CHORUS?"Chorus":"Sampler";
    if(snprintf(out,capacity,"%s/%s",root,device)>=(int)capacity || host_make_directory(out)) return 0;
    if(kind==PRESET_FM) for(int i=0;i<FM_FACTORY_COUNT;i++) {
        char factory[LL_PATH_CAPACITY];
        if(snprintf(factory,sizeof factory,"%s/%s.llpreset",out,fm_factory_name(i))<(int)sizeof factory && !host_file_exists(factory)) {
            DevicePreset preset={.kind=PRESET_FM,.fm=fm_factory(i)}; preset_save(factory,&preset);
        }
    }
    return 1;
}

static ControlDrag desktop_control_state(void) {
    return (ControlDrag){ui.control_drag,ui.control_low,ui.control_high,ui.control_raw,
        ui.control_bottom,ui.control_range,ui.control_fader,ui.control_reverse,
        ui.control_integer,ui.control_logarithmic,ui.control_magnetic};
}
static void desktop_control_commit(ControlDrag state) {
    ui.control_drag=state.value;ui.control_low=state.low;ui.control_high=state.high;ui.control_raw=state.raw;
    ui.control_bottom=state.bottom;ui.control_range=state.range;ui.control_fader=state.fader;
    ui.control_reverse=state.reverse;ui.control_integer=state.integer;ui.control_logarithmic=state.logarithmic;ui.control_magnetic=state.magnetic;
}
void capture_control(float *value,float low,float high,int fader) {
    ControlDrag state=desktop_control_state();control_capture(&state,&ui.project,value,low,high,fader);
    desktop_control_commit(state);ui.control_wheel.value=NULL;ui.control_horizontal=0;ui.input_enabled=0;
}
void desktop_control_update(float dy,float scale,float y) {
    ControlDrag state=desktop_control_state();control_drag_update_fine(&state,&ui.project,dy,scale,y,host_is_key_down(HOST_KEY_LEFT_CONTROL) || host_is_key_down(HOST_KEY_RIGHT_CONTROL));desktop_control_commit(state);
}

void knob_sized(int x,int y,float *value,float low,float high,float initial,const char *name,int style,float size) {
    float shown=displayed_value(value,*value);
    int over=hover(x-size-1,y-size-1,size*2+2,size*2+2);
    int fixed_coarse=style==KNOB_FIXED_COARSE;
    if(fixed_coarse) {
        high=3; initial=(int)initial&3; shown=(int)roundf(shown)&3;
        /* Old DX files may store aliases 4..31. Canonicalize on interaction,
           preserving their audible frequency and leaving unopened files alone. */
        if(over && (host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT) || host_get_mouse_wheel_move()))
            *value=(int)roundf(*value)&3;
    }
    if(over || ui.control_drag==value) {
        if(style==KNOB_VOLUME) snprintf(ui.status,sizeof ui.status,"%s: %.4g (%.2f dB) | Dot marks 0 dB; drag or wheel; right-click for value / automation",name,shown,gain_db(shown));
        else snprintf(ui.status,sizeof ui.status,"%s: %.4g | Drag up/down or wheel; right-click for value / automation",name,shown);
    }
    if(over) {
        if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)) control_menu(value,low,high,initial,name);
        if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) { capture_control(value,low,high,0); ui.control_low=low; ui.control_high=high; ui.control_reverse=style==KNOB_WIDTH; }
        float wheel=host_get_mouse_wheel_move();
        if(wheel)control_knob_wheel_fine(&ui.control_wheel,&ui.project,value,low,high,style,wheel,host_is_key_down(HOST_KEY_LEFT_CONTROL) || host_is_key_down(HOST_KEY_RIGHT_CONTROL));
    }
    float radius=size,fraction=(shown-low)/(high-low);
    if(style==KNOB_LOGARITHMIC || style==KNOB_MAGNETIC) fraction=logf(shown/low)/logf(high/low);
    if(style==KNOB_CENTER) fraction=shown<initial?.5f*(shown-low)/(initial-low):.5f+.5f*(shown-initial)/(high-initial);
    if(style==KNOB_WIDTH) fraction=1-fraction;
    if(style==KNOB_VOLUME) fraction=shown<=1?.75f*(shown):.75f+.25f*(shown-1)/(high-1);
    int centered=style==KNOB_PAN || style==KNOB_WIDTH || style==KNOB_CENTER;
    float origin=centered?PI/2:style==KNOB_SWING?2.4f:style==KNOB_VOLUME?PI/2:-PI/2;
    float angle=origin+fraction*(style==KNOB_SWING?4.6f:2*PI);
    if(ui.studio_style && !ui.profile_flat) {
        ui_knob_face(host_draw(),x,y,radius,angle,over || ui.control_drag==value);
        if(style==KNOB_VOLUME) {
            float unity=origin+.75f*2*PI;
            circle(x+cosf(unity)*size*1.22f,y+sinf(unity)*size*1.22f,1.2f,muted);
        }
        return;
    }
    DrawColor color=style==KNOB_SWING || style==KNOB_CENTER?ui_theme.swing:style==KNOB_PAN?(fraction<.5f?ui_theme.pan_left:ui_theme.pan_right):style==KNOB_WIDTH?(fraction<.5f?ui_theme.stereo:ui_theme.mono):ui_theme.knob;
    if((over || ui.control_drag==value) && (style==KNOB_NORMAL || style==KNOB_VOLUME || style==KNOB_LOGARITHMIC || style==KNOB_FIXED_COARSE)) color=accent;
    if(style==KNOB_SWING) {
        host_draw_texture_pro(ui.knob_arcs,(DrawRect){0,256,32,32},(DrawRect){x-radius,y-radius,radius*2,radius*2},(DrawPoint){0},0,ui_theme.knob_track);
        circle(x,y,radius*.68f,cell);
    } else { circle(x,y,radius,ui_theme.knob_track); circle(x,y,radius*.8f,cell); }
    if(style==KNOB_PAN || style==KNOB_WIDTH) {
        DrawColor left=style==KNOB_PAN?ui_theme.pan_left:ui_theme.stereo;
        DrawColor right=style==KNOB_PAN?ui_theme.pan_right:ui_theme.mono;
        float opacity=.35f;
        DrawRect ring={x,y,radius*2,radius*2}; DrawPoint center={radius,radius};
        host_draw_texture_pro(ui.knob_arcs,(DrawRect){512,0,32,32},ring,center,0,host_fade(left,opacity));
        host_draw_texture_pro(ui.knob_arcs,(DrawRect){512,256,32,32},ring,center,0,host_fade(right,opacity));
    }
    int level=fmaxf(0,fminf(128,roundf(fraction*128)));
    float rotation=style==KNOB_SWING || centered?0:(origin+PI/2)*RAD2DEG;
    host_draw_texture_pro(ui.knob_arcs,(DrawRect){(style==KNOB_SWING?0:centered?512:1024)+level%16*32,level/16*32,32,32},(DrawRect){x,y,radius*2,radius*2},(DrawPoint){radius,radius},rotation,color);
    if(style==KNOB_VOLUME) {
        float unity=origin+.75f*2*PI;
        circle(x+cosf(unity)*size*1.22f,y+sinf(unity)*size*1.22f,1.2f,muted);
    }
    DrawPoint pointer={x+cosf(angle)*radius*.85f,y+sinf(angle)*radius*.85f};
    float pointer_scale=size/KNOB_RADIUS;
    circle(pointer.x,pointer.y,2.3f*pointer_scale,(DrawColor){35,24,49,255});
    circle(pointer.x,pointer.y,1.7f*pointer_scale,WHITE);
}

void knob_style(int x,int y,float *value,float low,float high,float initial,const char *name,int style) {
    knob_sized(x,y,value,low,high,initial,name,style,style==KNOB_SWING?SWING_RADIUS:KNOB_RADIUS);
}

void knob(int x,int y,float *value,float low,float high,float initial,const char *name) {
    knob_style(x,y,value,low,high,initial,name,KNOB_NORMAL);
}

void label_key(const char *text,DrawRect bounds,DrawColor color) {
    int maximum=ui.studio_style?UI_TEXT_SIZE:12;
    DrawFont *font=text_font(host_fonts(),text,maximum);FontPoint p;float size;
    if(font_fit_ink(draw_font_layout(font),text,(GlyphRect){bounds.x,bounds.y,bounds.width,bounds.height},maximum,&p,&size))
        host_draw_text_ex(font,text,(DrawPoint){p.x,p.y},size,0,color);
}

void grid_selector(int id,int x,int y,int width,int height) {
    int choice=id==1?ui.playlist_grid:ui.piano_grid;
    if(button("",x,y,width,height,0))
        open_context(24,id,(DrawPoint){ui.mouse.x+desktop_editor_rect(id).x,ui.mouse.y+desktop_editor_rect(id).y});
    label(fit_text(grid_choice_name(choice),width-26,10),x+6,y+(height-(ui.studio_style?UI_TEXT_SIZE:12))/2,10,ink);
    if(ui.studio_style)ui_dropdown_arrow(host_draw(),(DrawRect){x,y,width,height},hover(x,y,width,height) && host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT));
    else {
        float arrow=x+width-11,mid=y+height/2;
        host_draw_triangle((DrawPoint){arrow-3,mid-2},(DrawPoint){arrow,mid+2},(DrawPoint){arrow+3,mid-2},ink);
    }
    if(hover(x,y,width,height))snprintf(ui.status,sizeof ui.status,"Grid / snap: %s | choose Auto, a fixed division, or None",grid_choice_name(choice));
}
